#include "draw.h"

unsigned char *scrBuffer;
int scr_buf0_size = 0;
int scr_buf1_size = 0;
bool cur_buf1;

u8 *ttfFont;
RGBAColor fcolor = {0xFFFFFFFF};
FT_Library  library;
FT_Face     face;

void initDraw(char* buf, int size0, int size1) {
	scrBuffer = buf;
	scr_buf0_size = size0;
	scr_buf1_size = size1;

	clearBuffers();
	uint32_t *screen2 = scrBuffer + scr_buf0_size;
	OSScreenPutPixelEx(1, 0, 0, 0xABCDEFFF);

	if (screen2[0] == 0xABCDEFFF) cur_buf1 = false;
	else cur_buf1 = true;
}

void flipBuffers() {
	//Flush the cache
	DCFlushRange(scrBuffer + scr_buf0_size, scr_buf1_size);
	DCFlushRange(scrBuffer, scr_buf0_size);
	//Flip the buffer
	OSScreenFlipBuffersEx(0);
	OSScreenFlipBuffersEx(1);
	cur_buf1 = !cur_buf1;
}

void clearBuffers() {
    for(int i = 0; i < 2; i++) {
        OSScreenClearBufferEx(0, 0);
        OSScreenClearBufferEx(1, 0);
        flipBuffers();
    }
}

void drawString(int x, int line, char* string) {
	OSScreenPutFontEx(0, x, line, string);
	OSScreenPutFontEx(1, x, line, string);
}

void fillScreen(u8 r, u8 g, u8 b, u8 a) {
	RGBAColor color;
	color.r = r; color.g = g; color.b = b; color.a = a;
	//uint32_t num = (r << 24) | (g << 16) | (b << 8) | a;
	OSScreenClearBufferEx(0, color.c);
	OSScreenClearBufferEx(1, color.c);
}

void drawPixel32(int x, int y, RGBAColor color) {
	drawPixel(x, y, color.r, color.g, color.b, color.a);
}

void drawPixel(int x, int y, u8 r, u8 g, u8 b, u8 a) {
	/*uint32_t num = (r << 24) | (g << 16) | (b << 8) | a;
	OSScreenPutPixelEx(0, x, y, num);
	OSScreenPutPixelEx(1, x, y, num);*/

	int width = 1280; //height = 1024 720?
	char *screen = scrBuffer;
	int otherBuff0 = scr_buf0_size / 2;
	int otherBuff1 = scr_buf1_size / 2;
	float opacity = a / 255.0;

	for (int yy = (y * 1.5); yy < ((y * 1.5) + 1); yy++) {
		for (int xx = (x * 1.5); xx < ((x * 1.5) + 1); xx++) {
			u32 v = (xx + yy * width) * 4;
			if (cur_buf1) v += otherBuff0;
			screen[v    ] = r * opacity + (1 - opacity) * screen[v];
			screen[v + 1] = g * opacity + (1 - opacity) * screen[v + 1];
			screen[v + 2] = b * opacity + (1 - opacity) * screen[v + 2];
			screen[v + 3] = a;
		}
	}
	/*u32 v = (x + y * width) * 4;
	if (cur_buf1) v += otherBuff0;
	screen[v    ] = r * opacity + (1 - opacity) * screen[v];
	screen[v + 1] = g * opacity + (1 - opacity) * screen[v + 1];
	screen[v + 2] = b * opacity + (1 - opacity) * screen[v + 2];
	screen[v + 3] = a;*/

	width = 896; //height = 480;
	char *screen2 = scrBuffer + scr_buf0_size;
	u32 v = (x + y * width) * 4;
	if (cur_buf1) v += otherBuff1;
	screen2[v    ] = r * opacity + (1 - opacity) * screen2[v];
	screen2[v + 1] = g * opacity + (1 - opacity) * screen2[v + 1];
	screen2[v + 2] = b * opacity + (1 - opacity) * screen2[v + 2];
	screen2[v + 3] = a;
}

