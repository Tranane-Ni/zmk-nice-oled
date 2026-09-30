#include "layer.h"
#include <fonts.h>
#include <stdio.h>
#include <zephyr/kernel.h>

#include "char_count.h"

/*
 * The separator between the layer number and the typed-character count: the
 * 11x9 filled heart, same one widgets/profile.c used to spend on the device
 * slots before those became stars. One byte pair per row of pixels, MSB is the
 * leftmost pixel, palette swapped so the ink is white on the inverted panel.
 *
 * Filled rather than outlined, because at 11x9 sitting between two rows of text
 * an outline reads as a smudge. It is also the only heart left on the screen, so
 * it has to carry the shape on its own.
 */
#define HEART_WIDTH  11
#define HEART_HEIGHT 9
#define HEART_STRIDE ((HEART_WIDTH + 7) / 8)

/* Spacing worked out against the widest row the count can make, five digits.
 * The count is anchored to the right edge, so its ink lands on x29..65 and
 * 29 columns are left for the other two pieces: a 5px digit and an 11px heart.
 * 29 - 5 - 11 = 13 columns of margin, 5 to the left of the digit and 4 either
 * side of the heart, so its ink runs x14..24. mono16 puts a digit 1px into its
 * cell, hence the +4.
 *
 * The +2 lines the heart up with the digits rather than with the top of the line
 * box: mono16 has a 13px line box, a 2px descender and 9px tall digits, so their
 * ink runs rows 2..10, and the heart is exactly 9 rows. */
#define LAYER_DIGIT_X (CONFIG_NICE_OLED_WIDGET_LAYER_CUSTOM_X + 4)
#define HEART_X 14
#define HEART_Y (CONFIG_NICE_OLED_WIDGET_LAYER_CUSTOM_Y + 2)

static const uint8_t heart_map[] = {
#if CONFIG_NICE_OLED_WIDGET_INVERTED
    0x00, 0x00, 0x00, 0xff, /* color 0 */
    0xff, 0xff, 0xff, 0xff, /* color 1 */
#else
    0xff, 0xff, 0xff, 0xff, /* color 0 */
    0x00, 0x00, 0x00, 0xff, /* color 1 */
#endif
    0x71, 0xc0, /* .###...###. */
    0xfb, 0xe0, /* #####.##### */
    0xff, 0xe0, /* ########### */
    0xff, 0xe0, /* ########### */
    0x7f, 0xc0, /* .#########. */
    0x3f, 0x80, /* ..#######.. */
    0x1f, 0x00, /* ...#####... */
    0x0e, 0x00, /* ....###.... */
    0x04, 0x00, /* .....#..... */
};

static const lv_img_dsc_t heart = {
    .header.cf = LV_IMG_CF_INDEXED_1BIT,
    .header.always_zero = 0,
    .header.reserved = 0,
    .header.w = HEART_WIDTH,
    .header.h = HEART_HEIGHT,
    .data_size = 8 + HEART_STRIDE * HEART_HEIGHT,
    .data = heart_map,
};

void draw_layer_status(lv_obj_t *canvas, const struct status_state *state) {
    lv_draw_label_dsc_t label_dsc;
    init_label_dsc(&label_dsc, LVGL_FOREGROUND, &pixel_operator_mono_16, LV_TEXT_ALIGN_LEFT);

    /* Three pieces, not one string: the count is pinned to the right edge so it
     * grows leftwards, and the layer number and the heart are spaced off it. The
     * boxes are the full 68px because LVGL wraps a line that outgrows it. */
    char layer_text[4] = {};
    char count_text[8] = {};

    snprintf(layer_text, sizeof(layer_text), "%u", (unsigned)state->layer_index);
    snprintf(count_text, sizeof(count_text), "%u", (unsigned)zmk_char_count_get());

    lv_canvas_draw_text(canvas, LAYER_DIGIT_X, CONFIG_NICE_OLED_WIDGET_LAYER_CUSTOM_Y, 68,
                        &label_dsc, layer_text);

    lv_draw_img_dsc_t img_dsc;
    lv_draw_img_dsc_init(&img_dsc);
    lv_canvas_draw_img(canvas, HEART_X, HEART_Y, &heart, &img_dsc);

    lv_draw_label_dsc_t right_dsc = label_dsc;
    right_dsc.align = LV_TEXT_ALIGN_RIGHT;
    lv_canvas_draw_text(canvas, CONFIG_NICE_OLED_WIDGET_LAYER_CUSTOM_X,
                        CONFIG_NICE_OLED_WIDGET_LAYER_CUSTOM_Y, 68, &right_dsc, count_text);
}
