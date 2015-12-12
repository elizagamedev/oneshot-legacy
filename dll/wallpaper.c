#include "wallpaper.h"
#include "util.h"

enum {
    WINETYPE_NONE,
    WINETYPE_X11,
    WINETYPE_GNOME,
    WINETYPE_OSX,
};

static int wallpaper_winetype = WINETYPE_NONE;

Wallpaper wallpaperOld = {0};

#if 0
static inline void scale(unsigned char *dst, int dstWidth, int dstX, int dstY, int dstW, int dstH, unsigned char *src, int srcWidth, int srcX, int srcY, int srcW, int srcH, int dstBpp)
{
    int x, y;
    for (y = 0; y < dstH; y++) {
        for (x = 0; x < dstW; x++) {
            int indexDst = ((dstY + y) * dstWidth + (dstX + x)) * dstBpp;
            int indexSrc = ((srcY + (y * srcH / dstH)) * srcWidth + (srcX + (x * srcW / dstW))) * 3;
            dst[indexDst + 0] = src[indexSrc + 0];
            dst[indexDst + 1] = src[indexSrc + 1];
            dst[indexDst + 2] = src[indexSrc + 2];
        }
    }
}
#endif

static inline void imgcpy(unsigned char *dst, int dstW, int dstX, int dstY, unsigned char *src, int srcW, int srcH, int dstBpp)
{
    int x, y;
    for (y = 0; y < srcH; y++) {
        for (x = 0; x < srcW; x++) {
            int indexDst = ((dstY + y) * dstW + (dstX + x)) * dstBpp;
            int indexSrc = ((y * srcW) + x) * 3;
            dst[indexDst + 0] = src[indexSrc + 0];
            dst[indexDst + 1] = src[indexSrc + 1];
            dst[indexDst + 2] = src[indexSrc + 2];
        }
    }
}

void wallpaper_get(Wallpaper *wallpaper)
{
    memset(wallpaper, 0, sizeof(Wallpaper));

    if (winver == WIN_UNSUPPORTED)
        return;

    if (winver == WIN_WINE) {
        if (wallpaper_winetype == WINETYPE_NONE)
            wallpaper_winetype = util_sh(L"wallpaper query");

        util_sh(L"wallpaper get");
        LPWSTR buff = util_sh_return();
        size_t size = wcslen(buff) + 1;

        if (wallpaper_winetype == WINETYPE_GNOME) {
            // Parse it for other info
            int i = 0, index = 0;
            WCHAR *start = buff;
            for (; i < size; i++) {
                if (buff[i] == '\n')
                    buff[i] = 0;

                if (!buff[i]) {
                    switch (index++) {
                    case 0:
                        wallpaper->filename = wcsdup(buff);
                        break;
                    case 1:
                        wallpaper->style = _wtoi(start);
                        break;
                    case 2:
                        wallpaper->shading = _wtoi(start);
                        break;
                    case 3:
                        wallpaper->colorPrimary = (int)wcstol(start, NULL, 16);
                        break;
                    case 4:
                        wallpaper->colorSecondary = (int)wcstol(start, NULL, 16);
                        break;
                    }
                    start = buff + i + 1;
                }
            }
            free(buff);
        } else {
            wallpaper->filename = buff;
        }
        return;
    }

    WCHAR szStyle[8] = {0};
    WCHAR szTile[8] = {0};

    DWORD szStyleSize = sizeof(szStyle) - 1;
    DWORD szTileSize = sizeof(szTile) - 1;

    // Transcoded/cached JPEG wallpaper
    WCHAR szFile[MAX_PATH+1];

    // ------------------
    // QUERY THE REGISTRY/STUFF
    // ------------------

    HKEY hKey = NULL;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Control Panel\\Desktop", 0, KEY_READ, &hKey) != ERROR_SUCCESS)
        goto end;

    // Style
    if (RegQueryValueExW(hKey, L"WallpaperStyle", 0, NULL, (LPBYTE)(szStyle), &szStyleSize) != ERROR_SUCCESS)
        goto end;

    // Tile
    if (RegQueryValueExW(hKey, L"TileWallpaper", 0, NULL, (LPBYTE)(szTile), &szTileSize) != ERROR_SUCCESS)
        goto end;

    // File path
    if (!SystemParametersInfoW(SPI_GETDESKWALLPAPER, MAX_PATH, (PVOID)szFile, 0))
        goto end;

    // Color
    wallpaper->colorPrimary = (unsigned int)GetSysColor(COLOR_BACKGROUND);
    // jint jcolor = ((color & 0x0000FF) << 16) | (color & 0x00FF00) | ((color & 0xFF0000) >> 16);

    // ----------
    // PARSE DATA
    // ----------

    wallpaper->filename = wcsdup(szFile);

    // Get the enum value from the registry keys
    if (!wcscmp(szTile, L"0")) {
        if (!wcscmp(szStyle, L"2"))
            wallpaper->style = STYLE_STRETCH;
        else if (!wcscmp(szStyle, L"6"))
            wallpaper->style = STYLE_FIT;
        else if (!wcscmp(szStyle, L"10"))
            wallpaper->style = STYLE_FILL;
        else
            wallpaper->style = STYLE_CENTER;
    } else {
        wallpaper->style = STYLE_TILE;
    }

end:
    if (hKey)
        RegCloseKey(hKey);
}

