#include <windows.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <stdio.h>
#include "../encrypt_key.h"

#if defined L_KOREAN
#define STR_ERROR L"RPG_RT.exe를 실행할 수 없습니다."
#elif defined L_RUSSIAN
#define FONT_FILE "RM2000.fon"
#define STR_ERROR L"Could not start RPG_RT.exe."
#else
#define STR_ERROR L"Could not start RPG_RT.exe."
#endif

wchar_t *_aswprintf(const wchar_t *fmt, ...)
{
    va_list va_args;
    va_start(va_args, fmt);
    int size = _vscwprintf(fmt, va_args) + 1;
    wchar_t *dst = malloc(size * 2);
    vsnwprintf(dst, size, fmt, va_args);
    va_end(va_args);
    return dst;
}

BOOL writeResource(HINSTANCE hInstance, const char *name, LPWSTR filename, BOOL decrypt)
{
    HGLOBAL hRes = NULL;
    HRSRC res;

    res = FindResourceA(hInstance, name, RT_RCDATA);
    if (!res)
        return TRUE;
    if (!(hRes = LoadResource(NULL, res)))
        return TRUE;

    unsigned char *data = (unsigned char *)LockResource(hRes);
    size_t size = (size_t)SizeofResource(NULL, res);

    FILE *f = _wfopen(filename, L"wb");
    if (!f)
        return TRUE;
    if (decrypt) {
        int i = 0;
        for (; i < size; i++) {
            fputc(data[i] ^ key[i % KEY_SIZE], f);
        }
    } else {
        fwrite(data, size, 1, f);
    }
    fclose(f);

    return FALSE;
}

#define STRLEN(str) (sizeof(str) / sizeof(str[0]) - 1)
#define DLL_NAME L"hook.dll"
#define FUNC_NAME L"func.sh"
#define SNDFILE_NAME L"libsndfile-1.dll"
#define PAPER_32_NAME L"paper32"
#define PAPER_64_NAME L"paper64"
#define EXE_ARGS L" x x Window"

#define TEMP_PATH_UNIX L"Z:\\tmp\\"
#define TEMP_PATH L"Oneshot\\"

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    int code = 0;

    //Injection variables
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    memset(&si, 0, sizeof(si));
    memset(&pi, 0, sizeof(pi));
    si.cb = sizeof(si);

    void *szDllPathRemote = NULL;
    HMODULE kernel32 = NULL;
    void *pLoadLibraryW = NULL;
    HANDLE thread = NULL;

    LPWSTR szTempPath = NULL;

    //Get this program's path and module name
    unsigned int size = 1;
    LPWSTR szGamePath;
    for (;;) {
        szGamePath = (WCHAR *)malloc(size * sizeof(WCHAR));
        if (GetModuleFileNameW(NULL, szGamePath, size) < size)
            break;
        free(szGamePath);
        size *= 2;
    }

    //Module name (RPG_RT.exe, usually)
    LPWSTR szExeName = wcsdup(szGamePath);
    PathStripPathW(szExeName);

    //Game path
    PathRemoveFileSpecW(szGamePath);
    PathAddBackslashW(szGamePath);

    //Construct the temporary file directory path
    kernel32 = LoadLibraryW(L"kernel32.dll");
    if (!kernel32)
        goto err;

    if (GetProcAddress(kernel32, "wine_get_unix_file_name")) {
        //Hack? fix this later to use a wine function, maybe
        szTempPath = malloc((STRLEN(TEMP_PATH_UNIX TEMP_PATH) + 2) * sizeof(WCHAR));
        wcscpy(szTempPath, TEMP_PATH_UNIX TEMP_PATH);
        szTempPath[STRLEN(TEMP_PATH_UNIX TEMP_PATH)+1] = 0; //Double null-terminate
    } else {
        WCHAR szTempPathRaw[MAX_PATH];
        int sizeTempPathRaw = GetTempPathW(MAX_PATH, szTempPathRaw);
        int sizeTempPath = sizeTempPathRaw + STRLEN(TEMP_PATH);
        szTempPath = malloc((sizeTempPath+2) * sizeof(WCHAR));
        wcscpy(szTempPath, szTempPathRaw);
        wcscat(szTempPath, TEMP_PATH);
        szTempPath[sizeTempPath+1] = 0; //Double null-terminate
    }

    //Construct the file paths
    LPWSTR szDllPath = _aswprintf(L"%ls" DLL_NAME, szTempPath);
    LPWSTR szExePath = _aswprintf(L"%ls%ls", szTempPath, szExeName);
    LPWSTR szFuncPath = _aswprintf(L"%ls" FUNC_NAME, szTempPath);
    LPWSTR szSndfilePath = _aswprintf(L"%ls" SNDFILE_NAME, szTempPath);
    LPWSTR szPaper32Path = _aswprintf(L"%ls" PAPER_32_NAME, szTempPath);
    LPWSTR szPaper64Path = _aswprintf(L"%ls" PAPER_64_NAME, szTempPath);
