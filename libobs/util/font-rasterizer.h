#pragma once
#include "c99defs.h"

#ifdef __cplusplus
extern "C" {
#endif

EXPORT bool font_rasterizer_initialize(const char *fontPath);

EXPORT void font_rasterizer_uninitialize();

EXPORT void font_rasterize(const char *text, int width, int height, unsigned char* bitmap);

#ifdef __cplusplus
}
#endif
