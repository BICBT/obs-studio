#pragma once
#include "c99defs.h"

#ifdef __cplusplus
extern "C" {
#endif

struct font_rasterizer_bitmap {
	uint8_t *data;
	uint32_t width;
	uint32_t height;
};

EXPORT bool font_rasterizer_initialize(const char *fontPath,
				       uint32_t fontHeight);

EXPORT void font_rasterizer_uninitialize();

EXPORT void
font_rasterizer_create_bitmap(int width, struct font_rasterizer_bitmap *bitmap);

EXPORT void
font_rasterizer_destroy_bitmap(struct font_rasterizer_bitmap *bitmap);

EXPORT void font_rasterize(const char *text,
			   struct font_rasterizer_bitmap *bitmap);

#ifdef __cplusplus
}
#endif
