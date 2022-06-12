#include "log_freetype.h"
#include "draw.h"

#define CONSOLE_FRAME_HEAP_TAG (0x0002B2B)
#define NUM_LINES              (16)
#define LINE_LENGTH            (128)

char queueBuffer[NUM_LINES][LINE_LENGTH];
uint32_t newLines = 0;
char renderedBuffer[NUM_LINES][LINE_LENGTH];

bool freetypeHasForeground = false;

uint8_t *frameBufferTVFrontPtr = nullptr;
uint8_t *frameBufferTVBackPtr = nullptr;
uint32_t frameBufferTVSize = 0;
uint8_t *frameBufferDRCFrontPtr = nullptr;
uint8_t *frameBufferDRCBackPtr = nullptr;
uint32_t frameBufferDRCSize = 0;
uint8_t *currTVFrameBuffer = nullptr;
uint8_t *currDRCFrameBuffer = nullptr;

RGBAColor ttfColor = {0xFFFFFFFF};
uint32_t fontColor = 0xFFFFFFFF;
uint32_t backgroundColor = 0x0B5D5E00;
FT_Library fontLibrary;
FT_Face fontFace;
uint8_t *fontBuffer;
FT_Pos cursorSpaceWidth = 0;

void drawPixel(int32_t x, int32_t y, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    uint8_t opacity = a;
    uint8_t backgroundOpacity = (255 - opacity);
    {
        uint32_t width = 1280;
        uint32_t v = (x + y * width) * 4;
        currTVFrameBuffer[v + 0] = (r * opacity + (backgroundOpacity * currTVFrameBuffer[v + 0])) / 255;
        currTVFrameBuffer[v + 1] = (g * opacity + (backgroundOpacity * currTVFrameBuffer[v + 1])) / 255;
        currTVFrameBuffer[v + 2] = (b * opacity + (backgroundOpacity * currTVFrameBuffer[v + 2])) / 255;
        currTVFrameBuffer[v + 3] = a;
    }

    {
        uint32_t width = 896;
        uint32_t v = (x + y * width) * 4;
        currDRCFrameBuffer[v + 0] = (r * opacity + (backgroundOpacity * currDRCFrameBuffer[v + 0])) / 255;
        currDRCFrameBuffer[v + 1] = (g * opacity + (backgroundOpacity * currDRCFrameBuffer[v + 1])) / 255;
        currDRCFrameBuffer[v + 2] = (b * opacity + (backgroundOpacity * currDRCFrameBuffer[v + 2])) / 255;
        currDRCFrameBuffer[v + 3] = a;
    }
}

auto initScreen() -> uint32_t {

    MEMHeapHandle heap = MEMGetBaseHeapHandle(MEM_BASE_HEAP_MEM1);
    if (frameBufferTVSize != 0u) {
        frameBufferTVFrontPtr = (uint8_t *) MEMAllocFromFrmHeapEx(heap, frameBufferTVSize, 4);
        frameBufferTVBackPtr = (uint8_t *) frameBufferTVFrontPtr + (1 * (1280 * 720 * 4));
    }

    if (frameBufferDRCSize != 0u) {
        frameBufferDRCFrontPtr = (uint8_t *) MEMAllocFromFrmHeapEx(heap, frameBufferDRCSize, 4);
        frameBufferDRCBackPtr = (uint8_t *) frameBufferDRCFrontPtr + (1 * (896 * 480 * 4));
    }

    freetypeHasForeground = true;
    OSScreenSetBufferEx(SCREEN_TV, frameBufferTVFrontPtr);
    OSScreenSetBufferEx(SCREEN_DRC, frameBufferDRCFrontPtr);

    OSScreenPutPixelEx(SCREEN_TV, 0, 0, 0xABCDEFFF);
    currTVFrameBuffer = (frameBufferTVFrontPtr[0] == 0xABCDEFFF) ? frameBufferTVFrontPtr : frameBufferTVBackPtr;
    OSScreenPutPixelEx(SCREEN_DRC, 0, 0, 0xABCDEFFF);
    currDRCFrameBuffer = (frameBufferDRCFrontPtr[0] == 0xABCDEFFF) ? frameBufferDRCFrontPtr : frameBufferDRCBackPtr;
    // Cemu doesn't like writing to raw buffers unless you flip the order for some reason
    OSScreenFlipBuffersEx(SCREEN_TV);
    OSScreenFlipBuffersEx(SCREEN_DRC);

    return 0;
}

