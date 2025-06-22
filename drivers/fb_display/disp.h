
#ifndef _DISP_H
#define _DISP_H

#include <stdint.h>
#include <stdint.h>
#include <linux/fb.h>
#include <sys/mman.h>

/*############################################################
 *  Macros & Defined
 *############################################################*/
typedef union {
    uint16_t value;
    struct {
        uint16_t blue :5; /* LSB */
        uint16_t green:6;
        uint16_t red  :5; /* MSB */
    } comp;
} tRGB565Pixel;

typedef struct {
    int fp;
    long buffer_size;
    long pixels;
    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;
    uint16_t *fbp;
} tDispInfo;

#define FB_DEV "/dev/fb0"

/*############################################################
 *  Functions
 *############################################################*/

void screen_refresh(tDispInfo *disp_info, tRGB565Pixel color_pixel);

void screen_draw_pixel(tDispInfo *disp_info, int x, int y,
                       tRGB565Pixel color_pixel);

void screen_fill(tDispInfo *disp_info, tRGB565Pixel color_pixel, int x_1,
                 int y_1, int x_2, int y_2);

tDispInfo *fb_display_init();

int fb_display_deinit(tDispInfo *disp_info);

int fb_test();

#endif /* _DISP_H */
