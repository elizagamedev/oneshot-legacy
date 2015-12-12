#include "funcs.h"
#include "util.h"
#include "window.h"
#include "ogg.h"
#include "wallpaper.h"
#include "strings.h"

#ifdef L_KOREAN

#define HOOK_LANG 949
#define HOOK_FONT
#define MBCS
static const char *const fonts[] = {
    "Dotum",
    "Baekmuk Dotum",
    "UnDotum",
    "Baekmuk Gulim",
    "Baekmuk Batang",
    "AppleGothic",
    "Apple SD Gothic Neo",
};

#elif defined L_RUSSIAN

#define HOOK_FONT
#define FONT_NAME "RM2000"
#define FONT_FILE "RM2000.fon"

#elif defined L_SPANISH

#define HOOK_LANG 1252
#define DOCUMENT_NAME "INFORMACION.oneshot.txt"

#endif

#ifdef FONT_LOOKUP
#define FONTS_SIZE (sizeof(fonts)/sizeof(fonts[0]))
static int font = -1;
static int CALLBACK callback_font(const LOGFONTA *lpelfe, const TEXTMETRICA *lpntme, DWORD FontType, LPARAM lParam)
{
    int i = 0;
    int limit = ((font == -1) ? FONTS_SIZE : font);
    for (; i < limit; i++) {
        if (!strcmp(fonts[i], lpelfe->lfFaceName)) {
            font = i;
            break;
        }
    }
    if (font == 0)
        return 0;
    return 1;
}
#endif

#define HKEY_FAKE       ((HKEY)0x80000010)
//#define HFILE_FAKE      ((HANDLE)(LONG_PTR)-2)
#define HFILE_FAKE      ((HANDLE)1)

#ifndef DOCUMENT_NAME
#define DOCUMENT_NAME       L"INFORMATION.oneshot.txt"
#endif
#define WALLPAPER_NAME      L"COMBINATION.oneshot.bmp"

//----------------
//HOOKED FUNCTIONS
//----------------

//KERNEL

#ifdef HOOK_LANG
//These functions are vital for telling RPGM to use a MBCS
BOOL WINAPI
hook_GetCPInfo(UINT CodePage, LPCPINFO lpCPInfo)
{
    return GetCPInfo(HOOK_LANG, lpCPInfo);
}
#endif

#ifdef MBCS
LCID WINAPI
hook_GetThreadLocale()
{
#ifdef LANG_KOREAN
    return MAKELCID(MAKELANGID(LANG_KOREAN, SUBLANG_KOREAN), SORT_DEFAULT);
#endif
}
#endif

#if 0
HMODULE WINAPI
hook_LoadLibraryA(LPCSTR lpFileName)
{
    printf("%s\n", lpFileName);
    return LoadLibraryA(lpFileName);
}

HMODULE WINAPI
hook_LoadLibraryExA(LPCSTR lpFileName, HANDLE hFile, DWORD dwFlags)
{
    printf("%s\n", lpFileName);
    return LoadLibraryExA(lpFileName, hFile, dwFlags);
}
#endif

DWORD WINAPI
hook_GetModuleFileNameA(HMODULE hModule, LPSTR lpFileName, DWORD nSize)
{
    lpFileName[0] = 0;
    return 0;
}

//Sound gets loaded as absolute path, for some reason, so trick it into using a relative path
DWORD WINAPI
hook_GetFullPathNameA(LPCSTR lpFileName, DWORD nBufferLength, LPSTR lpBuffer, LPSTR *lpFilePart)
{
    strcpy(lpBuffer, lpFileName);
    if (lpFilePart)
        *lpFilePart = lpBuffer;
    return strlen(lpBuffer);
}

HANDLE WINAPI
hook_FindFirstFileA(LPCSTR lpFileName, LPWIN32_FIND_DATAA lpFindFileData)
{
    LPWSTR lpFileName16 = util_getFilename(lpFileName);
    WIN32_FIND_DATAW findFileData16;
    HANDLE result = FindFirstFileW(lpFileName16, &findFileData16);
    memcpy(lpFindFileData, &findFileData16, sizeof(WIN32_FIND_DATAA) - sizeof(lpFindFileData->cFileName) - sizeof(lpFindFileData->cAlternateFileName));

    if (result == INVALID_HANDLE_VALUE) {
        *lpFindFileData->cFileName = 0;
        *lpFindFileData->cAlternateFileName = 0;
    } else {
        util_getAnsiBuff(findFileData16.cFileName, lpFindFileData->cFileName, MAX_PATH);
        util_getAnsiBuff(findFileData16.cAlternateFileName, lpFindFileData->cAlternateFileName, 14);
    }
    return result;
}

