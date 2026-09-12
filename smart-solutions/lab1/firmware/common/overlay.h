// Data overlay for the ESP image kiosk: a small, fixed set of text labels and
// percentage bars drawn on top of the displayed image.
//
// The point is that a host can push a picture once and then keep only the
// *numbers* fresh with cheap GET requests — no re-upload, no redraw work on the
// host side:
//
//   GET /set?b0=90                 bar 0 -> 90%
//   GET /set?n0=PRESSURE&b0=90     bar 0's caption + value in one call
//   GET /set?l0=SYSTEM%20READY     text label 0
//   GET /set?b0=-1                 hide bar 0
//   GET /set?clear=1               drop every label and bar
//   GET /set?on=0                  keep the values, stop drawing them
//
// Values live in RAM only (they are live telemetry, not configuration), so a
// reboot comes back to the bare image. Layout is derived from the panel size, so
// the same code lays out sensibly on a 128x128 Atom and a 320x480 WT32.
//
// Templated on the display type rather than typed against a base class: M5GFX
// ships as namespace m5gfx and LovyanGFX as namespace lgfx, so the two boards
// share no base type even though the drawing API is identical.
//
// Single-translation-unit use (each firmware compiles one main.cpp that includes
// this once), so plain file-scope statics are fine.
#pragma once
#include <Arduino.h>

namespace overlay {

static const int MAX_LABELS = 4;
static const int MAX_BARS   = 4;

// RGB565, spelled out so this header needs no TFT_*/M5 colour macros.
static const uint16_t COL_TEXT  = 0xFFFF;  // white
static const uint16_t COL_DIM   = 0xC618;  // light grey
static const uint16_t COL_PLATE = 0x0000;  // black backing behind text
static const uint16_t COL_FILL  = 0x07E0;  // green bar fill
static const uint16_t COL_TRACK = 0x4208;  // dark grey bar track

static String _label[MAX_LABELS];    // "" = that label is unused
static String _barName[MAX_BARS];    // caption drawn above its bar ("" = none)
static int    _bar[MAX_BARS] = { -1, -1, -1, -1 };  // 0..100, -1 = unused
static bool   _enabled = true;       // master on/off, GET /set?on=0

static bool enabled() { return _enabled; }
static void setEnabled(bool on) { _enabled = on; }

// True when there is nothing to draw, so a caller can skip the redraw entirely.
static bool empty() {
  for (int i = 0; i < MAX_LABELS; i++) if (_label[i].length()) return false;
  for (int i = 0; i < MAX_BARS; i++)   if (_bar[i] >= 0 || _barName[i].length()) return false;
  return true;
}

static void clear() {
  for (int i = 0; i < MAX_LABELS; i++) _label[i] = "";
  for (int i = 0; i < MAX_BARS; i++)   { _barName[i] = ""; _bar[i] = -1; }
}

static void setLabel(int i, const String& text) {
  if (i >= 0 && i < MAX_LABELS) _label[i] = text;
}

static void setBarName(int i, const String& text) {
  if (i >= 0 && i < MAX_BARS) _barName[i] = text;
}

// pct < 0 hides the bar; anything else is clamped into 0..100.
static void setBar(int i, int pct) {
  if (i < 0 || i >= MAX_BARS) return;
  _bar[i] = (pct < 0) ? -1 : (pct > 100 ? 100 : pct);
}

// ---- request parsing --------------------------------------------------------
// Apply one query parameter. Recognised keys: l0..l3 (label text), b0..b3 (bar
// percent), n0..n3 (bar caption), on (0/1), clear. Returns false for anything
// else so the caller can report an unknown key instead of silently ignoring it.
static bool applyParam(const String& key, const String& value) {
  if (key == "clear") { clear(); return true; }
  if (key == "on")    { setEnabled(value != "0" && !value.equalsIgnoreCase("false")); return true; }
  if (key.length() != 2 || !isDigit(key[1])) return false;
  int i = key[1] - '0';
  switch (key[0]) {
    case 'l': if (i >= MAX_LABELS) return false; setLabel(i, value);   return true;
    case 'n': if (i >= MAX_BARS)   return false; setBarName(i, value); return true;
    // An empty value ("?b0=") hides the bar, same as -1 — convenient from a
    // shell, where an unset variable expands to nothing.
    case 'b': if (i >= MAX_BARS)   return false;
              setBar(i, value.length() ? value.toInt() : -1);          return true;
  }
  return false;
}

static String json() {
  String s = String("{\"enabled\":") + (_enabled ? "true" : "false") + ",\"labels\":[";
  for (int i = 0; i < MAX_LABELS; i++) { if (i) s += ","; s += "\"" + _label[i] + "\""; }
  s += "],\"bars\":[";
  for (int i = 0; i < MAX_BARS; i++) {
    if (i) s += ",";
    s += "{\"name\":\"" + _barName[i] + "\",\"pct\":" + String(_bar[i]) + "}";
  }
  return s + "]}";
}

// ---- drawing ----------------------------------------------------------------
// Labels stack down from the top-left; bars stack up from the bottom, each under
// a caption line carrying its name and percentage. Sizes come off the panel
// width so a 128 px and a 480 px panel both stay legible.
//
// Text is drawn with an opaque background (setTextColor(fg, bg)) so it stays
// readable over any image — the panels have no alpha compositing.
//
// Draw this straight after pushing the frame; it paints over the image in the
// framebuffer-less sense (the stored slot bytes are never modified, so the next
// pushFrame() wipes the overlay clean).
template <class GFX>
static void draw(GFX& d) {
  if (!_enabled) return;

  const int W = d.width(), H = d.height();
  const int scale = (W >= 240) ? 2 : 1;       // the built-in font is 6x8 px per unit
  const int lineH = 8 * scale;
  const int pad   = (W >= 240) ? 6 : 2;
  const int barH  = (W >= 240) ? 14 : 6;
  const int barW  = W - 2 * pad;

  d.setTextSize(scale);

  int y = pad;
  for (int i = 0; i < MAX_LABELS; i++) {
    if (!_label[i].length()) continue;
    d.setTextColor(COL_TEXT, COL_PLATE);
    d.setCursor(pad, y);
    d.print(_label[i]);
    y += lineH + pad / 2;
  }

  // Bottom-up, so bar 0 keeps the same position no matter how many are in use.
  int by = H - pad;
  for (int i = 0; i < MAX_BARS; i++) {
    if (_bar[i] < 0 && !_barName[i].length()) continue;
    if (_bar[i] >= 0) {
      by -= barH;
      d.fillRect(pad, by, barW, barH, COL_TRACK);
      int fill = (int)((long)barW * _bar[i] / 100);
      if (fill > 0) d.fillRect(pad, by, fill, barH, COL_FILL);
      d.drawRect(pad, by, barW, barH, COL_DIM);
    }
    // Caption line: name on the left, "90%" right-aligned.
    by -= lineH;
    if (_barName[i].length()) {
      d.setTextColor(COL_DIM, COL_PLATE);
      d.setCursor(pad, by);
      d.print(_barName[i]);
    }
    if (_bar[i] >= 0) {
      String pctText = String(_bar[i]) + "%";
      d.setTextColor(COL_TEXT, COL_PLATE);
      d.setCursor(W - pad - (int)pctText.length() * 6 * scale, by);
      d.print(pctText);
    }
    by -= pad;
  }
}

}  // namespace overlay
