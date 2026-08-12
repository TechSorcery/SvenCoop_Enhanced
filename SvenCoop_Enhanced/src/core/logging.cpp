#include "logging.h"
#include <windows.h>
#include <cstdio>

static FILE* g_console_stdout = nullptr;

void Logging::Init()
{
    AllocConsole();
    freopen_s(&g_console_stdout, "CONOUT$", "w", stdout);
    SetConsoleTitleA("SvenCoop_Enhanced");
}

void Logging::Shutdown()
{
    if (g_console_stdout)
        fclose(g_console_stdout);
    FreeConsole();
}