LPWSTR szOggOld = NULL;

HANDLE WINAPI
hook_CreateFileA(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
    //See if this is a function or otherwise special file
    if (!stricmp(lpFileName, "Title\\title.xyz")) {
        if (ending <= ENDING_DEJAVU) {
            if (oneshot)
                ending = ENDING_DEAD;
            else if (gameStarted)
                ending = ENDING_DEJAVU;

            util_saveEnding();
        }

        //Create the kitty escape window if it doesn't exist
        if (!wndKitty)
            wndKitty_create();

        //Make sure we flag this as not being in-game anymore
        isInGame = FALSE;
        oneshot = FALSE;
    } else if (!stricmp(lpFileName, "Sound\\_init.wav")) {
        func_init();
    } else if (!stricmp(lpFileName, "Sound\\_func.wav")) {
        funcs[variables[VAR_FUNC]]();
    }

    LPWSTR lpFileName16 = util_getFilename(lpFileName);
    if (_wcsnicmp(lpFileName16, L"Music\\", 6)) {
        HANDLE result = CreateFileW(lpFileName16, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
        free(lpFileName16);
        return result;
    }

    //Convert the ogg to a wav in memory
    if (szOggOld && !wcsicmp(szOggOld, lpFileName16)) {
        //If we've already got an OGG loaded in memory with this filename,
        //just reset it
        wav_vio_seek(0, SEEK_SET, NULL);
        free(lpFileName16);
    } else {
        //Load the new ogg
        ogg_free();
        ogg_read(lpFileName16);
        free(szOggOld);
        szOggOld = lpFileName16;
    }
    return HFILE_FAKE;
}

BOOL WINAPI
hook_ReadFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped)
{
    if (hFile == HFILE_FAKE) {
        sf_count_t count = wav_vio_read(lpBuffer, nNumberOfBytesToRead, NULL);
        if (lpNumberOfBytesRead)
            *lpNumberOfBytesRead = count;
        return TRUE;
    }
    return ReadFile(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped);
}

DWORD WINAPI
hook_SetFilePointer(HANDLE hFile, LONG lDistanceToMove, PLONG lpDistanceToMoveHigh, DWORD dwMoveMethod)
{
    if (hFile == HFILE_FAKE) {
        int whence;
        switch (dwMoveMethod) {
        default:
        case FILE_BEGIN:
            whence = SEEK_SET;
            break;
        case FILE_CURRENT:
            whence = SEEK_CUR;
            break;
        case FILE_END:
            whence = SEEK_END;
            break;
        }
        return wav_vio_seek(lDistanceToMove, whence, NULL);
    }
    return SetFilePointer(hFile, lDistanceToMove, lpDistanceToMoveHigh, dwMoveMethod);
}

BOOL WINAPI
hook_CloseHandle(HANDLE hObject)
{
    if (hObject == HFILE_FAKE)
        return TRUE;
    return CloseHandle(hObject);
}

DWORD WINAPI
hook_GetFileAttributesA(LPCSTR lpFileName)
{
    LPWSTR lpFileName16 = util_getFilename(lpFileName);
    DWORD result = GetFileAttributesW(lpFileName16);
    free(lpFileName16);
    return result;
}

DWORD WINAPI
hook_GetPrivateProfileStringA(LPCSTR lpAppName, LPCSTR lpKeyName, LPCSTR lpDefault, LPSTR lpReturnedString, DWORD nSize, LPCSTR lpFileName)
{
    //Force FullPackageFlag to be 1
    if (!stricmp(lpKeyName, "FullPackageFlag")) {
        lpReturnedString[0] = '1';
        lpReturnedString[1] = 0;
        return 1;
    }

    //Everything else is an empty string
    lpReturnedString[0] = 0;
    return 0;
}

HRSRC logo1 = NULL;

HRSRC WINAPI
hook_FindResourceA(HMODULE hModule, LPCSTR lpName, LPCSTR lpType)
{
    if (!stricmp(lpName, "LOGO1")) {
        logo1 = FindResourceA(dll_hInstance, lpName, lpType);
        return logo1;
    }
    return FindResourceA(hModule, lpName, lpType);
}

DWORD WINAPI
hook_SizeofResource(HMODULE hModule, HRSRC hResInfo)
{
    if (hResInfo == logo1)
        return SizeofResource(dll_hInstance, hResInfo);
    return SizeofResource(hModule, hResInfo);
}

HGLOBAL WINAPI
hook_LoadResource(HMODULE hModule, HRSRC hResInfo)
{
    if (hResInfo == logo1)
        return LoadResource(dll_hInstance, hResInfo);
    return LoadResource(hModule, hResInfo);
}

