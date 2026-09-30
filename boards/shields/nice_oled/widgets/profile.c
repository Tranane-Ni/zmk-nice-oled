#include <zephyr/kernel.h>
#include "profile.h"
#if !IS_ENABLED(CONFIG_NICE_EPAPER_ON)
// use custom_fonts.h only for the draw_active_profile_text function
#include <fonts.h>
#include <stdio.h>
#endif // !IS_ENABLED(CONFIG_NICE_EPAPER_ON)

LV_IMG_DECLARE(profiles);

#if IS_ENABLED(CONFIG_NICE_OLED_WIDGET_PROFILE_BIG)
#define OFFSET_X 0
#define OFFSET_Y 129

LV_IMG_DECLARE(profile);
LV_IMG_DECLARE(profile_active);
#endif

#if !IS_ENABLED(CONFIG_NICE_OLED_WIDGET_PROFILE_BIG)

/*
 * Custom: three 16x16 five point stars (indexed 1bit), traced 1:1 off the user's
 * perler bead chart. The chart already is a 16 cell grid, so unlike the star this
 * row used to carry there is no resampling in here at all -- it is the drawing.
 * Outline star = a device slot that is not in use, filled star = the active one.
 * Layout: 16px wide on a 21px pitch, centered on the 68px canvas. That leaves 5px
 * between the stars and 5px at either end of the row, so the gaps and the margins
 * are the same measure and the row reads as evenly spread rather than as three
 * objects with air around them.
 *
 * Three slots, not five, because that is how many devices the keyboard actually
 * pairs with. active_profile_index can still outrun the row if the profile count
 * is raised again, so the filled star is skipped rather than drawn off the edge.
 */
#define STAR_WIDTH   16
#define STAR_HEIGHT  16
#define STAR_SPACING 21
#define STAR_SLOTS   3

static const uint8_t star_outline_map[] = {
#if CONFIG_NICE_OLED_WIDGET_INVERTED
    0x00, 0x00, 0x00, 0xff, /* color 0 */
    0xff, 0xff, 0xff, 0xff, /* color 1 */
#else
    0xff, 0xff, 0xff, 0xff, /* color 0 */
    0x00, 0x00, 0x00, 0xff, /* color 1 */
#endif
    0x01, 0x80, /* .......##....... */
    0x02, 0x40, /* ......#..#...... */
    0x02, 0x40, /* ......#..#...... */
    0x04, 0x20, /* .....#....#..... */
    0x04, 0x20, /* .....#....#..... */
    0xf8, 0x1f, /* #####......##### */
    0x80, 0x01, /* #..............# */
    0x40, 0x02, /* .#............#. */
    0x20, 0x04, /* ..#..........#.. */
    0x10, 0x08, /* ...#........#... */
    0x10, 0x08, /* ...#........#... */
    0x10, 0x08, /* ...#........#... */
    0x21, 0x84, /* ..#....##....#.. */
    0x22, 0x44, /* ..#...#..#...#.. */
    0x2c, 0x34, /* ..#.##....##.#.. */
    0x30, 0x0c, /* ..##........##.. */
};

static const lv_img_dsc_t star_outline = {
    .header.cf = LV_IMG_CF_INDEXED_1BIT,
    .header.always_zero = 0,
    .header.reserved = 0,
    .header.w = STAR_WIDTH,
    .header.h = STAR_HEIGHT,
    .data_size = 40,
    .data = star_outline_map,
};

static const uint8_t star_filled_map[] = {
#if CONFIG_NICE_OLED_WIDGET_INVERTED
    0x00, 0x00, 0x00, 0xff, /* color 0 */
    0xff, 0xff, 0xff, 0xff, /* color 1 */
#else
    0xff, 0xff, 0xff, 0xff, /* color 0 */
    0x00, 0x00, 0x00, 0xff, /* color 1 */
#endif
    0x01, 0x80, /* .......##....... */
    0x03, 0xc0, /* ......####...... */
    0x03, 0xc0, /* ......####...... */
    0x07, 0xe0, /* .....######..... */
    0x07, 0xe0, /* .....######..... */
    0xff, 0xff, /* ################ */
    0xff, 0xff, /* ################ */
    0x7f, 0xfe, /* .##############. */
    0x3f, 0xfc, /* ..############.. */
    0x1f, 0xf8, /* ...##########... */
    0x1f, 0xf8, /* ...##########... */
    0x1f, 0xf8, /* ...##########... */
    0x3f, 0xfc, /* ..############.. */
    0x3e, 0x7c, /* ..#####..#####.. */
    0x3c, 0x3c, /* ..####....####.. */
    0x30, 0x0c, /* ..##........##.. */
};

