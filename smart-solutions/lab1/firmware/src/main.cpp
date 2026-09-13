// AtomS3R web image server — a 4-slot image kiosk with single-button gestures.
//
// Holds 4 128x128 RGB565 images in flash. The displayed slot is switched with a
// simple HTTP GET (GET /show?slot=N). The AtomS3R has one button, so each slot
// maps three button gestures to HTTP GET actions: short click (<500 ms), long
// click (>1500 ms), and double click (two shorts within 2000 ms). A gesture URL
// of "2" shows slot 2 here; "/show?slot=1" is also local; "http://other/…" drives
// another unit.
//
// A live data overlay (up to 4 text labels + 4 percentage bars) can be drawn on
// top of the displayed image and updated with plain GETs — GET /set?n0=LOAD&b0=90
// — so a host pushes the picture once and then only refreshes the numbers.
//
// ?mid= on /frame and markerId in /state carry an optional tag id for the image
// in a slot (used by a marker-tracking host); leave them unset if you don't
// need one.
//
// Serial console (115200, USB-CDC): wifi <ssid>:<password> / ip / status / ap.

#include <M5Unified.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <LittleFS.h>
#include <esp_system.h>

#include "names_discovery.h"
#include "overlay.h"
#include "index_html.h"

static const uint16_t FRAME_W = 128;
static const uint16_t FRAME_H = 128;
static const size_t   FRAME_BYTES = (size_t)FRAME_W * FRAME_H * 2;  // 32768

static const int   NUM_SLOTS = 4;
static const int   GESTURES  = 3;            // short / long / double per slot
static const char* BUTTONS_PATH = "/buttons.txt";
static String slotPath(int n) { return "/" + String(n) + ".bin"; }

// Single-button gesture thresholds.
static const uint32_t SHORT_MAX_MS = 500;    // release before this = short click
static const uint32_t LONG_MIN_MS  = 700;    // held at least this  = long click
static const uint32_t DOUBLE_MS    = 1500;   // 2nd short within this of 1st = double

// Battery sense ADC. AtomS3R / AtomS3 read the pack through a 2:1 divider on
// GPIO 8 (the older Atom series uses GPIO 33). analogReadMilliVolts() already
// applies the eFuse calibration, so we just double it back to pack volts.
static const int      BAT_ADC_PIN = 8;
static const int      BAT_FULL_MV = 4200;  // 1S LiPo at 100%
static const int      BAT_EMPTY_MV = 3300; // treat as 0%

// SoftAP fallback credentials. Password must be >= 8 chars, or "" for open.
static const char* AP_SSID = "AtomFramer";
static const char* AP_PASS = "atomframer";

static const uint16_t DNS_PORT = 53;
static const uint32_t STA_TIMEOUT_MS = 20000;  // give up on a join after this
static const uint32_t SETTINGS_CONNECT_DELAY_MS = 250;
static const uint32_t LETTER_RETRY_DELAY_MS = 250;
static const uint32_t LETTER_FEEDBACK_MS = 1500;
static const uint8_t  LETTER_MAX_ATTEMPTS = 3;

WebServer server(80);
DNSServer dnsServer;
Preferences prefs;

static uint8_t frameBuf[FRAME_BYTES];
static size_t  frameLen = 0;     // bytes received so far in the current upload
static bool    frameOk  = false; // a full frame is currently stored/displayed
static int     markerId = -1;    // ArUco id of the current slot (-1 = unknown)
static int     curSlot  = 0;
static bool    slotFilled[NUM_SLOTS] = { false };

static String  lineBuf;          // serial line accumulator
static String  pendingSsid;      // ssid we're currently trying to join
static bool    staConnecting = false;
static uint32_t staDeadline = 0;
static bool    settingsConnectPending = false;
static uint32_t settingsConnectAt = 0;
static String  settingsConnectSsid;
static String  settingsConnectPass;

enum class LetterSendState { Idle, Pending, WaitingRetry, Success, Failed };
enum class LetterAttemptResult { Ack, RetryableError, StationMissing };

struct LetterEvent {
  char letter = 'A';
  String session;
  uint32_t seq = 0;
  uint32_t atomSentMs = 0;
};

static char selectedLetter = 'A';
static uint32_t letterSequence = 0;
static String letterSession;
static bool letterButtonMode = false;
static LetterEvent pendingLetter;
static LetterSendState letterSendState = LetterSendState::Idle;
static uint8_t letterAttempt = 0;
static uint32_t letterNextAttemptAt = 0;
static bool letterDisplayRestorePending = false;
static uint32_t letterDisplayRestoreAt = 0;

static bool apActive() {
  auto m = WiFi.getMode();
  return m == WIFI_AP || m == WIFI_AP_STA;
}

