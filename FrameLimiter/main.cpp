#include <Windows.h>
#include <TlHelp32.h>
#include <cstdio>

// Utilities
typedef LONG(NTAPI* NtSuspendProcess)(IN HANDLE ProcessHandle);
typedef LONG(NTAPI* NtResumeProcess)(IN HANDLE ProcessHandle);

NtSuspendProcess dSuspendProcess = nullptr;
NtResumeProcess dResumeProcess = nullptr;

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
void handle(int type, int process)
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

    // Get ID
    DWORD game = get(L"BadBloodGame.exe");
    
    if (game == 0)
    {
        // What a pity
        printf("Couldn't find game process...\n");

        // Let that sink in...
        Sleep(3000);

        // Return
        return;
    }

    // Initialize
    init();

    // Binds
    ULONGLONG last_use = GetTickCount64();

    // Infinite loop which we can break...
    while (true)
    {
        // INSERT for quit, obviously
        if (GetAsyncKeyState(VK_INSERT))
            break;
        else if (GetAsyncKeyState(VK_NEXT)) // Page Down
            handle(2, game);
        else if (GetAsyncKeyState(VK_PRIOR)) // Page Up
            handle(1, game);
        else if (GetAsyncKeyState(VK_RCONTROL)) // Right control
        {
            if (last_use - GetTickCount64() > 150)
            {
                // Suspend
                handle(1, game);

                // Wait
                Sleep(150);

                // Resume
                handle(2, game);
                    
                // Update timer
                last_use = GetTickCount64();
            }
        }
    }
    
    // Goodbye
    printf("Farewell!");

    // Free
    FreeConsole();
}
