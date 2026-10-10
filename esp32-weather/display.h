#pragma once
#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include "config.h"

// LovyanGFX device description for the ESP32-4832S040. Everything is declared
// in code rather than through a library-wide User_Setup header, so the board
// definition travels with the sketch.
class Display : public lgfx::LGFX_Device {
  lgfx::Panel_ST7796 _panel;
  lgfx::Bus_SPI      _bus;
  lgfx::Light_PWM    _light;
  lgfx::Touch_XPT2046 _touch;

 public:
  Display() {
    {
      auto cfg = _bus.config();
      cfg.spi_host    = SPI2_HOST;
      cfg.spi_mode    = 0;
      cfg.freq_write  = 40000000;
      cfg.freq_read   = 16000000;
      cfg.spi_3wire   = false;
      cfg.use_lock    = true;
      cfg.dma_channel = SPI_DMA_CH_AUTO;
      cfg.pin_sclk    = PIN_LCD_SCLK;
      cfg.pin_mosi    = PIN_LCD_MOSI;
      cfg.pin_miso    = PIN_LCD_MISO;
      cfg.pin_dc      = PIN_LCD_DC;
      _bus.config(cfg);
      _panel.setBus(&_bus);
    }
    {
      auto cfg = _panel.config();
      cfg.pin_cs           = PIN_LCD_CS;
      cfg.pin_rst          = PIN_LCD_RST;
      cfg.pin_busy         = -1;
      cfg.panel_width      = LCD_PANEL_W;
      cfg.panel_height     = LCD_PANEL_H;
      cfg.offset_x         = 0;
      cfg.offset_y         = 0;
      cfg.offset_rotation  = 0;
      cfg.dummy_read_pixel = 8;
      cfg.dummy_read_bits  = 1;
      cfg.readable         = true;
      cfg.invert           = true;   // this panel ships colour-inverted
      cfg.rgb_order        = false;
      cfg.dlen_16bit       = false;
      cfg.bus_shared       = true;   // the touch controller sits on this same SPI bus
      _panel.config(cfg);
    }
    {
      auto cfg = _light.config();
      cfg.pin_bl      = PIN_LCD_BL;
      cfg.invert      = false;
      cfg.freq        = 44100;
      cfg.pwm_channel = 7;
      _light.config(cfg);
      _panel.setLight(&_light);
    }
    {
      auto cfg = _touch.config();
      cfg.spi_host   = SPI2_HOST;   // same bus as the panel
      cfg.pin_sclk   = PIN_LCD_SCLK;
      cfg.pin_mosi   = PIN_LCD_MOSI;
      cfg.pin_miso   = PIN_LCD_MISO;
      cfg.pin_cs     = PIN_TP_CS;
      cfg.pin_int    = PIN_TP_IRQ;
      cfg.freq       = TP_SPI_HZ;
      cfg.bus_shared = true;
      // x/y_min and x/y_max are raw ADC bounds. These defaults only have to be
      // close enough for the first touch to register; calibrateTouch() then
      // replaces them with measured values that are persisted to NVS.
      cfg.x_min = 300;
      cfg.x_max = 3900;
      cfg.y_min = 400;
      cfg.y_max = 3900;
      _touch.config(cfg);
      _panel.setTouch(&_touch);
    }
    setPanel(&_panel);
  }
};