static String jsonString(const String& value) {
  String escaped = "\"";
  escaped.reserve(value.length() + 2);
  for (size_t i = 0; i < value.length(); i++) {
    char c = value[i];
    switch (c) {
      case '\"': escaped += "\\\""; break;
      case '\\': escaped += "\\\\"; break;
      case '\b': escaped += "\\b"; break;
      case '\f': escaped += "\\f"; break;
      case '\n': escaped += "\\n"; break;
      case '\r': escaped += "\\r"; break;
      case '\t': escaped += "\\t"; break;
      default:
        if ((uint8_t)c < 0x20) {
          char unicodeEscape[7];
          snprintf(unicodeEscape, sizeof(unicodeEscape), "\\u%04x", (unsigned char)c);
          escaped += unicodeEscape;
        } else {
          escaped += c;
        }
    }
  }
  escaped += "\"";
  return escaped;
}

// Koostab igal käivitusel uue session'i tunnuse, mis püsib sama kuni restartini.
static String makeBootSession() {
  char value[17];
  snprintf(value, sizeof(value), "%08lx%08lx",
           (unsigned long)esp_random(), (unsigned long)esp_random());
  return String(value);
}

// Normaliseerib seadetes oleva jaama aadressi üheks API endpoint'iks.
static String normalizeStationUrl(String station) {
  station.trim();
  if (!station.length()) return "";
  if (!station.startsWith("http://") && !station.startsWith("https://")) {
    station = "http://" + station;
  }

  int authorityStart = station.indexOf("://") + 3;
  int pathStart = station.indexOf('/', authorityStart);
  if (pathStart < 0) pathStart = station.length();
  String authority = station.substring(authorityStart, pathStart);
  if (authority.indexOf(':') < 0) station = station.substring(0, pathStart) + ":5000" + station.substring(pathStart);

  while (station.endsWith("/")) station.remove(station.length() - 1);
  if (!station.endsWith("/api/letter")) station += "/api/letter";
  return station;
}

// ---- frame display + persistence -------------------------------------------
// Push frameBuf to the panel. The page packs RGB565 big-endian (high byte
// first); typing the source as swap565_t lets M5GFX convert from that byte
// order to the panel's native order itself (reading it as a plain uint16
// rotates the channels: red->blue, green->red, blue->green).
static void pushFrame() {
  M5.Display.startWrite();
  M5.Display.pushImage(0, 0, FRAME_W, FRAME_H, (const m5gfx::swap565_t*)frameBuf);
  M5.Display.endWrite();
  overlay::draw(M5.Display);   // labels/bars sit on top of the image
}

static String midKey(int n) { return "mid" + String(n); }

static void refreshFilled() {
  for (int i = 0; i < NUM_SLOTS; i++) slotFilled[i] = LittleFS.exists(slotPath(i));
}

// Write frameBuf (and the current marker id) to slot n.
static void saveSlot(int n) {
  File f = LittleFS.open(slotPath(n), "w");
  if (!f) { Serial.println("save: open failed"); return; }
  size_t w = f.write(frameBuf, FRAME_BYTES);
  f.close();
  prefs.putInt(midKey(n).c_str(), markerId);
  if (w != FRAME_BYTES) Serial.printf("save: short write %u/%u\n", (unsigned)w, (unsigned)FRAME_BYTES);
  else { slotFilled[n] = true; }
}

// Load slot n into frameBuf, display it, and make it the current slot.
static bool loadSlot(int n) {
  if (n < 0 || n >= NUM_SLOTS || !LittleFS.exists(slotPath(n))) return false;
  File f = LittleFS.open(slotPath(n), "r");
  if (!f) return false;
  size_t r = f.read(frameBuf, FRAME_BYTES);
  f.close();
  if (r != FRAME_BYTES) return false;
  markerId = prefs.getInt(midKey(n).c_str(), -1);
  curSlot = n;
  frameOk = true;
  prefs.putInt("slot", curSlot);
  pushFrame();
  return true;
}

// ---- hotspot / gesture URL table -------------------------------------------
// /buttons.txt is NUM_SLOTS*GESTURES newline-separated lines (slot*GESTURES + g,
// g = 0 short / 1 long / 2 double); an empty line means that gesture has no
// action. Read all GESTURES urls for a slot.
static void readSlotUrls(int slot, String out[GESTURES]) {
  for (int i = 0; i < GESTURES; i++) out[i] = "";
  File f = LittleFS.open(BUTTONS_PATH, "r");
  if (!f) return;
  int base = slot * GESTURES;
  int line = 0;
  while (f.available() && line < base + GESTURES) {
    String l = f.readStringUntil('\n');
    if (line >= base) { l.trim(); out[line - base] = l; }
    line++;
  }
  f.close();
}

