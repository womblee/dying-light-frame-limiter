#include <Windows.h>
#include <TlHelp32.h>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <memory>

// Typedefs for function pointers
using NtSuspendProcess = LONG(NTAPI*)(IN HANDLE ProcessHandle);
using NtResumeProcess = LONG(NTAPI*)(IN HANDLE ProcessHandle);

// Global function pointers
NtSuspendProcess dSuspendProcess = nullptr;
NtResumeProcess dResumeProcess = nullptr;

// RAII wrapper for HANDLE
class ScopedHandle
{
    HANDLE handle;
public:
    explicit ScopedHandle(HANDLE h) : handle(h) {}
    ~ScopedHandle() { if (handle != INVALID_HANDLE_VALUE) CloseHandle(handle); }
    HANDLE get() const { return handle; }
    ScopedHandle(const ScopedHandle&) = delete;
    ScopedHandle& operator=(const ScopedHandle&) = delete;
};

// Check if process is running
bool is_running(DWORD process_id)
{
    ScopedHandle proc(OpenProcess(SYNCHRONIZE, FALSE, process_id));
    if (proc.get() == nullptr) return false;

    return WaitForSingleObject(proc.get(), 0) == WAIT_TIMEOUT;
}

// Get process ID by name
DWORD get_process_id(const std::wstring& name)
{
    PROCESSENTRY32 pt;
    pt.dwSize = sizeof(PROCESSENTRY32);
    ScopedHandle hsnap(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));
    if (hsnap.get() == INVALID_HANDLE_VALUE) throw std::runtime_error("CreateToolhelp32Snapshot failed");

    if (Process32First(hsnap.get(), &pt))
    {
        do
        {
            if (name == pt.szExeFile) return pt.th32ProcessID;
        } while (Process32Next(hsnap.get(), &pt));
    }
    return 0;
}

// Handle process operations
void handle_process(int operation, DWORD process_id)
{
    ScopedHandle h(OpenProcess(PROCESS_ALL_ACCESS, FALSE, process_id));
    if (h.get() == nullptr) return;

    switch (operation)
    {
    case 1: dSuspendProcess(h.get()); break;
    case 2: dResumeProcess(h.get()); break;
    }
}

// Initialize function pointers
void init()
{
    HMODULE ntmod = GetModuleHandleA("ntdll");
    if (!ntmod) throw std::runtime_error("Failed to get ntdll handle");

    dSuspendProcess = reinterpret_cast<NtSuspendProcess>(GetProcAddress(ntmod, "NtSuspendProcess"));
    dResumeProcess = reinterpret_cast<NtResumeProcess>(GetProcAddress(ntmod, "NtResumeProcess"));

    if (!dSuspendProcess || !dResumeProcess) throw std::runtime_error("Failed to get process functions");

    printf("RIGHT CONTROL - Freeze Lag, INSERT - Quit\n"
        "[Miscellaneous: PAGE UP - Freeze process | PAGE DOWN - Unfreeze process]\n\n"
        "Made with love by nloginov @ 2023\n");
}

int main()
{
    try
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

        const std::wstring PROCESS_NAME = L"DyingLightGame.exe";
        constexpr ULONGLONG FREEZE_INTERVAL = 150;

        while (true)
        {
            DWORD game_pid = get_process_id(PROCESS_NAME);
            if (!game_pid)
            {
                Sleep(1000); // Wait before trying again
                continue;
            }

            ULONGLONG last_use = GetTickCount64();

            while (is_running(game_pid))
            {
                if (GetAsyncKeyState(VK_INSERT) & 0x8000) return 0;
                if (GetAsyncKeyState(VK_NEXT) & 0x8000) handle_process(2, game_pid);
                if (GetAsyncKeyState(VK_PRIOR) & 0x8000) handle_process(1, game_pid);

                if (GetAsyncKeyState(VK_RCONTROL) & 0x8000) {
                    ULONGLONG current = GetTickCount64();
                    if (current - last_use > FREEZE_INTERVAL) {
                        handle_process(1, game_pid);
                        Sleep(FREEZE_INTERVAL);
                        handle_process(2, game_pid);
                        last_use = current;
                    }
                }

                Sleep(10); // Reduce CPU usage
            }
        }
    }
    catch (const std::exception& e)
    {
        MessageBoxA(NULL, e.what(), "Error", MB_ICONERROR);
    }

    FreeConsole();
    return 0;
}