#ifndef DRAW_H
#define DRAW_H

#include <gctypes.h>
//#include <math.h>
#include "wiiu.h"
#include "tga_reader.h"
#include <ft2build.h>
#include FT_FREETYPE_H

typedef union _RGBAColor {
    u32 c;
    struct {
        u8 r;
        u8 g;
        u8 b;
        u8 a;
    };
} RGBAColor;

#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif

//Function declarations for my graphics library
void drawInit();
void drawFini();
void flipBuffers();
void clearBuffers();
void fillScreen(u8 r, u8 g, u8 b, u8 a);
void drawString(int x, int line, char * string);
void drawPixel32(int x, int y, RGBAColor color);
void drawPixel(int x, int y, u8 r, u8 g, u8 b, u8 a);
void drawLine(int x1, int y1, int x2, int y2, u8 r, u8 g, u8 b, u8 a);
void drawRect(int x1, int y1, int x2, int y2, u8 r, u8 g, u8 b, u8 a);
void drawFillRect(int x1, int y1, int x2, int y2, u8 r, u8 g, u8 b, u8 a);
void drawCircle(int xCen, int yCen, int radius, u8 r, u8 g, u8 b, u8 a);
void drawFillCircle(int xCen, int yCen, int radius, u8 r, u8 g, u8 b, u8 a);
void drawCircleCircum(int cx, int cy, int x, int y, u8 r, u8 g, u8 b, u8 a);

void drawPic(int x, int y, u32 w, u32 h, float scale, u32* pixels);
void drawTGA(int x, int y, float scale, u8* fileContent);
void drawRGB5A3(int x, int y, float scale, u8* pixels);
void drawBackgroundDRC(u32 w, u32 h, u8* out);
void drawBackgroundTV(u32 w, u32 h, u8* out);

bool initFont();
void freeFont();
bool ttfFontSize(u8 w, u8 h);
void ttfFontColor32(u32 color);
void ttfFontColor(u8 r, u8 g, u8 b, u8 a);
int ttfPrintString(int x, int y, const char *string, bool wWrap, bool ceroX);
int ttfStringWidth(const char *string, s8 part);

#endif /* DRAW_H */
