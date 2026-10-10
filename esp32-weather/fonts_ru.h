#pragma once
#include <LovyanGFX.hpp>

// Glyph data lives in fonts_ru.c (compiled as C -- see the note there).
extern "C" {
extern const uint8_t u8g2_font_6x12_t_cyrillic[];
extern const uint8_t u8g2_font_9x15_t_cyrillic[];
extern const uint8_t u8g2_font_10x20_t_cyrillic[];
extern const uint8_t u8g2_font_fub25_tn[];
extern const uint8_t u8g2_font_fub42_tn[];
}

// LovyanGFX renders U8g2 font data directly, including the unicode lookup
// table, which is what makes codepoints above 0xFF (i.e. Cyrillic) work.
static const lgfx::U8g2font FontRU12(u8g2_font_6x12_t_cyrillic);
static const lgfx::U8g2font FontRU15(u8g2_font_9x15_t_cyrillic);
static const lgfx::U8g2font FontRU20(u8g2_font_10x20_t_cyrillic);

// FreeUniversal Bold, digits only: the heavy faces used for the numbers that
// have to read from across the room. No Cyrillic and no '%' in these, so units
// and labels are drawn separately in the faces above.
static const lgfx::U8g2font FontNum25(u8g2_font_fub25_tn);
static const lgfx::U8g2font FontNum42(u8g2_font_fub42_tn);
