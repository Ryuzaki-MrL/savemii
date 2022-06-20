#pragma once

#include "tga_reader.h"
#include <coreinit/screen.h>
#include <ft2build.h>
#include FT_FREETYPE_H

typedef union _RGBAColor {
    uint32_t c;
    struct {
        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t a;
    };
} RGBAColor;

void flipBuffers();
void clearBuffers();
void clearBuffersEx();
void drawTGA(int x, int y, float scale, uint8_t *fileContent);
void drawRGB5A3(int x, int y, float scale, uint8_t *pixels);
void drawBackgroundDRC(uint32_t w, uint32_t h, uint8_t *out);
void drawBackgroundTV(uint32_t w, uint32_t h, uint8_t *out);
auto ttfPrintString(int x, int y, char *string, bool wWrap, bool ceroX) -> int;
auto ttfStringWidth(char *string, int8_t part) -> int;
void draw_bitmap(FT_Bitmap *bitmap, FT_Int x, FT_Int y);