//USER

#ifdef MBCS
int WINAPI
hook_GetSystemMetrics(int nIndex)
{
    if (nIndex == SM_DBCSENABLED)
        return TRUE;
    return GetSystemMetrics(nIndex);
}
#endif

HWND WINAPI
hook_CreateWindowExA(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
{
    if (!strcmp(lpClassName, "TFormLcfGameMain")) {
        window = CreateWindowExW(dwExStyle, L"TFormLcfGameMain", L"Oneshot", dwStyle, x, y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
        return window;
    }
    return CreateWindowExA(dwExStyle & ~WS_EX_CLIENTEDGE, lpClassName, lpWindowName, dwStyle, x, y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
}

ATOM WINAPI
hook_RegisterClassA(const WNDCLASSA *lpWndClass)
{
    WNDCLASSW wndClass16;
    memcpy(&wndClass16, lpWndClass, sizeof(WNDCLASSA) - sizeof(LPCSTR) * 2);
    wndClass16.lpszMenuName = util_getUnicode(lpWndClass->lpszMenuName);
    wndClass16.lpszClassName = util_getUnicode(lpWndClass->lpszClassName);
    ATOM result = RegisterClassW(&wndClass16);
    free((void *)wndClass16.lpszMenuName);
    free((void *)wndClass16.lpszClassName);
    return result;
}

WNDPROC wndproc = NULL;

LRESULT CALLBACK
hook_wndproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_CLOSE:
        func_Close();
        return 0;

    case WM_DESTROY:
        func_Destroy();
        return 0;

    default:
        return wndproc(hwnd, msg, wParam, lParam);
    }
}

LONG WINAPI
hook_SetWindowLongA(HWND hWnd, int nIndex, LONG dwNewLong)
{
    if (window && hWnd == window && nIndex == GWL_WNDPROC) {
        LONG old1 = (LONG)wndproc;
        LONG old2 = SetWindowLongW(window, GWL_WNDPROC, (LONG)hook_wndproc);
        wndproc = (WNDPROC)dwNewLong;
        if (old1)
            return old1;
        return old2;
    }
    return SetWindowLongW(hWnd, nIndex, dwNewLong);
}

LONG WINAPI
hook_GetWindowLongA(HWND hWnd, int nIndex)
{
    return GetWindowLongW(hWnd, nIndex);
}

LRESULT WINAPI
hook_DefWindowProcA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg) {
    case WM_SETTEXT:
        //Ignore if the first byte is a null--this is the string sent by the game
        if (*((char *)lParam) == 0)
            return TRUE;
        break;
    case WM_GETTEXTLENGTH:
        return DefWindowProcW(hWnd, Msg, wParam, lParam) * 2;
    }
    return DefWindowProcW(hWnd, Msg, wParam, lParam);
}

#if 0
HICON WINAPI
hook_LoadIconA(HINSTANCE hInstance, LPCSTR lpIconName)
{
    return LoadIconA(dll_hInstance, "MAINICON");
}
#endif

BOOL WINAPI
hook_SetWindowPos(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags)
{
    if (hWnd == window && X && Y) {
        X -= 2;
        Y -= 2;
        cx -= 4;
        cy -= 4;
    }
    return SetWindowPos(hWnd, hWndInsertAfter, X, Y, cx, cy, uFlags);
}

//ADVAPI
LONG WINAPI
hook_RegCreateKeyExA(HKEY hKey, LPCSTR lpSubKey, DWORD Reserved, LPSTR lpClass, DWORD dwOptions,
                     REGSAM samDesired, LPSECURITY_ATTRIBUTES lpSecurityAttributes, PHKEY phkResult, LPDWORD lpdwDisposition)
{
    *phkResult = HKEY_FAKE;
    return ERROR_SUCCESS;
}

LONG WINAPI
hook_RegOpenKeyExA(HKEY hKey, LPCSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult)
{
    *phkResult = HKEY_FAKE;
    return ERROR_SUCCESS;
}

LONG WINAPI
hook_RegQueryValueExA(HKEY hKey, LPCSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData)
{
    return ERROR_FILE_NOT_FOUND;
}

LONG WINAPI
hook_RegCloseKey(HKEY hKey)
{
    return ERROR_SUCCESS;
}

//SHELL
//LSD files use this for some reason
BOOL
hook_SHGetPathFromIDListA(PCIDLIST_ABSOLUTE pidl, LPTSTR pszPath)
{
    *pszPath = 0;
    return TRUE;
}