#ifdef FONT_FILE
    LPWSTR szFontPath = _aswprintf(L"%ls" FONT_FILE, szTempPath);
#endif

    //Write the embedded data to the HDD
    SHCreateDirectoryExW(NULL, szTempPath, NULL);
    if (writeResource(hInstance, "HOOK_DLL", szDllPath, TRUE))
        goto err;
    if (writeResource(hInstance, "RPG_RT_EXE", szExePath, TRUE))
        goto err;
    if (writeResource(hInstance, "FUNC_SH", szFuncPath, FALSE))
        goto err;
    if (writeResource(hInstance, "SNDFILE_DLL", szSndfilePath, TRUE))
        goto err;
    if (writeResource(hInstance, "PAPER_32", szPaper32Path, FALSE))
        goto err;
    if (writeResource(hInstance, "PAPER_64", szPaper64Path, FALSE))
        goto err;
#ifdef FONT_FILE
    if (writeResource(hInstance, "FONTFONT", szFontPath, FALSE))
        goto err;
#endif

    //Append args to exe path
    wcscat(szExePath, EXE_ARGS);

    //Spawn the RPG_RT.exe process and inject

    if (!CreateProcessW(NULL, szExePath, 0, 0, FALSE, CREATE_SUSPENDED, NULL, szGamePath, &si, &pi))
        goto err;

    int sizeDllPath = (wcslen(szDllPath) + 1) * 2;
    if (!(szDllPathRemote = VirtualAllocEx(pi.hProcess, NULL, sizeDllPath, MEM_COMMIT, PAGE_READWRITE)))
        goto err;

    if (!WriteProcessMemory(pi.hProcess, szDllPathRemote, szDllPath, sizeDllPath, NULL))
        goto err;

    if (!(pLoadLibraryW = GetProcAddress(kernel32, "LoadLibraryW")))
        goto err;

    if (!(thread = CreateRemoteThread(pi.hProcess, NULL, 0, pLoadLibraryW, szDllPathRemote, 0, NULL)))
        goto err;

    WaitForSingleObject(thread, INFINITE);

    DWORD codeRemote;
    if (GetExitCodeThread(thread, &codeRemote)) {
        if (codeRemote == 0)
            goto err;
    } else {
        goto err;
    }

    goto done;

err:
    code = 1;
    MessageBoxW(NULL, STR_ERROR, L"Error", MB_ICONERROR);

done:
    //Cleanup
    if (thread)
        CloseHandle(thread);
    if (kernel32)
        FreeLibrary(kernel32);
    if (szDllPathRemote)
        VirtualFreeEx(pi.hProcess, szDllPathRemote, 0, MEM_RELEASE);

    if (code) {
        if (pi.hProcess)
            TerminateProcess(pi.hProcess, 1);
        SHFILEOPSTRUCTW opts;
        memset(&opts, 0, sizeof(opts));
        opts.wFunc = FO_DELETE;
        opts.fFlags = FOF_SILENT | FOF_NOCONFIRMATION;
        opts.pFrom = szTempPath;
        SHFileOperationW(&opts);
    } else {
        ResumeThread(pi.hThread);
    }

    if (pi.hThread)
        CloseHandle(pi.hThread);
    if (pi.hProcess)
        CloseHandle(pi.hProcess);

//  MessageBoxW(NULL, L"here", L"", 0);

    /*free(szGamePath);
    free(szExeName);
    free(szTempPath);
    free(szDllPath);
    free(szExePath);
    free(szFuncPath);
    free(szSndfilePath);
    free(szPaper32Path);
    free(szPaper64Path);*/

//  MessageBoxW(NULL, L"here", L"", 0);

    return code;
}
