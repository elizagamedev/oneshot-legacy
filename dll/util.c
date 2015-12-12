#include "util.h"

#if defined L_KOREAN
static unsigned char QUIT_MESSAGE[] = {
    0xc7, 0xcf, 0xc1, 0xf6, 0xb8, 0xb8, 0x20, 0xb9, 0xe6, 0xb1, 0xdd, 0x20,
    0xb8, 0xb7, 0x20, 0xbd, 0xc3, 0xc0, 0xdb, 0xc7, 0xdf, 0xc0, 0xdd, 0xbe,
    0xc6, 0xbf, 0xe4, 0x21, 0x00
};
static unsigned char QUIT_MESSAGE_ONESHOT[] = {
    0xb4, 0xcf, 0xc4, 0xda, 0xb4, 0xc2, 0x20, 0xbb, 0xec, 0xbe, 0xc6, 0xb3,
    0xb2, 0xc1, 0xf6, 0x20, 0xb8, 0xf8, 0xc7, 0xdf, 0xbd, 0xc0, 0xb4, 0xcf,
    0xb4, 0xd9, 0x2e, 0x00
};
#elif defined L_RUSSIAN
static unsigned char QUIT_MESSAGE[] = {
    0xcd, 0xee, 0x20, 0xf2, 0xfb, 0x20, 0xf2, 0xee, 0xeb, 0xfc, 0xea, 0xee,
    0x20, 0xed, 0xe0, 0xf7, 0xe0, 0xeb, 0x20, 0xe8, 0xe3, 0xf0, 0xe0, 0xf2,
    0xfc, 0x21, 0x0a
};
static unsigned char QUIT_MESSAGE_ONESHOT[] = {
    0xcd, 0xe8, 0xea, 0xee, 0x20, 0xed, 0xe5, 0x20, 0xe2, 0xfb, 0xe1, 0xe5,
    0xf0, 0xe5, 0xf2, 0xf1, 0xff, 0x20, 0xe6, 0xe8, 0xe2, 0xfb, 0xec, 0x2e,
    0x0a
};
#elif defined L_SPANISH
static unsigned char QUIT_MESSAGE[] = {
    0xa1, 0x50, 0x65, 0x72, 0x6f, 0x20, 0x73, 0x6f, 0x6c, 0x6f, 0x20, 0x61,
    0x63, 0x61, 0x62, 0x61, 0x73, 0x20, 0x64, 0x65, 0x20, 0x65, 0x6d, 0x70,
    0x65, 0x7a, 0x61, 0x72, 0x20, 0x61, 0x20, 0x6a, 0x75, 0x67, 0x61, 0x72,
    0x21, 0x0a
};
#define QUIT_MESSAGE_ONESHOT    "Niko no va a salir con vida."
#else
#define QUIT_MESSAGE            "But you only just started playing!"
#define QUIT_MESSAGE_ONESHOT        "  Niko won't make it out alive."
#endif

#define QUIT_MESSAGE_SIZE       (sizeof(QUIT_MESSAGE)-1)
#define QUIT_MESSAGE_ONESHOT_SIZE   (sizeof(QUIT_MESSAGE_ONESHOT)-1)

LPWSTR util_getUnicodeCP(LPCSTR sz8, int cp)
{
    if (!sz8)
        return NULL;

    int size = MultiByteToWideChar(cp, 0, sz8, -1, NULL, 0);

    if (size > 0) {
        LPWSTR sz16 = (LPWSTR)malloc(size * 2);
        if (MultiByteToWideChar(cp, 0, sz8, -1, sz16, size) == size)
            return sz16;
        free(sz16);
    }
    return NULL;
}

LPSTR util_getUtf8(LPCWSTR sz16)
{
    if (!sz16)
        return NULL;

    int size = WideCharToMultiByte(CP_UTF8, 0, sz16, -1, NULL, 0, NULL, NULL);

    if (size > 0) {
        LPSTR sz8 = (LPSTR)malloc(size);
        if (WideCharToMultiByte(CP_UTF8, 0, sz16, -1, sz8, size, NULL, NULL) == size)
            return sz8;
        free(sz8);
    }
    return NULL;
}

void *memmem(const void *haystack, size_t hlen, const void *needle, size_t nlen)
{
    int needle_first;
    const void *p = haystack;
    size_t plen = hlen;

    if (!nlen)
        return NULL;

    needle_first = *(unsigned char *)needle;

    while (plen >= nlen && (p = memchr(p, needle_first, plen - nlen + 1))) {
        if (!memcmp(p, needle, nlen))
            return (void *)p;

        p++;
        plen = hlen - (p - haystack);
    }

    return NULL;
}

