/* Verifies that every string the UI draws actually has glyphs in the fonts it
   is drawn with, replicating LovyanGFX's U8g2font::getGlyph lookup exactly.
   This is how the missing U+00B0 (degree sign) was found before it could show
   up on the panel as a blank box.  Build and run via `make check`. */
#include <stdio.h>
#include <stdint.h>
#include <string.h>

extern const uint8_t u8g2_font_6x12_t_cyrillic[];
extern const uint8_t u8g2_font_9x15_t_cyrillic[];
extern const uint8_t u8g2_font_10x20_t_cyrillic[];
extern const uint8_t u8g2_font_fub25_tn[];
extern const uint8_t u8g2_font_fub42_tn[];

static uint16_t be16(const uint8_t *p) { return (uint16_t)((p[0] << 8) | p[1]); }

static const uint8_t *getGlyph(const uint8_t *_font, uint16_t encoding) {
  const uint8_t *font = &_font[23];
  if (encoding <= 255) {
    if (encoding >= 'a')      font += be16(&_font[19]);
    else if (encoding >= 'A') font += be16(&_font[17]);
    for (; font[1]; font += font[1])
      if (font[0] == encoding) return font + 2;
  } else {
    uint32_t e;
    const uint8_t *lut;
    font += be16(&_font[21]);
    lut = font;
    do {
      font += (lut[0] << 8) + lut[1];
      e = (lut[2] << 8) + lut[3];
      lut += 4;
    } while (e < encoding);
    for (; 0 != (e = (font[0] << 8) + font[1]); font += font[2])
      if (e == encoding) return font + 3;
  }
  return NULL;
}

/* Minimal UTF-8 decoder: returns bytes consumed, writes codepoint. */
static int utf8next(const char *s, uint32_t *cp) {
  unsigned char c = (unsigned char)s[0];
  if (c < 0x80) { *cp = c; return 1; }
  if ((c & 0xE0) == 0xC0) { *cp = ((c & 0x1F) << 6) | (s[1] & 0x3F); return 2; }
  if ((c & 0xF0) == 0xE0) { *cp = ((c & 0x0F) << 12) | ((s[1] & 0x3F) << 6) | (s[2] & 0x3F); return 3; }
  if ((c & 0xF8) == 0xF0) { *cp = ((c & 0x07) << 18) | ((s[1] & 0x3F) << 12) | ((s[2] & 0x3F) << 6) | (s[3] & 0x3F); return 4; }
  *cp = '?'; return 1;
}

struct Font { const char *name; const uint8_t *data; };

static const struct Font FONTS[] = {
  {"ru12",  u8g2_font_6x12_t_cyrillic},
  {"ru15",  u8g2_font_9x15_t_cyrillic},
  {"ru20",  u8g2_font_10x20_t_cyrillic},
  {"num25", u8g2_font_fub25_tn},
  {"num42", u8g2_font_fub42_tn},
};
#define NFONTS (sizeof FONTS / sizeof FONTS[0])

/* Each argument is "face:text" -- the face a string is actually drawn with.
   Checking every string against every font would wrongly flag the digits-only
   fub faces for lacking Cyrillic they are never asked to render. */
int main(int argc, char **argv) {
  int bad = 0, checked = 0;
  for (int a = 1; a < argc; a++) {
    char *colon = strchr(argv[a], ':');
    if (!colon) { printf("bad argument %s (want face:text)\n", argv[a]); return 2; }
    *colon = 0;
    const char *face = argv[a], *text = colon + 1;

    const struct Font *f = NULL;
    for (unsigned i = 0; i < NFONTS; i++)
      if (!strcmp(FONTS[i].name, face)) { f = &FONTS[i]; break; }
    if (!f) { printf("unknown face %s\n", face); return 2; }

    int missing = 0;
    const char *s = text;
    while (*s) {
      uint32_t cp; s += utf8next(s, &cp);
      if (cp == ' ') continue;
      checked++;
      if (!getGlyph(f->data, (uint16_t)cp)) {
        printf("  MISSING U+%04X in %s (from \"%s\")\n", cp, face, text);
        missing++; bad++;
      }
    }
    if (!missing) printf("  ok   %-5s %s\n", face, text);
  }
  printf("%s (%d codepoints checked)\n",
         bad ? "GLYPHS: missing glyphs found" : "GLYPHS: every string renders", checked);
  return bad != 0;
}
