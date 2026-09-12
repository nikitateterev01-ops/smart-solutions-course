// Name + UDP discovery for the ESP image-kiosk fleet.
//
// Shared by both firmwares — the separate PlatformIO projects pick this up via
// `-I $PROJECT_DIR/../common` in their platformio.ini.
//
// Each device gets a persistent random "<colour>_<tree>" name (e.g. red_oak),
// broadcasts "<MAGIC> <name> <ip>" over UDP every few seconds, and listens to
// build a name -> ip table. That lets hotspot/gesture actions address peers by
// name ("red_oak:1") instead of a brittle IP, and it keeps working across a
// WiFi-infrastructure swap: when a device rejoins (same SSID) on a new router it
// gets a new IP and re-broadcasts it, so every peer's table refreshes within a
// broadcast cycle — back to normal in well under a minute.
//
// Single-translation-unit use (each firmware compiles one main.cpp that includes
// this once), so plain file-scope statics are fine.
#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <Preferences.h>
#include <esp_random.h>

namespace disco {

static const uint16_t PORT           = 50505;
static const uint32_t BCAST_EVERY_MS = 8000;    // re-announce this often
static const uint32_t PEER_TTL_MS    = 90000;   // forget a peer unheard this long
static const int      MAX_PEERS      = 16;
static const char*    MAGIC          = "ESPKIOSK1";

// "<colour>_<tree>" — colour/quality adjective + tree noun.
static const char* const ADJ[] = {
  "red","blue","green","gold","gray","white","black","amber","jade","rust",
  "teal","pink","ivory","coral","slate","mint","copper","crimson","azure","olive",
  "cyan","plum","sand","ash","frost","ember","dusk","dawn","moss","navy","ruby","lime"
};
static const char* const NOUN[] = {
  "oak","pine","elm","ash","fir","birch","cedar","maple","willow","aspen",
  "beech","hazel","alder","spruce","poplar","yew","rowan","larch","holly","linden",
  "walnut","cypress","juniper","sequoia","magnolia","sycamore","hawthorn","redwood",
  "dogwood","mulberry","laurel","cherry"
};
static const int N_ADJ  = sizeof(ADJ)  / sizeof(ADJ[0]);
static const int N_NOUN = sizeof(NOUN) / sizeof(NOUN[0]);

struct Peer { String name; IPAddress ip; uint32_t seen; };

static WiFiUDP     _udp;
static Preferences _prefs;
static String      _name;
static Peer        _peers[MAX_PEERS];
static int         _peerCount = 0;
static uint32_t    _lastBcast = 0;

static String randomName() {
  uint32_t r = esp_random();
  return String(ADJ[r % N_ADJ]) + "_" + NOUN[(r >> 8) % N_NOUN];
}

static String name() { return _name; }

static void setName(const String& nm) {
  _name = nm;
  _prefs.putString("name", nm);
}

// Call after WiFi is up (needs the network stack + RF on for esp_random()).
static void begin() {
  _prefs.begin("ident", false);
  _name = _prefs.getString("name", "");
  if (!_name.length()) { _name = randomName(); _prefs.putString("name", _name); }
  _udp.begin(PORT);
}

static IPAddress myIP() {
  return (WiFi.status() == WL_CONNECTED) ? WiFi.localIP() : WiFi.softAPIP();
}

static void broadcast() {
  IPAddress ip = myIP();
  if ((uint32_t)ip == 0) return;                 // no address yet
  String msg = String(MAGIC) + " " + _name + " " + ip.toString();
  _udp.beginPacket(IPAddress(255, 255, 255, 255), PORT);
  _udp.write((const uint8_t*)msg.c_str(), msg.length());
  _udp.endPacket();
}

static void updatePeer(const String& nm, IPAddress ip) {
  if (nm == _name) return;                        // ignore our own echo
  for (int i = 0; i < _peerCount; i++)
    if (_peers[i].name == nm) { _peers[i].ip = ip; _peers[i].seen = millis(); return; }
  int idx;
  if (_peerCount < MAX_PEERS) idx = _peerCount++;
  else { idx = 0; for (int i = 1; i < _peerCount; i++) if (_peers[i].seen < _peers[idx].seen) idx = i; }
  _peers[idx].name = nm; _peers[idx].ip = ip; _peers[idx].seen = millis();
}

static void poll() {
  int sz = _udp.parsePacket();
  if (sz <= 0) return;
  char buf[128];
  int n = _udp.read(buf, sizeof(buf) - 1);
  if (n <= 0) return;
  buf[n] = 0;
  String s(buf);
  if (!s.startsWith(MAGIC)) return;
  int a = s.indexOf(' ');
  int b = s.indexOf(' ', a + 1);
  if (a < 0 || b < 0) return;
  String nm = s.substring(a + 1, b);
  String ips = s.substring(b + 1); ips.trim();
  IPAddress ip;
  if (ip.fromString(ips)) updatePeer(nm, ip);
}

// Call every loop().
static void loop() {
  poll();
  uint32_t now = millis();
  if (now - _lastBcast >= BCAST_EVERY_MS) { _lastBcast = now; broadcast(); }
}

// Resolve a peer name to its IP (fresh entries only).
static bool lookup(const String& nm, IPAddress& out) {
  for (int i = 0; i < _peerCount; i++)
    if (_peers[i].name == nm && (millis() - _peers[i].seen) < PEER_TTL_MS) { out = _peers[i].ip; return true; }
  return false;
}

// JSON array of currently-known peers for the framer UI.
static String peersJson() {
  String s = "[";
  bool first = true;
  for (int i = 0; i < _peerCount; i++) {
    if ((millis() - _peers[i].seen) >= PEER_TTL_MS) continue;
    if (!first) s += ",";
    first = false;
    s += "{\"name\":\"" + _peers[i].name + "\",\"ip\":\"" + _peers[i].ip.toString()
       + "\",\"age\":" + String((millis() - _peers[i].seen) / 1000) + "}";
  }
  s += "]";
  return s;
}

}  // namespace disco