//This function searches the program's memory for a string of data.
//It returns the pointer of the first matching block of data after "start".
void *util_findMemory(void *start, void *data, size_t size, size_t after)
{
    MEMORY_BASIC_INFORMATION mbi = {0};

    //Start at start
    mbi.BaseAddress = start;

    while (VirtualQuery(mbi.BaseAddress + mbi.RegionSize, &mbi, sizeof(mbi))) {
        if (mbi.State & MEM_COMMIT) {
            if (((mbi.Protect) & (PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY | PAGE_READWRITE | PAGE_WRITECOPY)) && !((mbi.Protect) & (PAGE_GUARD | PAGE_NOCACHE | PAGE_NOACCESS))) {
                //If "start" is in this page, start reading after start + size
                void *searchStart = mbi.BaseAddress;
                size_t searchSize = mbi.RegionSize;
                if (mbi.BaseAddress <= start && start < mbi.BaseAddress + mbi.RegionSize) {
                    searchStart = start + size;
                    searchSize = mbi.RegionSize - (start - mbi.BaseAddress + size);
                }

                void *pointer = memmem(searchStart, searchSize, data, size);
                if (pointer && pointer != data && (unsigned int)pointer + size + after < (unsigned int)(mbi.BaseAddress + mbi.RegionSize))
                    return pointer;
            }
        }
    }
    return NULL;
}

