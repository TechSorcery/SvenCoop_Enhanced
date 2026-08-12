#pragma once

namespace HookManager
{
    bool Init();
    bool Create(const char* name, void* target, void* detour, void** original);
    bool EnableAll();
    void DisableAll();
}