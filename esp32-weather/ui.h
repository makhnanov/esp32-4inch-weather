#pragma once
#include <math.h>
#include <stdio.h>
#include "config.h"
#include "display.h"
#include "fonts_ru.h"
#include "icons.h"
#include "theme.h"
#include "weather.h"

// ---------------------------------------------------------------------------
// Layout (landscape 480x320)
//
//   +--------------------------------------------------+  0
//   | Алматы | Омск | Томск | Астана | Питер           |  tab bar
//   +--------------------------------------------------+  46
//   |  СЕЙЧАС             |  ЗАВТРА                    |
//   |   12°  [icon]       |   18°  [icon]              |  temperature beside
//   |  Пасмурно           |  Пасмурно                  |  the weather art
//   |  ---------------    |  ---------------           |
//   |   61%     700 мм    |   56%     698 мм           |  the two big numbers
//   |  Влажность Давление |  Влажность Давление        |
//   |  Осадки      0.0 мм |  Осадки          0%        |
//   |  Мин/макс   9°/20°  |  Ночью           9°        |
//   +--------------------------------------------------+  292
//   | status                              обновлено .. |
//   +--------------------------------------------------+  320
// ---------------------------------------------------------------------------

static constexpr int TAB_H     = 46;
static constexpr int PANEL_TOP = 50;
static constexpr int PANEL_BOT = 292;
static constexpr int PANEL_H   = PANEL_BOT - PANEL_TOP;
static constexpr int PANEL_W   = 234;
static constexpr int PANEL_L_X = 4;
static constexpr int PANEL_R_X = 242;
static constexpr int PAD       = 14;
static constexpr int STATUS_Y  = 296;

// Vertical anchors inside a panel
static constexpr int Y_TITLE = PANEL_TOP + 16;
static constexpr int Y_BAND  = PANEL_TOP + 62;   // temperature and icon
static constexpr int Y_DESC  = PANEL_TOP + 108;
static constexpr int Y_RULE  = PANEL_TOP + 120;
static constexpr int Y_STAT  = PANEL_TOP + 142;  // the big stat numbers
static constexpr int Y_STATL = PANEL_TOP + 165;  // their captions
static constexpr int Y_ROW1  = PANEL_TOP + 192;
static constexpr int Y_ROW2  = PANEL_TOP + 218;

// Horizontal anchors inside a panel. Putting the temperature beside the art
// rather than under it frees the vertical room the big faces need.
static constexpr int TEMP_CX   = 74;
static constexpr int ICON_CX   = 178;
static constexpr int ICON_SZ   = 66;
static constexpr int STAT_L_CX = 60;
static constexpr int STAT_R_CX = 174;

// Local forecasts quote station pressure in millimetres of mercury.
static constexpr float HPA_TO_MMHG = 0.750062f;

// ---------------------------------------------------------------------------
// Temperatures. None of the fonts carries U+00B0, so the degree mark is drawn
// as a ring -- which also scales with the digits better than a glyph would.
// The caller sets the font first; `cy` is the vertical centre of the digits.
// ---------------------------------------------------------------------------

inline void fmtTemp(float v, char* buf, size_t n) {
  int r = (int)lroundf(v);
  if (r == 0) r = 0;  // fold -0 into 0
  snprintf(buf, n, "%d", r);
}

inline int measureTemp(lgfx::LGFXBase& g, float v, int ringR) {
  char buf[8];
  fmtTemp(v, buf, sizeof(buf));
  return g.textWidth(buf) + ringR * 3;
}

// Draws "<v>°" from x, returns the x advance.
inline int putTemp(lgfx::LGFXBase& g, int x, int cy, float v, uint16_t col,
                   int ringR, int ringLift) {
  char buf[8];
  fmtTemp(v, buf, sizeof(buf));
  const int tw = g.textWidth(buf);
  g.setTextColor(col);
  g.setTextDatum(lgfx::middle_left);
  g.drawString(buf, x, cy);
  const int rcx = x + tw + ringR * 2;
  const int rcy = cy - ringLift;
  g.drawCircle(rcx, rcy, ringR, col);
  if (ringR >= 4) g.drawCircle(rcx, rcy, ringR - 1, col);
  if (ringR >= 6) g.drawCircle(rcx, rcy, ringR - 2, col);  // heavier stroke
  return tw + ringR * 3;
}

// ---------------------------------------------------------------------------
// Tabs
// ---------------------------------------------------------------------------

inline int tabWidth(uint8_t i) {
  const int w = SCREEN_W / CITY_COUNT;
  return (i == CITY_COUNT - 1) ? SCREEN_W - w * (CITY_COUNT - 1) : w;
}
inline int tabX(uint8_t i) { return (SCREEN_W / CITY_COUNT) * i; }

