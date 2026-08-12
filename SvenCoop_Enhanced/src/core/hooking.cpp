#include "hooking.h"
#include "logging.h"
#include <cstdio>
#include "../../ext/minhook/include/MinHook.h"

bool HookManager::Init()
{
    if (MH_Initialize() != MH_OK)
    {
        printf("[hooking] MH_Initialize failed\n");
        return false;
    }
    return true;
}

bool HookManager::Create(const char* name, void* target, void* detour, void** original)
{
    MH_STATUS status = MH_CreateHook(target, detour, original);
    if (status != MH_OK)
    {
        printf("[hooking] failed to create hook '%s': %s\n", name, MH_StatusToString(status));
        return false;
    }
    printf("[hooking] created hook '%s'\n", name);
    return true;
}

bool HookManager::EnableAll()
{
    if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK)
    {
        printf("[hooking] MH_EnableHook(ALL) failed\n");
        return false;
    }
    printf("[hooking] all hooks enabled\n");
    return true;
}

void HookManager::DisableAll()
{
    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();
    printf("[hooking] all hooks disabled\n");
}