void drawLine(int x1, int y1, int x2, int y2, u8 r, u8 g, u8 b, u8 a) {

	int x, y;
	if (x1 == x2){
		if (y1 < y2) for (y = y1; y <= y2; y++) drawPixel(x1, y, r, g, b, a);
		else for (y = y2; y <= y1; y++) drawPixel(x1, y, r, g, b, a);
	}
	else {
		if (x1 < x2) for (x = x1; x <= x2; x++) drawPixel(x, y1, r, g, b, a);
		else for (x = x2; x <= x1; x++) drawPixel(x, y1, r, g, b, a);
	}
}

void drawRect(int x1, int y1, int x2, int y2, u8 r, u8 g, u8 b, u8 a) {
	drawLine(x1, y1, x2, y1, r, g, b, a);
	drawLine(x2, y1, x2, y2, r, g, b, a);
	drawLine(x1, y2, x2, y2, r, g, b, a);
	drawLine(x1, y1, x1, y2, r, g, b, a);
}

void drawFillRect(int x1, int y1, int x2, int y2, u8 r, u8 g, u8 b, u8 a) {
	int X1, X2, Y1, Y2, i, j;

	if (x1 < x2){
		X1 = x1;
		X2 = x2;
	}
	else {
		X1 = x2;
		X2 = x1;
	}

	if (y1 < y2){
		Y1 = y1;
		Y2 = y2;
	}
	else {
		Y1 = y2;
		Y2 = y1;
	}
	for (i = X1; i <= X2; i++){
		for (j = Y1; j <= Y2; j++){
			drawPixel(i, j, r, g, b, a);
		}
	}
}

void drawCircle(int xCen, int yCen, int radius, u8 r, u8 g, u8 b, u8 a) {
	int x = 0;
	int y = radius;
	int p = (5 - radius * 4) / 4;
	drawCircleCircum(xCen, yCen, x, y, r, g, b, a);
	while (x < y){
		x++;
		if (p < 0){
			p += 2 * x + 1;
		}
		else{
			y--;
			p += 2 * (x - y) + 1;
		}
		drawCircleCircum(xCen, yCen, x, y, r, g, b, a);
	}
}

void drawFillCircle(int xCen, int yCen, int radius, u8 r, u8 g, u8 b, u8 a) {
	drawCircle(xCen, yCen, radius, r, g, b, a);
	int x, y;
	for (y = -radius; y <= radius; y++){
		for (x = -radius; x <= radius; x++)
			if (x*x + y*y <= radius*radius + radius * .8f)
				drawPixel(xCen + x, yCen + y, r, g, b, a);
	}
}