inline void drawTabs(Display& g, uint8_t sel) {
  for (uint8_t i = 0; i < CITY_COUNT; i++) {
    const int x = tabX(i), w = tabWidth(i);
    const bool on = (i == sel);
    g.fillRect(x, 0, w, TAB_H, on ? C_TAB_ON : C_TAB_OFF);
    if (i) g.drawFastVLine(x, 8, TAB_H - 16, C_DIVIDER);
    if (on) g.fillRect(x, TAB_H - 4, w, 4, C_ACCENT);
    g.setFont(&FontRU15);
    g.setTextColor(on ? C_TEXT : C_TEXT_DIM);
    g.setTextDatum(lgfx::middle_center);
    g.drawString(CITIES[i].name, x + w / 2, TAB_H / 2 - 1);
  }
  g.drawFastHLine(0, TAB_H, SCREEN_W, C_DIVIDER);
}

// Returns the tab index under a touch point, or -1.
inline int tabHitTest(int x, int y) {
  if (y < 0 || y >= TAB_H) return -1;
  for (uint8_t i = 0; i < CITY_COUNT; i++)
    if (x >= tabX(i) && x < tabX(i) + tabWidth(i)) return i;
  return -1;
}

// ---------------------------------------------------------------------------
// Panel pieces
// ---------------------------------------------------------------------------

// Temperature on the left, weather art on the right.
inline void drawTempBand(Display& g, int px, float temp, WxIcon icon) {
  g.setFont(&FontNum42);
  const int w = measureTemp(g, temp, 8);
  putTemp(g, px + TEMP_CX - w / 2, Y_BAND, temp, tempColor(temp), 8, 13);
  drawWxIcon(g, icon, px + ICON_CX, Y_BAND, ICON_SZ);
}

// A heavy number with a small unit beside it and a caption underneath -- the
// shape that stays readable from across the room.
inline void drawBigStat(Display& g, int cx, const char* num, const char* unit,
                        const char* label, uint16_t numCol) {
  g.setFont(&FontNum25);
  const int wn = g.textWidth(num);
  g.setFont(&FontRU15);
  const int wu = g.textWidth(unit);
  const int gap = 3;
  const int x = cx - (wn + gap + wu) / 2;

  g.setFont(&FontNum25);
  g.setTextColor(numCol);
  g.setTextDatum(lgfx::middle_left);
  g.drawString(num, x, Y_STAT);

  g.setFont(&FontRU15);
  g.setTextColor(C_TEXT_DIM);
  g.drawString(unit, x + wn + gap, Y_STAT + 7);  // sits on the digits' baseline

  g.setFont(&FontRU12);
  g.setTextColor(C_TEXT_MUT);
  g.setTextDatum(lgfx::middle_center);
  g.drawString(label, cx, Y_STATL);
}

inline void drawHumidityAndPressure(Display& g, int px, int rh, float hPa) {
  char buf[12];

  snprintf(buf, sizeof(buf), "%d", rh);
  drawBigStat(g, px + STAT_L_CX, buf, "%", "Влажность", C_TEXT);

  // A missing or zero reading would otherwise render as a confident "0 мм".
  if (hPa > 0) {
    snprintf(buf, sizeof(buf), "%d", (int)lroundf(hPa * HPA_TO_MMHG));
    drawBigStat(g, px + STAT_R_CX, buf, "мм", "Давление", C_TEXT);
  } else {
    drawBigStat(g, px + STAT_R_CX, "--", "", "Давление", C_TEXT_MUT);
  }
}

inline void drawRowLabel(Display& g, int px, int y, const char* label) {
  g.setFont(&FontRU15);
  g.setTextColor(C_TEXT_DIM);
  g.setTextDatum(lgfx::middle_left);
  g.drawString(label, px + PAD, y);
}

inline void drawRowText(Display& g, int px, int y, const char* label,
                        const char* value, uint16_t col) {
  drawRowLabel(g, px, y, label);
  g.setFont(&FontRU15);
  g.setTextColor(col);
  g.setTextDatum(lgfx::middle_right);
  g.drawString(value, px + PANEL_W - PAD, y);
}

inline void drawRowTemp(Display& g, int px, int y, const char* label, float v,
                        uint16_t col) {
  drawRowLabel(g, px, y, label);
  g.setFont(&FontRU15);
  const int x = px + PANEL_W - PAD - measureTemp(g, v, 3);
  putTemp(g, x, y, v, col, 3, 5);
}

inline void drawRowTempPair(Display& g, int px, int y, const char* label,
                            float lo, float hi) {
  drawRowLabel(g, px, y, label);
  g.setFont(&FontRU15);
  const int sepW = g.textWidth("/") + 8;
  const int total = measureTemp(g, lo, 3) + sepW + measureTemp(g, hi, 3);
  int x = px + PANEL_W - PAD - total;
  x += putTemp(g, x, y, lo, C_COLD, 3, 5);
  g.setTextColor(C_TEXT_MUT);
  g.setTextDatum(lgfx::middle_center);
  g.drawString("/", x + sepW / 2, y);
  x += sepW;
  putTemp(g, x, y, hi, C_WARM, 3, 5);
}

