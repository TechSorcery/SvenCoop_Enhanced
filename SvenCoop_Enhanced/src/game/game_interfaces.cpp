#include "game_interfaces.h"
#include <windows.h>

#define GENGFUNCS_OFFSET 0x1F8998

cl_enginefunc_t* GetEngineFuncs()
{
    static cl_enginefunc_t* ptr = nullptr;
    if (!ptr)
    {
        uintptr_t base = (uintptr_t)GetModuleHandleA("client.dll");
        ptr = (cl_enginefunc_t*)(base + GENGFUNCS_OFFSET);
    }
    return ptr;
}