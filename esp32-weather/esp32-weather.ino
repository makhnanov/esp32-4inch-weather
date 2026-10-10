// Weather display for a Sunton/Guition-style 4.0" board (ESP32-WROOM-32E,
// 480x320 ST7796 panel, XPT2046 resistive touch sharing the panel's SPI bus).
//
// Current conditions for the selected city on the left, tomorrow's forecast on
// the right, with a row of city tabs across the top. Data comes from
// Open-Meteo, which needs no API key.
//
// Wi-Fi credentials come from .env via tools/gen_secrets.py -> secrets.h.

#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>
#include <string.h>

#include "config.h"
#include "display.h"
#include "secrets.h"
#include "ui.h"
#include "weather.h"

static Display tft;

// One cached reading per city, so switching tabs is instant and a failed
// refresh never blanks out a good reading.
static Wx      cache[CITY_COUNT];
static uint8_t curCity = DEFAULT_CITY;

static const char* statusMsg = "";
static uint16_t    statusCol = C_TEXT_DIM;

static bool     touchDown     = false;
static uint32_t lastTapMs     = 0;
static uint32_t lastWifiTry   = 0;
static uint8_t  refreshCursor = 0;  // round-robin over the non-selected cities

static constexpr uint32_t WIFI_RETRY_MS = 15000;
static constexpr uint32_t TAP_DEBOUNCE_MS = 200;

// ---------------------------------------------------------------------------

static constexpr char NVS_NS[]  = "weather";
static constexpr char NVS_CAL[] = "tpcal";

static bool loadTouchCalibration() {
  Preferences p;
  if (!p.begin(NVS_NS, /*readOnly=*/true)) return false;
  uint16_t cal[8];
  const size_t n = p.getBytes(NVS_CAL, cal, sizeof(cal));
  p.end();
  if (n != sizeof(cal)) return false;
  tft.setTouchCalibrate(cal);
  Serial.println(F("[tp] calibration loaded from NVS"));
  return true;
}

static void runTouchCalibration() {
  tft.fillScreen(C_BG);
  tft.setFont(&FontRU20);
  tft.setTextColor(C_TEXT);
  tft.setTextDatum(lgfx::middle_center);
  tft.drawString("Калибровка касания", SCREEN_W / 2, SCREEN_H / 2 - 26);
  tft.setFont(&FontRU15);
  tft.setTextColor(C_TEXT_DIM);
  tft.drawString("Нажмите на стрелку", SCREEN_W / 2, SCREEN_H / 2 + 8);
  tft.drawString("в каждом углу экрана", SCREEN_W / 2, SCREEN_H / 2 + 30);
  delay(1800);

  uint16_t cal[8];
  tft.calibrateTouch(cal, C_ACCENT, C_BG, 16);

  Preferences p;
  if (p.begin(NVS_NS, /*readOnly=*/false)) {
    p.putBytes(NVS_CAL, cal, sizeof(cal));
    p.end();
  }
  Serial.printf("[tp] calibrated: %u %u %u %u %u %u %u %u\n",
                cal[0], cal[1], cal[2], cal[3], cal[4], cal[5], cal[6], cal[7]);
}

// A resistive panel has no fixed coordinate mapping, so it needs a one-time
// four-corner calibration. The result is kept in NVS; holding a finger on the
// screen while the board boots forces a redo.
static void setupTouch() {
  int32_t x = 0, y = 0;
  const bool heldAtBoot = tft.getTouch(&x, &y);
  if (heldAtBoot) Serial.println(F("[tp] touch held at boot -- recalibrating"));
  if (heldAtBoot || !loadTouchCalibration()) runTouchCalibration();
}

static void redrawStatus() {
  drawStatus(tft, statusMsg, statusCol, cache[curCity]);
}

// Repaints the status bar only when the message actually changed, to keep the
// SPI bus quiet in the idle loop.
static void setStatus(const char* msg, uint16_t col) {
  if (statusCol == col && strcmp(statusMsg, msg) == 0) return;
  statusMsg = msg;
  statusCol = col;
  redrawStatus();
}