// Fire an outbound HTTP GET to another unit. Blocking, but only for a button
// press. Never aim this at our own IP — the single-threaded WebServer would
// deadlock on a self-request; self actions are handled locally in doAction().
static void fireGet(const String& url) {
  Serial.println("GET " + url);
  HTTPClient http;
  http.setConnectTimeout(2000);
  http.setTimeout(3000);
  if (http.begin(url)) {
    int code = http.GET();
    Serial.printf("  -> %d\n", code);
    http.end();
  } else {
    Serial.println("  begin failed");
  }
}

// Run a gesture action. Self page-switches are done locally (no self HTTP):
//   "2"            -> show slot 2 here (bare number 1..NUM_SLOTS)
//   "/show?slot=1" -> show slot 1 here (0-based, self)
//   "http://ip/…"  -> GET another unit
static void doAction(String url) {
  url.trim();
  if (!url.length()) return;
  bool numeric = true;
  for (size_t i = 0; i < url.length(); i++) if (!isDigit(url[i])) { numeric = false; break; }
  if (numeric) {
    int n = url.toInt();
    if (n >= 1 && n <= NUM_SLOTS) { if (!loadSlot(n - 1)) Serial.printf("slot %d empty\n", n); }
    return;
  }
  if (url.startsWith("/show?slot=")) {
    if (!loadSlot(url.substring(11).toInt())) Serial.println("self slot empty");
    return;
  }
  if (url.startsWith("http://") || url.startsWith("https://")) { fireGet(url); return; }
  // "name:slot" — resolve a fleet name to its IP (slot is 1-based, like the UI).
  int colon = url.indexOf(':');
  if (colon > 0) {
    String nm = url.substring(0, colon);
    String rest = url.substring(colon + 1); rest.trim();
    bool restNum = rest.length() > 0;
    for (size_t i = 0; i < rest.length(); i++) if (!isDigit(rest[i])) restNum = false;
    if (restNum) {
      int sl = rest.toInt();
      if (nm == disco::name()) { if (!loadSlot(sl - 1)) Serial.println("self slot empty"); return; }
      IPAddress ip;
      if (disco::lookup(nm, ip)) fireGet("http://" + ip.toString() + "/show?slot=" + String(sl - 1));
      else Serial.println("action: name not found: " + nm);
      return;
    }
  }
  Serial.println("action: unsupported target: " + url);
}

// ---- battery ---------------------------------------------------------------
static int batteryMilliVolts() {
  return (int)analogReadMilliVolts(BAT_ADC_PIN) * 2;  // undo the 2:1 divider
}

static int batteryPercent(int mv) {
  int pct = (mv - BAT_EMPTY_MV) * 100 / (BAT_FULL_MV - BAT_EMPTY_MV);
  return pct < 0 ? 0 : (pct > 100 ? 100 : pct);
}

// ---- on-screen status ------------------------------------------------------
static void showStatus() {
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextColor(TFT_GREEN, TFT_BLACK);
  M5.Display.setTextSize(1);
  M5.Display.setCursor(3, 3);
  M5.Display.println(disco::name());
  M5.Display.setTextColor(0x8410, TFT_BLACK);  // dim grey
  if (WiFi.status() == WL_CONNECTED) {
    M5.Display.println("STA " + WiFi.SSID());
    M5.Display.setTextColor(TFT_GREEN, TFT_BLACK);
    M5.Display.println(WiFi.localIP().toString());
  } else if (staConnecting) {
    M5.Display.println("joining");
    M5.Display.println(pendingSsid);
  } else if (apActive()) {
    M5.Display.printf("AP %s\n", AP_SSID);
    if (AP_PASS[0]) M5.Display.printf("pw %s\n", AP_PASS);
    M5.Display.setTextColor(TFT_GREEN, TFT_BLACK);
    M5.Display.println(WiFi.softAPIP().toString());
  } else {
    M5.Display.println("offline");
  }
  overlay::draw(M5.Display);
}

// Repaint whatever should currently be on the panel. Called after an overlay
// change, since the labels/bars are drawn over the image rather than into it.
static void redraw() {
  if (frameOk) pushFrame();   // pushFrame() draws the overlay itself
  else showStatus();
}

// Tähevaade kasutab ekraani ajutiselt, kuid ei muuda frameBuf'i ega salvestatud slotte.
static void showLetterPanel(const char* heading, char letter) {
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextColor(TFT_GREEN, TFT_BLACK);
  M5.Display.setTextSize(2);
  M5.Display.setCursor(6, 8);
  M5.Display.println(heading);
  M5.Display.setTextSize(7);
  M5.Display.setCursor(43, 43);
  M5.Display.print(letter);
  M5.Display.setTextSize(1);
}

static void showSelectedLetter() {
  showLetterPanel("SELECTED", selectedLetter);
}

static void scheduleLetterDisplayRestore() {
  letterDisplayRestorePending = true;
  letterDisplayRestoreAt = millis() + LETTER_FEEDBACK_MS;
}

static bool letterSendBusy() {
  return letterSendState == LetterSendState::Pending
      || letterSendState == LetterSendState::WaitingRetry;
}

