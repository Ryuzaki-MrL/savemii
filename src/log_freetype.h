#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <coreinit/cache.h>
#include <coreinit/memfrmheap.h>
#include <coreinit/memheap.h>
#include <coreinit/memory.h>
#include <coreinit/screen.h>

#include <freetype2/ft2build.h>
#include FT_FREETYPE_H

#ifndef max1
#define max1(a, b) (((a) > (b)) ? (a) : (b))
#endif

// Initialization functions
bool WHBLogFreetypeInit();

void WHBLogFreetypeFree();

void drawPixel(int32_t x, int32_t y, uint8_t r, uint8_t g, uint8_t b, uint8_t a);

uint32_t initScreen();

void ttfFontColor32(uint32_t color);

void ttfFontColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a);

void WHBLogFreetypeDraw();