// ---------------------------------------------------------------------------
// Panels
// ---------------------------------------------------------------------------

inline void drawPanelFrame(Display& g, int px, const char* title) {
  g.fillRoundRect(px, PANEL_TOP, PANEL_W, PANEL_H, 10, C_PANEL);
  g.setFont(&FontRU15);
  g.setTextColor(C_TEXT_DIM);
  g.setTextDatum(lgfx::middle_center);
  g.drawString(title, px + PANEL_W / 2, Y_TITLE);
  g.drawFastHLine(px + PAD, Y_RULE, PANEL_W - PAD * 2, C_DIVIDER);
}

// Shown while the first fetch for a city is still in flight.
inline void drawPanelPlaceholder(Display& g, int px) {
  g.setFont(&FontRU20);
  g.setTextColor(C_TEXT_MUT);
  g.setTextDatum(lgfx::middle_center);
  g.drawString("...", px + PANEL_W / 2, Y_BAND);
}

inline void drawTodayPanel(Display& g, const Wx& wx) {
  const int px = PANEL_L_X;
  drawPanelFrame(g, px, "СЕЙЧАС");
  if (!wx.valid) { drawPanelPlaceholder(g, px); return; }

  const WxInfo info = wxDecode(wx.code_now);
  drawTempBand(g, px, wx.t_now, info.icon);

  g.setFont(&FontRU15);
  g.setTextColor(C_TEXT);
  g.setTextDatum(lgfx::middle_center);
  g.drawString(info.text, px + PANEL_W / 2, Y_DESC);

  drawHumidityAndPressure(g, px, wx.rh_now, wx.p_now);

  // Current precipitation is a measured amount, so show millimetres.
  char buf[16];
  snprintf(buf, sizeof(buf), "%.1f мм", wx.precip_now);
  drawRowText(g, px, Y_ROW1, "Осадки", buf, wx.precip_now > 0 ? C_RAIN : C_TEXT);

  drawRowTempPair(g, px, Y_ROW2, "Мин/макс", wx.t_min_today, wx.t_max_today);
}

inline void drawTomorrowPanel(Display& g, const Wx& wx) {
  const int px = PANEL_R_X;
  drawPanelFrame(g, px, "ЗАВТРА");
  if (!wx.valid) { drawPanelPlaceholder(g, px); return; }

  const WxInfo info = wxDecode(wx.code_tmr);
  drawTempBand(g, px, wx.t_max_tmr, info.icon);

  g.setFont(&FontRU15);
  g.setTextColor(C_TEXT);
  g.setTextDatum(lgfx::middle_center);
  g.drawString(info.text, px + PANEL_W / 2, Y_DESC);

  drawHumidityAndPressure(g, px, wx.rh_tmr, wx.p_tmr);

  // Tomorrow's precipitation is a forecast, so show probability.
  char buf[16];
  snprintf(buf, sizeof(buf), "%d%%", wx.pop_tmr);
  drawRowText(g, px, Y_ROW1, "Осадки", buf, wx.pop_tmr >= 40 ? C_RAIN : C_TEXT);

  drawRowTemp(g, px, Y_ROW2, "Ночью", wx.t_min_tmr, C_COLD);
}

// ---------------------------------------------------------------------------
// Status bar
// ---------------------------------------------------------------------------

inline void drawStatus(Display& g, const char* msg, uint16_t col, const Wx& wx) {
  g.fillRect(0, STATUS_Y, SCREEN_W, SCREEN_H - STATUS_Y, C_BG);
  g.drawFastHLine(0, STATUS_Y, SCREEN_W, C_DIVIDER);

  g.setFont(&FontRU12);
  g.setTextColor(col);
  g.setTextDatum(lgfx::middle_left);
  g.drawString(msg, PAD, STATUS_Y + 13);

  if (wx.valid) {
    char buf[32];
    snprintf(buf, sizeof(buf), "обновлено %s", wx.local_time);
    g.setTextColor(C_TEXT_MUT);
    g.setTextDatum(lgfx::middle_right);
    g.drawString(buf, SCREEN_W - PAD, STATUS_Y + 13);
  }
}

// ---------------------------------------------------------------------------
// Boot / splash
// ---------------------------------------------------------------------------

inline void drawSplash(Display& g, const char* line) {
  g.fillScreen(C_BG);
  drawWxIcon(g, WX_PARTLY, SCREEN_W / 2, SCREEN_H / 2 - 36, 96);
  g.setFont(&FontRU20);
  g.setTextColor(C_TEXT);
  g.setTextDatum(lgfx::middle_center);
  g.drawString("Погода", SCREEN_W / 2, SCREEN_H / 2 + 42);
  g.setFont(&FontRU15);
  g.setTextColor(C_TEXT_DIM);
  g.drawString(line, SCREEN_W / 2, SCREEN_H / 2 + 76);
}
