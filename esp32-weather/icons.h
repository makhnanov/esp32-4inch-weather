#pragma once
#include <LovyanGFX.hpp>
#include <math.h>
#include "theme.h"

// Weather art is drawn with primitives rather than stored as bitmaps: it costs
// no flash, and one icon scales to any size the layout asks for.

enum WxIcon : uint8_t {
  WX_CLEAR, WX_PARTLY, WX_CLOUDY, WX_FOG,
  WX_DRIZZLE, WX_RAIN, WX_SHOWER, WX_SNOW, WX_THUNDER,
};

struct WxInfo {
  WxIcon      icon;
  const char* text;  // short Russian label, kept under ~22 chars to fit a panel
};

// WMO weather interpretation codes, as returned by Open-Meteo.
inline WxInfo wxDecode(int code) {
  switch (code) {
    case 0:  return { WX_CLEAR,   "Ясно" };
    case 1:  return { WX_CLEAR,   "В основном ясно" };
    case 2:  return { WX_PARTLY,  "Переменная обл." };
    case 3:  return { WX_CLOUDY,  "Пасмурно" };
    case 45: return { WX_FOG,     "Туман" };
    case 48: return { WX_FOG,     "Изморозь" };
    case 51: return { WX_DRIZZLE, "Слабая морось" };
    case 53: return { WX_DRIZZLE, "Морось" };
    case 55: return { WX_DRIZZLE, "Сильная морось" };
    case 56:
    case 57: return { WX_DRIZZLE, "Ледяная морось" };
    case 61: return { WX_RAIN,    "Слабый дождь" };
    case 63: return { WX_RAIN,    "Дождь" };
    case 65: return { WX_RAIN,    "Сильный дождь" };
    case 66:
    case 67: return { WX_RAIN,    "Ледяной дождь" };
    case 71: return { WX_SNOW,    "Слабый снег" };
    case 73: return { WX_SNOW,    "Снег" };
    case 75: return { WX_SNOW,    "Сильный снег" };
    case 77: return { WX_SNOW,    "Снежная крупа" };
    case 80: return { WX_SHOWER,  "Слабый ливень" };
    case 81: return { WX_SHOWER,  "Ливень" };
    case 82: return { WX_SHOWER,  "Сильный ливень" };
    case 85: return { WX_SNOW,    "Снегопад" };
    case 86: return { WX_SNOW,    "Сильный снегопад" };
    case 95: return { WX_THUNDER, "Гроза" };
    case 96:
    case 99: return { WX_THUNDER, "Гроза с градом" };
    default: return { WX_CLOUDY,  "Нет данных" };
  }
}

namespace wxart {

inline int clampMin(int v, int lo) { return v < lo ? lo : v; }

inline void sun(lgfx::LGFXBase& g, int cx, int cy, int r, uint16_t col) {
  g.fillCircle(cx, cy, r, col);
  const int ri = r + clampMin(r / 3, 3);
  const int ro = r + clampMin(r * 3 / 4, 7);
  const int th = clampMin(r / 4, 2);
  for (int i = 0; i < 8; i++) {
    const float a = i * (float)M_PI / 4.0f;
    const float ca = cosf(a), sa = sinf(a);
    g.drawWideLine(cx + ca * ri, cy + sa * ri, cx + ca * ro, cy + sa * ro, th, col);
  }
}

// cy is the vertical middle of the cloud's body; w its full width.
inline void cloud(lgfx::LGFXBase& g, int cx, int cy, int w, uint16_t col) {
  const int h = clampMin(w * 36 / 100, 8);
  g.fillRoundRect(cx - w / 2, cy - h / 2, w, h, h / 2, col);
  g.fillCircle(cx - w / 5, cy - h / 2 + 1, h * 6 / 10, col);
  g.fillCircle(cx + w / 9, cy - h * 8 / 10, h * 8 / 10, col);
  g.fillCircle(cx + w / 3, cy - h / 3, h * 55 / 100, col);
}

inline void drops(lgfx::LGFXBase& g, int cx, int cy, int w, int n, int len, uint16_t col) {
  const int span = w * 2 / 3;
  for (int i = 0; i < n; i++) {
    const int x = cx - span / 2 + (n > 1 ? span * i / (n - 1) : span / 2);
    const int y = cy + (i & 1 ? len / 3 : 0);
    g.drawWideLine(x + len / 3, y, x - len / 3, y + len, clampMin(len / 4, 2), col);
  }
}

inline void flakes(lgfx::LGFXBase& g, int cx, int cy, int w, int n, int r, uint16_t col) {
  const int span = w * 2 / 3;
  const int th = clampMin(r / 3, 2);
  for (int i = 0; i < n; i++) {
    const int x = cx - span / 2 + (n > 1 ? span * i / (n - 1) : span / 2);
    const int y = cy + r + (i & 1 ? r : 0);
    for (int k = 0; k < 3; k++) {
      const float a = k * (float)M_PI / 3.0f;
      const float ca = cosf(a) * r, sa = sinf(a) * r;
      g.drawWideLine(x - ca, y - sa, x + ca, y + sa, th, col);
    }
  }
}

// A thick zigzag reads as lightning far more reliably than a triangulated
// polygon does at these sizes.
inline void bolt(lgfx::LGFXBase& g, int cx, int cy, int h, uint16_t col) {
  const int w = clampMin(h * 55 / 100, 8);
  const int th = clampMin(h / 7, 3);
  g.drawWideLine(cx + w * 35 / 100, cy, cx - w * 15 / 100, cy + h / 2, th, col);
  g.drawWideLine(cx - w * 15 / 100, cy + h / 2, cx + w * 12 / 100, cy + h / 2, th, col);
  g.drawWideLine(cx + w * 12 / 100, cy + h / 2, cx - w * 35 / 100, cy + h, th, col);
}

inline void fogBars(lgfx::LGFXBase& g, int cx, int cy, int w, uint16_t col) {
  const int t = clampMin(w / 14, 3);
  for (int i = 0; i < 3; i++) {
    const int ww = w - (i & 1 ? w / 3 : 0);
    g.fillRoundRect(cx - ww / 2, cy + i * (t + t), ww, t, t / 2, col);
  }
}

}  // namespace wxart

