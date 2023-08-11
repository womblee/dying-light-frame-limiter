#include <Windows.h>
#include <TlHelp32.h>
#include <cstdio>

// Utilities
typedef LONG(NTAPI* NtSuspendProcess)(IN HANDLE ProcessHandle);
typedef LONG(NTAPI* NtResumeProcess)(IN HANDLE ProcessHandle);

NtSuspendProcess dSuspendProcess = nullptr;
NtResumeProcess dResumeProcess = nullptr;

// Process running?
BOOL is_running(DWORD process)
{
    HANDLE proc = OpenProcess(SYNCHRONIZE, FALSE, process);
    DWORD ret = WaitForSingleObject(proc, 0);
    CloseHandle(proc);
    return ret == WAIT_TIMEOUT;
}

// Get process
DWORD get(LPCTSTR name)
{
    PROCESSENTRY32 pt;
    HANDLE hsnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    pt.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(hsnap, &pt))
    {
        do
        {
            if (!lstrcmpi(pt.szExeFile, name))
            {
                CloseHandle(hsnap);

                return pt.th32ProcessID;
            }
        } while (Process32Next(hsnap, &pt));
    }

    CloseHandle(hsnap);
    return 0;
}

// Handling process
void handle(int type, DWORD process)
{
    HANDLE h = OpenProcess(PROCESS_ALL_ACCESS, FALSE, process);

    if (h == nullptr)
        return;

    switch (type)
    {
    case 1:
        dSuspendProcess(h);

        break;
    case 2:
        dResumeProcess(h);

        break;
    }

    CloseHandle(h);
}

// Init
void init()
{
    // Load NtSuspendProcess from ntdll.dll
    HMODULE ntmod = GetModuleHandleA("ntdll");

    dSuspendProcess = (NtSuspendProcess)GetProcAddress(ntmod, "NtSuspendProcess");
    dResumeProcess = (NtResumeProcess)GetProcAddress(ntmod, "NtResumeProcess");

    // Congratulations
    printf("RIGHT CONTROL - Freeze Lag, INSERT - Quit\n[Miscellaneous: PAGE UP - Freeze process | PAGE DOWN - Unfreeze process]\n\nMade with love by nloginov @ 2023");
}

// Definitions
#define GET_PROC get(L"DyingLightGame.exe")
#define CURRENT GetTickCount64()

// Main
void main()
{
    // Allocate console
    AllocConsole();
    AttachConsole(GetCurrentProcessId());
    FILE* out{};
    freopen_s(&out, "CONOUT$", "w", stdout);

    // Console title
    SetConsoleTitleA("Frame Limiter");

    // Initialize
    init();

    // We do this so that we can grab the process not just 1 time
    while (true)
    {
        // Getting game process
        DWORD game = GET_PROC;

        // Variable for freeze lag
        ULONGLONG last_use = CURRENT;

        // Cool loop here...
        while (is_running(game))
        {
            // INSERT for quit, obviously
            if (GetAsyncKeyState(VK_INSERT))
                TerminateProcess(GetCurrentProcess(), 0);
            else if (GetAsyncKeyState(VK_NEXT)) // Page Down
                handle(2, game);
            else if (GetAsyncKeyState(VK_PRIOR)) // Page Up
                handle(1, game);
            else if (GetAsyncKeyState(VK_RCONTROL)) // Right control
            {
                if (last_use - CURRENT > 150)
                {
                    // Suspend
                    handle(1, game);

                    // Wait
                    Sleep(150);

                    // Resume
                    handle(2, game);

                    // Update timer
                    last_use = CURRENT;
                }
            }
        }
    }

    // Free everything
    FreeConsole();
}