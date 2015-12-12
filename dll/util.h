#ifndef UTIL_H
#define UTIL_H

#include "globals.h"

enum
{
    MESSAGE_NORMAL,
    MESSAGE_INFO,
    MESSAGE_WARNING,
    MESSAGE_ERROR,
    MESSAGE_QUESTION,
};

#define MESSAGEA(x) MessageBoxA(NULL, x, "", 0)
#define MESSAGEW(x) MessageBoxW(NULL, x, L"", 0)

#define util_getUnicode(sz8) util_getUnicodeCP(sz8, CP_ACP)
#define util_getUnicodeBuff(sz8, sz16, size) MultiByteToWideChar(CP_ACP, 0, sz8, -1, sz16, size)
#define util_getUnicodeBuffCP(sz8, sz16, size, cp) MultiByteToWideChar(cp, 0, sz8, -1, sz16, size)
#define util_getAnsiBuff(sz16, sz8, size) WideCharToMultiByte(CP_ACP, 0, sz16, -1, sz8, size, NULL, FALSE)

LPWSTR util_getUnicodeCP(LPCSTR sz8, int cp);
LPSTR util_getUtf8(LPCWSTR sz16);
void* util_findMemory(void* start, void* data, size_t size, size_t after);
int util_sh(LPWSTR fmt, ...);
LPWSTR util_sh_return();
LPWSTR util_getFilenameFromUnicode(LPCWSTR szFile);
LPWSTR util_getFilename(LPCSTR szFile);
int util_messagebox(LPCWSTR text, LPCWSTR title, int type);
void util_updateQuitMessage();
BOOL util_getResource(const char* name, void** data, size_t* size);
int util_loadEnding();
void util_saveEnding();

#endif