//GDI
#ifdef HOOK_LANG
BOOL WINAPI
hook_GetTextExtentPoint32A(HDC hdc, LPCSTR lpString, int c, LPSIZE lpSize)
{
    LPWSTR lpString16 = util_getUnicodeCP(lpString, HOOK_LANG);
    BOOL result = GetTextExtentPoint32W(hdc, lpString16, c, lpSize);
    free(lpString16);
    return result;
}

BOOL WINAPI
hook_ExtTextOutA(HDC hdc, int X, int Y, UINT fuOptions, const RECT *lprc, LPCSTR lpString, UINT cbCount, const INT *lpDx)
{
    LPWSTR lpString16 = util_getUnicodeCP(lpString, HOOK_LANG);
    BOOL result = ExtTextOutW(hdc, X, Y, fuOptions, lprc, lpString16, cbCount, lpDx);
    free(lpString16);
    return result;
}
#endif

#ifdef HOOK_FONT
HFONT WINAPI
hook_CreateFontIndirectA(const LOGFONTA *lplf)
{
#ifdef FONT_LOOKUP
    if (font != -1) {
        LOGFONTA lf;
        memcpy(&lf, lplf, sizeof(LOGFONTA) - sizeof(lplf->lfFaceName));
        strcpy(lf.lfFaceName, fonts[font]);
        return CreateFontIndirectA(&lf);
    }
    return CreateFontIndirectA(lplf);
#else
    LOGFONTA lf;
    memcpy(&lf, lplf, sizeof(LOGFONTA) - sizeof(lplf->lfFaceName));
    strcpy(lf.lfFaceName, FONT_NAME);
    return CreateFontIndirectA(&lf);
#endif
}
#endif

//COM OBJECTS

#if 0
void hook_QueryInterface(REFIID riid, void **ppvObject);

#define GET_VTBL(type) type* vtbl = ((void**)This->lpVtbl)[sizeof(type)/4]

#if 0
//IBasicAudio
STDMETHODIMP
hook_IBasicAudio_QueryInterface(IBasicAudio *This, REFIID riid, void **ppvObject)
{
    GET_VTBL(IBasicAudioVtbl);

    HRESULT result = vtbl->QueryInterface(This, riid, ppvObject);

    hook_QueryInterface(riid, ppvObject);

    return result;
}

ULONG WINAPI
hook_IBasicAudio_Release(IBasicAudio *This)
{
    GET_VTBL(IBasicAudioVtbl);

    printf("IBasicAudio: %p: Release\n", This);

    free((void *)This->lpVtbl);
    This->lpVtbl = vtbl;
    return vtbl->Release(This);
}

STDMETHODIMP
hook_IBasicAudio_get_Balance(IBasicAudio *This, long *plBalance)
{
    GET_VTBL(IBasicAudioVtbl);

    printf("IBasicAudio: %p: get_Balance\n", This);

    return vtbl->get_Balance(This, plBalance);
}

STDMETHODIMP
hook_IBasicAudio_get_Volume(IBasicAudio *This, long *plVolume)
{
    GET_VTBL(IBasicAudioVtbl);

    printf("IBasicAudio: %p: get_Volume\n", This);

    return vtbl->get_Volume(This, plVolume);
}

STDMETHODIMP
hook_IBasicAudio_put_Balance(IBasicAudio *This, long lBalance)
{
    GET_VTBL(IBasicAudioVtbl);

    printf("IBasicAudio: %p: put_Balance\n", This);

    return vtbl->put_Balance(This, lBalance);
}

STDMETHODIMP
hook_IBasicAudio_put_Volume(IBasicAudio *This, long lVolume)
{
    GET_VTBL(IBasicAudioVtbl);

    printf("IBasicAudio: %p: put_Volume\n", This);

    return vtbl->put_Volume(This, lVolume);
}

//IMediaControl
STDMETHODIMP
hook_IMediaControl_QueryInterface(IMediaControl *This, REFIID riid, void **ppvObject)
{
    GET_VTBL(IMediaControlVtbl);

    HRESULT result = vtbl->QueryInterface(This, riid, ppvObject);

    hook_QueryInterface(riid, ppvObject);

    return result;
}

ULONG WINAPI
hook_IMediaControl_Release(IMediaControl *This)
{
    GET_VTBL(IMediaControlVtbl);

    printf("IMediaControl: %p: Release\n", This);

    free((void *)This->lpVtbl);
    This->lpVtbl = vtbl;
    return vtbl->Release(This);
}

STDMETHODIMP
hook_IMediaControl_Run(IMediaControl *This)
{
    GET_VTBL(IMediaControlVtbl);

    printf("IMediaControl: %p: Run\n", This);

    return vtbl->Run(This);
}

