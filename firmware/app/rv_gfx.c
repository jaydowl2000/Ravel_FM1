/* SPDX-License-Identifier: GPL-3.0-only */
/* Ravel: drawing. The screen is drawn in bands of GB_H rows: the UI paints the whole frame
 * once per band, every primitive clipped to it, and each band is sent to the panel as soon as
 * it is done. No full-screen buffer: 19 KB instead of 113 KB.
 * Fonts: a runic stroke alphabet of our own, 4-bit anti-aliased (tools/gen_rv_font.py). Lines and discs are anti-aliased
 * (each pixel covered by how much of the stroke falls on it), so the drawing reads as drawn. */
#include "rv.h"

typedef struct {
    uint8_t h, first, last;
    const uint8_t *adv;
    const int8_t *bx;          /* where the bitmap starts, from the pen */
    const uint8_t *bw;         /* bitmap width; rows of (bw + 1) / 2 bytes, 4 bits a pixel */
    const uint16_t *off;
    const uint8_t *data;
} rv_font_t;
#include "rv_font.h"

#define GB_H 16
static uint16_t gb[PLAT_W * GB_H];
static int g_y0, g_y1;
static int g_cx0 = 0, g_cx1 = PLAT_W;   /* a clip across (the start screen's melt draws in strips) */

#define RGB(r, g, b) ((uint16_t)((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3)))
/* a deep black ground, quiet greys, light ink; teal for live movement, amber for memory and the tape */
/* THEMES. Every colour the UI draws with is one of these entries (C_xxx reads th_c[]), filled by theme_apply() from
 * GLO > PANEL: THEME (DARK or LIGHT) and ACCENT (BONE, BLUE, GREEN or PURPLE), kept with the FM-1's other settings.
 * th_dark[] is the palette Ravel always had. LIGHT turns the greys over (white on black becomes black on white, the
 * same contrasts), the dim washes of the track kinds become tints of their colours, and the accents (the first and
 * second colour, the trigger colour) come from th_acc[], deeper so they read on white. The track kinds' own colours
 * (red, the strike blue-grey, tape green, perc maroon) are the same in both. The screen is painted afresh every
 * frame, so a change shows at once. */
