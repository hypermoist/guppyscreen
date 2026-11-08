/*
 * Resolution-independent icons for the Android build.
 *
 * The desktop and printer builds compile assets/material (64px) or
 * assets/material_46 bitmaps; a phone or tablet screen wants them two to
 * five times that, and zooming a 64px bitmap up only blurs it. Here every
 * icon is instead a 256px PNG (assets/material_png, embedded by
 * embed_assets.cmake) behind an LV_IMG_CF_USER_ENCODED_0 descriptor of the
 * same name whose data is a grumpy_icon_src_t, and this decoder hands LVGL
 * each one resampled once to the size the screen needs: the 64px the 800x480
 * layout was drawn for, scaled as the rest of the UI scales (Theme::scale_r).
 * Code that fits icons to their space only ever shrinks them, so they stay
 * sharp.
 *
 * The icons are white masks recoloured by style, as the C arrays are, so
 * only the PNG's alpha is used.
 */
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "lvgl/lvgl.h"
#include <android/log.h>
#include "src/extra/libs/png/lodepng.h"
#if LV_USE_TINY_TTF
#include "src/extra/libs/tiny_ttf/lv_tiny_ttf.h"
#endif

#if LV_COLOR_DEPTH != 32
#error "grumpy_icons.c writes 32-bit pixels"
#endif

#define SOURCE_PX 256
// the size the 64px assets were drawn for on an 800x480 screen, per scale_r
// unit (scale_r: px * min(w / 480, h / 272); 800x480 gives 5/3)
#define BASE_PX_PER_R (64.0 * 3 / 5)
// never below the small screen's 46px set
#define MIN_PX 46

static int icon_px = 64;

typedef struct {
  const void *src;
  uint8_t *px;
} entry_t;
static entry_t *entries;

// what an icon descriptor's data points at (embed_assets.cmake)
typedef struct {
  const uint8_t *png;
  size_t size;
} grumpy_icon_src_t;
static int n_entries, cap_entries;

/* Box-filters n_in samples (stride in_step) down to n_out (stride out_step):
 * each output averages the source span it covers, partial pixels weighted. */
static void resample(const float *in, int n_in, int in_step, float *out, int n_out, int out_step) {
  const double scale = (double)n_in / n_out;
  for (int i = 0; i < n_out; i++) {
    const double a = i * scale, b = (i + 1) * scale;
    double sum = 0;
    for (int j = (int)a; j < n_in && j < b; j++) {
      const double lo = j < a ? a : j, hi = j + 1 > b ? b : j + 1;
      sum += in[j * in_step] * (hi - lo);
    }
    out[i * out_step] = (float)(sum / scale);
  }
}

static uint8_t *render(const lv_img_dsc_t *dsc) {
  unsigned char *rgba = NULL;
  unsigned w = 0, h = 0;
  const grumpy_icon_src_t *src = (const grumpy_icon_src_t *)dsc->data;
  if (lodepng_decode32(&rgba, &w, &h, src->png, src->size) != 0 || w == 0 || h == 0) {
    if (rgba) lv_mem_free(rgba);
    return NULL;
  }
  const int n = icon_px;
  float *alpha = malloc(sizeof(float) * w * h);
  float *rows = malloc(sizeof(float) * n * h);  // h rows of n
  float *done = malloc(sizeof(float) * n * n);
  uint8_t *out = malloc((size_t)n * n * LV_IMG_PX_SIZE_ALPHA_BYTE);
  if (alpha && rows && done && out) {
    for (unsigned i = 0; i < w * h; i++) alpha[i] = rgba[i * 4 + 3];
    for (unsigned y = 0; y < h; y++) resample(alpha + y * w, w, 1, rows + y * n, n, 1);
    for (int x = 0; x < n; x++) resample(rows + x, h, n, done + x, n, n);
    for (int i = 0; i < n * n; i++) {
      int a = (int)lroundf(done[i]);
      out[i * 4 + 0] = 0xff;  // b
      out[i * 4 + 1] = 0xff;  // g
      out[i * 4 + 2] = 0xff;  // r
      out[i * 4 + 3] = (uint8_t)(a < 0 ? 0 : a > 255 ? 255 : a);
    }
  } else {
    free(out);
    out = NULL;
  }
  free(alpha);
  free(rows);
  free(done);
  lv_mem_free(rgba);
  return out;
}