BOOL wallpaper_set(Wallpaper *wallpaper)
{
    if (winver == WIN_UNSUPPORTED)
        return FALSE;
    if (winver == WIN_WINE) {
        if (wallpaper_winetype == WINETYPE_NONE)
            wallpaper_winetype = util_sh(L"wallpaper query");
        switch (wallpaper_winetype) {
        case WINETYPE_GNOME:
            return !util_sh(L"wallpaper set \"%ls\" %d %d %06x %06x", wallpaper->filename, wallpaper->style, wallpaper->shading, wallpaper->colorPrimary, wallpaper->colorSecondary);
        default:
        case WINETYPE_X11:
        case WINETYPE_OSX:
            return !util_sh(L"wallpaper set \"%ls\"", wallpaper->filename);
        }
    }

    // Set the wallpaper
    HKEY hKey = NULL;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Control Panel\\Desktop", 0, KEY_WRITE, &hKey) != ERROR_SUCCESS)
        goto end;

    LPCWSTR szStyle;
    LPCWSTR szTile;

    int style = wallpaper->style;

    // Find the correct style/tile strings
    if (style == STYLE_SPAN)
        style = STYLE_FIT;
    if (winver < WIN_7) {
        if (style == STYLE_FIT || style == STYLE_FILL)
            style = STYLE_STRETCH;
    }
    switch (style) {
    case STYLE_CENTER:
        szStyle = L"0";
        szTile = L"0";
        break;
    case STYLE_TILE:
        szStyle = L"0";
        szTile = L"1";
        break;
    case STYLE_STRETCH:
        szStyle = L"2";
        szTile = L"0";
        break;
    case STYLE_FIT:
        szStyle = L"6";
        szTile = L"0";
        break;
    case STYLE_FILL:
        szStyle = L"10";
        szTile = L"0";
        break;
    default:
        goto end;
    }

    // Set the style
    if (RegSetValueExW(hKey, L"WallpaperStyle", 0, REG_SZ, (PVOID)szStyle, (wcslen(szStyle) + 1) * 2) != ERROR_SUCCESS)
        goto end;

    if (RegSetValueExW(hKey, L"TileWallpaper", 0, REG_SZ, (PVOID)szTile, 2) != ERROR_SUCCESS)
        goto end;

    // Set the wallpaper
    if (!SystemParametersInfoW(SPI_SETDESKWALLPAPER, 0, (PVOID)wallpaper->filename, SPIF_UPDATEINIFILE))
        goto end;

    // Set the color
    int colorId = COLOR_BACKGROUND;
    DWORD color = ((wallpaper->colorPrimary & 0x0000FF) << 16) | (wallpaper->colorPrimary & 0x00FF00) | ((wallpaper->colorPrimary & 0xFF0000) >> 16);
    if (!SetSysColors(1, &colorId, (const COLORREF *)&color))
        goto end;

end:
    if (hKey)
        RegCloseKey(hKey);

    return TRUE;
}

typedef struct {
    int x, y, w, h;
} Monitor;

typedef struct {
    int index;
    void *data;
} EnumMonitorData;

BOOL CALLBACK enum_monitor_count(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
    int *nMonitors = (int *)dwData;
    (*nMonitors)++;

    return TRUE;
}