auto WHBLogFreetypeInit() -> bool {
    // Initialize screen
    OSScreenInit();
    frameBufferTVSize = OSScreenGetBufferSizeEx(SCREEN_TV);
    frameBufferDRCSize = OSScreenGetBufferSizeEx(SCREEN_DRC);

    initScreen();
    OSScreenEnableEx(SCREEN_TV, 1);
    OSScreenEnableEx(SCREEN_DRC, 1);

    // Initialize freetype2
    FT_Error result;
    if ((result = FT_Init_FreeType(&fontLibrary)) != 0) {
        return true;
    }

    uint32_t fontSize;
    OSGetSharedData(OS_SHAREDDATATYPE_FONT_STANDARD, 0, (void **) &fontBuffer, &fontSize);

    if ((result = FT_New_Memory_Face(fontLibrary, fontBuffer, fontSize, 0, &fontFace)) != 0)
        return true;
    if ((result = FT_Select_Charmap(fontFace, FT_ENCODING_UNICODE)) != 0)
        return true;
    if ((result = FT_Set_Pixel_Sizes(fontFace, 0, 22)))
        return true;

    return false;
}

void WHBLogFreetypeFree() {
    FT_Done_Face(fontFace);
    FT_Done_FreeType(fontLibrary);
    OSScreenShutdown();
}

auto ttfPrintString(int x, int y, char *string, bool wWrap, bool ceroX) -> int {
    FT_GlyphSlot slot = fontFace->glyph;
    FT_Error error;
    int pen_x = x;
    int pen_y = y;
    FT_UInt previous_glyph = 0;

    while (*string != 0) {
        uint32_t buf = *string++;

        if ((buf >> 6) == 3) {
            if ((buf & 0xF0) == 0xC0) {
                uint8_t b1 = buf & 0xFF;
                uint8_t b2 = *string++;
                if ((b2 & 0xC0) == 0x80) {
                    b2 &= 0x3F;
                }
                buf = ((b1 & 0xF) << 6) | b2;
            } else if ((buf & 0xF0) == 0xD0) {
                uint8_t b1 = buf & 0xFF;
                uint8_t b2 = *string++;
                if ((b2 & 0xC0) == 0x80) {
                    b2 &= 0x3F;
                }
                buf = 0x400 | ((b1 & 0xF) << 6) | b2;
            } else if ((buf & 0xF0) == 0xE0) {
                uint8_t b1 = buf & 0xFF;
                uint8_t b2 = *string++;
                uint8_t b3 = *string++;
                if ((b2 & 0xC0) == 0x80) {
                    b2 &= 0x3F;
                }
                if ((b3 & 0xC0) == 0x80) {
                    b3 &= 0x3F;
                }
                buf = ((b1 & 0xF) << 12) | (b2 << 6) | b3;
            }
        } else if ((buf & 0x80) != 0u) {
            string++;
            continue;
        }

        if (buf == '\n') {
            pen_y += (fontFace->size->metrics.height >> 6);
            if (ceroX)
                pen_x = 0;
            else
                pen_x = x;
            continue;
        }


        FT_UInt glyph_index;
        glyph_index = FT_Get_Char_Index(fontFace, buf);

        if (FT_HAS_KERNING(fontFace)) {
            FT_Vector vector;
            FT_Get_Kerning(fontFace, previous_glyph, glyph_index, FT_KERNING_DEFAULT, &vector);
            pen_x += (vector.x >> 6);
        }

        error = FT_Load_Glyph(fontFace, glyph_index, FT_LOAD_DEFAULT);
        if (error)
            continue;

        error = FT_Render_Glyph(fontFace->glyph, FT_RENDER_MODE_NORMAL);
        if (error)
            continue;

        if ((pen_x + (slot->advance.x >> 6)) > 853) {
            if (wWrap) {
                pen_y += (fontFace->size->metrics.height >> 6);
                if (ceroX)
                    pen_x = 0;
                else
                    pen_x = x;
            } else {
                return pen_x;
            }
        }

        draw_bitmap(&slot->bitmap, pen_x + slot->bitmap_left, (fontFace->height >> 6) + pen_y - slot->bitmap_top);

        pen_x += (slot->advance.x >> 6);
        previous_glyph = glyph_index;
    }
    return pen_x;
}

