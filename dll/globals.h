#ifndef GLOBALS_H
#define GLOBALS_H

#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0500
#define SECURITY_WIN32
#include <windows.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <security.h>
#include <shlobj.h>
#include <malloc.h>
#include <stdio.h>
#include <time.h>
#define NO_DSHOW_STRSAFE
#include <dshow.h>

#define TITLE_SIZE 256

#ifdef L_KOREAN
#define FONT_LOOKUP
#elif defined L_RUSSIAN
#define FONT_EMBED
#endif

enum {
    WIN_UNSUPPORTED,
    WIN_WINE,
    WIN_XP,
    WIN_VISTA,
    WIN_7,
    WIN_8,
    WIN_8_1,
    WIN_FUTURE,
};

enum {
    VAR_FUNC,
    VAR_ARG1,
    VAR_ARG2,
    VAR_ARG3,
    VAR_ARG4,
    VAR_RETURN,
    VAR_ENDING,
    VAR_SAFE_CODE = 28,
    VAR_GEORGE = 46,
};

enum {
    ENDING_BEGINNING,
    ENDING_DEJAVU,
    ENDING_DEAD,
    ENDING_ESCAPED,
    ENDING_TRAPPED,
};

wchar_t *_aswprintf(const wchar_t *fmt, ...);

//Wine function
extern char *(CDECL *wine_get_unix_file_name)(LPCWSTR dosW);

//What version of windows?
extern int winver;

//DLL hinstance
extern HINSTANCE dll_hInstance;

//RPG game window/title
extern HWND window;

//LAUNCHER VARIABLES
extern LPWSTR szFuncCmd;
extern LPWSTR szDataPath;
extern LPWSTR szRcPath;
extern LPWSTR szReturnPath;
extern LPWSTR szEndingPath;
extern LPWSTR szSavePath;
extern WCHAR szBatPath[];
#ifdef FONT_EMBED
extern LPWSTR szFontPath;
#endif

//Strings for funcs
extern LPWSTR szDocumentPath;
extern LPWSTR szDesktopPath;

//How do we start out the game?
extern int ending;

//The array of game variables
extern int *variables;

//The array of switches
extern char *switches;

//Username that we choose for the player, gets overwritten by the real name
extern char username[256];
extern DWORD usernameSize;

//TRUE if the game started. For the deja vu "ending".
extern BOOL gameStarted;

//Are we in the game?
extern BOOL isInGame;

//TRUE if the oneshot function was called. If the game is exited while this is true, the kitty dies.
extern BOOL oneshot;

//TRUE if the main window is destroyed
extern BOOL isWindowDestroyed;

//For saving and loading; stored as a global so we can read/write items
extern FILE *saveFile;

//Set to TRUE if we shouldn't fuck with the ending
extern BOOL forceEnding;

//Set this to FALSE to disable closing the game
extern BOOL closeEnabled;

#endif