// Uus kasutaja sündmus saab seq väärtuse ainult siin; korduskatsed seda ei muuda.
static bool queueLetterEvent(char letter) {
  if (letterSendBusy()) return false;
  pendingLetter.letter = letter;
  pendingLetter.session = letterSession;
  pendingLetter.seq = ++letterSequence;
  pendingLetter.atomSentMs = millis();
  letterAttempt = 0;
  letterNextAttemptAt = millis();
  letterSendState = LetterSendState::Pending;
  letterDisplayRestorePending = false;
  Serial.printf("LETTER queue %c session=%s seq=%lu\n", letter,
                pendingLetter.session.c_str(), (unsigned long)pendingLetter.seq);
  showLetterPanel("SENDING", letter);
  return true;
}

// Teeb ühe piiratud HTTP katse. Korduskatsete ajastamine toimub loop()-is.
static LetterAttemptResult sendLetterToStation(const LetterEvent& event, String& detail) {
  String station = prefs.getString("station", "");
  String url = normalizeStationUrl(station);
  if (!url.length()) {
    detail = "station not configured";
    Serial.println("LETTER failed: station not configured");
    return LetterAttemptResult::StationMissing;
  }

  String letterValue(event.letter);
  String payload = "{\"letter\":" + jsonString(letterValue)
                 + ",\"session\":" + jsonString(event.session)
                 + ",\"seq\":" + String(event.seq)
                 + ",\"atom_sent_ms\":" + String(event.atomSentMs) + "}";

  Serial.printf("LETTER attempt %u %s\n", letterAttempt, url.c_str());
  HTTPClient http;
  http.setConnectTimeout(1000);
  http.setTimeout(2000);
  if (!http.begin(url)) {
    detail = "HTTP begin failed";
    Serial.println("LETTER HTTP begin failed");
    return LetterAttemptResult::RetryableError;
  }

  http.addHeader("Content-Type", "application/json");
  int code = http.POST(payload);
  String responseBody;
  if (code > 0) responseBody = http.getString();
  http.end();

  if (code == 200 || code == 202) {
    detail = responseBody;
    Serial.printf("LETTER ACK %d session=%s seq=%lu\n", code,
                  event.session.c_str(), (unsigned long)event.seq);
    return LetterAttemptResult::Ack;
  }

  if (code <= 0) detail = HTTPClient::errorToString(code);
  else detail = "HTTP " + String(code) + " " + responseBody;
  Serial.printf("LETTER error %s\n", detail.c_str());
  return LetterAttemptResult::RetryableError;
}

static void processLetterSender() {
  if (letterDisplayRestorePending
      && (int32_t)(millis() - letterDisplayRestoreAt) >= 0) {
    letterDisplayRestorePending = false;
    if (letterButtonMode) showSelectedLetter();
    else redraw();
  }

  if (!letterSendBusy()) return;
  if ((int32_t)(millis() - letterNextAttemptAt) < 0) return;

  letterAttempt++;
  String detail;
  LetterAttemptResult result = sendLetterToStation(pendingLetter, detail);
  if (result == LetterAttemptResult::Ack) {
    letterSendState = LetterSendState::Success;
    showLetterPanel("SENT", pendingLetter.letter);
    scheduleLetterDisplayRestore();
    return;
  }
  if (result == LetterAttemptResult::StationMissing) {
    letterSendState = LetterSendState::Failed;
    showLetterPanel("NO STATION", pendingLetter.letter);
    scheduleLetterDisplayRestore();
    return;
  }
  if (letterAttempt >= LETTER_MAX_ATTEMPTS) {
    letterSendState = LetterSendState::Failed;
    Serial.printf("LETTER failed session=%s seq=%lu after %u attempts\n",
                  pendingLetter.session.c_str(), (unsigned long)pendingLetter.seq,
                  letterAttempt);
    showLetterPanel("FAILED", pendingLetter.letter);
    scheduleLetterDisplayRestore();
    return;
  }

  letterSendState = LetterSendState::WaitingRetry;
  letterNextAttemptAt = millis() + LETTER_RETRY_DELAY_MS;
  Serial.printf("LETTER retry scheduled %u/%u\n", letterAttempt + 1,
                LETTER_MAX_ATTEMPTS);
}

// ---- serial reporting ------------------------------------------------------
static void reportIP() {
  if (WiFi.status() == WL_CONNECTED) {
    String ip = WiFi.localIP().toString();
    Serial.printf("IP %s  http://%s/  (STA \"%s\", RSSI %d)\n",
                  ip.c_str(), ip.c_str(), WiFi.SSID().c_str(), WiFi.RSSI());
  } else if (staConnecting) {
    Serial.println("joining... (no IP from DHCP yet — try 'ip' again)");
  } else if (apActive()) {
    String ip = WiFi.softAPIP().toString();
    Serial.printf("IP %s  http://%s/  (SoftAP \"%s\")\n", ip.c_str(), ip.c_str(), AP_SSID);
  } else {
    Serial.println("not connected");
  }
}

