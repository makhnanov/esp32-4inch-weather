/* Host-side layout verification: computes text widths from the real u8g2 font
   data exactly as LovyanGFX does on the device (sum of per-glyph x_advance),
   then asserts the panel geometry in ui.h leaves no overlap or overflow. */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

extern const uint8_t u8g2_font_6x12_t_cyrillic[];
extern const uint8_t u8g2_font_9x15_t_cyrillic[];
extern const uint8_t u8g2_font_10x20_t_cyrillic[];
extern const uint8_t u8g2_font_fub25_tn[];
extern const uint8_t u8g2_font_fub42_tn[];

#define RU12  u8g2_font_6x12_t_cyrillic
#define RU15  u8g2_font_9x15_t_cyrillic
#define RU20  u8g2_font_10x20_t_cyrillic
#define NUM25 u8g2_font_fub25_tn
#define NUM42 u8g2_font_fub42_tn

static uint16_t be16(const uint8_t *p) { return (uint16_t)((p[0] << 8) | p[1]); }

static const uint8_t *getGlyph(const uint8_t *f, uint16_t enc) {
  const uint8_t *g = &f[23];
  if (enc <= 255) {
    if (enc >= 'a') g += be16(&f[19]); else if (enc >= 'A') g += be16(&f[17]);
    for (; g[1]; g += g[1]) if (g[0] == enc) return g + 2;
  } else {
    uint32_t e; const uint8_t *lut;
    g += be16(&f[21]); lut = g;
    do { g += (lut[0] << 8) + lut[1]; e = (lut[2] << 8) + lut[3]; lut += 4; } while (e < enc);
    for (; 0 != (e = (g[0] << 8) + g[1]); g += g[2]) if (e == enc) return g + 3;
  }
  return NULL;
}

/* Bit reader, mirroring u8g2_font_decode_t. */
struct Dec { const uint8_t *p; uint8_t bit; };
static unsigned getbits(struct Dec *d, uint8_t cnt) {
  unsigned val = *d->p >> d->bit;
  unsigned nxt = d->bit + cnt;
  if (nxt >= 8) { nxt -= 8; val |= (unsigned)(*(++d->p)) << (8 - d->bit); }
  d->bit = (uint8_t)nxt;
  return val & ((1U << cnt) - 1);
}
static int getsigned(struct Dec *d, uint8_t cnt) {
  return (int)getbits(d, cnt) - (1 << (cnt - 1));
}

static int advance(const uint8_t *f, uint16_t enc) {
  const uint8_t *g = getGlyph(f, enc);
  if (!g) return f[9];                 /* missing -> max_char_width */
  struct Dec d = { g, 0 };
  getbits(&d, f[4]);                   /* width  */
  getbits(&d, f[5]);                   /* height */
  getsigned(&d, f[6]);                 /* x off  */
  getsigned(&d, f[7]);                 /* y off  */
  return getsigned(&d, f[8]);          /* delta x */
}

static int utf8next(const char *s, uint32_t *cp) {
  unsigned char c = (unsigned char)s[0];
  if (c < 0x80) { *cp = c; return 1; }
  if ((c & 0xE0) == 0xC0) { *cp = ((c & 0x1F) << 6) | (s[1] & 0x3F); return 2; }
  if ((c & 0xF0) == 0xE0) { *cp = ((c & 0x0F) << 12) | ((s[1] & 0x3F) << 6) | (s[2] & 0x3F); return 3; }
  *cp = '?'; return 1;
}

static int tw(const uint8_t *f, const char *s) {
  int w = 0; uint32_t cp;
  while (*s) { s += utf8next(s, &cp); w += advance(f, (uint16_t)cp); }
  return w;
}

/* --- geometry mirrored from ui.h ---
   These must be kept in step with ui.h by hand; the sketch cannot be included
   here because it pulls in LovyanGFX and the Arduino core. A mismatch makes
   this check meaningless rather than wrong, so re-run `make check` after
   touching the layout constants. */