enum {
    TC_BG, TC_PANEL, TC_LINE, TC_DIM, TC_MID, TC_INK, TC_WHITE, TC_KPANEL, TC_KDIV, TC_KTRACK, TC_KOFF, TC_K1, TC_K2, TC_K3, TC_K4, TC_O_PANEL, TC_O_LINE, TC_O_DIM, TC_O_MID, TC_SYNTH_D, TC_CLOCK, TC_RING, TC_SYNTH, TC_RED, TC_EXC, TC_SPARK, TC_STRIKE, TC_TAPE, TC_PERC, TC_STRIKE_D, TC_TAPE_D, TC_PERC_D, TC_TEAL, TC_TEAL_D, TC_AMBER, TC_AMBER_D, TC_TRIG, TC_N
};
#define TC_NINV 23                          /* the first TC_NINV entries are the greys that turn over in LIGHT */
static const uint16_t th_dark[TC_N] = {
    RGB(3, 3, 3), /* BG */
    RGB(17, 17, 17), /* PANEL */
    RGB(36, 36, 36), /* LINE */
    RGB(92, 92, 92), /* DIM */
    RGB(152, 152, 152), /* MID */
    RGB(232, 232, 230), /* INK */
    RGB(255, 255, 255), /* WHITE */
    RGB(10, 10, 10), /* KPANEL the knob strip: a shade off black, with grain */
    RGB(58, 58, 58), /* KDIV its dividers, one between each physical knob */
    RGB(34, 34, 34), /* KTRACK a knob bar's empty track on the strip */
    RGB(66, 66, 66), /* KOFF a greyed knob's name and value on the strip */
    RGB(236, 236, 234), /* K1 the knobs: white down through the greys */
    RGB(196, 196, 196), /* K2 */
    RGB(156, 156, 156), /* K3 */
    RGB(118, 118, 118), /* K4 */
    RGB(28, 35, 31), /* O_PANEL */
    RGB(48, 58, 52), /* O_LINE */
    RGB(86, 100, 91), /* O_DIM */
    RGB(140, 153, 143), /* O_MID */
    RGB(58, 58, 58), /* SYNTH_D */
    RGB(170, 170, 170), /* CLOCK */
    RGB(100, 100, 100), /* RING */
    RGB(232, 232, 230), /* SYNTH */
    RGB(170, 44, 54), /* RED */
    RGB(112, 130, 152), /* EXC */
    RGB(190, 150, 104), /* SPARK SPARK: a brass tap */
    RGB(112, 130, 152), /* STRIKE */
    RGB(104, 146, 128), /* TAPE */
    RGB(131, 28, 41), /* PERC a deep, dark maroon */
    RGB(40, 48, 58), /* STRIKE_D */
    RGB(38, 56, 48), /* TAPE_D */
    RGB(72, 16, 24), /* PERC_D */
    RGB(222, 210, 184), /* TEAL the first accent: bone */
    RGB(84, 80, 70), /* TEAL_D */
    RGB(104, 146, 128), /* AMBER the second accent: verdigris */
    RGB(40, 56, 49), /* AMBER_D */
    RGB(226, 214, 190), /* TRIG */
};
/* the accents, per ACCENT (BONE, BLUE, GREEN, PURPLE) and mode (DARK, LIGHT): TEAL, TEAL_D, AMBER, AMBER_D, TRIG */
static const uint16_t th_acc[4][2][5] = {
    {{RGB(222, 210, 184), RGB(84, 80, 70), RGB(104, 146, 128), RGB(40, 56, 49), RGB(226, 214, 190)},
     {RGB(140, 104, 40), RGB(232, 224, 206), RGB(40, 110, 86), RGB(206, 230, 220), RGB(150, 112, 48)}},
    {{RGB(110, 168, 255), RGB(30, 48, 78), RGB(110, 200, 210), RGB(28, 56, 60), RGB(150, 190, 255)},
     {RGB(24, 96, 200), RGB(206, 224, 250), RGB(0, 128, 148), RGB(200, 236, 240), RGB(40, 110, 210)}},
    {{RGB(110, 214, 140), RGB(30, 66, 42), RGB(176, 212, 112), RGB(52, 60, 28), RGB(150, 230, 170)},
     {RGB(22, 128, 60), RGB(208, 238, 216), RGB(96, 128, 20), RGB(230, 238, 200), RGB(30, 140, 70)}},
    {{RGB(178, 140, 255), RGB(52, 36, 84), RGB(232, 140, 214), RGB(66, 36, 60), RGB(200, 170, 255)},
     {RGB(110, 56, 200), RGB(226, 214, 248), RGB(176, 40, 140), RGB(246, 214, 236), RGB(120, 70, 210)}},
};
static uint16_t th_c[TC_N];

#define C_BG th_c[TC_BG]
#define C_PANEL th_c[TC_PANEL]
#define C_LINE th_c[TC_LINE]
#define C_DIM th_c[TC_DIM]
#define C_MID th_c[TC_MID]
#define C_INK th_c[TC_INK]
#define C_WHITE th_c[TC_WHITE]
#define C_KPANEL th_c[TC_KPANEL]
#define C_KDIV th_c[TC_KDIV]
#define C_KTRACK th_c[TC_KTRACK]
#define C_KOFF th_c[TC_KOFF]
#define C_K1 th_c[TC_K1]
#define C_K2 th_c[TC_K2]
#define C_K3 th_c[TC_K3]
#define C_K4 th_c[TC_K4]
#define C_O_PANEL th_c[TC_O_PANEL]
#define C_O_LINE th_c[TC_O_LINE]
#define C_O_DIM th_c[TC_O_DIM]
#define C_O_MID th_c[TC_O_MID]
#define C_SYNTH_D th_c[TC_SYNTH_D]
#define C_CLOCK th_c[TC_CLOCK]
#define C_RING th_c[TC_RING]
#define C_SYNTH th_c[TC_SYNTH]
#define C_RED th_c[TC_RED]
#define C_EXC th_c[TC_EXC]
#define C_SPARK th_c[TC_SPARK]
#define C_STRIKE th_c[TC_STRIKE]
#define C_TAPE th_c[TC_TAPE]
#define C_PERC th_c[TC_PERC]
#define C_STRIKE_D th_c[TC_STRIKE_D]
#define C_TAPE_D th_c[TC_TAPE_D]
#define C_PERC_D th_c[TC_PERC_D]
#define C_TEAL th_c[TC_TEAL]
#define C_TEAL_D th_c[TC_TEAL_D]
#define C_AMBER th_c[TC_AMBER]
#define C_AMBER_D th_c[TC_AMBER_D]
#define C_TRIG th_c[TC_TRIG]