//This calls the shell script - for use in WINE
int util_sh(LPWSTR fmt, ...)
{
    va_list va_args;
    va_start(va_args, fmt);
    int size = _vscwprintf(fmt, va_args) + 1;
    LPWSTR args = malloc(size * 2);
    vsnwprintf(args, size, fmt, va_args);
    va_end(va_args);

    //Delete old rc file, just in case
    DeleteFileW(szRcPath);

    //Create buffer for args + shell script path and construct string
    LPWSTR cmdline = _aswprintf(L"%ls %ls", szFuncCmd, args);
    free(args);

    //Spawn the process and wait
    STARTUPINFOW si = {0};
    PROCESS_INFORMATION pi = {0};
    si.cb = sizeof(si);
    if (CreateProcessW(NULL, cmdline, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        free(cmdline);

        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        //Poll for the RC file
        for (;;) {
            DWORD attrib = GetFileAttributesW(szRcPath);
            if (attrib != INVALID_FILE_ATTRIBUTES && !(attrib & FILE_ATTRIBUTE_DIRECTORY)) {
                //Read the code and quit
                char rc[64];
                FILE *f = _wfopen(szRcPath, L"rt");
                fgets(rc, sizeof(rc), f);
                fclose(f);
                DeleteFileW(szRcPath);
                return atoi(rc);
            }
            Sleep(100);
        }
    }
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    free(cmdline);
    return 0;
}

LPWSTR util_sh_return()
{
    //Read the entire contents of the return file
    FILE *f = _wfopen(szReturnPath, L"rb");
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buff = malloc(size);
    fread(buff, size, 1, f);
    fclose(f);
    DeleteFileW(szReturnPath);

    //Convert it to UTF-16
    buff[size-1] = 0;
    LPWSTR result = util_getUnicodeCP(buff, CP_UTF8);
    free(buff);
    return result;
}

LPWSTR util_getFilenameFromUnicode(LPCWSTR szFile)
{
    //Stupid LSD files
    if (*szFile == '\\')
        szFile++;

    //Replace special assets with "dead" version
    if (ending == ENDING_DEAD && !gameStarted) {
        if (!wcsicmp(szFile, L"Title\\title.xyz"))
            return wcsdup(L"Title\\title_dead.xyz");
        if (!wcsicmp(szFile, L"Music\\MyBurdenIsLight.wav"))
            return wcsdup(L"Music\\MyBurdenIsDead.ogg");
    } else if (ending == ENDING_TRAPPED) {
        if (!wcsicmp(szFile, L"Title\\title.xyz"))
            return wcsdup(L"Title\\title_stay.xyz");
        if (!wcsicmp(szFile, L"System\\title.xyz"))
            return wcsdup(L"System\\title_stay.xyz");
        if (!wcsicmp(szFile, L"Music\\MyBurdenIsLight.wav"))
            return wcsdup(L"Music\\Distant.ogg");
    }

    //Replace music wav with ogg
    if (!_wcsnicmp(szFile, L"Music\\", 6)) {
        LPWSTR result = wcsdup(szFile);
        LPWSTR ext = PathFindExtensionW(result) + 1;
        if (!wcsicmp(ext, L"wav"))
            wcscpy(ext, L"ogg");
        return result;
    }

    return wcsdup(szFile);
}

LPWSTR util_getFilename(LPCSTR szFile)
{
    LPWSTR szFile16 = util_getUnicode(szFile);
    LPWSTR result = util_getFilenameFromUnicode(szFile16);
    free(szFile16);
    return result;
}

int util_messagebox(LPCWSTR text, LPCWSTR title, int type)
{
    if (winver == WIN_WINE) {
        int result = util_sh(L"messagebox \"%ls\" \"%ls\" %d", text, title, type);

        if (result != -1)
            return result;
    }

    //We're running windows or we don't have zenity installed

    int flags = 0;
    switch (type) {
    case MESSAGE_NORMAL:
        flags = MB_OK;
        break;
    case MESSAGE_INFO:
        flags = MB_OK | MB_ICONINFORMATION;
        break;
    case MESSAGE_WARNING:
        flags = MB_OK | MB_ICONWARNING;
        break;
    case MESSAGE_ERROR:
        flags = MB_OK | MB_ICONERROR;
        break;
    case MESSAGE_QUESTION:
        flags = MB_YESNO | MB_ICONQUESTION;
        break;
    }

    switch (MessageBoxW(window, text, title, flags)) {
    case IDNO:
        return 0;
        break;
    case IDYES:
    case IDOK:
        return 1;
        break;
    }
    return 0;
}

void util_updateQuitMessage()
{
    static char *lastMessage = QUIT_MESSAGE;
    static int lastMessageSize = QUIT_MESSAGE_SIZE;

    char *message = NULL;
    int messageSize = 0;
    switch (ending) {
    case ENDING_BEGINNING:
    case ENDING_DEJAVU:
        message = QUIT_MESSAGE_ONESHOT;
        messageSize = QUIT_MESSAGE_ONESHOT_SIZE;
        break;
    default:
        break;
    }
    if (message) {
        char *oldMessage = NULL;
        while ((oldMessage = util_findMemory(oldMessage, lastMessage, lastMessageSize, 0))) {
            memcpy(oldMessage, message, messageSize);
            memset(oldMessage + messageSize, ' ', QUIT_MESSAGE_SIZE - messageSize);
        }
    }
    lastMessage = message;
    lastMessageSize = messageSize;
}

BOOL util_getResource(const char *name, void **data, size_t *size)
{
    HGLOBAL hRes = NULL;
    HRSRC res;

    res = FindResourceA(dll_hInstance, name, RT_RCDATA);
    if (!res)
        return TRUE;
    if (!(hRes = LoadResource(dll_hInstance, res)))
        return TRUE;

    *data = (void *)LockResource(hRes);
    *size = (size_t)SizeofResource(dll_hInstance, res);

    return FALSE;
}

int util_loadEnding()
{
    int ending = 0;

    if (winver == WIN_WINE) {
        //Read the ending
        FILE *f = _wfopen(szEndingPath, L"rb");
        if (f) {
            fread(&ending, 1, 1, f);
            fclose(f);
        }
    } else {
        //Read the ending
        HKEY hKey = NULL;
        if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Oneshot", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            DWORD size = sizeof(ending);
            if (RegQueryValueExW(hKey, L"ending", 0, NULL, (LPBYTE)(&ending), &size) != ERROR_SUCCESS)
                ending = ENDING_BEGINNING;
        }
        RegCloseKey(hKey);
    }

    return ending;
}

void util_saveEnding()
{
    if (!forceEnding) {
        if (oneshot && ending <= ENDING_DEJAVU)
            ending = ENDING_DEAD;
        if (gameStarted && ending == ENDING_BEGINNING)
            ending = ENDING_DEJAVU;
    }

    if (ending != ENDING_BEGINNING) {
        int endingOld = util_loadEnding();

        //If niko's dead, and niko was dead when we loaded,
        //just bring him back
        if (ending == ENDING_DEAD && endingOld == ENDING_DEAD) {
            DeleteFileW(szSavePath);
            ending = ENDING_DEJAVU;
        }

        //On Windows, save in the registry.
        //On *nix, save in a file in the home folder (My Documents).

        if (winver == WIN_WINE) {
            FILE *f = _wfopen(szEndingPath, L"wb");
            fwrite(&ending, 1, 1, f);
            fclose(f);
        } else {
            HKEY hKey = NULL;
            if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Oneshot", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
                RegSetValueExW(hKey, L"ending", 0, REG_DWORD, (PVOID)&ending, sizeof(ending));
            }
            RegCloseKey(hKey);
        }
    }
}
