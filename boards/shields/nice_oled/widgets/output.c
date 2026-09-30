#include "output.h"
// #include "../assets/custom_fonts.h"
#include <fonts.h>
#include <zephyr/kernel.h>

LV_IMG_DECLARE(wifi);
LV_IMG_DECLARE(usb_icon);

/* Custom: status glyph drawn directly on the background (no black block).
 * Connected = plain WiFi; disconnected/unbonded = WiFi with a slash.
 * Both the 20x15 WiFi fan and the 20x15 USB trident occupy the same box, so
 * switching transport does not move the icon. The row shares one optical
 * centre line at y=10.5: the battery shell spans y5..16 and the percentage
 * digits y8..13, so y3 puts this glyph's ink at y3..17 and leaves 2px of
 * margin on both ends of the 68px canvas. */
#define WIFI_X 46
#define WIFI_Y 3

static void draw_wifi(lv_obj_t *canvas) {
    lv_draw_img_dsc_t img_dsc;
    lv_draw_img_dsc_init(&img_dsc);
    lv_canvas_draw_img(canvas, WIFI_X, WIFI_Y, &wifi, &img_dsc);
}

static void draw_wifi_disconnected(lv_obj_t *canvas) {
    draw_wifi(canvas);

    lv_draw_line_dsc_t line_dsc;
    init_line_dsc(&line_dsc, LVGL_FOREGROUND, 2);

    static lv_point_t slash[2];
    slash[0].x = WIFI_X + 2;
    slash[0].y = WIFI_Y + 1;
    slash[1].x = WIFI_X + 17;
    slash[1].y = WIFI_Y + 13;
    lv_canvas_draw_line(canvas, slash, 2, &line_dsc);
}

#if !IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
static void draw_usb_connected(lv_obj_t *canvas) {
    lv_draw_img_dsc_t img_dsc;
    lv_draw_img_dsc_init(&img_dsc);
    lv_canvas_draw_img(canvas, WIFI_X, WIFI_Y, &usb_icon, &img_dsc);
}
#endif

void draw_output_status(lv_obj_t *canvas, const struct status_state *state) {
#if !IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    switch (state->selected_endpoint.transport) {
    case ZMK_TRANSPORT_USB:
        draw_usb_connected(canvas);
        break;

    case ZMK_TRANSPORT_BLE:
        if (state->active_profile_bonded) {
            if (state->active_profile_connected) {
                draw_wifi(canvas);
            } else {
                draw_wifi_disconnected(canvas);
            }
        } else {
            draw_wifi_disconnected(canvas);
        }
        break;
    }
#else
    if (state->connected) {
        draw_wifi(canvas);
    } else {
        draw_wifi_disconnected(canvas);
    }
#endif
}