static void printHelp() {
  Serial.println("commands:");
  Serial.println("  wifi <ssid>:<password>  - join a network (saved, auto-reconnects)");
  Serial.println("  ip                      - show current IP");
  Serial.println("  status                  - mode / ssid / ip / rssi");
  Serial.println("  ap                      - forget wifi, start SoftAP");
  Serial.println("  help");
}

// ---- WiFi mode control -----------------------------------------------------
static void startAP() {
  staConnecting = false;
  dnsServer.stop();
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
  Serial.printf("SoftAP \"%s\"  http://%s/\n", AP_SSID, WiFi.softAPIP().toString().c_str());
  if (!frameOk) showStatus();  // keep a restored frame on screen; status is on the button
}

static void startSTA(const String& ssid, const String& pass, bool save) {
  if (save) {
    prefs.putString("ssid", ssid);
    prefs.putString("pass", pass);
  }
  dnsServer.stop();
  pendingSsid = ssid;
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);    // rejoin automatically after a WiFi-infra swap
  WiFi.begin(ssid.c_str(), pass.c_str());
  staConnecting = true;
  staDeadline = millis() + STA_TIMEOUT_MS;
  Serial.printf("joining \"%s\"... (waiting for DHCP)\n", ssid.c_str());
  if (!frameOk) showStatus();  // keep a restored frame on screen; status is on the button
}

// Watch a pending STA join; report the IP once DHCP lands, or fall back to AP.
static void pollSTA() {
  if (!staConnecting) return;
  if (WiFi.status() == WL_CONNECTED) {
    staConnecting = false;
    Serial.println("connected.");
    reportIP();
    if (frameOk) pushFrame(); else showStatus();  // don't wipe a restored frame
  } else if ((int32_t)(millis() - staDeadline) >= 0) {
    staConnecting = false;
    Serial.println("join timed out — starting SoftAP.");
    startAP();
  }
}

// ---- serial command parsing ------------------------------------------------
static void handleSerialLine(String line) {
  line.trim();
  if (!line.length()) return;
  if (line.equalsIgnoreCase("ip"))     { reportIP();  return; }
  if (line.equalsIgnoreCase("status")) { reportIP();  return; }
  if (line.equalsIgnoreCase("help"))   { printHelp(); return; }
  if (line.equalsIgnoreCase("name"))   { Serial.println("name: " + disco::name()); return; }
  if (line.length() > 5 && line.substring(0, 5).equalsIgnoreCase("name ")) {
    String nm = line.substring(5); nm.trim();
    if (nm.length()) { disco::setName(nm); Serial.println("renamed: " + disco::name()); }
    return;
  }
  if (line.equalsIgnoreCase("ap")) {
    prefs.remove("ssid");
    prefs.remove("pass");
    Serial.println("forgot saved wifi.");
    startAP();
    return;
  }

  // "wifi ssid:pass", or a bare "ssid:pass" line.
  String creds;
  if (line.length() > 5 && line.substring(0, 5).equalsIgnoreCase("wifi ")) {
    creds = line.substring(5);
  } else if (line.indexOf(':') >= 0) {
    creds = line;
  } else {
    Serial.println("unknown command — type 'help'");
    return;
  }
  creds.trim();
  int colon = creds.indexOf(':');               // split on the FIRST colon
  if (colon < 0) { Serial.println("usage: wifi <ssid>:<password>"); return; }
  String ssid = creds.substring(0, colon); ssid.trim();
  String pass = creds.substring(colon + 1);      // password may contain colons
  if (!ssid.length()) { Serial.println("empty ssid"); return; }
  startSTA(ssid, pass, true);
}

static void pumpSerial() {
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\r') continue;
    if (c == '\n') { handleSerialLine(lineBuf); lineBuf = ""; }
    else if (lineBuf.length() < 160) lineBuf += c;
  }
}

// ---- single-button gestures ------------------------------------------------
static uint32_t pressStart   = 0;
static bool     shortPending = false;
static uint32_t firstShortAt = 0;

// Run the current slot's action for gesture g (0 short / 1 long / 2 double).
// An unconfigured long-press falls back to showing WiFi/IP status on the LCD.
static void runGesture(int g) {
  String urls[GESTURES];
  readSlotUrls(curSlot, urls);
  String url = urls[g];
  Serial.printf("gesture %d -> \"%s\"\n", g, url.c_str());
  if (url.length()) doAction(url);
  else if (g == 1) showStatus();
}