STDMETHODIMP
hook_IMediaControl_Pause(IMediaControl *This)
{
    GET_VTBL(IMediaControlVtbl);

    printf("IMediaControl: %p: Pause\n", This);

    return vtbl->Pause(This);
}

STDMETHODIMP
hook_IMediaControl_Stop(IMediaControl *This)
{
    GET_VTBL(IMediaControlVtbl);

    printf("IMediaControl: %p: Stop\n", This);

    return vtbl->Stop(This);
}

STDMETHODIMP
hook_IMediaControl_StopWhenReady(IMediaControl *This)
{
    GET_VTBL(IMediaControlVtbl);

    printf("IMediaControl: %p: StopWhenReady\n", This);

    return vtbl->StopWhenReady(This);
}

//IGraphBuilder
STDMETHODIMP
hook_IGraphBuilder_QueryInterface(IGraphBuilder *This, REFIID riid, void **ppvObject)
{
    GET_VTBL(IGraphBuilderVtbl);

    HRESULT result = vtbl->QueryInterface(This, riid, ppvObject);

    hook_QueryInterface(riid, ppvObject);

    return result;
}
#endif

ULONG WINAPI
hook_IGraphBuilder_Release(IGraphBuilder *This)
{
    GET_VTBL(IGraphBuilderVtbl);

    free((void *)This->lpVtbl);
    This->lpVtbl = vtbl;
    return vtbl->Release(This);
}

STDMETHODIMP
hook_IGraphBuilder_RenderFile(IGraphBuilder *This, LPCWSTR lpwstrFile, LPCWSTR lpwstrPlayList)
{
    GET_VTBL(IGraphBuilderVtbl);

    LPWSTR str = util_getFilenameFromUnicode(lpwstrFile);
    HRESULT result = vtbl->RenderFile(This, str, lpwstrPlayList);
    free(str);

    return result;
}

//IUnknown
STDMETHODIMP
hook_IUnknown_QueryInterface(IUnknown *This, REFIID riid, void **ppvObject)
{
    GET_VTBL(IUnknownVtbl);

    HRESULT result = vtbl->QueryInterface(This, riid, ppvObject);

    hook_QueryInterface(riid, ppvObject);

    return result;
}

//Generic
void
hook_QueryInterface(REFIID riid, void **ppvObject)
{
    if (IsEqualIID(riid, (REFGUID)&IID_IGraphBuilder)) {
        IGraphBuilder *obj = (IGraphBuilder *)*ppvObject;

        //printf("IGraphBuilder: %p: Create\n", obj);

        IGraphBuilderVtbl *vtbl = malloc(sizeof(IGraphBuilderVtbl) + 4);
        ((void **)vtbl)[sizeof(IGraphBuilderVtbl)/4] = obj->lpVtbl;
        memcpy(vtbl, obj->lpVtbl, sizeof(IGraphBuilderVtbl));
        obj->lpVtbl = vtbl;

        //vtbl->QueryInterface = hook_IGraphBuilder_QueryInterface;
        vtbl->Release = hook_IGraphBuilder_Release;
        vtbl->RenderFile = hook_IGraphBuilder_RenderFile;
    }
#if 0
    else if (IsEqualIID(riid, (REFGUID)&IID_IMediaControl)) {
        IMediaControl *obj = (IMediaControl *)*ppvObject;

        printf("IMediaControl: %p: Create\n", obj);

        IMediaControlVtbl *vtbl = malloc(sizeof(IMediaControlVtbl) + 4);
        ((void **)vtbl)[sizeof(IMediaControlVtbl)/4] = obj->lpVtbl;
        memcpy(vtbl, obj->lpVtbl, sizeof(IMediaControlVtbl));
        obj->lpVtbl = vtbl;

        //vtbl->QueryInterface = hook_IMediaControl_QueryInterface;
        vtbl->Release = hook_IMediaControl_Release;
        vtbl->Run = hook_IMediaControl_Run;
        vtbl->Pause = hook_IMediaControl_Pause;
        vtbl->Stop = hook_IMediaControl_Stop;
        vtbl->StopWhenReady = hook_IMediaControl_StopWhenReady;
    } else if (IsEqualIID(riid, (REFGUID)&IID_IBasicAudio)) {
        IBasicAudio *obj = (IBasicAudio *)*ppvObject;

        printf("IBasicAudio: %p: Create\n", obj);

        IBasicAudioVtbl *vtbl = malloc(sizeof(IBasicAudioVtbl) + 4);
        ((void **)vtbl)[sizeof(IBasicAudioVtbl)/4] = obj->lpVtbl;
        memcpy(vtbl, obj->lpVtbl, sizeof(IBasicAudioVtbl));
        obj->lpVtbl = vtbl;

        //vtbl->QueryInterface = hook_IBasicAudio_QueryInterface;
        vtbl->Release = hook_IBasicAudio_Release;
        vtbl->get_Balance = hook_IBasicAudio_get_Balance;
        vtbl->get_Volume = hook_IBasicAudio_get_Volume;
        vtbl->put_Balance = hook_IBasicAudio_put_Balance;
        vtbl->put_Volume = hook_IBasicAudio_put_Volume;
    } else if (IsEqualIID(riid, (REFGUID)&IID_IMediaEventEx)) {
        IMediaEventEx *obj = (IMediaEventEx *)*ppvObject;

        printf("IMediaEventEx: %p: Create\n", obj);

        IMediaEventExVtbl *vtbl = malloc(sizeof(IMediaEventExVtbl) + 4);
        ((void **)vtbl)[sizeof(IMediaEventExVtbl)/4] = obj->lpVtbl;
        memcpy(vtbl, obj->lpVtbl, sizeof(IMediaEventExVtbl));
        obj->lpVtbl = vtbl;

        vtbl->QueryInterface = hook_IBasicAudio_QueryInterface;
        vtbl->Release = hook_IBasicAudio_Release;
    }
#endif
}

