#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "disp.h"
#include "lvgl.h"
#include "lv_demos.h"

static uint8_t gLvglBuf[320 * 240 * 2 / 10];
static uint8_t gLvglBuf2[320 * 240 * 2 / 10];

void my_flush_cb(lv_display_t *display, const lv_area_t *area, uint8_t *px_map)
{
    tDispInfo *disp_info = (tDispInfo *)lv_display_get_user_data(display);
    /* The most simple case (also the slowest) to send all rendered pixels to
     * the screen one-by-one.  `put_px` is just an example.  It needs to be
     * implemented by you. */
    uint16_t *gLvglBuf6 =
        (uint16_t *)px_map; /* Let's say it's a 16 bit (RGB565) display */
    int32_t x, y;
    for (y = area->y1; y <= area->y2; y++) {
        for (x = area->x1; x <= area->x2; x++) {
            // put_px(x, y, *gLvglBuf6);
            screen_draw_pixel(disp_info, x, y,
                              (tRGB565Pixel){.value = *gLvglBuf6});
            gLvglBuf6++;
        }
    }

    usleep(LV_REFRESH_TIME * 500); // Simulate some delay for flushing

    /* IMPORTANT!!!
     * Inform LVGL that flushing is complete so buffer can be modified again. */
    lv_display_flush_ready(display);
}

int main()
{
    printf("Hello, World!\n");

    // fb_test();

    tDispInfo *disp_info = fb_display_init();

    lv_init();

    lv_display_t *display1 =
        lv_display_create(disp_info->vinfo.xres, disp_info->vinfo.yres);

    lv_display_set_buffers(display1, gLvglBuf, gLvglBuf2, sizeof(gLvglBuf),
                           LV_DISPLAY_RENDER_MODE_PARTIAL);

    lv_display_set_flush_cb(display1, my_flush_cb);

    lv_display_set_user_data(display1, (void *)disp_info);

    lv_display_set_antialiasing(display1, true);

    // lv_demo_benchmark();

    // lv_demo_widgets();

    // lv_demo_music();

    lv_demo_stress();

    while (1) {
        lv_timer_handler();

        lv_tick_inc(LV_REFRESH_TIME);
        usleep(LV_REFRESH_TIME * 1000); // About 24Hz
    }

    return 0;
}
