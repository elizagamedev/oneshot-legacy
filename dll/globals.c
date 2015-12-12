#include "globals.h"
#include "util.h"
#include "wallpaper.h"

//This goes in globals so we can use it like a std function
wchar_t*
_aswprintf(const wchar_t* fmt, ...)
{
	va_list va_args;
	va_start(va_args, fmt);
	int size = _vscwprintf(fmt, va_args) + 1;
	wchar_t* dst = malloc(size * 2);
	vsnwprintf(dst, size, fmt, va_args);
	va_end(va_args);
	return dst;
}

int winver = WIN_UNSUPPORTED;

//Wine function
char* (CDECL *wine_get_unix_file_name)(LPCWSTR dosW) = NULL;

//DLL hinstance
HINSTANCE dll_hInstance = NULL;

//RPG game window
HWND window = NULL;

//LAUNCHER VARIABLES
LPWSTR szDataPath = NULL;
LPWSTR szFuncCmd = NULL;
LPWSTR szRcPath = NULL;
LPWSTR szReturnPath = NULL;
LPWSTR szEndingPath = NULL;
LPWSTR szSavePath = NULL;
WCHAR szBatPath[MAX_PATH + 11];
#ifdef FONT_EMBED
LPWSTR szFontPath = NULL;
#endif

LPWSTR szDocumentPath = NULL;
LPWSTR szDesktopPath = NULL;

//How do we start out the game?
int ending = ENDING_BEGINNING;

//The array of game variables
int* variables = NULL;

//The array of switches
char* switches = NULL;

//Username that we choose for the player, gets overwritten by the real name
char username[256];
DWORD usernameSize = sizeof(username);

//TRUE if the game started. For the deja vu "ending".
BOOL gameStarted = FALSE;

//TRUE if we're in the game
BOOL isInGame = FALSE;

//TRUE if the oneshot function was called. If the game is exited while this is true, the kitty dies.
BOOL oneshot = FALSE;

BOOL isWindowDestroyed = FALSE;

FILE* saveFile = NULL;

BOOL forceEnding = FALSE;

BOOL closeEnabled = TRUE;