// Tähe nupurežiim on eraldi valik, et olemasolevad sloti žestid säiliksid.
static void runButtonAction(int g) {
  if (!letterButtonMode) {
    runGesture(g);
    return;
  }
  if (g == 0) {
    selectedLetter = selectedLetter == 'Z' ? 'A' : selectedLetter + 1;
    Serial.printf("LETTER selected %c\n", selectedLetter);
    showSelectedLetter();
  } else if (g == 1) {
    if (!queueLetterEvent(selectedLetter)) {
      Serial.println("LETTER queue rejected: sender busy");
    }
  } else {
    Serial.println("LETTER double click ignored in letter button mode");
  }
}

// Tähe nupurežiimis töödeldakse lühike vajutus kohe vabastamisel. Tavarežiimi
// short/long/double loogika ja double-click'i ooteaken jäävad muutmata.
static void pumpButton() {
  if (M5.BtnA.wasPressed())  pressStart = millis();
  if (M5.BtnA.wasReleased()) {
    uint32_t dur = millis() - pressStart;
    if (letterButtonMode) {
      shortPending = false;
      if (dur >= LONG_MIN_MS) {
        runButtonAction(1);                            // long
      } else if (dur < SHORT_MAX_MS) {
        runButtonAction(0);                            // short kohe pärast release'i
      }
      return;
    }

    if (dur >= LONG_MIN_MS) {
      shortPending = false;
      runButtonAction(1);                              // long
    } else if (dur < SHORT_MAX_MS) {
      if (shortPending && (millis() - firstShortAt) <= DOUBLE_MS) {
        shortPending = false;
        runButtonAction(2);                            // double
      } else {
        shortPending = true;
        firstShortAt = millis();
      }
    }
  }
  if (!letterButtonMode && shortPending && (millis() - firstShortAt) > DOUBLE_MS) {
    shortPending = false;
    runButtonAction(0);                                // single short
  }
}

// ---- HTTP handlers ---------------------------------------------------------
static void handleRoot() {
  server.send_P(200, "text/html", INDEX_HTML);
}

static void handleSettingsGet() {
  String mode = "offline";
  String ip;
  wifi_mode_t wifiMode = WiFi.getMode();
  if (wifiMode == WIFI_AP || wifiMode == WIFI_AP_STA) {
    mode = "ap";
    ip = WiFi.softAPIP().toString();
  } else if (wifiMode == WIFI_STA) {
    mode = "sta";
    if (WiFi.status() == WL_CONNECTED) ip = WiFi.localIP().toString();
  }

  String response = "{\"ssid\":" + jsonString(prefs.getString("ssid", ""))
                  + ",\"station\":" + jsonString(prefs.getString("station", ""))
                  + ",\"mode\":" + jsonString(mode)
                  + ",\"ip\":" + jsonString(ip)
                  + ",\"letter_mode\":" + (letterButtonMode ? "true" : "false") + "}";
  server.send(200, "application/json", response);
}

static void handleSettingsPost() {
  String station = server.arg("station");
  String ssid = server.arg("ssid");
  String pass = server.arg("pass");
  bool connect = server.hasArg("connect") && server.arg("connect") == "1";
  bool previousLetterButtonMode = letterButtonMode;

  station.trim();
  ssid.trim();
  prefs.putString("station", station);
  if (server.hasArg("letter_mode")) {
    letterButtonMode = server.arg("letter_mode") == "1";
    prefs.putBool("letterMode", letterButtonMode);
  }
  if (ssid.length()) {
    prefs.putString("ssid", ssid);
    prefs.putString("pass", pass);
  }
  if (letterButtonMode != previousLetterButtonMode) {
    shortPending = false;
    if (letterButtonMode) showSelectedLetter();
    else redraw();
  }

  if (!connect) {
    server.send(200, "application/json", "{\"saved\":true,\"connecting\":false}");
    return;
  }

  String connectSsid = ssid.length() ? ssid : prefs.getString("ssid", "");
  String connectPass = ssid.length() ? pass : prefs.getString("pass", "");
  if (!connectSsid.length()) {
    server.send(400, "application/json", "{\"error\":\"ssid required for connect\"}");
    return;
  }

  server.send(200, "application/json", "{\"saved\":true,\"connecting\":true}");
  settingsConnectSsid = connectSsid;
  settingsConnectPass = connectPass;
  settingsConnectPending = true;
  settingsConnectAt = millis() + SETTINGS_CONNECT_DELAY_MS;
}

static void handleDisplayTest() {
  showStatus();
  server.send(200, "application/json", "{\"displayed\":true}");
}

static void handleLetterTest() {
  String value = server.arg("letter");
  value.trim();
  value.toUpperCase();
  if (value.length() != 1 || value[0] < 'A' || value[0] > 'Z') {
    server.send(400, "application/json", "{\"error\":\"letter must be A-Z\"}");
    return;
  }
  if (letterSendBusy()) {
    server.send(409, "application/json", "{\"error\":\"letter sender busy\"}");
    return;
  }

  selectedLetter = value[0];
  queueLetterEvent(selectedLetter);
  String response = "{\"queued\":true,\"state\":\"pending\",\"letter\":"
                  + jsonString(value) + ",\"session\":"
                  + jsonString(pendingLetter.session) + ",\"seq\":"
                  + String(pendingLetter.seq) + "}";
  server.send(202, "application/json", response);
}