auto ttfStringWidth(char *string, int8_t part) -> int {
    FT_GlyphSlot slot = fontFace->glyph;
    FT_Error error;
    int pen_x = 0;
    int max_x = 0;
    int spart = 1;
    FT_UInt previous_glyph = 0;

    while (*string != 0) {
        uint32_t buf = *string++;
        if ((buf >> 6) == 3) {
            if ((buf & 0xF0) == 0xC0) {
                uint8_t b1 = buf & 0xFF;
                uint8_t b2 = *string++;
                if ((b2 & 0xC0) == 0x80)
                    b2 &= 0x3F;
                buf = ((b1 & 0xF) << 6) | b2;
            } else if ((buf & 0xF0) == 0xD0) {
                uint8_t b1 = buf & 0xFF;
                uint8_t b2 = *string++;
                if ((b2 & 0xC0) == 0x80)
                    b2 &= 0x3F;
                buf = 0x400 | ((b1 & 0xF) << 6) | b2;
            } else if ((buf & 0xF0) == 0xE0) {
                uint8_t b1 = buf & 0xFF;
                uint8_t b2 = *string++;
                uint8_t b3 = *string++;
                if ((b2 & 0xC0) == 0x80)
                    b2 &= 0x3F;
                if ((b3 & 0xC0) == 0x80)
                    b3 &= 0x3F;
                buf = ((b1 & 0xF) << 12) | (b2 << 6) | b3;
            }
        } else if ((buf & 0x80) != 0u) {
            string++;
            continue;
        }

        FT_UInt glyph_index;
        glyph_index = FT_Get_Char_Index(fontFace, buf);

        if (FT_HAS_KERNING(fontFace)) {
            FT_Vector vector;
            FT_Get_Kerning(fontFace, previous_glyph, glyph_index, FT_KERNING_DEFAULT, &vector);
            pen_x += (vector.x >> 6);
        }

        if (buf == '\n') {
            if (part != 0) {
                if ((part > 0) && (spart == part))
                    return pen_x;
                if (part == -2)
                    max_x = max1(pen_x, max_x);
                pen_x = 0;
                spart++;
            }
            continue;
        }

        error = FT_Load_Glyph(fontFace, glyph_index, FT_LOAD_BITMAP_METRICS_ONLY);
        if (error) {
            continue;
        }

        pen_x += (slot->advance.x >> 6);
        previous_glyph = glyph_index;
    }
    if (spart < part)
        pen_x = 0;
    return max1(pen_x, max_x);
}

void ttfFontColor32(uint32_t color) {
    ttfColor.c = color;
}

void ttfFontColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    RGBAColor color = {.r = r, .g = g, .b = b, .a = a};
    ttfFontColor32(color.c);
}

void WHBLogFreetypeDraw() {
    currTVFrameBuffer = (currTVFrameBuffer == frameBufferTVFrontPtr) ? frameBufferTVBackPtr : frameBufferTVFrontPtr;
    currDRCFrameBuffer = (currDRCFrameBuffer == frameBufferDRCFrontPtr) ? frameBufferDRCBackPtr
                                                                        : frameBufferDRCFrontPtr;
}
