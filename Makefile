# ESP32 weather display -- build and flash helpers.
#
#   make deps     install the core and libraries (once)
#   make flash    regenerate secrets, compile, upload
#   make monitor  watch the serial log
#
# Override the port if the board shows up elsewhere: make flash PORT=/dev/ttyUSB1

PORT  ?= /dev/ttyUSB0
FQBN  ?= esp32:esp32:esp32
BUILD ?= build

# The sketch sits in its own directory because arduino-cli insists the folder
# name match the .ino inside it. Keeping it separate from the repo root means
# the repository can be named -- and cloned as -- anything.
SKETCH := esp32-weather

# Rotate the image 180 degrees without editing config.h, e.g. `make flash FLIP=0`.
# Left unset, the default in esp32-weather/config.h applies.
FLIP ?=
ifneq ($(FLIP),)
FLIPFLAG := --build-property compiler.cpp.extra_flags=-DFLIP_DISPLAY=$(FLIP)
else
FLIPFLAG :=
endif

.PHONY: all deps secrets compile flash monitor clean

all: compile

deps:
	arduino-cli config add board_manager.additional_urls \
	  https://espressif.github.io/arduino-esp32/package_esp32_index.json
	arduino-cli core update-index
	arduino-cli core install esp32:esp32
	arduino-cli lib install LovyanGFX ArduinoJson

# secrets.h is derived from .env, never edited by hand.
secrets:
	python3 tools/gen_secrets.py

compile: secrets
	arduino-cli compile -b $(FQBN) $(FLIPFLAG) --build-path $(BUILD) $(SKETCH)

flash: compile
	arduino-cli upload -b $(FQBN) -p $(PORT) --input-dir $(BUILD) $(SKETCH)

monitor:
	arduino-cli monitor -p $(PORT) -c baudrate=115200

clean:
	rm -rf $(BUILD)

# Host-side checks that need no hardware: that every string the UI draws has
# glyphs in the face it is drawn with, and that nothing overflows or overlaps
# at worst-case widths. Run after touching layout constants or UI strings.
.PHONY: check
check:
	@gcc -O1 -o /tmp/glyphcheck tools/glyphcheck.c $(SKETCH)/fonts_ru.c -lm && /tmp/glyphcheck \
	  "ru15:Алматы Омск Томск Астана Питер" \
	  "ru15:СЕЙЧАС ЗАВТРА" \
	  "ru15:Влажность Осадки Мин/макс Ночью" \
	  "ru15:% мм /" \
	  "ru15:Ясно В основном ясно Переменная обл. Пасмурно Туман Изморозь" \
	  "ru15:Слабая морось Морось Сильная морось Ледяная морось" \
	  "ru15:Слабый дождь Дождь Сильный дождь Ледяной дождь" \
	  "ru15:Слабый снег Снег Сильный снег Снежная крупа Снегопад Сильный снегопад" \
	  "ru15:Слабый ливень Ливень Сильный ливень" \
	  "ru15:Гроза Гроза с градом Нет данных" \
	  "ru15:Подключение к Wi-Fi... Загрузка погоды... Нажмите на стрелку" \
	  "ru15:На сколько отключить? Отмена" \
	  "ru15:в каждом углу экрана" \
	  "ru12:Влажность Давление" \
	  "ru12:Онлайн Нет Wi-Fi Обновление... Ошибка сети обновлено 00:00" \
	  "ru12:Отключить" \
	  "ru20:Погода Калибровка касания ..." \
	  "ru20:1 ч 2345678" \
	  "num42:0123456789-" \
	  "num25:0123456789-"
	@echo
	@gcc -O1 -o /tmp/layoutcheck tools/layoutcheck.c $(SKETCH)/fonts_ru.c -lm && /tmp/layoutcheck