static void redrawContent() {
  drawTodayPanel(tft, cache[curCity]);
  drawTomorrowPanel(tft, cache[curCity]);
  redrawStatus();
}

static void selectCity(uint8_t i) {
  curCity = i;
  Serial.printf("[ui] city -> %s\n", CITIES[i].name);
  drawTabs(tft, curCity);
  redrawContent();
}

static void wifiBegin() {
  lastWifiTry = millis();
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
}

// Acts on the press edge, so holding a tab does not re-trigger it.
static void handleTouch() {
  int32_t x = 0, y = 0;
  const bool down = tft.getTouch(&x, &y);

  if (down && !touchDown) {
    touchDown = true;
    const uint32_t now = millis();
    if (now - lastTapMs >= TAP_DEBOUNCE_MS) {
      lastTapMs = now;
      const int tab = tabHitTest(x, y);
      if (tab >= 0 && tab != curCity) selectCity((uint8_t)tab);
    }
  } else if (!down && touchDown) {
    touchDown = false;
  }
}

// ---------------------------------------------------------------------------

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println();
  Serial.println(F("[boot] weather display, ST7796 + XPT2046"));
  Serial.printf("[boot] rotation %d%s\n", LCD_ROTATION,
                FLIP_DISPLAY ? " (flipped 180)" : "");

  tft.init();
  tft.setRotation(LCD_ROTATION);  // landscape: 480x320
  tft.setBrightness(255);
  tft.fillScreen(C_BG);

  setupTouch();

  drawSplash(tft, "Подключение к Wi-Fi...");
  wifiBegin();
  const uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - t0) < WIFI_TIMEOUT_MS) {
    delay(100);
  }

  const bool online = (WiFi.status() == WL_CONNECTED);
  if (online) {
    Serial.printf("[boot] Wi-Fi ok, ip %s, rssi %d\n",
                  WiFi.localIP().toString().c_str(), WiFi.RSSI());
    drawSplash(tft, "Загрузка погоды...");
    fetchWeather(CITIES[curCity], cache[curCity]);
  } else {
    Serial.printf("[boot] Wi-Fi failed for ssid \"%s\"\n", WIFI_SSID);
  }

  statusMsg = online ? "Онлайн" : "Нет Wi-Fi";
  statusCol = online ? C_OK : C_ERR;

  tft.fillScreen(C_BG);
  drawTabs(tft, curCity);
  redrawContent();
  Serial.printf("[boot] ready, free heap %u\n", (unsigned)ESP.getFreeHeap());
}

void loop() {
  handleTouch();

  const uint32_t now = millis();

  if (WiFi.status() != WL_CONNECTED) {
    setStatus("Нет Wi-Fi", C_ERR);
    if (now - lastWifiTry >= WIFI_RETRY_MS) wifiBegin();
    delay(20);
    return;
  }

  // The visible city comes first.
  Wx& cur = cache[curCity];
  if (cur.shouldFetch(now)) {
    setStatus("Обновление...", C_TEXT_DIM);
    const bool ok = fetchWeather(CITIES[curCity], cur);
    statusMsg = ok ? "Онлайн" : "Ошибка сети";
    statusCol = ok ? C_OK : C_ERR;
    redrawContent();  // also repaints the status bar with the new timestamp
    return;
  }

  // Then keep the other tabs warm, one city per pass, so a tab switch shows
  // data immediately instead of a spinner.
  for (uint8_t k = 0; k < CITY_COUNT; k++) {
    const uint8_t i = (uint8_t)((refreshCursor + k) % CITY_COUNT);
    if (i == curCity) continue;
    if (cache[i].shouldFetch(now)) {
      refreshCursor = (uint8_t)((i + 1) % CITY_COUNT);
      fetchWeather(CITIES[i], cache[i]);
      return;
    }
  }

  setStatus("Онлайн", C_OK);
  delay(20);
}
