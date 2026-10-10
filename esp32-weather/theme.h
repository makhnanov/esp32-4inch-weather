#pragma once
#include <stdint.h>

// 24-bit hex -> RGB565, so the palette below stays readable as web colours.
static constexpr uint16_t rgb(uint32_t c) {
  return (uint16_t)((((c >> 19) & 0x1F) << 11) | (((c >> 10) & 0x3F) << 5) | ((c >> 3) & 0x1F));
}

// Surfaces
static constexpr uint16_t C_BG       = rgb(0x0B1220);
static constexpr uint16_t C_PANEL    = rgb(0x16213A);
static constexpr uint16_t C_TAB_OFF  = rgb(0x121B2E);
static constexpr uint16_t C_TAB_ON   = rgb(0x1E3A6B);
static constexpr uint16_t C_ACCENT   = rgb(0x3B82F6);
static constexpr uint16_t C_DIVIDER  = rgb(0x2A3A5C);

// Text
static constexpr uint16_t C_TEXT     = rgb(0xFFFFFF);
static constexpr uint16_t C_TEXT_DIM = rgb(0x93A7C6);
static constexpr uint16_t C_TEXT_MUT = rgb(0x5C6F8C);

// Weather art
static constexpr uint16_t C_SUN      = rgb(0xFFC83D);
static constexpr uint16_t C_CLOUD    = rgb(0xCBD8E8);
static constexpr uint16_t C_CLOUD_DK = rgb(0x8495B0);
static constexpr uint16_t C_RAIN     = rgb(0x4FA3F7);
static constexpr uint16_t C_SNOW     = rgb(0xEAF5FF);
static constexpr uint16_t C_BOLT     = rgb(0xFFD34D);
static constexpr uint16_t C_FOG      = rgb(0xA8B8CC);

// Semantic
static constexpr uint16_t C_WARM     = rgb(0xFF9A4C);
static constexpr uint16_t C_COLD     = rgb(0x7CC8FF);
static constexpr uint16_t C_OK       = rgb(0x34D399);
static constexpr uint16_t C_ERR      = rgb(0xF87171);

// Temperature tint: warm above freezing, cold below.
static inline uint16_t tempColor(float t) {
  if (t >= 25.0f) return C_WARM;
  if (t <= 0.0f)  return C_COLD;
  return C_TEXT;
}