ULONG WINAPI
hook_IUnknown_Release(IUnknown *This)
{
    GET_VTBL(IUnknownVtbl);

    free((void *)This->lpVtbl);
    This->lpVtbl = vtbl;
    return vtbl->Release(This);
}

//OLE
HRESULT WINAPI
hook_CoCreateInstance(REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID *ppv)
{
    HRESULT result = CoCreateInstance(rclsid, pUnkOuter, dwClsContext, riid, ppv);

    if (IsEqualIID(riid, (REFIID)&IID_IDirectDraw7)) {
        printf("directdraw\n");
    }

    //Do we need to hook this?
    /*if (IsEqualCLSID(rclsid, (REFGUID)&CLSID_FilterGraph)) {
        IUnknown* obj = (IUnknown*)*ppv;

        IUnknownVtbl* vtbl = malloc(sizeof(IUnknownVtbl) + 4);
        ((void**)vtbl)[sizeof(IUnknownVtbl)/4] = obj->lpVtbl;
        memcpy(vtbl, obj->lpVtbl, sizeof(IUnknownVtbl));
        obj->lpVtbl = vtbl;

        vtbl->QueryInterface = hook_IUnknown_QueryInterface;
        vtbl->Release = hook_IUnknown_Release;
    }*/

    return result;
}
#endif

//---------
//HOOKING
//---------
typedef struct {
    const char *name;
    void *funcHook;
    void *funcOrig;
} Hook;

#define MODULE(name)    {#name "32.dll", NULL, NULL},
#define HOOK(name)      {#name, hook_##name, NULL},

#define HOOK_MAX (sizeof(hooks) / sizeof(hooks[0]))
Hook hooks[] = {
    MODULE(kernel)
#ifdef HOOK_LANG
    HOOK(GetCPInfo)
#endif
#ifdef MBCS
    HOOK(GetThreadLocale)
#endif
    //HOOK(LoadLibraryA)
    //HOOK(LoadLibraryExA)
    HOOK(GetFullPathNameA)
    HOOK(GetModuleFileNameA)
    HOOK(FindFirstFileA)
    HOOK(CreateFileA)
    HOOK(ReadFile)
    HOOK(SetFilePointer)
    HOOK(CloseHandle)
    HOOK(GetPrivateProfileStringA)
    HOOK(GetFileAttributesA)
    HOOK(FindResourceA)
    HOOK(SizeofResource)
    HOOK(LoadResource)
    //HOOK(LockResource)
    //HOOK(GetProcAddress)

    MODULE(user)
#ifdef MBCS
    HOOK(GetSystemMetrics)
#endif
    HOOK(CreateWindowExA)
    HOOK(RegisterClassA)
    HOOK(SetWindowLongA)
    HOOK(GetWindowLongA)
    HOOK(DefWindowProcA)
    //HOOK(LoadIconA)
    HOOK(SetWindowPos)

    MODULE(advapi)
    HOOK(RegCreateKeyExA)
    HOOK(RegOpenKeyExA)
    HOOK(RegQueryValueExA)
    HOOK(RegCloseKey)

    MODULE(shell)
    HOOK(SHGetPathFromIDListA)

    MODULE(gdi)
#ifdef HOOK_LANG
    HOOK(GetTextExtentPoint32A)
    HOOK(ExtTextOutA)
#endif
#ifdef HOOK_FONT
    HOOK(CreateFontIndirectA)
#endif

    //MODULE(ole)
    //HOOK(CoCreateInstance)
};