BOOL CALLBACK enum_monitor_data(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
    EnumMonitorData *data = (EnumMonitorData *)dwData;
    Monitor *monitor = ((Monitor *)data->data) + data->index;

    MONITORINFO mi;
    ZeroMemory(&mi, sizeof(mi));
    mi.cbSize = sizeof(mi);
    GetMonitorInfo(hMonitor, &mi);

    monitor->x = mi.rcMonitor.left;
    monitor->y = mi.rcMonitor.top;
    monitor->w = mi.rcMonitor.right - mi.rcMonitor.left;
    monitor->h = mi.rcMonitor.bottom - mi.rcMonitor.top;

    data->index++;

    return TRUE;
}

int writeBmp(LPWSTR filename, int width, int height, unsigned char *pixels)
{
    int y, rc = 0;

    FILE *f = NULL;

    BITMAPFILEHEADER fileHeader;
    BITMAPINFOHEADER infoHeader;

    unsigned char *bmpPixels = NULL;
    int rowSize = 0;
    int bmpSize = 0;

    if (!(f = _wfopen(filename, L"wb"))) {
        rc = 1;
        goto end;
    }

    rowSize = ((24 * width + 31) / 32) * 4;
    bmpSize = rowSize * height;

    // Create file header
    fileHeader.bfType = MAKEWORD('B', 'M');
    fileHeader.bfSize = sizeof(fileHeader) + sizeof(infoHeader) + bmpSize;
    fileHeader.bfReserved1 = 0;
    fileHeader.bfReserved2 = 0;
    fileHeader.bfOffBits = sizeof(fileHeader) + sizeof(infoHeader);

    // Write file header
    if (fwrite(&fileHeader, sizeof(fileHeader), 1, f) != 1) {
        rc = 1;
        goto end;
    }

    // Create info header
    infoHeader.biSize = sizeof(infoHeader);
    infoHeader.biWidth = width;
    infoHeader.biHeight = height;
    infoHeader.biPlanes = 1;
    infoHeader.biBitCount = 24;
    infoHeader.biCompression = BI_RGB;
    infoHeader.biSizeImage = 0;
    infoHeader.biXPelsPerMeter = 0;
    infoHeader.biYPelsPerMeter = 0;
    infoHeader.biClrUsed = 0;
    infoHeader.biClrImportant = 0;

    // Write info header
    if (fwrite(&infoHeader, sizeof(infoHeader), 1, f) != 1) {
        rc = 1;
        goto end;
    }

    // Copy to BMP buffer
    bmpPixels = malloc(bmpSize);

    for (y = 0; y < height; y++) {
        memcpy(bmpPixels + rowSize * (height-1-y), pixels + width * y * 3, width * 3);
    }

    // Write bitmap data
    if (fwrite(bmpPixels, bmpSize, 1, f) != 1) {
        rc = 1;
        goto end;
    }

end:
    if (bmpPixels)
        free(bmpPixels);
    if (f)
        fclose(f);

    return rc;
}

