#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <sys/mman.h>

#include "disp.h"

void screen_refresh(tDispInfo *disp_info, tRGB565Pixel color_pixel)
{
    uint16_t v = color_pixel.value;
    if (v == 0 || v == 0xFFFF) {
        // Accerlate the process
        memset(disp_info->fbp, v == 0 ? 0 : 0xFF,
               disp_info->pixels * sizeof(uint16_t));
    }
    else {
        // No better way
        uint32_t vv   = ((uint32_t)v << 16) | v;
        long i        = 0;
        uint32_t *p32 = (uint32_t *)(disp_info->fbp);
        long n32      = disp_info->pixels / 2;
        for (; i < n32; ++i) {
            p32[i] = vv;
        }
        if (disp_info->pixels & 1) {
            disp_info->fbp[disp_info->pixels - 1] = v;
        }
    }
}

void screen_draw_pixel(tDispInfo *disp_info, int x, int y,
                       tRGB565Pixel color_pixel)
{
    if (x < 0 || x >= disp_info->vinfo.xres || y < 0
        || y >= disp_info->vinfo.yres) {
        return; // Out of bounds
    }

    uint16_t v                                    = color_pixel.value;
    disp_info->fbp[y * disp_info->vinfo.xres + x] = v;
}

void screen_fill(tDispInfo *disp_info, tRGB565Pixel color_pixel, int x_1,
                 int y_1, int x_2, int y_2)
{
    if (x_1 > x_2) {
        int temp = x_1;
        x_1      = x_2;
        x_2      = temp;
    }
    if (y_1 > y_2) {
        int temp = y_1;
        y_1      = y_2;
        y_2      = temp;
    }

    x_1 = (x_1 < 0) ? 0 : x_1;
    x_2 = (x_2 > disp_info->vinfo.xres) ? disp_info->vinfo.xres : x_2;
    y_1 = (y_1 < 0) ? 0 : y_1;
    y_2 = (y_2 > disp_info->vinfo.yres) ? disp_info->vinfo.yres : y_2;

    uint16_t v     = color_pixel.value;
    int fill_width = x_2 - x_1;
    if (fill_width <= 0)
        return;

    for (int y = y_1; y < y_2; y++) {
        uint16_t *line_start = disp_info->fbp + y * disp_info->vinfo.xres + x_1;
        if (v == 0 || v == 0xFFFF) {
            memset(line_start, v == 0 ? 0 : 0xFF,
                   fill_width * sizeof(uint16_t));
        }
        else {
            uint32_t vv   = ((uint32_t)v << 16) | v;
            int i         = 0;
            uint32_t *p32 = (uint32_t *)line_start;
            int n32       = fill_width / 2;
            for (; i < n32; ++i) {
                p32[i] = vv;
            }
            if (fill_width & 1) {
                line_start[fill_width - 1] = v;
            }
        }
    }
}

tDispInfo *fb_display_init()
{
    tDispInfo *disp_info = malloc(sizeof(tDispInfo));

    disp_info->fp = open(FB_DEV, O_RDWR);
    if (disp_info->fp < 0) {
        perror("Error opening framebuffer");
        exit(1);
    }

    if (ioctl(disp_info->fp, FBIOGET_FSCREENINFO, &(disp_info->finfo))) {
        perror("Error reading fixed info");
        close(disp_info->fp);
        exit(2);
    }

    if (ioctl(disp_info->fp, FBIOGET_VSCREENINFO, &(disp_info->vinfo))) {
        perror("Error reading variable info");
        close(disp_info->fp);
        exit(3);
    }

    printf("The mem is :%u\n", disp_info->finfo.smem_len);
    printf("The line_length is :%d\n", disp_info->finfo.line_length);
    printf("The xres is :%d\n", disp_info->vinfo.xres);
    printf("The yres is :%d\n", disp_info->vinfo.yres);
    printf("bits_per_pixel is :%d\n", disp_info->vinfo.bits_per_pixel);

    // Total Pixels
    disp_info->pixels = disp_info->vinfo.xres * disp_info->vinfo.yres;

    // Total Size in Bytes
    disp_info->buffer_size =
        disp_info->pixels * (disp_info->vinfo.bits_per_pixel / 8);

    // Verify the size
    if (disp_info->buffer_size > disp_info->finfo.smem_len) {
        fprintf(stderr, "Calculated size %ld > framebuffer size %u\n",
                disp_info->buffer_size, disp_info->finfo.smem_len);
        close(disp_info->fp);
        exit(4);
    }

    // Map the framebuffer to memory
    disp_info->fbp = mmap(NULL, disp_info->buffer_size, PROT_READ | PROT_WRITE,
                          MAP_SHARED, disp_info->fp, 0);
    if (disp_info->fbp == MAP_FAILED) {
        perror("Error mapping framebuffer");
        close(disp_info->fp);
        exit(5);
    }

    printf("Mapped %ld pixels (%ld bytes)\n", disp_info->pixels,
           disp_info->buffer_size);

    return disp_info;
}

int fb_display_deinit(tDispInfo *disp_info)
{
    if (disp_info->fbp) {
        munmap(disp_info->fbp, 0); // Unmap the framebuffer
    }
    if (disp_info->fp >= 0) {
        close(disp_info->fp); // Close the framebuffer device
    }
    return 0;
}

int fb_test()
{
    tRGB565Pixel color_pixel = {0};
    tDispInfo *disp_info     = fb_display_init();

    if (disp_info == NULL) {
        fprintf(stderr, "Failed to initialize framebuffer display\n");
        return -1;
    }

    // Clear the screen to black
    color_pixel.value = 0;
    screen_refresh(disp_info, color_pixel);
    usleep(2000000); // 2秒

    // Fill the screen with a red color
    color_pixel.comp.red   = 0x1F;
    color_pixel.comp.green = 0;
    color_pixel.comp.blue  = 0;
    screen_refresh(disp_info, color_pixel);
    usleep(2000000);

    // Fill the screen with a green color
    color_pixel.comp.red   = 0;
    color_pixel.comp.green = 0x3F;
    color_pixel.comp.blue  = 0;
    screen_refresh(disp_info, color_pixel);
    usleep(2000000);

    // Fill the screen with a blue color
    color_pixel.comp.red   = 0;
    color_pixel.comp.green = 0;
    color_pixel.comp.blue  = 0x1F;
    screen_refresh(disp_info, color_pixel);
    usleep(2000000);

    // Fill the screen with a white color
    color_pixel.comp.red   = 0x1F;
    color_pixel.comp.green = 0x3F;
    color_pixel.comp.blue  = 0x1F;
    screen_refresh(disp_info, color_pixel);
    usleep(2000000);

    // Fill a rectangle with a yellow color
    color_pixel.comp.red   = 0x1F;
    color_pixel.comp.green = 0x3F;
    color_pixel.comp.blue  = 0;
    screen_fill(disp_info, color_pixel, 10, 10, 310, 230);
    usleep(2000000);

    // Unmap the framebuffer
    fb_display_deinit(disp_info);

    return 0;
}
