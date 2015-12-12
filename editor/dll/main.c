#include "ogg.h"

#define HFILE_FAKE ((HANDLE)1)

LPSTR
util_getFilename(LPCSTR szFile)
{
    LPSTR szFileNew = strdup(szFile);
    LPSTR szFileRelative = szFileNew + strlen(szFileNew) - 1;

    //Get the "relative" path of the file
    BOOL firstSlash = FALSE;
    for(; szFileRelative > szFileNew; szFileRelative--)
    {
        if(*szFileRelative == '\\')
        {
            if(firstSlash)
            {
                szFileRelative++;
                break;
            }
            firstSlash = TRUE;
        }
    }

    //Replace music wav with ogg
    if(!strnicmp(szFileRelative, "Music\\", 6))
    {
        LPSTR ext = PathFindExtensionA(szFileRelative) + 1;
        if(!stricmp(ext, "wav"))
            strcpy(ext, "ogg");
    }

    return szFileNew;
}

void
util_oggToWav(LPSTR szFile)
{
    LPSTR ext = PathFindExtensionA(szFile) + 1;
    if(!stricmp(ext, "ogg"))
        strcpy(ext, "wav");
}

//----------------
//HOOKED FUNCTIONS
//----------------

//KERNEL

HANDLE WINAPI
hook_FindFirstFileA(LPCSTR lpFileName, LPWIN32_FIND_DATAA lpFindFileData)
{
    LPSTR lpFileNameNew = util_getFilename(lpFileName);
    HANDLE result = FindFirstFileA(lpFileNameNew, lpFindFileData);
    free(lpFileNameNew);

    if(result != INVALID_HANDLE_VALUE)
    {
        util_oggToWav(lpFindFileData->cFileName);
        util_oggToWav(lpFindFileData->cAlternateFileName);
    }
    return result;
}

BOOL WINAPI
hook_FindNextFileA(HANDLE hFindFile, LPWIN32_FIND_DATAA lpFindFileData)
{
    BOOL result = FindNextFileA(hFindFile, lpFindFileData);
    if(result)
    {
        util_oggToWav(lpFindFileData->cFileName);
        util_oggToWav(lpFindFileData->cAlternateFileName);
    }
    return result;
}

LPSTR szOggOld = NULL;

HANDLE WINAPI
hook_CreateFileA(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
    LPSTR lpFileNameNew = util_getFilename(lpFileName);
    LPSTR ext = PathFindExtensionA(lpFileNameNew) + 1;

    if(stricmp(ext, "ogg"))
    {
        HANDLE result = CreateFileA(lpFileNameNew, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
        free(lpFileNameNew);
        return result;
    }

    //Convert the ogg to a wav in memory
    if(szOggOld && !stricmp(szOggOld, lpFileNameNew))
    {
        //If we've already got an OGG loaded in memory with this filename,
        //just reset it
        wav_vio_seek(0, SEEK_SET, NULL);
        free(lpFileNameNew);
    }
    else
    {
        //Load the new ogg
        ogg_free();
        ogg_read(lpFileNameNew);
        free(szOggOld);
        szOggOld = lpFileNameNew;
    }
    return HFILE_FAKE;
}

BOOL WINAPI
hook_ReadFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped)
{
    if(hFile == HFILE_FAKE)
    {
        sf_count_t count = wav_vio_read(lpBuffer, nNumberOfBytesToRead, NULL);
        if(lpNumberOfBytesRead)
            *lpNumberOfBytesRead = count;
        return TRUE;
    }
    return ReadFile(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped);
}

DWORD WINAPI
hook_SetFilePointer(HANDLE hFile, LONG lDistanceToMove, PLONG lpDistanceToMoveHigh, DWORD dwMoveMethod)
{
    if(hFile == HFILE_FAKE)
    {
        int whence;
        switch(dwMoveMethod)
        {
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
    if(hObject == HFILE_FAKE)
        return TRUE;
    return CloseHandle(hObject);
}

//---------
//HOOKING
//---------
typedef struct
{
    const char* name;
    void* funcHook;
    void* funcOrig;
} Hook;

#define MODULE(name)    {#name "32.dll", NULL, NULL},
#define HOOK(name)      {#name, hook_##name, NULL},

#define HOOK_MAX (sizeof(hooks) / sizeof(hooks[0]))
Hook hooks[] =
{
    MODULE(kernel)
    HOOK(FindFirstFileA)
    HOOK(FindNextFileA)
    HOOK(CreateFileA)
    HOOK(ReadFile)
    HOOK(SetFilePointer)
    HOOK(CloseHandle)
};

#define MAKE_POINTER(cast, ptr, offset) (cast)((DWORD)(ptr)+(DWORD)(offset))

BOOL
hook()
{
    //Initialize original function addresses
    HMODULE module = NULL;
    int i, j;
    for(i = 0; i < HOOK_MAX; i++)
    {
        if(hooks[i].funcHook)
            hooks[i].funcOrig = (void*)GetProcAddress(module, hooks[i].name);
        else
            module = GetModuleHandleA(hooks[i].name);
    }

    //Check DOS Header
    PIMAGE_DOS_HEADER dos = (PIMAGE_DOS_HEADER)GetModuleHandleA(NULL);
    if(dos->e_magic != IMAGE_DOS_SIGNATURE)
        return FALSE;

    //Check NT Header
    PIMAGE_NT_HEADERS nt = MAKE_POINTER(PIMAGE_NT_HEADERS, dos, dos->e_lfanew);
    if(nt->Signature != IMAGE_NT_SIGNATURE)
        return FALSE;

    //Check import module table
    PIMAGE_IMPORT_DESCRIPTOR modules = MAKE_POINTER(PIMAGE_IMPORT_DESCRIPTOR, dos, nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
    if(modules == (PIMAGE_IMPORT_DESCRIPTOR)nt)
        return FALSE;

    //Find the correct module
    while(modules->Name)
    {
        const char* szModule = MAKE_POINTER(const char*, dos, modules->Name);

        for(i = 0; i < HOOK_MAX; i++)
        {
            if(!hooks[i].funcHook && !strcasecmp(hooks[i].name, szModule))
                break;
        }

        if(i < HOOK_MAX)
        {
            //Find the correct function
            PIMAGE_THUNK_DATA thunk = MAKE_POINTER(PIMAGE_THUNK_DATA, dos, modules->FirstThunk);
            while(thunk->u1.Function)
            {
                for(j = i + 1; j < HOOK_MAX && hooks[j].funcHook; j++)
                {
                    if(thunk->u1.Function == (DWORD)hooks[j].funcOrig)
                    {
                        //Overwrite
                        DWORD flags;
                        if(!VirtualProtect(&thunk->u1.Function, sizeof(thunk->u1.Function), PAGE_READWRITE, &flags))
                            return FALSE;
                        thunk->u1.Function = (DWORD)hooks[j].funcHook;
                        if(!VirtualProtect(&thunk->u1.Function, sizeof(thunk->u1.Function), flags, &flags))
                            return FALSE;
                    }
                }
                thunk++;
            }
        }
        modules++;
    }
    return TRUE;
}

//Entry point
BOOL WINAPI
DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch(fdwReason)
    {
        case DLL_PROCESS_ATTACH:
        {
            return hook();
        }
        break;

        case DLL_PROCESS_DETACH:
        {
            free(szOggOld);
            ogg_free();
        }
        break;
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    }
    return TRUE;
}
