// font rasterizer utilities using stb_truetype https://github.com/nothings/stb/blob/master/stb_truetype.h
// example code: https://github.com/justinmeiners/stb-truetype-example/blob/master/main.c
#include "font-rasterizer.h"
#include "base.h"
#include <stdio.h>
#include <stdlib.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb/stb_truetype.h"

unsigned char *rasterizer_font_buffer = NULL;
stbtt_fontinfo rasterizer_font_info;
uint32_t rasterizer_font_height = 0;

bool font_rasterizer_initialize(const char *fontPath, uint32_t fontHeight)
{
	rasterizer_font_height = fontHeight;

	/* load font file */
	long size = 0;
	FILE *fontFile = fopen(fontPath, "rb");
	if (!fontFile) {
		blog(LOG_ERROR, "Failed to load font from %s", fontPath);
		return false;
	}
	fseek(fontFile, 0, SEEK_END);
	size = ftell(fontFile);       /* how long is the file ? */
	fseek(fontFile, 0, SEEK_SET); /* reset */

	rasterizer_font_buffer = malloc(size);

	fread(rasterizer_font_buffer, size, 1, fontFile);
	fclose(fontFile);

	/* prepare font */
	if (!stbtt_InitFont(&rasterizer_font_info, rasterizer_font_buffer, 0)) {
		blog(LOG_ERROR, "Failed to initialize font rasterizer");
		return false;
	}

	return true;
}

void font_rasterizer_uninitialize()
{
	if (rasterizer_font_buffer) {
		free(rasterizer_font_buffer);
	}
}

void font_rasterizer_create_bitmap(int width,
				   struct font_rasterizer_bitmap *bitmap)
{
	uint32_t height = rasterizer_font_height;
	bitmap->data = calloc(width * height, sizeof(uint8_t));
	bitmap->width = width;
	bitmap->height = height;
}

void font_rasterizer_destroy_bitmap(struct font_rasterizer_bitmap *bitmap)
{
	free(bitmap->data);
	bitmap->data = NULL;
	bitmap->width = 0;
	bitmap->height = 0;
}

void font_rasterize(const char *text, struct font_rasterizer_bitmap *bitmap)
{
	if (rasterizer_font_height == 0) {
		return;
	}

	uint32_t width = bitmap->width;
	uint32_t height = bitmap->height;

	/* calculate font scaling */
	float scale = stbtt_ScaleForPixelHeight(&rasterizer_font_info, height);

	int x = 0;

	int ascent, descent, lineGap;
	stbtt_GetFontVMetrics(&rasterizer_font_info, &ascent, &descent,
			      &lineGap);

	ascent = roundf(ascent * scale);
	descent = roundf(descent * scale);

	for (size_t i = 0; i < strlen(text); ++i) {
		/* how wide is this character */
		int ax;
		int lsb;
		stbtt_GetCodepointHMetrics(&rasterizer_font_info, text[i], &ax,
					   &lsb);

		/* get bounding box for character (may be offset to account for chars that dip above or below the line */
		int c_x1, c_y1, c_x2, c_y2;
		stbtt_GetCodepointBitmapBox(&rasterizer_font_info, text[i],
					    scale, scale, &c_x1, &c_y1, &c_x2,
					    &c_y2);

		/* compute y (different characters have different heights */
		int y = ascent + c_y1;

		/* render character (stride and offset is important here) */
		int byteOffset = x + roundf(lsb * scale) + (y * width);
		stbtt_MakeCodepointBitmap(&rasterizer_font_info,
					  bitmap->data + byteOffset,
					  c_x2 - c_x1, c_y2 - c_y1, width,
					  scale, scale, text[i]);

		/* advance x */
		x += roundf(ax * scale);

		/* add kerning */
		int kern;
		kern = stbtt_GetCodepointKernAdvance(&rasterizer_font_info,
						     text[i], text[i + 1]);
		x += roundf(kern * scale);
	}
}