void drawCircleCircum(int cx, int cy, int x, int y, u8 r, u8 g, u8 b, u8 a) {

	if (x == 0){
		drawPixel(cx, cy + y, r, g, b, a);
		drawPixel(cx, cy - y, r, g, b, a);
		drawPixel(cx + y, cy, r, g, b, a);
		drawPixel(cx - y, cy, r, g, b, a);
	}
	if (x == y){
		drawPixel(cx + x, cy + y, r, g, b, a);
		drawPixel(cx - x, cy + y, r, g, b, a);
		drawPixel(cx + x, cy - y, r, g, b, a);
		drawPixel(cx - x, cy - y, r, g, b, a);
	}
	if (x < y){
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

void drawPic(int x, int y, u32 w, u32 h, float scale, u32* pixels) {
	u32 nw = w, nh = h;
	RGBAColor color;
	if (scale <= 0) scale = 1;
	nw = w * scale; nh = h * scale;


	for (u32 j = y; j < (y + nh); j++) {
		for (u32 i = x; i < (x + nw); i++) {
			color.c = pixels[((i - x) * w / nw) + ((j - y) * h / nh) * w];
			drawPixel32(i, j, color);
		}
	}
}

void drawTGA(int x, int y, float scale, u8* fileContent) {
	u32 w = tgaGetWidth(fileContent), h = tgaGetHeight(fileContent);
	u32 nw = w, nh = h;
	u32* out = tgaRead(fileContent, TGA_READER_RGBA);

	if (scale <= 0) scale = 1;
	nw = w * scale; nh = h * scale;

	drawPic(x, y, w, h, scale, out);
	if (scale > 0.5) {
		if ((x > 0) && (y > 0)) drawRect(x - 1, y - 1, x + nw, y + nh, 255, 255, 255, 128);
		if ((x > 1) && (y > 1)) drawRect(x - 2, y - 2, x + nw + 1, y + nh + 1, 255, 255, 255, 128);
		if ((x > 2) && (y > 2)) drawRect(x - 3, y - 3, x + nw + 2, y + nh + 2, 255, 255, 255, 128);
	}
	free(out);
}

void drawRGB5A3(int x, int y, float scale, u8* fileContent) {
	u32 w = 192, h = 64, num = 0;
	u32 nw = w, nh = h;

	if (scale <= 0) scale = 1;
	nw = w * scale; nh = h * scale;

	u32 pos = 0, npos = 0;
	RGBAColor color;
	u16* pixels = (u16*)fileContent;

	u8 sum = (4 * scale);
	for (u32 j = y; j < (y + nh); j += sum) {
		for (u32 i = x; i < (x + nw); i += sum) {
			for (u32 sj = j; sj < (j + sum); sj++, pos++) {
				for (u32 si = i; si < (i + sum); si++) {
					npos = ((si - i) / scale) + ((pos / scale) * 4);
					if (pixels[npos] & 0x8000)
						color.c = ((pixels[npos] & 0x7C00) << 17) | ((pixels[npos] & 0x3E0) << 14) | ((pixels[npos] & 0x1F) << 11) | 0xFF;
					else
						color.c = (((pixels[npos] & 0xF00) * 0x11) << 16) | (((pixels[npos] & 0xF0) * 0x11) << 12) | (((pixels[npos] & 0xF) * 0x11) << 8) | (((pixels[npos] & 0x7000) >> 12) * 0x24);
					drawPixel32(si, sj, color);
				}
			}
		}
	}
	if (scale > 0.5) {
		if ((x > 0) && (y > 0)) drawRect(x - 1, y - 1, x + nw, y + nh, 255, 255, 255, 128);
		if ((x > 1) && (y > 1)) drawRect(x - 2, y - 2, x + nw + 1, y + nh + 1, 255, 255, 255, 128);
		if ((x > 2) && (y > 2)) drawRect(x - 3, y - 3, x + nw + 2, y + nh + 2, 255, 255, 255, 128);
	}
}

void drawBackgroundDRC(u32 w, u32 h, u8* out) {
	uint32_t *screen2 = NULL;
	int otherBuff1 = scr_buf1_size / 2;

	if (cur_buf1) screen2 = scrBuffer + scr_buf0_size + otherBuff1;
	else screen2 = scrBuffer + scr_buf0_size;
	memcpy(screen2, out, w * h * 4);
}

void drawBackgroundTV(u32 w, u32 h, u8* out) {
	uint32_t *screen1 = scrBuffer;
	int otherBuff0 = scr_buf0_size / 2;

	if (cur_buf1) screen1 = scrBuffer + otherBuff0;
	memcpy(screen1, out, w * h * 4);
}

bool initFont(void* fontBuf, FT_Long fsize) {
    FT_Long size = fsize;
	if (fontBuf) {
		ttfFont = fontBuf;
	} else {
    	OSGetSharedData(2, 0, &ttfFont, &size);
	}

	FT_Error error;
	error = FT_Init_FreeType(&library);
	if (error) return false;

	error = FT_New_Memory_Face(library, ttfFont, size, 0, &face);
	if (error) return false;

	/*error = FT_Set_Char_Size(face, 8*64, 10*64, 158, 158);
	if (error) return false;*/

	error = FT_Set_Pixel_Sizes(face, 0, 22);   //pixel width, height
	if (error) return false;

	return true;
}

void freeFont(void* fontBuf) {
	FT_Done_Face(face);
  	FT_Done_FreeType(library);
	//if (fontBuf) free(fontBuf);
}

void draw_bitmap(FT_Bitmap* bitmap, FT_Int x, FT_Int y) {
	FT_Int i, j, p, q;
	FT_Int x_max;
	FT_Int y_max = y + bitmap->rows;

	switch(bitmap->pixel_mode) {
		case FT_PIXEL_MODE_GRAY:
			x_max = x + bitmap->width;
			for (i = x, p = 0; i < x_max; i++, p++) {
				for (j = y, q = 0; j < y_max; j++, q++) {
					if (i < 0 || j < 0 || i >= 854 || j >= 480) continue;
					u8 col = bitmap->buffer[q * bitmap->pitch + p];
					if (col == 0) continue;
					float opacity = col / 255.0;
					/*u8 cr = fcolor.r * opacity + (1 - opacity) * 0;
					u8 cg = fcolor.g * opacity + (1 - opacity) * 0;
					u8 cb = fcolor.b * opacity + (1 - opacity) * 0;
					drawPixel(i, j, cr, cg, cb, fcolor.a);*/
					drawPixel(i, j, fcolor.r, fcolor.g, fcolor.b, (u8)(fcolor.a * opacity));
				}
			}
			break;
		case FT_PIXEL_MODE_LCD:
			x_max = x + bitmap->width / 3;
			for (i = x, p = 0; i < x_max; i++, p++) {
				for (j = y, q = 0; j < y_max; j++, q++) {
					if (i < 0 || j < 0 || i >= 854 || j >= 480) continue;
					u8 cr = bitmap->buffer[q * bitmap->pitch + p * 3];
					u8 cg = bitmap->buffer[q * bitmap->pitch + p * 3 + 1];
					u8 cb = bitmap->buffer[q * bitmap->pitch + p * 3 + 2];

					if ((cr | cg | cb) == 0) continue;
					drawPixel(i, j, cr, cg, cb, 255);
				}
			}
			break;
		// case FT_PIXEL_MODE_BGRA:
		// 	x_max = x + bitmap->width/2;
		// 	for (i = x, p = 0; i < x_max; i++, p++) {
		// 		for (j = y, q = 0; j < y_max; j++, q++) {
		// 			if (i < 0 || j < 0 || i >= 854 || j >= 480) continue;
		// 			u8 cb = bitmap->buffer[q * bitmap->pitch + p * 4];
		// 			u8 cg = bitmap->buffer[q * bitmap->pitch + p * 4 + 1];
		// 			u8 cr = bitmap->buffer[q * bitmap->pitch + p * 4 + 2];
		// 			u8 ca = bitmap->buffer[q * bitmap->pitch + p * 4 + 3];
        //
		// 			if ((cr | cg | cb) == 0) continue;
		// 			drawPixel(i, j, cr, cg, cb, ca);
		// 		}
		// 	}
		// 	break;
	}
}

bool ttfFontSize(u8 w, u8 h) {
	FT_Error error;

	/*error = FT_Set_Char_Size(face, 8*64, 10*64, 158, 158);
	if (error) return false;*/

	error = FT_Set_Pixel_Sizes(face, w, h);   //pixel width, height
	if (error) return false;
	return true;
}

void ttfFontColor32(u32 color) {
	fcolor.c = color;
}

void ttfFontColor(u8 r, u8 g, u8 b, u8 a) {
	RGBAColor color = {.r = r, .g = g, .b = b, .a = a};
	ttfFontColor32(color.c);
}

int ttfPrintString(int x, int y, char *string, bool wWrap, bool ceroX) {
	FT_GlyphSlot slot = face->glyph;
	FT_Error error;
	int pen_x = x, pen_y = y;
	FT_UInt previous_glyph;

    while(*string) {
		uint32_t buf = *string++;
		int dy = 0;

		if ((buf >> 6) == 3) {
			if ((buf & 0xF0) == 0xC0) {
				uint8_t b1 = buf & 0xFF, b2 = *string++;
				if ((b2 & 0xC0) == 0x80) b2 &= 0x3F;
				buf = ((b1 & 0xF) << 6) | b2;
			} else if ((buf & 0xF0) == 0xD0) {
				uint8_t b1 = buf & 0xFF, b2 = *string++;
				if ((b2 & 0xC0) == 0x80) b2 &= 0x3F;
				buf = 0x400 | ((b1 & 0xF) << 6) | b2;
			} else if ((buf & 0xF0) == 0xE0) {
				uint8_t b1 = buf & 0xFF, b2 = *string++, b3 = *string++;
				if ((b2 & 0xC0) == 0x80) b2 &= 0x3F;
				if ((b3 & 0xC0) == 0x80) b3 &= 0x3F;
				buf = ((b1 & 0xF) << 12) | (b2 << 6) | b3;
			}
		} else if (buf & 0x80) {
			string++;
			continue;
		}

		if (buf == '\n') {
			pen_y += (face->size->metrics.height >> 6);
			if (ceroX) pen_x = 0;
			else pen_x = x;
			continue;
		}

		//error = FT_Load_Char(face, buf, FT_LOAD_RENDER);

        FT_UInt glyph_index;
		glyph_index = FT_Get_Char_Index(face, buf);

		if (FT_HAS_KERNING(face)) {
			FT_Vector vector;
			FT_Get_Kerning(face, previous_glyph, glyph_index, FT_KERNING_DEFAULT, &vector);
			pen_x += (vector.x >> 6);
			dy = vector.y >> 6;
		}

		error = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);//FT_LOAD_COLOR);//
		if (error)
			continue;

		error = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);//FT_RENDER_MODE_LCD);//
		if (error)
			continue;

		if ((pen_x + (slot->advance.x >> 6)) > 853) {
			if (wWrap) {
				pen_y += (face->size->metrics.height >> 6);
				if (ceroX) pen_x = 0;
				else pen_x = x;
			} else {
				return pen_x;
			}
		}

		draw_bitmap(&slot->bitmap, pen_x + slot->bitmap_left, (face->height >> 6) + pen_y - slot->bitmap_top);

		pen_x += (slot->advance.x >> 6);
		previous_glyph = glyph_index;
	}
	return pen_x;
}