#define MAKE_POINTER(cast, ptr, offset) (cast)((DWORD)(ptr)+(DWORD)(offset))

BOOL hook()
{
    //Initialize original function addresses
    HMODULE module = NULL;
    int i, j;
    for (i = 0; i < HOOK_MAX; i++) {
        if (hooks[i].funcHook)
            hooks[i].funcOrig = (void *)GetProcAddress(module, hooks[i].name);
        else
            module = GetModuleHandleA(hooks[i].name);
    }

    //Check DOS Header
    PIMAGE_DOS_HEADER dos = (PIMAGE_DOS_HEADER)GetModuleHandleA(NULL);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE)
        return FALSE;

    //Check NT Header
    PIMAGE_NT_HEADERS nt = MAKE_POINTER(PIMAGE_NT_HEADERS, dos, dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE)
        return FALSE;

    //Check import module table
    PIMAGE_IMPORT_DESCRIPTOR modules = MAKE_POINTER(PIMAGE_IMPORT_DESCRIPTOR, dos, nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
    if (modules == (PIMAGE_IMPORT_DESCRIPTOR)nt)
        return FALSE;

    //Find the correct module
    while (modules->Name) {
        const char *szModule = MAKE_POINTER(const char *, dos, modules->Name);

        for (i = 0; i < HOOK_MAX; i++) {
            if (!hooks[i].funcHook && !strcasecmp(hooks[i].name, szModule))
                break;
        }

        if (i < HOOK_MAX) {
            //Find the correct function
            PIMAGE_THUNK_DATA thunk = MAKE_POINTER(PIMAGE_THUNK_DATA, dos, modules->FirstThunk);
            while (thunk->u1.Function) {
                for (j = i + 1; j < HOOK_MAX && hooks[j].funcHook; j++) {
                    if (thunk->u1.Function == (DWORD)hooks[j].funcOrig) {
                        //Overwrite
                        DWORD flags;
                        if (!VirtualProtect(&thunk->u1.Function, sizeof(thunk->u1.Function), PAGE_READWRITE, &flags)) {
                            return FALSE;
                        }
                        thunk->u1.Function = (DWORD)hooks[j].funcHook;
                        if (!VirtualProtect(&thunk->u1.Function, sizeof(thunk->u1.Function), flags, &flags)) {
                            return FALSE;
                        }
                    }
                }
                thunk++;
            }
        }
        modules++;
    }
    return TRUE;
}

HMODULE kernel32 = NULL;

