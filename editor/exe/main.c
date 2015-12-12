#include <windows.h>
#include <shlwapi.h>
#include <stdio.h>

#define STRLEN(str) (sizeof(str) / sizeof(str[0]) - 1)

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    int code = 0;

    //Get this program's path, replace extension with DLL
    unsigned int size = 1;
    LPWSTR szDllPath;
    for (;;) {
        szDllPath = (WCHAR *)malloc(size * sizeof(WCHAR));
        if (GetModuleFileNameW(NULL, szDllPath, size) < size)
            break;
        free(szDllPath);
        size *= 2;
    }
    LPWSTR szExt = PathFindExtensionW(szDllPath) + 1;
    wcscpy(szExt, L"dll");
    int sizeDllPath = (szExt - szDllPath) + 4;

    //Copy and add replace name with RPG2003.EXE
    //Allocate a slightly larger buffer than necessary so we don't overflow
    LPWSTR szExePath = malloc((sizeDllPath + 11) * sizeof(WCHAR));
    wcscpy(szExePath, szDllPath);
    PathRemoveFileSpecW(szExePath);
    PathAddBackslashW(szExePath);
    wcscat(szExePath, L"RPG2003.EXE");

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

    //Spawn the RPG2003.EXE process and inject

    if (!CreateProcessW(NULL, szExePath, 0, 0, FALSE, CREATE_SUSPENDED, NULL, NULL, &si, &pi))
        goto err;

    if (!(szDllPathRemote = VirtualAllocEx(pi.hProcess, NULL, sizeDllPath * sizeof(WCHAR), MEM_COMMIT, PAGE_READWRITE)))
        goto err;

    if (!WriteProcessMemory(pi.hProcess, szDllPathRemote, szDllPath, sizeDllPath * sizeof(WCHAR), NULL))
        goto err;

    if (!(kernel32 = LoadLibraryW(L"kernel32.dll")))
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
    MessageBoxW(NULL, L"Could not inject DLL into RPG2003.EXE.", L"Error", MB_ICONERROR);

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
    } else {
        ResumeThread(pi.hThread);
    }

    free(szDllPath);
    free(szExePath);

    return code;
}