int ttfStringWidth(char *string, s8 part) {
	FT_GlyphSlot slot = face->glyph;
	FT_Error error;
	int pen_x = 0, max_x = 0, spart = 1;
	FT_UInt previous_glyph;

    while(*string) {
		uint32_t buf = *string++;
		if ((buf >> 6) == 3) {
			if ((buf & 0xF0) == 0xC0) {
				uint8_t b1 = buf & 0xFF, b2 = *string++;
				if ((b2 & 0xC0) == 0x80) b2 &= 0x3F;
				buf = ((b1 & 0xF) << 6) | b2;
			} else if ((buf & 0xF0) == 0xD0) {
				uint8_t b1 = buf & 0xFF, b2 = *string++;
				if ((b2 & 0xC0) == 0x80) b2 &= 0x3F;
				buf = 0x400 | ((b1 & 0xF) << 6) | b2;
			} else if ((buf & 0xF0) == 0xE0) {
				uint8_t b1 = buf & 0xFF, b2 = *string++, b3 = *string++;
				if ((b2 & 0xC0) == 0x80) b2 &= 0x3F;
				if ((b3 & 0xC0) == 0x80) b3 &= 0x3F;
				buf = ((b1 & 0xF) << 12) | (b2 << 6) | b3;
			}
		} else if (buf & 0x80) {
			string++;
			continue;
		}

		//error = FT_Load_Char(face, buf, FT_LOAD_RENDER);

        FT_UInt glyph_index;
		glyph_index = FT_Get_Char_Index(face, buf);

		if (FT_HAS_KERNING(face)) {
			FT_Vector vector;
			FT_Get_Kerning(face, previous_glyph, glyph_index, FT_KERNING_DEFAULT, &vector);
			pen_x += (vector.x >> 6);
		}

		if (buf == '\n') {
			if (part != 0) {
				if ((part > 0) && (spart == part)) return pen_x;
				if (part == -2) max_x = max(pen_x, max_x);
				pen_x = 0;
				spart++;
			}
			continue;
		}

		error = FT_Load_Glyph(face, glyph_index, 1/*FT_LOAD_BITMAP_METRICS_ONLY*/);
		if (error)
			continue;

		pen_x += (slot->advance.x >> 6);
		previous_glyph = glyph_index;
	}
	if (spart < part) pen_x = 0;
	return max(pen_x, max_x);
}