//Entry point
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH: {
        dll_hInstance = hinstDLL;

        //Get the location of RPG_RT.exe
        unsigned int size = 1;
        for (;;) {
            szDataPath = (WCHAR *)malloc(size * sizeof(WCHAR));
            if (GetModuleFileNameW(NULL, szDataPath, size) < size)
                break;
            free(szDataPath);
            size *= 2;
        }
        PathRemoveFileSpecW(szDataPath);
        PathAddBackslashW(szDataPath);

        //Construct the close bat path/write it
        GetTempPathW(MAX_PATH, szBatPath);
        wcscat(szBatPath, L"oneshot.bat");
        FILE *f = _wfopen(szBatPath, L"wt");
        fwprintf(f, L"@echo off\n:loop\nrmdir /s /q \"%ls\"\nif exist \"%ls\" goto loop\ndel /f /q \"%ls\"", szDataPath, szDataPath, szBatPath);
        fclose(f);

        //Random
        srand(time(NULL));

        //Get the host os!
        kernel32 = LoadLibraryA("kernel32.dll");
        if (!kernel32) {
            return FALSE;
        }
        wine_get_unix_file_name = (void *)GetProcAddress(kernel32, "wine_get_unix_file_name");

        if (wine_get_unix_file_name) {
            winver = WIN_WINE;

            //Create the shell command

            //Get the unix path of func.sh
            LPWSTR szFuncPath = _aswprintf(L"%lsfunc.sh", szDataPath);
            char *szFuncPathUnix8 = wine_get_unix_file_name(szFuncPath);
            LPWSTR szFuncPathUnix = util_getUnicodeCP(szFuncPathUnix8, CP_UTF8);
            HeapFree(GetProcessHeap(), 0, szFuncPathUnix8);
            free(szFuncPath);

            szFuncCmd = _aswprintf(L"/bin/bash \"%ls\"", szFuncPathUnix);
            free(szFuncPathUnix);

            //Construct return and rc paths
            szRcPath = _aswprintf(L"%lsrc", szDataPath);
            szReturnPath = _aswprintf(L"%lsreturn", szDataPath);

            //Get the config path
            util_sh(L"home");
            LPWSTR szConfigPath = util_sh_return();

            //Construct ending and save paths
            szEndingPath = _aswprintf(L"%ls\\ending", szConfigPath);
            szSavePath = _aswprintf(L"%ls\\save", szConfigPath);
            free(szConfigPath);
        } else {
            //Construct the save path, create directory
            szSavePath = malloc((MAX_PATH + 13) * 2);
            if (SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, szSavePath) != S_OK) {
                return FALSE;
            }
            wcscat(szSavePath, L"\\Oneshot");
            CreateDirectoryW(szSavePath, NULL);
            wcscat(szSavePath, L"\\save");

            //What version of windows is this?
            OSVERSIONINFOW version;
            version.dwOSVersionInfoSize = sizeof(version);
            if (GetVersionExW(&version)) {
                switch (version.dwMajorVersion) {
                case 5:
                    switch (version.dwMinorVersion) {
                    case 1:
                        winver = WIN_XP;
                        break;
                    default:
                        winver = WIN_UNSUPPORTED;
                        break;
                    }
                    break;
                case 6:
                    switch (version.dwMinorVersion) {
                    case 0:
                        winver = WIN_VISTA;
                        break;
                    case 1:
                        winver = WIN_7;
                        break;
                    case 2:
                        winver = WIN_8;
                        break;
                    case 3:
                        winver = WIN_8_1;
                        break;
                    default:
                        winver = WIN_FUTURE;
                        break;
                    }
                    break;
                default:
                    winver = WIN_FUTURE;
                    break;
                }
            }
        }

        //Read the ending
        ending = util_loadEnding();

        //Get the document path
        if (winver == WIN_WINE) {
            util_sh(L"documents");
            LPWSTR buff = util_sh_return();
            szDocumentPath = _aswprintf(L"%ls\\" DOCUMENT_NAME, buff);
            free(buff);

            util_sh(L"desktop");
            buff = util_sh_return();
            szDesktopPath = _aswprintf(L"%ls\\" WALLPAPER_NAME, buff);
            free(buff);
        } else {
            WCHAR buff[MAX_PATH];
            if (SHGetFolderPathW(NULL, CSIDL_PERSONAL, NULL, 0, buff) != S_OK)
                return FALSE;
            szDocumentPath = _aswprintf(L"%ls\\" DOCUMENT_NAME, buff);

            if (SHGetFolderPathW(NULL, CSIDL_DESKTOP, NULL, 0, buff) != S_OK)
                return FALSE;
            szDesktopPath = _aswprintf(L"%ls\\" WALLPAPER_NAME, buff);
        }

        //Make sure the document doesn't exist, by unlikely
        //coincidence
        /*DWORD attrib = GetFileAttributesW(szDocumentPath);
        if (attrib != INVALID_FILE_ATTRIBUTES) {
            LPWSTR msg = _aswprintf(L"Please move or rename the file \"%ls\" in order to run the game properly!", szDocumentPath);
            MessageBoxW(NULL, msg, L"Error", MB_ICONERROR);
            free(msg);
            return FALSE;
        }*/

        if (ending == ENDING_ESCAPED) {
            util_messagebox(STR_GONE, L"Fatal Error", MESSAGE_ERROR);
            return FALSE;
        }
#if defined FONT_EMBED
        //Add font to table
        szFontPath = _aswprintf(L"%ls" FONT_FILE, szDataPath);
        AddFontResourceW(szFontPath);
#elif defined FONT_LOOKUP
        //Determine font to use for text (non-english only)
        HDC dc = GetDC(NULL);
        LOGFONTA lf = {0};
        EnumFontFamiliesExA(dc, &lf, callback_font, 0, 0);
        ReleaseDC(NULL, dc);
#endif

        return hook();
    }
    break;

    case DLL_PROCESS_DETACH: {
        //Save the ending here in case the game is abnormally terminated
        util_saveEnding();

        wndKitty_destroy();

#if defined FONT_EMBED
        RemoveFontResourceW(szFontPath);
        free(szFontPath);
#endif
        free(szFuncCmd);
        free(szDataPath);
        free(szRcPath);
        free(szReturnPath);
        free(szEndingPath);
        free(szSavePath);

        free(szDocumentPath);
        free(szDesktopPath);

        free(szOggOld);
        ogg_free();

        wallpaper_free(&wallpaperOld);

        FreeLibrary(kernel32);

        //Delete ourself
        STARTUPINFOW si = {0};
        PROCESS_INFORMATION pi = {0};
        si.cb = sizeof(si);
        CreateProcessW(NULL, szBatPath, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }
    break;
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    }
    return TRUE;
}
