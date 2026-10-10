# ESP32 weather display with a 4" touchscreen

*[Русская версия](README.md)*

Current conditions on the left, tomorrow's forecast on the right, city tabs
across the top: **Almaty** (default), Omsk, Tomsk, Astana, Saint Petersburg.

Each half shows the temperature in large digits beside the weather icon, then
humidity and pressure as heavy numbers meant to be read from across the room,
then precipitation and the day's range.

```
 Алматы | Омск | Томск | Астана | Питер
-----------------------------------------
  СЕЙЧАС              |  ЗАВТРА          <- now / tomorrow
   13° [icon]          |   18° [icon]
  В основном ясно     |  Пасмурно        <- condition
  ------------------  |  ------------------
   61%      700 мм    |   56%      698 мм
  Влажность Давление  |  Влажность Давление   <- humidity / pressure
  Осадки      0.0 мм  |  Осадки         0%    <- precipitation
  Мин/макс   9°/20°   |  Ночью           9°   <- range / overnight low
-----------------------------------------
 Онлайн                     обновлено 13:45   <- status / last update
```

The on-screen language is Russian, since that is what the device was built for.
To localise it, edit the string literals in `icons.h` (condition names) and
`ui.h` (captions) — but see the note on fonts below first, because the bundled
faces only carry ASCII and Cyrillic.

Pressure is the **station-level** reading (`surface_pressure`), shown in
millimetres of mercury the way local forecasts quote it. For Almaty at 785 m
elevation that is ~700 mmHg rather than ~760: the sea-level-reduced figure
(`pressure_msl`) would read about 70 mmHg higher and disagree with a barometer
in the room.

Data comes from [Open-Meteo](https://open-meteo.com/) — no API key, no sign-up.
The observation time is rendered in each city's own timezone (`timezone=auto`),
so the board needs no NTP client and no offset table.

## Hardware

ESP32-WROOM-32E (ESP32-D0WD-V3, 4 MB flash), CH340 USB bridge, 480x320 panel.

The pinout below was **found by probing the live board**, not taken from a
datasheet — and it differs from what the board was sold as. There is no
capacitive GT911 on it (an exhaustive scan of every ordered GPIO pair found no
I2C device at all); instead there is a **resistive XPT2046 sitting on the same
SPI bus as the display**.

| Part | Signal | GPIO |
|---|---|---|
| ST7796 (display, SPI2) | SCLK / MOSI / MISO | 14 / 13 / 12 |
| | CS / DC / RST | 15 / 2 / — |
| | backlight (PWM) | 27 |
| XPT2046 (touch, same bus) | CS | 33 |
| | IRQ | unused |

Things that cost debugging time and are worth knowing:

* **The panel is colour-inverted** (`cfg.invert = true`); without it everything
  renders as a negative.
* **The touch IRQ line is deliberately left unconfigured** (`pin_int = -1`).
  LovyanGFX treats a high level on that pin as "not touched", so a wrongly
  guessed pin silently kills touch altogether. Polling over SPI works no matter
  how — or whether — the line is wired.
* **Open-Meteo replies with `Transfer-Encoding: chunked`** and no
  `Content-Length`. `http.getStream()` hands ArduinoJson the raw chunk framing
  and parsing fails with `InvalidInput`, so the body is read via
  `http.getString()`, which de-chunks it.
* **Neither `°` nor `…` exists in any of the Cyrillic fonts.** `make check`
  found this by walking the glyph tables — and `…` had already reached four
  strings, where it was drawing as blank space. The degree mark is drawn as a
  ring; the ellipsis is three ASCII dots.
* **The large numbers use the `fub*_tn` faces** (FreeUniversal Bold, digits
  only). U8g2 has no bold Cyrillic above 13 px, so the numbers are bold and
  their captions are not.

## Wi-Fi

Credentials live in `.env` and nowhere else:

```
Wifi=YourNetworkName
Password=YourPassword
```

`tools/gen_secrets.py` turns that into `esp32-weather/secrets.h` on every
build. Both files are gitignored; `.env.example` shows the format. Note that a compiled binary
embeds the credentials as plain strings, which is why `build/` is ignored too.

## Build and flash

```sh
cp .env.example .env   # then fill in your network
make deps     # once: esp32 core + LovyanGFX + ArduinoJson
make check    # hardware-free checks, see below
make flash    # compile and upload
make monitor  # watch the serial log
```

Override the port if the board appears elsewhere: `make flash PORT=/dev/ttyUSB1`

### `make check`

Two checks that need no board and catch what is otherwise only visible by
looking at the screen:

* **glyphs** — every string is checked against the face it is actually drawn
  with, so the digits-only `fub` faces are never asked for Cyrillic they will
  never render. This is what found `°` and `…`.
* **layout** — text widths are computed from the same u8g2 metrics the device
  uses, then asserted against worst-case values: the temperature must not
  collide with the icon, the humidity and pressure columns must not meet, and
  descriptions and tab labels must not clip.

The geometry constants in `tools/layoutcheck.c` mirror `esp32-weather/ui.h` by hand — the real
header cannot be included, as it pulls in LovyanGFX and the Arduino core. A
mismatch makes the check meaningless rather than wrong, so re-run `make check`
after touching the layout.

## Touch

A resistive panel has no fixed coordinate mapping, so on first boot the firmware
asks for a tap on an arrow in each of the four corners. The result is stored in
NVS and survives reflashing, since NVS lives in its own partition.

**To recalibrate:** hold a finger on the screen while the board powers up.

### Known limitation

Calibration blocks startup. If NVS is ever erased while the touchscreen is
faulty, the board will wait there forever instead of showing weather. The fix is
a timeout; it has not been implemented.

## Layout of the source

The sketch lives in the `esp32-weather/` subdirectory because arduino-cli
requires the folder name to match the `.ino` inside it. Keeping it there means
the repository can be named — and cloned as — anything without breaking the
build.

| File | Purpose |
|---|---|
| `esp32-weather/esp32-weather.ino` | setup/loop, tabs, calibration, refresh cycle |
| `esp32-weather/config.h` | pins, city list, timings |
| `esp32-weather/display.h` | panel and touch description for LovyanGFX |
| `esp32-weather/weather.h` | Open-Meteo request and parsing |
| `esp32-weather/ui.h` | screen layout and drawing |
| `esp32-weather/icons.h` | WMO codes → text and vector icons |
| `esp32-weather/theme.h` | palette |
| `esp32-weather/fonts_ru.c/h` | Cyrillic and bold numeric fonts extracted from U8g2 |
| `tools/gen_secrets.py` | `.env` → `esp32-weather/secrets.h` |
| `tools/glyphcheck.c` | are glyphs present for every string |
| `tools/layoutcheck.c` | does the text fit without overlapping |

### Adding a city

One line in `CITIES[]` in `esp32-weather/config.h` (name, latitude, longitude) — tabs divide
the width automatically. At five tabs of 96 px the labels fit with room to
spare; longer names want a shortened label.

### How refreshing works

Readings are considered fresh for 10 minutes. The visible city is refreshed
first, the others are topped up round-robin in the background, so switching tabs
shows numbers immediately instead of a spinner. A failed request never wipes
values already on screen and is retried no more than once every 30 seconds.

## Licence

The firmware is MIT (see `LICENSE`). The bundled font data in `fonts_ru.c` is
extracted from [U8g2](https://github.com/olikraus/u8g2) and remains under its
BSD-2-Clause licence, as noted at the top of that file.
