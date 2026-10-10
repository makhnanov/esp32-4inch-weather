#pragma once
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "config.h"

// One city's snapshot: what is happening now, plus today's and tomorrow's
// daily summaries. Open-Meteo is queried over plain HTTP -- it serves the
// endpoint without redirecting to TLS, which saves the ESP32 a ~40 KB TLS
// handshake and a certificate store for data that is not sensitive.
struct Wx {
  bool  valid = false;  // this struct holds real data

  // Current conditions
  float t_now      = 0;
  int   rh_now     = 0;
  int   code_now   = 0;
  float precip_now = 0;
  float p_now      = 0;  // station-level pressure, hPa

  // Today
  float t_max_today = 0;
  float t_min_today = 0;
  int   pop_today   = 0;  // max precipitation probability, %

  // Tomorrow
  float t_max_tmr = 0;
  float t_min_tmr = 0;
  int   rh_tmr    = 0;  // daily mean relative humidity, %
  float p_tmr     = 0;  // daily mean station-level pressure, hPa
  int   pop_tmr   = 0;
  int   code_tmr  = 0;

  char local_time[6] = "--:--";  // observation time in the city's own timezone

  uint32_t stamp    = 0;      // millis() of the last success
  uint32_t last_try = 0;      // millis() of the last attempt, success or not
  bool     failed   = false;  // the last attempt failed

  bool isStale(uint32_t now) const { return !valid || (now - stamp) >= REFRESH_MS; }

  // True when it is worth hitting the network again.
  bool shouldFetch(uint32_t now) const {
    if (!isStale(now)) return false;
    if (last_try && (now - last_try) < RETRY_MS) return false;  // back off
    return true;
  }
};

inline void buildWeatherUrl(const City& c, char* out, size_t n) {
  snprintf(out, n,
           "http://api.open-meteo.com/v1/forecast"
           "?latitude=%.4f&longitude=%.4f"
           "&current=temperature_2m,relative_humidity_2m,weather_code,precipitation,"
           "surface_pressure"
           "&daily=weather_code,temperature_2m_max,temperature_2m_min,"
           "precipitation_probability_max,relative_humidity_2m_mean,"
           "surface_pressure_mean"
           "&timezone=auto&forecast_days=2",
           c.lat, c.lon);
}

// Fetches and parses one city. Leaves `out` untouched on failure, so a stale
// but valid reading stays on screen instead of being replaced by blanks.
inline bool fetchWeather(const City& c, Wx& out) {
  out.last_try = millis();

  if (WiFi.status() != WL_CONNECTED) {
    out.failed = true;
    return false;
  }

  char url[400];
  buildWeatherUrl(c, url, sizeof(url));

  HTTPClient http;
  http.setConnectTimeout(8000);
  http.setTimeout(8000);
  http.setReuse(false);

  if (!http.begin(url)) {
    Serial.println(F("[wx] http.begin failed"));
    out.failed = true;
    return false;
  }

  const int status = http.GET();
  if (status != HTTP_CODE_OK) {
    Serial.printf("[wx] %s: HTTP %d\n", c.name, status);
    http.end();
    out.failed = true;
    return false;
  }

  // Open-Meteo answers with Transfer-Encoding: chunked and no Content-Length.
  // getStream() would hand ArduinoJson the raw chunk framing (and it would
  // fail with InvalidInput), so take the de-chunked body from getString();
  // the payload is well under 1 KB.
  const String body = http.getString();
  http.end();

  JsonDocument doc;
  const DeserializationError err = deserializeJson(doc, body);

  if (err) {
    Serial.printf("[wx] %s: json %s\n", c.name, err.c_str());
    out.failed = true;
    return false;
  }

  JsonObject cur = doc["current"];
  JsonObject day = doc["daily"];
  if (cur.isNull() || day.isNull()) {
    Serial.printf("[wx] %s: unexpected payload\n", c.name);
    out.failed = true;
    return false;
  }

  out.t_now      = cur["temperature_2m"]       | 0.0f;
  out.rh_now     = cur["relative_humidity_2m"] | 0;
  out.code_now   = cur["weather_code"]         | 0;
  out.precip_now = cur["precipitation"]        | 0.0f;
  out.p_now      = cur["surface_pressure"]     | 0.0f;

  // "2026-10-10T07:15" -> "07:15". Already local to the city thanks to
  // timezone=auto, so no NTP or offset table is needed on the device.
  const char* iso = cur["time"] | "";
  if (strlen(iso) >= 16) {
    memcpy(out.local_time, iso + 11, 5);
    out.local_time[5] = '\0';
  }

  out.t_max_today = day["temperature_2m_max"][0]            | 0.0f;
  out.t_min_today = day["temperature_2m_min"][0]            | 0.0f;
  out.pop_today   = day["precipitation_probability_max"][0] | 0;

  out.t_max_tmr = day["temperature_2m_max"][1]            | 0.0f;
  out.t_min_tmr = day["temperature_2m_min"][1]            | 0.0f;
  out.rh_tmr    = day["relative_humidity_2m_mean"][1]     | 0;
  out.pop_tmr   = day["precipitation_probability_max"][1] | 0;
  out.p_tmr     = day["surface_pressure_mean"][1]         | 0.0f;
  out.code_tmr  = day["weather_code"][1]                  | 0;

  out.valid  = true;
  out.failed = false;
  out.stamp  = millis();

  Serial.printf("[wx] %s: now %.1fC rh%d%% %.0fhPa code%d | tomorrow %.1f..%.1fC code%d\n",
                c.name, out.t_now, out.rh_now, out.p_now, out.code_now,
                out.t_min_tmr, out.t_max_tmr, out.code_tmr);
  return true;
}
