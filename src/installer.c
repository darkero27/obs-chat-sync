#define WIN32_LEAN_AND_MEAN
#define UNICODE
#define _UNICODE

#include <windows.h>
#include <tlhelp32.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <stdio.h>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "ole32.lib")

#define IDR_DLL_FILE 101

#define APP_TITLE L"OBS Auto Chat Sync - Installer"

static BOOL IsOBS64Running(void) {
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) return FALSE;
    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(pe);
    BOOL running = FALSE;
    if (Process32FirstW(hSnap, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, L"obs64.exe") == 0) {
                running = TRUE;
                break;
            }
        } while (Process32NextW(hSnap, &pe));
    }
    CloseHandle(hSnap);
    return running;
}

static BOOL CheckDirectoryHasOBS(const wchar_t *dir) {
    wchar_t testPath[MAX_PATH];
    swprintf_s(testPath, MAX_PATH, L"%s\\bin\\64bit\\obs64.exe", dir);
    return PathFileExistsW(testPath);
}

static BOOL FindOBSDirectory(wchar_t *outDir, DWORD maxLen) {
    // 1. Steam path
    const wchar_t *steamPath = L"C:\\Program Files (x86)\\Steam\\steamapps\\common\\OBS Studio";
    if (CheckDirectoryHasOBS(steamPath)) {
        wcscpy_s(outDir, maxLen, steamPath);
        return TRUE;
    }

    // 2. Standard 64-bit path
    const wchar_t *stdPath = L"C:\\Program Files\\obs-studio";
    if (CheckDirectoryHasOBS(stdPath)) {
        wcscpy_s(outDir, maxLen, stdPath);
        return TRUE;
    }

    // 3. Check Windows Registry
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\OBS Studio", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD type = 0;
        DWORD size = maxLen * sizeof(wchar_t);
        if (RegQueryValueExW(hKey, NULL, NULL, &type, (LPBYTE)outDir, &size) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            if (CheckDirectoryHasOBS(outDir)) {
                return TRUE;
            }
        }
        RegCloseKey(hKey);
    }

    return FALSE;
}

static BOOL ExtractResourceToFile(HMODULE hMod, int resId, const wchar_t *destPath) {
    HRSRC hRes = FindResourceW(hMod, MAKEINTRESOURCEW(resId), RT_RCDATA);
    if (!hRes) return FALSE;

    HGLOBAL hData = LoadResource(hMod, hRes);
    if (!hData) return FALSE;

    DWORD size = SizeofResource(hMod, hRes);
    const void *pData = LockResource(hData);
    if (!pData || size == 0) return FALSE;

    // Ensure parent directory exists
    wchar_t dir[MAX_PATH];
    wcscpy_s(dir, MAX_PATH, destPath);
    wchar_t *lastSlash = wcsrchr(dir, L'\\');
    if (lastSlash) {
        *lastSlash = L'\0';
        SHCreateDirectoryExW(NULL, dir, NULL);
    }

    HANDLE hFile = CreateFileW(destPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        // If file locked by OBS, attempt rename
        wchar_t oldPath[MAX_PATH];
        swprintf_s(oldPath, MAX_PATH, L"%s.old", destPath);
        MoveFileExW(destPath, oldPath, MOVEFILE_REPLACE_EXISTING);
        hFile = CreateFileW(destPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    }

    if (hFile == INVALID_HANDLE_VALUE) return FALSE;

    DWORD written = 0;
    BOOL ok = WriteFile(hFile, pData, size, &written, NULL);
    CloseHandle(hFile);

    return ok && (written == size);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nShowCmd) {
    // 1. Check if OBS is running
    while (IsOBS64Running()) {
        int choice = MessageBoxW(
            NULL,
            L"OBS Studio is currently running!\n\n"
            L"Please save your work and close OBS Studio before proceeding with installation.\n\n"
            L"Click 'Retry' after closing OBS Studio, or 'Cancel' to abort.",
            APP_TITLE,
            MB_RETRYCANCEL | MB_ICONWARNING
        );
        if (choice != IDRETRY) {
            return 0;
        }
    }

    // 2. Locate OBS Studio
    wchar_t obsDir[MAX_PATH] = {0};
    if (!FindOBSDirectory(obsDir, MAX_PATH)) {
        // Let user browse folder
        BROWSEINFOW bi = {0};
        bi.lpszTitle = L"Select your OBS Studio installation folder (contains bin\\64bit\\obs64.exe):";
        bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
        PIDLIST_ABSOLUTE pidl = SHBrowseForFolderW(&bi);
        if (pidl != 0) {
            SHGetPathFromIDListW(pidl, obsDir);
            CoTaskMemFree(pidl);
        }

        if (!CheckDirectoryHasOBS(obsDir)) {
            MessageBoxW(
                NULL,
                L"Valid OBS Studio installation directory was not found.\n\n"
                L"Make sure the selected folder contains 'bin\\64bit\\obs64.exe'.",
                APP_TITLE,
                MB_OK | MB_ICONERROR
            );
            return 1;
        }
    }

    // 3. Confirm installation
    wchar_t confirmMsg[MAX_PATH * 2];
    swprintf_s(
        confirmMsg,
        MAX_PATH * 2,
        L"OBS Studio detected at:\n%s\n\n"
        L"The installer will install:\n"
        L"  - Built-in Plugin DLL (obs-plugins\\64bit\\obs-chat-sync.dll)\n\n"
        L"Do you want to proceed with the installation?",
        obsDir
    );

    int confirm = MessageBoxW(NULL, confirmMsg, APP_TITLE, MB_YESNO | MB_ICONQUESTION);
    if (confirm != IDYES) {
        return 0;
    }

    // 4. Install DLL
    wchar_t targetDll[MAX_PATH];
    swprintf_s(targetDll, MAX_PATH, L"%s\\obs-plugins\\64bit\\obs-chat-sync.dll", obsDir);
    BOOL dllOk = ExtractResourceToFile(hInstance, IDR_DLL_FILE, targetDll);

    if (!dllOk) {
        MessageBoxW(
            NULL,
            L"Installation failed.\n\n"
            L"Could not write plugin DLL to the OBS Studio directory.\n"
            L"Please make sure you have Administrator privileges and OBS Studio is not running.",
            APP_TITLE,
            MB_OK | MB_ICONERROR
        );
        return 1;
    }

    // 5. Success message
    wchar_t successMsg[MAX_PATH * 3];
    swprintf_s(
        successMsg,
        MAX_PATH * 3,
        L"Installation Completed Successfully!\n\n"
        L"Installed:\n"
        L"  [OK] Plugin DLL -> %s\n\n"
        L"How to use in OBS Studio:\n"
        L"  Open 'Tools' -> 'Auto Chat Browser Source Settings'\n\n"
        L"You can now launch OBS Studio!",
        targetDll
    );

    MessageBoxW(NULL, successMsg, APP_TITLE, MB_OK | MB_ICONINFORMATION);
    return 0;
}
