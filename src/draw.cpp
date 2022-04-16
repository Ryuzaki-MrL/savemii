#include "draw.h"
#include "log_freetype.h"

uint8_t *scrBuffer;
bool cur_buf1;
size_t tvBufferSize = 0;
size_t drcBufferSize = 0;

void *tvBuffer;
void *drcBuffer;

uint8_t *ttfFont;
RGBAColor fcolor = {0xFFFFFFFF};
FT_Library library;
FT_Face face;

void flipBuffers() {
    DCFlushRange(tvBuffer, tvBufferSize);
    DCFlushRange(drcBuffer, drcBufferSize);

    OSScreenFlipBuffersEx(SCREEN_TV);
    OSScreenFlipBuffersEx(SCREEN_DRC);
}

void clearBuffers() {
    OSScreenClearBufferEx(SCREEN_TV, 0x00000000);
    OSScreenClearBufferEx(SCREEN_DRC, 0x00000000);
    flipBuffers();
}

void clearBuffersEx() {
    OSScreenClearBufferEx(SCREEN_TV, 0x00000000);
    OSScreenClearBufferEx(SCREEN_DRC, 0x00000000);
}

void fillScreen(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    RGBAColor color;
    color.r = r;
    color.g = g;
    color.b = b;
    color.a = a;
    clearBuffersEx();
}

void drawPixel32(int x, int y, RGBAColor color) {
    drawPixel(x, y, color.r, color.g, color.b, color.a);
}