static const lv_img_dsc_t star_filled = {
    .header.cf = LV_IMG_CF_INDEXED_1BIT,
    .header.always_zero = 0,
    .header.reserved = 0,
    .header.w = STAR_WIDTH,
    .header.h = STAR_HEIGHT,
    .data_size = 40,
    .data = star_filled_map,
};

/*
 * Center the slot row on the drawing canvas: the row is
 * (slots-1)*spacing + width px wide.
 */
static int star_start_x(void) {
    int row_width = (STAR_SLOTS - 1) * STAR_SPACING + STAR_WIDTH;
    return (CONFIG_NICE_OLED_CUSTOM_CANVAS_WIDTH - row_width) / 2;
}

static void draw_inactive_profiles(lv_obj_t *canvas, const struct status_state *state) {
    lv_draw_img_dsc_t img_dsc;
    lv_draw_img_dsc_init(&img_dsc);

    int start_x = star_start_x();

    for (int i = 0; i < STAR_SLOTS; i++) {
        lv_canvas_draw_img(canvas, start_x + (i * STAR_SPACING),
                           CONFIG_NICE_OLED_WIDGET_PROFILE_CUSTOM_Y, &star_outline, &img_dsc);
    }
}

static void draw_active_profile(lv_obj_t *canvas, const struct status_state *state) {
    if (state->active_profile_index < 0 || state->active_profile_index >= STAR_SLOTS)
        return;

    lv_draw_img_dsc_t img_dsc;
    lv_draw_img_dsc_init(&img_dsc);

    int start_x = star_start_x();
    int offset = state->active_profile_index * STAR_SPACING;

    lv_canvas_draw_img(canvas, start_x + offset,
                       CONFIG_NICE_OLED_WIDGET_PROFILE_CUSTOM_Y, &star_filled, &img_dsc);
}
#endif // !IS_ENABLED(CONFIG_NICE_OLED_WIDGET_PROFILE_BIG)

#if !IS_ENABLED(CONFIG_NICE_EPAPER_ON)
static void draw_active_profile_text(lv_obj_t *canvas, const struct status_state *state) {
    // new label_dsc
    lv_draw_label_dsc_t label_dsc;
    init_label_dsc(&label_dsc, LVGL_FOREGROUND, &pixel_operator_mono_8, LV_TEXT_ALIGN_LEFT);

    char text[14] = {};
    snprintf(text, sizeof(text), "%d", state->active_profile_index + 1);

    lv_canvas_draw_text(canvas, CONFIG_NICE_OLED_WIDGET_PROFILE_TEXT_CUSTOM_X, CONFIG_NICE_OLED_WIDGET_PROFILE_TEXT_CUSTOM_Y, 35, &label_dsc, text);
}
#endif // CONFIG_NICE_EPAPER_ON

void draw_profile_status(lv_obj_t *canvas, const struct status_state *state) {
#if !IS_ENABLED(CONFIG_NICE_EPAPER_ON)
    draw_active_profile_text(canvas, state);
#endif // CONFIG_NICE_EPAPER_ON

#if IS_ENABLED(CONFIG_NICE_OLED_WIDGET_PROFILE_BIG) //  && IS_ENABLED(CONFIG_NICE_EPAPER_ON)
    lv_draw_img_dsc_t img_dsc;
    lv_draw_img_dsc_init(&img_dsc);

    for (int i = 0; i < 5; i++) {
        lv_canvas_draw_img(canvas, OFFSET_X + (i * 14), OFFSET_Y,
                           i == state->active_profile_index ? &profile_active : &profile, &img_dsc);
    }
#else
    draw_inactive_profiles(canvas, state);
    draw_active_profile(canvas, state);
#endif
}