static void processSettingsConnect() {
  if (!settingsConnectPending) return;
  if ((int32_t)(millis() - settingsConnectAt) < 0) return;
  settingsConnectPending = false;
  startSTA(settingsConnectSsid, settingsConnectPass, false);
}

// Multipart file upload: binary-safe, streamed in chunks by the core WebServer.
static void handleFrameUpload() {
  HTTPUpload& up = server.upload();
  if (up.status == UPLOAD_FILE_START) {
    frameLen = 0;
    frameOk = false;
  } else if (up.status == UPLOAD_FILE_WRITE) {
    size_t n = up.currentSize;
    if (frameLen + n > FRAME_BYTES) n = FRAME_BYTES - frameLen;  // clamp overflow
    memcpy(frameBuf + frameLen, up.buf, n);
    frameLen += n;
  } else if (up.status == UPLOAD_FILE_END) {
    frameOk = (frameLen == FRAME_BYTES);
  }
}

// Runs after the upload completes; stores the frame into the target slot
// (?slot=N, default = current) and displays it. ?mid=<n> records the ArUco id.
static void handleFrameDone() {
  if (!frameOk) {
    server.send(400, "text/plain", "expected 32768 bytes of RGB565");
    return;
  }
  int slot = server.hasArg("slot") ? server.arg("slot").toInt() : curSlot;
  if (slot < 0 || slot >= NUM_SLOTS) slot = curSlot;
  markerId = server.hasArg("mid") ? server.arg("mid").toInt() : -1;
  saveSlot(slot);
  curSlot = slot;
  prefs.putInt("slot", curSlot);
  pushFrame();
  server.send(200, "text/plain", "OK slot " + String(slot));
}

// GET /frame?slot=N — stream a slot's raw 32768-byte RGB565 back (read-back).
static void handleFrameGet() {
  int slot = server.hasArg("slot") ? server.arg("slot").toInt() : curSlot;
  if (slot < 0 || slot >= NUM_SLOTS) { server.send(400, "text/plain", "slot 0..3"); return; }
  if (!LittleFS.exists(slotPath(slot))) { server.send(404, "text/plain", "slot empty"); return; }
  File f = LittleFS.open(slotPath(slot), "r");
  if (!f) { server.send(500, "text/plain", "open failed"); return; }
  server.streamFile(f, "application/octet-stream");
  f.close();
}

// GET /show?slot=N — display a stored slot. This is the gesture GET target.
static void handleShow() {
  int slot = server.hasArg("slot") ? server.arg("slot").toInt() : -1;
  if (slot < 0 || slot >= NUM_SLOTS) { server.send(400, "text/plain", "slot 0..3"); return; }
  if (!loadSlot(slot)) { server.send(404, "text/plain", "slot empty"); return; }
  server.send(200, "text/plain", "OK slot " + String(slot));
}

static void handleButtonsGet() {
  String body;
  File f = LittleFS.open(BUTTONS_PATH, "r");
  if (f) { body = f.readString(); f.close(); }
  server.send(200, "text/plain", body);
}

static void handleButtonsPost() {
  String body = server.arg("plain");
  File f = LittleFS.open(BUTTONS_PATH, "w");
  if (!f) { server.send(500, "text/plain", "open failed"); return; }
  f.print(body);
  f.close();
  server.send(200, "text/plain", "OK");
}

// GET /state: current device state. Keeps markerId + battery for atom-manager,
// adds the current slot and which slots are filled.
static void handleState() {
  int mv = batteryMilliVolts();
  String filled = "[";
  for (int i = 0; i < NUM_SLOTS; i++) {
    filled += slotFilled[i] ? "true" : "false";
    if (i < NUM_SLOTS - 1) filled += ",";
  }
  filled += "]";
  String s = "{\"name\":\"" + disco::name() + "\""
           + ",\"slot\":" + String(curSlot)
           + ",\"slots\":" + String(NUM_SLOTS)
           + ",\"filled\":" + filled
           + ",\"markerId\":" + String(markerId)
           + ",\"hasFrame\":" + (frameOk ? "true" : "false")
           + ",\"battery\":{\"mv\":" + String(mv)
           + ",\"pct\":" + String(batteryPercent(mv)) + "}"
           + ",\"overlay\":" + overlay::json() + "}";
  server.send(200, "application/json", s);
}

