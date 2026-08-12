// Minimal DLL injector for SvenCoop_Enhanced.dll.
//
// Usage: Injector.exe [dll_path] [process_name]
//   dll_path      - defaults to "SvenCoop_Enhanced.dll" sitting next to this exe
//   process_name  - defaults to "svencoop.exe"
//
// Classic CreateRemoteThread + LoadLibraryA injection: allocate a small
// buffer in the target process, write the DLL's absolute path into it, then
// start a remote thread whose entry point is kernel32's LoadLibraryA with
// that buffer as its argument - the target process ends up calling
// LoadLibraryA(dllPath) on itself, which triggers the DLL's own DllMain
// exactly as if it had loaded the DLL normally.
//
// This relies on kernel32.dll being mapped at the same address in this
// process and the target - true for every process in the same Windows
// session (system DLLs like kernel32 aren't independently ASLR-relocated
// per process), so resolving LoadLibraryA's address here and handing it to
// CreateRemoteThread for a *different* process is valid.
//
// Both this project and SvenCoop_Enhanced.dll are intentionally Win32 (x86)
// only - Sven Co-op is a 32-bit GoldSrc game. A 64-bit injector can't
// CreateRemoteThread into a 32-bit target process (the WOW64 boundary
// breaks the technique above), so there is no x64 configuration here at all
// rather than a broken/unused one.

#include <windows.h>
#include <tlhelp32.h>
#include <cstdio>
#include <cstring>
#include <string>

// Case-insensitive match against every running process's image name.
// Returns 0 if not found (0 is never a valid PID).
static DWORD FindProcessId(const char* processName)
{
    DWORD pid = 0;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
        return 0;

    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(entry);
    if (Process32First(snapshot, &entry))
    {
        do
        {
            if (_stricmp(entry.szExeFile, processName) == 0)
            {
                pid = entry.th32ProcessID;
                break;
            }
        } while (Process32Next(snapshot, &entry));
    }

    CloseHandle(snapshot);
    return pid;
}

// Polls for the target process instead of requiring it to already be
// running - lets you start the injector first and launch the game after,
// which is often more convenient than timing it the other way around.
static DWORD WaitForProcessId(const char* processName, DWORD timeoutMs)
{
    DWORD pid = FindProcessId(processName);
    if (pid != 0)
        return pid;

    printf("waiting for %s to start...\n", processName);

    const DWORD pollInterval = 500;
    for (DWORD waited = 0; waited < timeoutMs; waited += pollInterval)
    {
        Sleep(pollInterval);
        pid = FindProcessId(processName);
        if (pid != 0)
            return pid;
    }
    return 0;
}

static bool InjectDll(DWORD pid, const char* dllPath)
{
    HANDLE process = OpenProcess(
        PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION |
        PROCESS_VM_WRITE | PROCESS_VM_READ,
        FALSE, pid);
    if (!process)
    {
        printf("OpenProcess failed (err=%lu) - try running the injector as administrator\n", GetLastError());
        return false;
    }

    size_t pathSize = strlen(dllPath) + 1;
    LPVOID remotePath = VirtualAllocEx(process, nullptr, pathSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remotePath)
    {
        printf("VirtualAllocEx failed (err=%lu)\n", GetLastError());
        CloseHandle(process);
        return false;
    }

    if (!WriteProcessMemory(process, remotePath, dllPath, pathSize, nullptr))
    {
        printf("WriteProcessMemory failed (err=%lu)\n", GetLastError());
        VirtualFreeEx(process, remotePath, 0, MEM_RELEASE);
        CloseHandle(process);
        return false;
    }

    // See the file header comment for why resolving this locally (rather
    // than in the remote process) is valid.
    LPTHREAD_START_ROUTINE loadLibraryAddr =
        (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA");

    HANDLE remoteThread = CreateRemoteThread(process, nullptr, 0, loadLibraryAddr, remotePath, 0, nullptr);
    if (!remoteThread)
    {
        printf("CreateRemoteThread failed (err=%lu)\n", GetLastError());
        VirtualFreeEx(process, remotePath, 0, MEM_RELEASE);
        CloseHandle(process);
        return false;
    }

    WaitForSingleObject(remoteThread, INFINITE);

    DWORD loadedModule = 0;
    GetExitCodeThread(remoteThread, &loadedModule); // LoadLibraryA's return value, as an HMODULE

    CloseHandle(remoteThread);
    VirtualFreeEx(process, remotePath, 0, MEM_RELEASE);
    CloseHandle(process);

    if (loadedModule == 0)
    {
        printf("LoadLibraryA returned NULL in the target process - the DLL failed to load "
               "(wrong bitness, a missing dependency, or it rejected the process).\n");
        return false;
    }

    printf("injected successfully (remote HMODULE = 0x%08lX)\n", loadedModule);
    return true;
}

// The DLL is loaded by the *target* process, so a relative path would
// resolve against the target's working directory, not the injector's -
// silently failing to find a DLL that's sitting right next to the injector
// on disk. Resolve to an absolute path next to this executable instead.
static std::string ResolveDllPath(const char* dllName)
{
    char selfPath[MAX_PATH];
    GetModuleFileNameA(nullptr, selfPath, MAX_PATH);

    std::string dir(selfPath);
    size_t lastSlash = dir.find_last_of("\\/");
    dir = (lastSlash == std::string::npos) ? "" : dir.substr(0, lastSlash + 1);

    return dir + dllName;
}

int main(int argc, char** argv)
{
    const char* dllName = (argc > 1) ? argv[1] : "SvenCoop_Enhanced.dll";
    const char* processName = (argc > 2) ? argv[2] : "svencoop.exe";

    std::string dllPath = ResolveDllPath(dllName);

    if (GetFileAttributesA(dllPath.c_str()) == INVALID_FILE_ATTRIBUTES)
    {
        printf("'%s' not found - build SvenCoop_Enhanced.dll (Win32) first and place it next to "
               "the injector, or pass its path as the first argument.\n", dllPath.c_str());
        return 1;
    }

    printf("target process : %s\n", processName);
    printf("dll            : %s\n", dllPath.c_str());

    DWORD pid = WaitForProcessId(processName, 30000);
    if (pid == 0)
    {
        printf("'%s' not found after waiting 30s - is the game running?\n", processName);
        return 1;
    }

    printf("found %s (pid %lu)\n", processName, pid);

    return InjectDll(pid, dllPath.c_str()) ? 0 : 1;
}