void drawLine(int x1, int y1, int x2, int y2, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {

    int x;
    int y;
    if (x1 == x2) {
        if (y1 < y2) {
            for (y = y1; y <= y2; y++) {
                drawPixel(x1, y, r, g, b, a);
            }
        } else {
            for (y = y2; y <= y1; y++) {
                drawPixel(x1, y, r, g, b, a);
            }
        }
    } else {
        if (x1 < x2) {
            for (x = x1; x <= x2; x++) {
                drawPixel(x, y1, r, g, b, a);
            }
        } else {
            for (x = x2; x <= x1; x++) {
                drawPixel(x, y1, r, g, b, a);
            }
        }
    }
}

void drawRect(int x1, int y1, int x2, int y2, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    drawLine(x1, y1, x2, y1, r, g, b, a);
    drawLine(x2, y1, x2, y2, r, g, b, a);
    drawLine(x1, y2, x2, y2, r, g, b, a);
    drawLine(x1, y1, x1, y2, r, g, b, a);
}

void drawFillRect(int x1, int y1, int x2, int y2, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    int X1;
    int X2;
    int Y1;
    int Y2;
    int i;
    int j;

    if (x1 < x2) {
        X1 = x1;
        X2 = x2;
    } else {
        X1 = x2;
        X2 = x1;
    }

    if (y1 < y2) {
        Y1 = y1;
        Y2 = y2;
    } else {
        Y1 = y2;
        Y2 = y1;
    }
    for (i = X1; i <= X2; i++) {
        for (j = Y1; j <= Y2; j++) {
            drawPixel(i, j, r, g, b, a);
        }
    }
}

void drawCircle(int xCen, int yCen, int radius, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    int x = 0;
    int y = radius;
    int p = (5 - radius * 4) / 4;
    drawCircleCircum(xCen, yCen, x, y, r, g, b, a);
    while (x < y) {
        x++;
        if (p < 0) {
            p += 2 * x + 1;
        } else {
            y--;
            p += 2 * (x - y) + 1;
        }
        drawCircleCircum(xCen, yCen, x, y, r, g, b, a);
    }
}

void drawFillCircle(int xCen, int yCen, int radius, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    drawCircle(xCen, yCen, radius, r, g, b, a);
    int x;
    int y;
    for (y = -radius; y <= radius; y++) {
        for (x = -radius; x <= radius; x++) {
            if (x * x + y * y <= radius * radius + radius * .8f) {
                drawPixel(xCen + x, yCen + y, r, g, b, a);
            }
        }
    }
}

void drawCircleCircum(int cx, int cy, int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {

    if (x == 0) {
        drawPixel(cx, cy + y, r, g, b, a);
        drawPixel(cx, cy - y, r, g, b, a);
        drawPixel(cx + y, cy, r, g, b, a);
        drawPixel(cx - y, cy, r, g, b, a);
    }
    if (x == y) {
        drawPixel(cx + x, cy + y, r, g, b, a);
        drawPixel(cx - x, cy + y, r, g, b, a);
        drawPixel(cx + x, cy - y, r, g, b, a);
        drawPixel(cx - x, cy - y, r, g, b, a);
    }
    if (x < y) {
        drawPixel(cx + x, cy + y, r, g, b, a);
        drawPixel(cx - x, cy + y, r, g, b, a);
        drawPixel(cx + x, cy - y, r, g, b, a);
        drawPixel(cx - x, cy - y, r, g, b, a);
        drawPixel(cx + y, cy + x, r, g, b, a);
        drawPixel(cx - y, cy + x, r, g, b, a);
        drawPixel(cx + y, cy - x, r, g, b, a);
        drawPixel(cx - y, cy - x, r, g, b, a);
    }
}

void drawPic(int x, int y, uint32_t w, uint32_t h, float scale, uint32_t *pixels) {
    uint32_t nw = w, nh = h;
    RGBAColor color;
    if (scale <= 0) scale = 1;
    nw = w * scale;
    nh = h * scale;


    for (uint32_t j = y; j < (y + nh); j++) {
        for (uint32_t i = x; i < (x + nw); i++) {
            color.c = pixels[((i - x) * w / nw) + ((j - y) * h / nh) * w];
            drawPixel32(i, j, color);
        }
    }
}

void drawTGA(int x, int y, float scale, uint8_t *fileContent) {
    uint32_t w = tgaGetWidth(fileContent), h = tgaGetHeight(fileContent);
    uint32_t nw = w, nh = h;
    auto *out = (uint32_t *) tgaRead(fileContent, TGA_READER_RGBA);

    if (scale <= 0) scale = 1;
    nw = w * scale;
    nh = h * scale;

    drawPic(x, y, w, h, scale, out);
    if (scale > 0.5) {
        if ((x > 0) && (y > 0)) drawRect(x - 1, y - 1, x + nw, y + nh, 255, 255, 255, 128);
        if ((x > 1) && (y > 1)) drawRect(x - 2, y - 2, x + nw + 1, y + nh + 1, 255, 255, 255, 128);
        if ((x > 2) && (y > 2)) drawRect(x - 3, y - 3, x + nw + 2, y + nh + 2, 255, 255, 255, 128);
    }
    free(out);
}

void drawRGB5A3(int x, int y, float scale, uint8_t *fileContent) {
    uint32_t w = 192;
    uint32_t h = 64;
    uint32_t nw = w;
    uint32_t nh = h;

    if (scale <= 0) {
        scale = 1;
    }
    nw = w * scale;
    nh = h * scale;

    uint32_t pos = 0;
    uint32_t npos = 0;
    RGBAColor color;
    auto *pixels = (uint16_t *) fileContent;

    uint8_t sum = (4 * scale);
    for (uint32_t j = y; j < (y + nh); j += sum) {
        for (uint32_t i = x; i < (x + nw); i += sum) {
            for (uint32_t sj = j; sj < (j + sum); sj++, pos++) {
                for (uint32_t si = i; si < (i + sum); si++) {
                    npos = ((si - i) / scale) + ((pos / scale) * 4);
                    if ((pixels[npos] & 0x8000) != 0) {
                        color.c = ((pixels[npos] & 0x7C00) << 17) | ((pixels[npos] & 0x3E0) << 14) |
                                  ((pixels[npos] & 0x1F) << 11) | 0xFF;
                    } else {
                        color.c = (((pixels[npos] & 0xF00) * 0x11) << 16) | (((pixels[npos] & 0xF0) * 0x11) << 12) |
                                  (((pixels[npos] & 0xF) * 0x11) << 8) | (((pixels[npos] & 0x7000) >> 12) * 0x24);
                    }
                    drawPixel32(si, sj, color);
                }
            }
        }
    }
    if (scale > 0.5) {
        if ((x > 0) && (y > 0)) {
            drawRect(x - 1, y - 1, x + nw, y + nh, 255, 255, 255, 128);
        }
        if ((x > 1) && (y > 1)) {
            drawRect(x - 2, y - 2, x + nw + 1, y + nh + 1, 255, 255, 255, 128);
        }
        if ((x > 2) && (y > 2)) {
            drawRect(x - 3, y - 3, x + nw + 2, y + nh + 2, 255, 255, 255, 128);
        }
    }
}

void drawBackgroundDRC(uint32_t w, uint32_t h, uint8_t *out) {
    uint32_t *screen2 = nullptr;
    int otherBuff1 = drcBufferSize / 2;

    if (cur_buf1) {
        screen2 = (uint32_t *) scrBuffer + tvBufferSize + otherBuff1;
    } else {
        screen2 = (uint32_t *) scrBuffer + tvBufferSize;
    }
    memcpy(screen2, out, w * h * 4);
}

void drawBackgroundTV(uint32_t w, uint32_t h, uint8_t *out) {
    auto *screen1 = (uint32_t *) scrBuffer;
    int otherBuff0 = tvBufferSize / 2;

    if (cur_buf1) {
        screen1 = (uint32_t *) scrBuffer + otherBuff0;
    }
    memcpy(screen1, out, w * h * 4);
}

void draw_bitmap(FT_Bitmap *bitmap, FT_Int x, FT_Int y) {
    FT_Int i, j, p, q;
    FT_Int x_max;
    FT_Int y_max = y + bitmap->rows;

    switch (bitmap->pixel_mode) {
        case FT_PIXEL_MODE_GRAY:
            x_max = x + bitmap->width;
            for (i = x, p = 0; i < x_max; i++, p++) {
                for (j = y, q = 0; j < y_max; j++, q++) {
                    if (i < 0 || j < 0 || i >= 854 || j >= 480) continue;
                    uint8_t col = bitmap->buffer[q * bitmap->pitch + p];
                    if (col == 0) continue;
                    float opacity = col / 255.0;
                    drawPixel(i, j, fcolor.r, fcolor.g, fcolor.b, (uint8_t) (fcolor.a * opacity));
                }
            }
            break;
        case FT_PIXEL_MODE_LCD:
            x_max = x + bitmap->width / 3;
            for (i = x, p = 0; i < x_max; i++, p++) {
                for (j = y, q = 0; j < y_max; j++, q++) {
                    if (i < 0 || j < 0 || i >= 854 || j >= 480) continue;
                    uint8_t cr = bitmap->buffer[q * bitmap->pitch + p * 3];
                    uint8_t cg = bitmap->buffer[q * bitmap->pitch + p * 3 + 1];
                    uint8_t cb = bitmap->buffer[q * bitmap->pitch + p * 3 + 2];

                    if ((cr | cg | cb) == 0) continue;
                    drawPixel(i, j, cr, cg, cb, 255);
                }
            }
            break;
        case FT_PIXEL_MODE_BGRA:
            x_max = x + bitmap->width / 2;
            for (i = x, p = 0; i < x_max; i++, p++) {
                for (j = y, q = 0; j < y_max; j++, q++) {
                    if (i < 0 || j < 0 || i >= 854 || j >= 480) continue;
                    uint8_t cb = bitmap->buffer[q * bitmap->pitch + p * 4];
                    uint8_t cg = bitmap->buffer[q * bitmap->pitch + p * 4 + 1];
                    uint8_t cr = bitmap->buffer[q * bitmap->pitch + p * 4 + 2];
                    uint8_t ca = bitmap->buffer[q * bitmap->pitch + p * 4 + 3];

                    if ((cr | cg | cb) == 0) continue;
                    drawPixel(i, j, cr, cg, cb, ca);
                }
            }
            break;
    }
}