#define PANEL_W 234
#define PAD 14
#define TEMP_CX 74
#define ICON_CX 178
#define ICON_SZ 66
#define STAT_L_CX 60
#define STAT_R_CX 174
#define SCREEN_W 480
#define CITY_COUNT 5
#define BTN_W 120
#define BTN_X ((SCREEN_W - BTN_W) / 2)
#define SCREEN_H 320
#define SLEEP_MAX_HOURS 8
#define MENU_COLS 4
#define MENU_ROWS ((SLEEP_MAX_HOURS + MENU_COLS - 1) / MENU_COLS)
#define MENU_BW 68
#define MENU_BH 44
#define MENU_GAP 10
#define MENU_PAD 14
#define MENU_TITLE_H 44
#define MENU_CANCEL_H 34
#define MENU_GRID_H (MENU_ROWS * MENU_BH + (MENU_ROWS - 1) * MENU_GAP)
#define MENU_W (MENU_PAD * 2 + MENU_COLS * MENU_BW + (MENU_COLS - 1) * MENU_GAP)
#define MENU_H (MENU_TITLE_H + MENU_GRID_H + 12 + MENU_CANCEL_H + MENU_PAD)

static int fails = 0;
static void check(const char *what, int ok, const char *detail) {
  if (!ok) { printf("  FAIL %-34s %s\n", what, detail); fails++; }
  else     { printf("  ok   %-34s %s\n", what, detail); }
}

static int measureTemp(const uint8_t *f, int v, int ringR) {
  char b[8]; snprintf(b, sizeof b, "%d", v);
  return tw(f, b) + ringR * 3;
}

