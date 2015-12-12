#ifndef WALLPAPER_H
#define WALLPAPER_H

#include "globals.h"

typedef enum
{
	STYLE_NONE,
	STYLE_CENTER,
	STYLE_TILE,
	STYLE_STRETCH,
	STYLE_FIT,
	STYLE_FILL,
	STYLE_SPAN,
} WallpaperStyle;

typedef enum
{
	SHADING_SOLID,
	SHADING_VERTICAL,
	SHADING_HORIZONTAL,
} WallpaperShading;

//There are two possibilities for representing the wallpaper:
//As a filename or as RGB data.

typedef struct
{
	LPWSTR filename;
	WallpaperStyle style;
	WallpaperShading shading;
	unsigned int colorPrimary;
	unsigned int colorSecondary;
} Wallpaper;

void wallpaper_get(Wallpaper* wallpaper);
BOOL wallpaper_set(Wallpaper* wallpaper);
void wallpaper_gen(Wallpaper* wallpaper, int width, int height, unsigned char* pixels);
void wallpaper_free(Wallpaper* wallpaper);

//This is the user's wallpaper before we switch it
extern Wallpaper wallpaperOld;

#endif