// GET /set?l0=…&n0=…&b0=90 — update the data overlay and repaint. Every
// recognised parameter in the query is applied, so one request can set several
// values at once. Unknown keys are named in a 400 rather than ignored silently.
static void handleSet() {
  String bad;
  for (int i = 0; i < server.args(); i++) {
    if (server.argName(i) == "plain") continue;
    if (!overlay::applyParam(server.argName(i), server.arg(i))) {
      if (bad.length()) bad += ",";
      bad += server.argName(i);
    }
  }
  redraw();
  if (bad.length()) server.send(400, "text/plain", "unknown parameter(s): " + bad);
  else              server.send(200, "application/json", overlay::json());
}

// GET /overlay — the current overlay values, without changing anything.
static void handleOverlay() { server.send(200, "application/json", overlay::json()); }

static void handlePeers() { server.send(200, "application/json", disco::peersJson()); }

static void handleNameGet() { server.send(200, "text/plain", disco::name()); }

static void handleNamePost() {
  String nm = server.arg("plain"); nm.trim();
  if (!nm.length()) { server.send(400, "text/plain", "empty name"); return; }
  disco::setName(nm);
  server.send(200, "text/plain", disco::name());
}

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);
  M5.Display.setRotation(0);

  // Battery ADC: 12-bit, 2:1 divider on GPIO 8 (see batteryMilliVolts()).
  pinMode(BAT_ADC_PIN, INPUT);
  analogReadResolution(12);

  Serial.begin(115200);
  prefs.begin("wifi", false);
  letterSession = makeBootSession();
  letterButtonMode = prefs.getBool("letterMode", false);

  // Mount flash and restore the last-shown slot immediately, so the panel comes
  // straight up on its page — no boot animation, no status screen, no WiFi wait.
  if (!LittleFS.begin(true)) Serial.println("LittleFS mount failed — slots won't persist");
  refreshFilled();
  curSlot = prefs.getInt("slot", 0);
  if (curSlot < 0 || curSlot >= NUM_SLOTS) curSlot = 0;
  if (slotFilled[curSlot]) loadSlot(curSlot);   // displays it and sets frameOk

  // Bring WiFi up FIRST — this initialises the lwIP/TCP-IP stack. Starting the
  // web server before any WiFi.mode() call asserts "Invalid mbox" in lwIP.
  String savedSsid = prefs.getString("ssid", "");
  if (savedSsid.length()) startSTA(savedSsid, prefs.getString("pass", ""), false);
  else startAP();

  disco::begin();   // random name (persisted) + UDP discovery, after WiFi is up

  server.on("/", HTTP_GET, handleRoot);
  server.on("/generate_204", HTTP_GET, handleRoot);
  server.on("/gen_204", HTTP_GET, handleRoot);
  server.on("/hotspot-detect.html", HTTP_GET, handleRoot);
  server.on("/library/test/success.html", HTTP_GET, handleRoot);
  server.on("/connecttest.txt", HTTP_GET, handleRoot);
  server.on("/ncsi.txt", HTTP_GET, handleRoot);
  server.on("/redirect", HTTP_GET, handleRoot);
  server.on("/canonical.html", HTTP_GET, handleRoot);
  server.on("/success.txt", HTTP_GET, handleRoot);
  server.on("/settings", HTTP_GET, handleSettingsGet);
  server.on("/settings", HTTP_POST, handleSettingsPost);
  server.on("/test/display", HTTP_POST, handleDisplayTest);
  server.on("/test/letter", HTTP_POST, handleLetterTest);
  server.on("/state", HTTP_GET, handleState);
  server.on("/set", HTTP_GET, handleSet);
  server.on("/overlay", HTTP_GET, handleOverlay);
  server.on("/peers", HTTP_GET, handlePeers);
  server.on("/name", HTTP_GET, handleNameGet);
  server.on("/name", HTTP_POST, handleNamePost);
  server.on("/show", HTTP_GET, handleShow);
  server.on("/buttons", HTTP_GET, handleButtonsGet);
  server.on("/buttons", HTTP_POST, handleButtonsPost);
  server.on("/frame", HTTP_GET, handleFrameGet);
  // POST /frame: (responder, upload-handler) — the upload handler fires first.
  server.on("/frame", HTTP_POST, handleFrameDone, handleFrameUpload);
  server.onNotFound([]() {
    if (apActive()) handleRoot();
    else server.send(404, "text/plain", "Not found");
  });
  server.begin();
  if (letterButtonMode) showSelectedLetter();

  Serial.println();
  Serial.printf("ATOM FRAMER ready — name \"%s\".\n", disco::name().c_str());
  Serial.printf("LETTER session=%s\n", letterSession.c_str());
  printHelp();
}

void loop() {
  M5.update();
  server.handleClient();
  processSettingsConnect();
  processLetterSender();
  pumpSerial();
  pollSTA();
  if (apActive()) dnsServer.processNextRequest();
  disco::loop();   // UDP announce + peer table upkeep
  pumpButton();    // short / long / double click -> the current slot's actions
}