int main(void) {
  char d[80];

  printf("== temperature band (left of the icon) ==\n");
  const int iconL = ICON_CX - ICON_SZ / 2;      /* icon's left edge */
  int worst = 0, worstV = 0;
  const int temps[] = {0, 9, -9, 12, 18, 35, -35, -42, 100};
  for (unsigned i = 0; i < sizeof temps / sizeof *temps; i++) {
    const int w = measureTemp(NUM42, temps[i], 8);
    if (w > worst) { worst = w; worstV = temps[i]; }
  }
  snprintf(d, sizeof d, "widest \"%d\" = %dpx -> spans %d..%d, icon starts %d",
           worstV, worst, TEMP_CX - worst / 2, TEMP_CX + worst / 2, iconL);
  check("temp fits left of icon", TEMP_CX + worst / 2 <= iconL - 4, d);
  snprintf(d, sizeof d, "left edge %d", TEMP_CX - worst / 2);
  check("temp inside panel", TEMP_CX - worst / 2 >= 4, d);

  printf("== big stats (humidity / pressure) ==\n");
  /* Worst cases: humidity 100%%, pressure up to 4 mmHg digits. */
  const int wHum = tw(NUM25, "100") + 3 + tw(RU15, "%");
  const int wPrs = tw(NUM25, "1050") + 3 + tw(RU15, "мм");
  snprintf(d, sizeof d, "\"100%%\" = %dpx -> %d..%d", wHum,
           STAT_L_CX - wHum / 2, STAT_L_CX + wHum / 2);
  check("humidity inside panel", STAT_L_CX - wHum / 2 >= PAD - 10, d);
  snprintf(d, sizeof d, "\"1050мм\" = %dpx -> %d..%d", wPrs,
           STAT_R_CX - wPrs / 2, STAT_R_CX + wPrs / 2);
  check("pressure inside panel", STAT_R_CX + wPrs / 2 <= PANEL_W - PAD + 10, d);
  snprintf(d, sizeof d, "gap = %d px",
           (STAT_R_CX - wPrs / 2) - (STAT_L_CX + wHum / 2));
  check("stats do not collide", (STAT_R_CX - wPrs / 2) > (STAT_L_CX + wHum / 2) + 4, d);

  const int cl = tw(RU12, "Влажность"), cr = tw(RU12, "Давление");
  snprintf(d, sizeof d, "captions %dpx / %dpx, gap %d", cl, cr,
           (STAT_R_CX - cr / 2) - (STAT_L_CX + cl / 2));
  check("captions do not collide", (STAT_R_CX - cr / 2) > (STAT_L_CX + cl / 2) + 4, d);

  printf("== description line ==\n");
  const char *descs[] = {"В основном ясно", "Переменная обл.", "Сильный снегопад",
                         "Сильная морось", "Ледяная морось", "Сильный ливень",
                         "Снежная крупа", "Гроза с градом", "Нет данных"};
  int wd = 0; const char *wds = "";
  for (unsigned i = 0; i < sizeof descs / sizeof *descs; i++) {
    const int w = tw(RU15, descs[i]);
    if (w > wd) { wd = w; wds = descs[i]; }
  }
  snprintf(d, sizeof d, "widest \"%s\" = %dpx, room %d", wds, wd, PANEL_W - 2 * PAD);
  check("description fits", wd <= PANEL_W - 2 * PAD, d);

  printf("== bottom rows ==\n");
  const int wl = tw(RU15, "Мин/макс");
  const int sep = tw(RU15, "/") + 8;
  const int wv = measureTemp(RU15, -35, 3) + sep + measureTemp(RU15, -42, 3);
  snprintf(d, sizeof d, "label %dpx + value %dpx = %d, room %d", wl, wv, wl + wv,
           PANEL_W - 2 * PAD);
  check("min/max row fits", wl + wv <= PANEL_W - 2 * PAD, d);

  const int wo = tw(RU15, "Осадки") + tw(RU15, "99.9 мм");
  snprintf(d, sizeof d, "= %dpx, room %d", wo, PANEL_W - 2 * PAD);
  check("precipitation row fits", wo <= PANEL_W - 2 * PAD, d);

  printf("== tabs ==\n");
  const char *cities[] = {"Алматы", "Омск", "Томск", "Астана", "Питер"};
  const int tabw = SCREEN_W / CITY_COUNT;
  for (int i = 0; i < CITY_COUNT; i++) {
    const int w = tw(RU15, cities[i]);
    snprintf(d, sizeof d, "\"%s\" = %dpx of %d", cities[i], w, tabw);
    check("tab label fits", w <= tabw - 8, d);
  }

  printf("== status bar ==\n");
  /* Three things share one 480 px strip: the connection state on the left, the
     blanking button in the middle, the timestamp on the right. */
  const char *stats[] = {"Онлайн", "Нет Wi-Fi", "Обновление...", "Ошибка сети"};
  int ws = 0; const char *wss = "";
  for (unsigned i = 0; i < sizeof stats / sizeof *stats; i++) {
    const int w = tw(RU12, stats[i]);
    if (w > ws) { ws = w; wss = stats[i]; }
  }
  snprintf(d, sizeof d, "widest \"%s\" ends %d, button starts %d", wss, PAD + ws, BTN_X);
  check("status text clears button", PAD + ws < BTN_X - 8, d);

  const int wts = tw(RU12, "обновлено 00:00");
  snprintf(d, sizeof d, "\"обновлено 00:00\" starts %d, button ends %d",
           SCREEN_W - PAD - wts, BTN_X + BTN_W);
  check("timestamp clears button", SCREEN_W - PAD - wts > BTN_X + BTN_W + 8, d);

  const int wb12 = tw(RU12, "Отключить");
  snprintf(d, sizeof d, "\"Отключить\" = %dpx of %d", wb12, BTN_W);
  check("button label fits", wb12 <= BTN_W - 12, d);

  printf("== blanking menu ==\n");
  snprintf(d, sizeof d, "card %dx%d on a %dx%d screen", MENU_W, MENU_H, SCREEN_W, SCREEN_H);
  check("menu card fits on screen", MENU_W <= SCREEN_W - 16 && MENU_H <= SCREEN_H - 16, d);

  const int wt = tw(RU15, "На сколько отключить?");
  snprintf(d, sizeof d, "title %dpx, room %d", wt, MENU_W - 2 * MENU_PAD);
  check("menu title fits", wt <= MENU_W - 2 * MENU_PAD, d);

  int wb = 0; char hb[8];
  for (int i = 1; i <= SLEEP_MAX_HOURS; i++) {
    snprintf(hb, sizeof hb, "%d ч", i);
    const int w = tw(RU20, hb);
    if (w > wb) wb = w;
  }
  snprintf(d, sizeof d, "widest hour label %dpx, button %d wide", wb, MENU_BW);
  check("hour labels fit buttons", wb <= MENU_BW - 10, d);

  const int wc = tw(RU15, "Отмена");
  snprintf(d, sizeof d, "\"Отмена\" %dpx, button %d wide", wc, MENU_W - 2 * MENU_PAD);
  check("cancel label fits", wc <= MENU_W - 2 * MENU_PAD - 10, d);

  /* The grid is laid out from MENU_PAD, so the last column must land inside
     the card by exactly the same padding. */
  const int lastRight = MENU_PAD + MENU_COLS * MENU_BW + (MENU_COLS - 1) * MENU_GAP;
  snprintf(d, sizeof d, "last column ends %d, card is %d wide", lastRight, MENU_W);
  check("grid fits the card", lastRight == MENU_W - MENU_PAD, d);

  printf("\n%s\n", fails ? "LAYOUT: problems found" : "LAYOUT: all checks passed");
  return fails != 0;
}