void wallpaper_gen(Wallpaper *wallpaper, int width, int height, unsigned char *pixels)
{
    int i;

    if (winver == WIN_UNSUPPORTED)
        return;

    if (winver == WIN_WINE) {
        if (wallpaper_winetype == WINETYPE_NONE)
            wallpaper_winetype = util_sh(L"wallpaper query");

        if (wallpaper_winetype == WINETYPE_OSX || wallpaper_winetype == WINETYPE_X11) {
            int nMonitors = 0;
            Monitor *monitors = NULL;

            int dstWidth = 0;
            int dstHeight = 0;

            int bpp = 0;

            if (wallpaper_winetype == WINETYPE_OSX) {
                // For OS X, use only one monitor
                MONITORINFO mi;
                ZeroMemory(&mi, sizeof(mi));
                mi.cbSize = sizeof(mi);
                HMONITOR monitor = MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST);
                GetMonitorInfo(monitor, &mi);
                monitors = malloc(sizeof(Monitor));
                monitors[0].x = 0;
                monitors[0].y = 0;
                monitors[0].w = mi.rcMonitor.right - mi.rcMonitor.left;
                monitors[0].h = mi.rcMonitor.bottom - mi.rcMonitor.top;
                nMonitors = 1;

                dstWidth = monitors[0].w;
                dstHeight = monitors[0].h;

                // 3 bytes per pixel (RGB)
                bpp = 3;
            } else {
                // For X11, use all monitors
                DISPLAY_DEVICE dd;
                ZeroMemory(&dd, sizeof(dd));
                dd.cb = sizeof(dd);

                EnumDisplayMonitors(NULL, NULL, enum_monitor_count, (LPARAM)&nMonitors);
                monitors = malloc(sizeof(Monitor) * nMonitors);
                EnumMonitorData emd = {0, monitors};
                EnumDisplayMonitors(NULL, NULL, enum_monitor_data, (LPARAM)&emd);

                dstWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
                dstHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);

                // 4 bytes per pixel (BGRX)
                bpp = 4;
            }

            // Create image buffer; render the image on every monitor
            unsigned char *pixelsScaled = malloc(dstWidth * dstHeight * bpp);
            for (i = 0; i < dstWidth * dstHeight; i++) {
                pixelsScaled[i * bpp + 0] = (wallpaper->colorPrimary >>  0) & 0xFF;
                pixelsScaled[i * bpp + 1] = (wallpaper->colorPrimary >>  8) & 0xFF;
                pixelsScaled[i * bpp + 2] = (wallpaper->colorPrimary >> 16) & 0xFF;
                if (bpp == 4) {
                    pixelsScaled[i * bpp + 3] = 0;
                }
            }

            for (i = 0; i < nMonitors; i++) {
                int dstX = monitors[i].x + monitors[i].w / 2 - width / 2;
                int dstY = monitors[i].y + monitors[i].h / 2 - height / 2;
                imgcpy(pixelsScaled, dstWidth, dstX, dstY, pixels, width, height, bpp);
                /*
                int dstX = monitors[i].x;
                int dstY = monitors[i].y;
                int dstW = monitors[i].w;
                int dstH = monitors[i].h;

                if (monitors[i].h * width / height > monitors[i].w) {
                    // Center the image vertically
                    dstH = monitors[i].w * height / width;
                    dstY += (monitors[i].h - dstH) / 2;
                } else {
                    // Center the image horizontally
                    dstW = monitors[i].h * width / height;
                    dstX += (monitors[i].w - dstW) / 2;
                }

                scale(pixelsScaled, dstWidth, dstX, dstY, dstW, dstH, pixels, width, 0, 0, width, height, bpp);
                */
            }

            if (wallpaper_winetype == WINETYPE_OSX) {
                // save to bmp
                writeBmp(wallpaper->filename, dstWidth, dstHeight, pixelsScaled);
            } else {
                // save to bmp first
                writeBmp(wallpaper->filename, width, height, pixels);
                free(wallpaper->filename);

                // now save to raw data

                // for the filename of the wallpaper; guarantee "uniqueness"
                // with file time's low date time
                FILETIME filetime;
                GetSystemTimeAsFileTime(&filetime);
                wallpaper->filename = _aswprintf(L"%ls%u", szDataPath, filetime.dwLowDateTime);
                FILE *f = _wfopen(wallpaper->filename, L"wb");
                fwrite(&dstWidth, 4, 1, f);
                fwrite(&dstHeight, 4, 1, f);
                fwrite(pixelsScaled, dstWidth * dstHeight * bpp, 1, f);
                fclose(f);
            }

            free(pixelsScaled);
            free(monitors);

            char *filenameUnix = wine_get_unix_file_name(wallpaper->filename);
            free(wallpaper->filename);
            wallpaper->filename = util_getUnicodeCP(filenameUnix, CP_UTF8);
            HeapFree(GetProcessHeap(), 0, filenameUnix);

            return;
        }
    }

    // Save the image to bmp
    writeBmp(wallpaper->filename, width, height, pixels);

    if (winver == WIN_WINE) {
        // Convert to file://<unix path>
        char *filenameUnix8 = wine_get_unix_file_name(wallpaper->filename);
        free(wallpaper->filename);
        LPWSTR filenameUnix = util_getUnicodeCP(filenameUnix8, CP_UTF8);
        HeapFree(GetProcessHeap(), 0, filenameUnix8);
        wallpaper->filename = _aswprintf(L"file:// %ls", filenameUnix);
        free(filenameUnix);
    }
}

void wallpaper_free(Wallpaper *wallpaper)
{
    free(wallpaper->filename);
}
