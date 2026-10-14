#pragma once
#include <stdint.h>

// ---------------------------------------------------------------------------
// Board: Sunton/Guition ESP32-4832S040
//   4.0" IPS 480x320, ST7796 over SPI, GT911 capacitive touch, ESP32-WROOM-32E
// ---------------------------------------------------------------------------

// ST7796 display, SPI2 (HSPI)
#define PIN_LCD_SCLK 14
#define PIN_LCD_MOSI 13
#define PIN_LCD_MISO 12
#define PIN_LCD_CS   15
#define PIN_LCD_DC    2
#define PIN_LCD_RST  -1  // tied to the module's own reset circuit
#define PIN_LCD_BL   27  // backlight, driven as PWM

// The panel is wired portrait; the UI runs landscape.
#define LCD_PANEL_W 320
#define LCD_PANEL_H 480

// Screen orientation. Set FLIP_DISPLAY to 1 to turn the image 180 degrees,
// for when the board is mounted with its USB socket on the other side, or to 0
// for the other way up. Override without editing this file:
//   make flash FLIP=0
//
// Touch follows the image on its own and the calibration stored in NVS stays
// valid: LovyanGFX keeps it as a raw-to-panel-native mapping
// (Panel_Device::setCalibrate) and applies the current rotation separately
// (Panel_Device::convertRawXY), so rotating never invalidates it.
#ifndef FLIP_DISPLAY
#define FLIP_DISPLAY 1
#endif
#define LCD_ROTATION (FLIP_DISPLAY ? 3 : 1)

// XPT2046 resistive touch. It shares the panel's SPI bus (SCLK/MOSI/MISO
// above) with its own chip select; discovered by probing, since the board
// carries the resistive controller rather than a capacitive GT911.
#define PIN_TP_CS 33
// The pen-interrupt line is left unused: LovyanGFX treats a high level on it
// as "not touched", so a mis-guessed pin would silently kill touch. Polling
// over SPI works no matter how that line is wired.
#define PIN_TP_IRQ -1
// XPT2046 is a slow part; 1 MHz is well inside spec.
#define TP_SPI_HZ 1000000

// Landscape screen geometry, after setRotation(1).
static constexpr int SCREEN_W = 480;
static constexpr int SCREEN_H = 320;

// ---------------------------------------------------------------------------
// Cities. Tab order; index 0 is the default shown at boot.
// ---------------------------------------------------------------------------
struct City {
  const char* name;  // tab label (UTF-8)
  float lat;
  float lon;
};

static const City CITIES[] = {
  { "Алматы", 43.2567f, 76.9286f },
  { "Омск",   54.9914f, 73.3645f },
  { "Томск",  56.4846f, 84.9476f },
  { "Астана", 51.1694f, 71.4491f },
  { "Питер",  59.9311f, 30.3609f },
};
static constexpr uint8_t CITY_COUNT   = sizeof(CITIES) / sizeof(CITIES[0]);
static constexpr uint8_t DEFAULT_CITY = 0;  // Алматы

// The clock in the status bar is always read in this city's timezone, whichever
// tab is open: the board hangs on a wall in Almaty, so "обновлено 13:45" has to
// mean the wall clock in the room and not noon in Saint Petersburg.
static constexpr uint8_t HOME_CITY = 0;  // Алматы
// Used until the home city's own reading arrives and brings the real offset
// along: Almaty is UTC+5 and keeps no daylight saving.
static constexpr int32_t HOME_UTC_OFFSET_FALLBACK = 5 * 3600;

// ---------------------------------------------------------------------------
// Timing
// ---------------------------------------------------------------------------
static constexpr uint32_t REFRESH_MS      = 10UL * 60UL * 1000UL;  // data is stale after this
static constexpr uint32_t RETRY_MS        = 30UL * 1000UL;         // backoff after a failed fetch
static constexpr uint32_t WIFI_TIMEOUT_MS = 20UL * 1000UL;         // per connection attempt

// ---------------------------------------------------------------------------
// Backlight and screen blanking
// ---------------------------------------------------------------------------

// Backlight level while the screen is on; blanking drives it to 0.
static constexpr uint8_t BACKLIGHT_ON = 255;

// The status-bar button opens a menu of spans, one button per hour up to this
// many. It also sets the shape of that menu's grid, so keep it a multiple of
// MENU_COLS in ui.h (or accept a short last row).
static constexpr uint8_t  SLEEP_MAX_HOURS = 8;
// A menu left open gets itself out of the way again, so a stray tap cannot
// park the display behind a dialog for good.
static constexpr uint32_t MENU_TIMEOUT_MS = 15UL * 1000UL;
static constexpr uint32_t HOUR_MS         = 3600UL * 1000UL;
