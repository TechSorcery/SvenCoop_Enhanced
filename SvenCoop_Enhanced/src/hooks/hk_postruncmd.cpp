#include "hk_postruncmd.h"
#include "../core/hooking.h"
#include "../shared/game_state.h"
#include <windows.h>
#include <cstdio>

PostRunCmd_t Original_PostRunCmd = nullptr;

void __cdecl Hooked_PostRunCmd(local_state_t* from, local_state_t* to, usercmd_t* cmd, int runfuncs, double time, unsigned int random_seed)
{
    Original_PostRunCmd(from, to, cmd, runfuncs, time, random_seed);

    // "to" is the resulting predicted state for this usercmd - punchangle
    // here is what actually drives the camera kick on the next frame(s).
    // Zeroing it every time removes weapon recoil's view-punch entirely;
    // doesn't touch anything server-authoritative (damage, spread, ammo),
    // purely the client-side camera effect.
    if (GameState::norecoil_enabled && to)
    {
        to->client.punchangle[0] = 0.0f;
        to->client.punchangle[1] = 0.0f;
        to->client.punchangle[2] = 0.0f;
    }
}

bool Hook_PostRunCmd()
{
    HMODULE hClient = GetModuleHandleA("client.dll");
    if (!hClient)
    {
        printf("[hk_postruncmd] client.dll not found\n");
        return false;
    }
    void* target = GetProcAddress(hClient, "HUD_PostRunCmd");
    if (!target)
    {
        printf("[hk_postruncmd] export 'HUD_PostRunCmd' not found\n");
        return false;
    }
    return HookManager::Create("PostRunCmd", target, (void*)Hooked_PostRunCmd, (void**)&Original_PostRunCmd);
}
