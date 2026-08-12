#include "hk_createmove.h"
#include "../features/bhop/bunnyhop.h"
#include "../features/aimbot/aimbot.h"
#include "../features/triggerbot/triggerbot.h"
#include "../core/hooking.h"
#include "../game/game_interfaces.h"
#include "../game/entity_utils.h"
#include "../ui/menu_state.h"
#include <windows.h>
#include <cstdio>

CreateMove_t Original_CreateMove = nullptr;

static float s_FrozenAngles[3];
static bool s_MenuWasOpenLastFrame = false;

void __cdecl Hooked_CreateMove(float frametime, usercmd_t* cmd, int active)
{
    Original_CreateMove(frametime, cmd, active);

    if (Menu_IsOpen())
    {
        // Hold the view still while the menu is up so arrow-key
        // navigation doesn't fight with mouselook, and don't run any
        // gameplay features while it's open. SetViewAngles is called
        // every frame (same mechanism the aimbot already uses to steer
        // the camera) to keep overriding whatever mouse movement did to
        // cl.viewangles this frame.
        if (!s_MenuWasOpenLastFrame)
        {
            s_FrozenAngles[0] = cmd->viewangles[0];
            s_FrozenAngles[1] = cmd->viewangles[1];
            s_FrozenAngles[2] = cmd->viewangles[2];
        }
        s_MenuWasOpenLastFrame = true;

        cmd->viewangles[0] = s_FrozenAngles[0];
        cmd->viewangles[1] = s_FrozenAngles[1];
        cmd->viewangles[2] = s_FrozenAngles[2];
        GetEngineFuncs()->SetViewAngles(cmd->viewangles);
        return;
    }
    s_MenuWasOpenLastFrame = false;

    Bhop_OnCreateMove(cmd);
    Aimbot_OnCreateMove(cmd);
    Triggerbot_OnCreateMove(cmd);
}

bool Hook_CreateMove()
{
    HMODULE hClient = GetModuleHandleA("client.dll");
    if (!hClient)
    {
        printf("[hk_createmove] client.dll not found\n");
        return false;
    }
    void* target = GetProcAddress(hClient, "CL_CreateMove");
    if (!target)
    {
        printf("[hk_createmove] export 'CL_CreateMove' not found\n");
        return false;
    }
    return HookManager::Create("CreateMove", target, (void*)Hooked_CreateMove, (void**)&Original_CreateMove);
}