static uint16_t th_tint(uint16_t c)                  /* c, seven eighths of the way to white */
{
    int r = c >> 11, g = (c >> 5) & 63, b = c & 31;
    r += (31 - r) * 7 >> 3;
    g += (63 - g) * 7 >> 3;
    b += (31 - b) * 7 >> 3;
    return (uint16_t)(r << 11 | g << 5 | b);
}

static void theme_apply(int light, int accent)
{
    int i;
    light &= 1;
    for (i = 0; i < TC_N; i++)
        th_c[i] = th_dark[i];
    if (light) {
        for (i = 0; i < TC_NINV; i++)
            th_c[i] = (uint16_t)~th_dark[i];
        th_c[TC_STRIKE_D] = th_tint(th_dark[TC_STRIKE]);
        th_c[TC_TAPE_D] = th_tint(th_dark[TC_TAPE]);
        th_c[TC_PERC_D] = th_tint(th_dark[TC_PERC]);
    }
    for (i = 0; i < 5; i++)
        th_c[TC_TEAL + i] = th_acc[accent & 3][light][i];
}

static void g_band(int y0)
{
    g_y0 = y0;
    g_y1 = y0 + GB_H;
}

static void g_send(void) { plat_blit(0, g_y0, PLAT_W, GB_H, gb); }

static inline void g_pset(int x, int y, uint16_t c)
{
    if (x >= g_cx0 && x < g_cx1 && y >= g_y0 && y < g_y1)
        gb[(y - g_y0) * PLAT_W + x] = c;
}

/* a fill with a fixed grain: each pixel nudged a step lighter or darker by a hash of where it is,
 * so the surface reads as a material rather than a flat colour (the same grain every frame) */
static void g_grain(int x, int y, int w, int h, uint16_t c)
{
    int x1 = x + w, y1 = y + h, i, j;
    uint16_t lo = (uint16_t)(c - 0x0821u), hi = (uint16_t)(c + 0x0821u);   /* one step down, one up, in each channel */
    if (x < 0)
        x = 0;
    if (x1 > PLAT_W)
        x1 = PLAT_W;
    if (y < g_y0)
        y = g_y0;
    if (y1 > g_y1)
        y1 = g_y1;
    for (j = y; j < y1; j++) {
        uint16_t *p = gb + (j - g_y0) * PLAT_W;
        for (i = x; i < x1; i++) {
            uint32_t hsh = (uint32_t)i * 374761393u + (uint32_t)j * 668265263u;
            hsh = (hsh ^ (hsh >> 13)) * 1274126177u;
            hsh ^= hsh >> 16;
            hsh &= 7u;
            p[i] = hsh == 0u ? lo : hsh >= 6u ? hi : c;
        }
    }
}

static void g_fill(int x, int y, int w, int h, uint16_t c)
{
    int x1 = x + w, y1 = y + h, i, j;
    if (x < g_cx0)
        x = g_cx0;
    if (x1 > g_cx1)
        x1 = g_cx1;
    if (y < g_y0)
        y = g_y0;
    if (y1 > g_y1)
        y1 = g_y1;
    for (j = y; j < y1; j++) {
        uint16_t *p = gb + (j - g_y0) * PLAT_W;
        for (i = x; i < x1; i++)
            p[i] = c;
    }
}