static int is_ours(const void *src) {
  return lv_img_src_get_type(src) == LV_IMG_SRC_VARIABLE &&
         ((const lv_img_dsc_t *)src)->header.cf == LV_IMG_CF_USER_ENCODED_0 &&
         ((const lv_img_dsc_t *)src)->data_size == sizeof(grumpy_icon_src_t);
}

static lv_res_t icon_info(lv_img_decoder_t *decoder, const void *src, lv_img_header_t *header) {
  LV_UNUSED(decoder);
  if (!is_ours(src)) return LV_RES_INV;
  header->always_zero = 0;
  header->cf = LV_IMG_CF_TRUE_COLOR_ALPHA;
  header->w = icon_px;
  header->h = icon_px;
  return LV_RES_OK;
}

/* Rendered on first use and kept: LVGL's image cache holds only a few. */
static lv_res_t icon_open(lv_img_decoder_t *decoder, lv_img_decoder_dsc_t *dsc) {
  LV_UNUSED(decoder);
  if (!is_ours(dsc->src)) return LV_RES_INV;
  for (int i = 0; i < n_entries; i++) {
    if (entries[i].src == dsc->src) {
      dsc->img_data = entries[i].px;
      return LV_RES_OK;
    }
  }
  uint8_t *px = render((const lv_img_dsc_t *)dsc->src);
  if (!px) return LV_RES_INV;
  if (n_entries == cap_entries) {
    int cap = cap_entries ? cap_entries * 2 : 64;
    entry_t *grown = realloc(entries, sizeof(entry_t) * cap);
    if (!grown) {
      free(px);
      return LV_RES_INV;
    }
    entries = grown;
    cap_entries = cap;
  }
  entries[n_entries].src = dsc->src;
  entries[n_entries].px = px;
  n_entries++;
  dsc->img_data = px;
  return LV_RES_OK;
}

static void icon_close(lv_img_decoder_t *decoder, lv_img_decoder_dsc_t *dsc) {
  LV_UNUSED(decoder);
  LV_UNUSED(dsc);  // the pixels stay cached
}

void grumpy_icons_init(int hor_res, int ver_res) {
  const double r = fmin(hor_res / 480.0, ver_res / 272.0);
  int px = (int)lround(BASE_PX_PER_R * r);
  icon_px = px < MIN_PX ? MIN_PX : px > SOURCE_PX ? SOURCE_PX : px;
  lv_img_decoder_t *d = lv_img_decoder_create();
  lv_img_decoder_set_info_cb(d, icon_info);
  lv_img_decoder_set_open_cb(d, icon_open);
  lv_img_decoder_set_close_cb(d, icon_close);
  __android_log_print(ANDROID_LOG_INFO, "grumpyscreen", "icons at %dpx", icon_px);
}

/* The nav bar glyphs at any size (assets/materialdesign_nav.ttf); NULL when
 * the 40px bitmap font is big enough. */
extern const uint8_t grumpy_nav_ttf[];
extern const size_t grumpy_nav_ttf_size;

const lv_font_t *android_icon_font(int px) {
#if LV_USE_TINY_TTF
  static lv_font_t *font;
  static int font_px;
  if (px <= 40) return NULL;
  if (font && font_px == px) return font;
  if (font) lv_tiny_ttf_destroy(font);
  font = lv_tiny_ttf_create_data(grumpy_nav_ttf, grumpy_nav_ttf_size, px);
  font_px = font ? px : 0;
  return font;
#else
  LV_UNUSED(px);
  return NULL;
#endif
}