// Draws one weather icon centred on (cx, cy) inside a `size` x `size` box.
inline void drawWxIcon(lgfx::LGFXBase& g, WxIcon ic, int cx, int cy, int size) {
  const int s = size;
  switch (ic) {
    case WX_CLEAR:
      wxart::sun(g, cx, cy, s * 26 / 100, C_SUN);
      break;

    case WX_PARTLY:
      wxart::sun(g, cx + s * 18 / 100, cy - s * 20 / 100, s * 17 / 100, C_SUN);
      wxart::cloud(g, cx - s * 8 / 100, cy + s * 18 / 100, s * 76 / 100, C_CLOUD);
      break;

    case WX_CLOUDY:
      wxart::cloud(g, cx + s * 12 / 100, cy - s * 14 / 100, s * 58 / 100, C_CLOUD_DK);
      wxart::cloud(g, cx - s * 8 / 100, cy + s * 14 / 100, s * 78 / 100, C_CLOUD);
      break;

    case WX_FOG:
      wxart::cloud(g, cx, cy - s * 18 / 100, s * 76 / 100, C_CLOUD);
      wxart::fogBars(g, cx, cy + s * 14 / 100, s * 64 / 100, C_FOG);
      break;

    case WX_DRIZZLE:
      wxart::cloud(g, cx, cy - s * 18 / 100, s * 76 / 100, C_CLOUD);
      wxart::drops(g, cx, cy + s * 14 / 100, s * 70 / 100, 3, s * 13 / 100, C_RAIN);
      break;

    case WX_RAIN:
      wxart::cloud(g, cx, cy - s * 20 / 100, s * 76 / 100, C_CLOUD);
      wxart::drops(g, cx, cy + s * 10 / 100, s * 74 / 100, 4, s * 20 / 100, C_RAIN);
      break;

    case WX_SHOWER:
      wxart::sun(g, cx + s * 24 / 100, cy - s * 28 / 100, s * 13 / 100, C_SUN);
      wxart::cloud(g, cx - s * 6 / 100, cy - s * 14 / 100, s * 72 / 100, C_CLOUD);
      wxart::drops(g, cx - s * 6 / 100, cy + s * 16 / 100, s * 68 / 100, 3, s * 18 / 100, C_RAIN);
      break;

    case WX_SNOW:
      wxart::cloud(g, cx, cy - s * 20 / 100, s * 76 / 100, C_CLOUD);
      wxart::flakes(g, cx, cy + s * 8 / 100, s * 70 / 100, 3, s * 9 / 100, C_SNOW);
      break;

    case WX_THUNDER:
      wxart::cloud(g, cx, cy - s * 20 / 100, s * 76 / 100, C_CLOUD_DK);
      wxart::bolt(g, cx, cy + s * 2 / 100, s * 36 / 100, C_BOLT);
      break;
  }
}