static void g_hline(int x, int y, int w, uint16_t c) { g_fill(x, y, w, 1, c); }
static void g_vline(int x, int y, int h, uint16_t c) { g_fill(x, y, 1, h, c); }

static void g_frame(int x, int y, int w, int h, uint16_t c)
{
    g_hline(x, y, w, c);
    g_hline(x, y + h - 1, w, c);
    g_vline(x, y, h, c);
    g_vline(x + w - 1, y, h, c);
}

static void g_seg(float x0, float y0, float x1, float y1, float w, uint16_t c);
static void g_line_px(int x0, int y0, int x1, int y1, uint16_t c);
/* lines are drawn smooth: a stroke 1.3 pixels wide (straight ones stay one pixel, crisp) */
static void g_line(int x0, int y0, int x1, int y1, uint16_t c)
{
    if (x0 == x1 || y0 == y1) {
        g_line_px(x0, y0, x1, y1, c);
        return;
    }
    if ((y0 < g_y0 - 2 && y1 < g_y0 - 2) || (y0 > g_y1 + 1 && y1 > g_y1 + 1))
        return;
    g_seg((float)x0 + 0.5f, (float)y0 + 0.5f, (float)x1 + 0.5f, (float)y1 + 0.5f, 1.3f, c);
}
static void g_line_px(int x0, int y0, int x1, int y1, uint16_t c)
{
    int dx = x1 > x0 ? x1 - x0 : x0 - x1, sx = x0 < x1 ? 1 : -1;
    int dy = y1 > y0 ? y0 - y1 : y1 - y0, sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, guard = 1000;
    if ((y0 < g_y0 && y1 < g_y0) || (y0 >= g_y1 && y1 >= g_y1))
        return;
    while (guard--) {
        int e2 = 2 * err;
        g_pset(x0, y0, c);
        if (x0 == x1 && y0 == y1)
            break;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

static void g_discf(float cx, float cy, float r, uint16_t c);
static void g_disc(int cx, int cy, int r, uint16_t c)
{
    if (cy + r < g_y0 || cy - r >= g_y1)
        return;
    g_discf((float)cx + 0.5f, (float)cy + 0.5f, (float)r + 0.3f, c);
}

static inline uint16_t g_blend(uint16_t bg, uint16_t fg, uint32_t a)   /* a 0..15 */
{
    uint32_t r = ((fg >> 11) * a + (bg >> 11) * (15u - a)) / 15u;
    uint32_t g = (((fg >> 5) & 63u) * a + ((bg >> 5) & 63u) * (15u - a)) / 15u;
    uint32_t b = ((fg & 31u) * a + (bg & 31u) * (15u - a)) / 15u;
    return (uint16_t)((r << 11) | (g << 5) | b);
}

/* a pixel covered a (0..1) by colour c */
static inline void g_cover(int x, int y, uint16_t c, float a)
{
    uint16_t *p;
    if (x < g_cx0 || x >= g_cx1 || y < g_y0 || y >= g_y1 || a <= 0.03f)
        return;
    p = &gb[(y - g_y0) * PLAT_W + x];
    *p = a >= 0.97f ? c : g_blend(*p, c, (uint32_t)(a * 15.0f + 0.5f));
}

/* a stroke from (x0, y0) to (x1, y1), w pixels wide, round-ended, anti-aliased */
static void g_seg(float x0, float y0, float x1, float y1, float w, uint16_t c)
{
    float r = w * 0.5f, dx = x1 - x0, dy = y1 - y0, l2 = dx * dx + dy * dy, ext = r + 1.0f;
    int xa = (int)(fm_minf(x0, x1) - ext), xb = (int)(fm_maxf(x0, x1) + ext) + 1;
    int ya = (int)(fm_minf(y0, y1) - ext), yb = (int)(fm_maxf(y0, y1) + ext) + 1, x, y;
    if (ya < g_y0)
        ya = g_y0;
    if (yb > g_y1)
        yb = g_y1;
    if (xa < g_cx0)                                  /* (the clip across: the melt's strips) */
        xa = g_cx0;
    if (xb > g_cx1)
        xb = g_cx1;
    for (y = ya; y < yb; y++)
        for (x = xa; x < xb; x++) {
            float px = (float)x + 0.5f - x0, py = (float)y + 0.5f - y0, k = l2 > 1e-6f ? (px * dx + py * dy) / l2 : 0.0f, ex, ey, d2;
            k = k < 0.0f ? 0.0f : k > 1.0f ? 1.0f : k;
            ex = px - k * dx;
            ey = py - k * dy;
            d2 = ex * ex + ey * ey;
            if (d2 < ext * ext) {
                float a = r + 0.5f - fm_sqrtf(d2);
                if (a > 0.0f) {
                    uint16_t *p = &gb[(y - g_y0) * PLAT_W + x];
                    if (a >= 1.0f)
                        *p = c;
                    else
                        *p = g_blend(*p, c, (uint32_t)(a * 15.0f + 0.5f));
                }
            }
        }
}

/* a filled disc and a ring, anti-aliased (centre and radius in pixels, as floats) */
static void g_discf(float cx, float cy, float r, uint16_t c)
{
    int x, y;
    for (y = (int)(cy - r - 1); y <= (int)(cy + r + 1); y++)
        for (x = (int)(cx - r - 1); x <= (int)(cx + r + 1); x++) {
            float dx = (float)x + 0.5f - cx, dy = (float)y + 0.5f - cy;
            g_cover(x, y, c, r + 0.5f - fm_sqrtf(dx * dx + dy * dy));
        }
}
static void g_ring(float cx, float cy, float r, float w, uint16_t c)
{
    int x, y;
    for (y = (int)(cy - r - w); y <= (int)(cy + r + w); y++)
        for (x = (int)(cx - r - w); x <= (int)(cx + r + w); x++) {
            float dx = (float)x + 0.5f - cx, dy = (float)y + 0.5f - cy, d = fm_sqrtf(dx * dx + dy * dy) - r;
            g_cover(x, y, c, w * 0.5f + 0.5f - (d < 0.0f ? -d : d));
        }
}

/* a rounded box: filled, or its outline one pixel wide (r: the corners' radius) */
static void g_rbox(int x, int y, int w, int h, int r, uint16_t c, int filled)
{
    int j;
    if (r * 2 > w)
        r = w / 2;
    if (r * 2 > h)
        r = h / 2;
    if (y + h <= g_y0 || y >= g_y1)
        return;
    if (filled) {
        g_fill(x, y + r, w, h - 2 * r, c);
        g_fill(x + r, y, w - 2 * r, r, c);
        g_fill(x + r, y + h - r, w - 2 * r, r, c);
        for (j = 0; j < 4; j++) {                     /* the corners: quarter discs */
            float cx = (float)(x + ((j & 1) ? w - r : r)), cy = (float)(y + ((j & 2) ? h - r : r));
            int xx, yy, x0 = (j & 1) ? x + w - r : x, y0 = (j & 2) ? y + h - r : y;
            for (yy = y0; yy < y0 + r; yy++)
                for (xx = x0; xx < x0 + r; xx++) {
                    float dx = (float)xx + 0.5f - cx, dy = (float)yy + 0.5f - cy;
                    g_cover(xx, yy, c, (float)r + 0.5f - fm_sqrtf(dx * dx + dy * dy));
                }
        }
    } else {
        g_fill(x + r, y, w - 2 * r, 1, c);
        g_fill(x + r, y + h - 1, w - 2 * r, 1, c);
        g_fill(x, y + r, 1, h - 2 * r, c);
        g_fill(x + w - 1, y + r, 1, h - 2 * r, c);
        for (j = 0; j < 4; j++) {
            float cx = (float)(x + ((j & 1) ? w - r : r)), cy = (float)(y + ((j & 2) ? h - r : r));
            int xx, yy, x0 = (j & 1) ? x + w - r : x, y0 = (j & 2) ? y + h - r : y;
            for (yy = y0; yy < y0 + r; yy++)
                for (xx = x0; xx < x0 + r; xx++) {
                    float dx = (float)xx + 0.5f - cx, dy = (float)yy + 0.5f - cy, d = fm_sqrtf(dx * dx + dy * dy) - ((float)r - 0.5f);
                    g_cover(xx, yy, c, 1.0f - (d < 0.0f ? -d : d));
                }
        }
    }
}

static uint32_t g_glyph(const rv_font_t *f, uint32_t ch)
{
    if (ch >= 'a' && ch <= 'z' && (f->last < 'a' || !f->adv[ch - f->first]))
        ch -= 32u;
    if (ch < f->first || ch > f->last || f->adv[ch - f->first] == 0)
        ch = ch == ' ' ? ' ' : '-';
    if (ch < f->first || ch > f->last)
        ch = f->first;
    return ch - f->first;
}

#ifdef RAVEL_TEXTCHECK                                  /* (a host check: text that runs off the screen, named once) */
#include <stdio.h>
#include <string.h>
static void g_text_check(int x, int y, const rv_font_t *f, const char *s)
{
    static char seen[512][48];
    static int ns;
    int w = 0, i;
    const char *p;
    for (p = s; *p; p++)
        w += f->adv[g_glyph(f, (uint8_t)*p)];
    if (x >= 0 && x + w <= PLAT_W + 1)
        return;
    for (i = 0; i < ns; i++)
        if (!strncmp(seen[i], s, 47))
            return;
    if (ns < 512)
        snprintf(seen[ns++], 48, "%s", s);
    fprintf(stderr, "TEXTCHECK off-screen x=%d..%d y=%d h=%d: \"%s\"\n", x, x + w, y, f->h, s);
}
#endif
static int g_text(int x, int y, const rv_font_t *f, const char *s, uint16_t c)
{
#ifdef RAVEL_TEXTCHECK
    g_text_check(x, y, f, s);
#endif
    if (y + f->h < g_y0 || y >= g_y1) {
        for (; *s; s++)
            x += f->adv[g_glyph(f, (uint8_t)*s)];
        return x;
    }
    for (; *s; s++) {
        uint32_t gi = g_glyph(f, (uint8_t)*s), gx, gy, w = f->bw[gi], bpr = (w + 1u) / 2u;
        const uint8_t *gd = f->data + f->off[gi];
        for (gy = 0; gy < f->h && w; gy++) {
            int py = y + (int)gy;
            uint16_t *row;
            if (py < g_y0 || py >= g_y1)
                continue;
            row = gb + (py - g_y0) * PLAT_W;
            for (gx = 0; gx < w; gx++) {
                uint32_t a = (gd[gy * bpr + gx / 2u] >> ((gx & 1u) ? 0u : 4u)) & 15u;
                int px = x + f->bx[gi] + (int)gx;
                if (!a || px < g_cx0 || px >= g_cx1)
                    continue;
                row[px] = a == 15u ? c : g_blend(row[px], c, a);
            }
        }
        x += f->adv[gi];
    }
    return x;
}

static int g_text_w(const rv_font_t *f, const char *s)
{
    int w = 0;
    for (; *s; s++)
        w += f->adv[g_glyph(f, (uint8_t)*s)];
    return w;
}

static void g_text_c(int cx, int y, const rv_font_t *f, const char *s, uint16_t c) { g_text(cx - g_text_w(f, s) / 2, y, f, s, c); }
static void g_text_r(int rx, int y, const rv_font_t *f, const char *s, uint16_t c) { g_text(rx - g_text_w(f, s), y, f, s, c); }

static uint16_t g_mix(uint16_t a, uint16_t b, float t)
{
    int k = (int)(t * 15.0f + 0.5f);
    if (k < 0)
        k = 0;
    if (k > 15)
        k = 15;
    return g_blend(a, b, (uint32_t)k);
}
