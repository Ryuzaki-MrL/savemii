#ifndef _LOG_FREETYPE_H
#define _LOG_FREETYPE_H


#include <cstdint>
#include <cstring>

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
auto WHBLogFreetypeInit() -> bool;

void WHBLogFreetypeFree();

void drawPixel(int32_t x, int32_t y, uint8_t r, uint8_t g, uint8_t b, uint8_t a);

auto initScreen() -> uint32_t;

void ttfFontColor32(uint32_t color);

void ttfFontColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a);

void WHBLogFreetypeDraw();

#endif