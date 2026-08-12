#include <windows.h>
#include "core/logging.h"
#include <cstdio>
#include "core/hooking.h"
#include "hooks/hk_createmove.h"
#include "shared/game_state.h"
#include "hooks/hk_playermove.h"
#include "hooks/hk_hud_redraw.h"
#include "hooks/hk_mouse.h"
#include "hooks/hk_postruncmd.h"
#include "game/entity_utils.h"
#include "game/game_interfaces.h"
#include "ui/menu_state.h"


static HMODULE g_hModule = nullptr;

DWORD WINAPI MainThread(LPVOID)
{

    Logging::Init();
    printf("SvenCoop_Enhanced loaded\n");

    if (!HookManager::Init())
    {
        printf("hook init failed, unloading\n");
        Logging::Shutdown();
        FreeLibraryAndExitThread(g_hModule, 0);
        return 0;
    }

    if (!Hook_CreateMove())
    {
        printf("failed to hook CreateMove, unloading\n");
        HookManager::DisableAll();
        Logging::Shutdown();
        FreeLibraryAndExitThread(g_hModule, 0);
        return 0;
    }
    if (!Hook_PlayerMove())
    {
        printf("failed to hook PlayerMove, unloading\n");
        HookManager::DisableAll();
        Logging::Shutdown();
        FreeLibraryAndExitThread(g_hModule, 0);
        return 0;
    }
    if (!Hook_HudRedraw())
    {
        printf("failed to hook HUD_Redraw, unloading\n");
        HookManager::DisableAll();
        Logging::Shutdown();
        FreeLibraryAndExitThread(g_hModule, 0);
        return 0;
    }
    // Not fatal - just means mouse-wheel hitbox cycling won't work; the
    // aimbot itself doesn't depend on this hook.
    if (!Hook_MouseWheel())
        printf("[hk_mouse] wheel hitbox-cycling unavailable, continuing without it\n");

    // Not fatal - just means the no-recoil menu toggle won't do anything;
    // everything else still works without this hook.
    if (!Hook_PostRunCmd())
        printf("[hk_postruncmd] no-recoil unavailable, continuing without it\n");

    HookManager::EnableAll();
    printf("press END to unload, INSERT for menu\n");
    while (true)
    {
        // INSERT now opens/closes the menu (moved bhop's old toggle to
        // ']' below to free up the key, per request).
        static bool menu_key_was_down = false;
        bool menu_key_is_down = (GetAsyncKeyState(VK_INSERT) & 0x8000) != 0;
        if (menu_key_is_down && !menu_key_was_down)
        {
            Menu_Toggle();
            printf("[menu] %s\n", Menu_IsOpen() ? "opened" : "closed");
        }
        menu_key_was_down = menu_key_is_down;

        // ']' : toggle bhop (was INSERT, moved to make room for the menu).
        static bool bhop_key_was_down = false;
        bool bhop_key_is_down = (GetAsyncKeyState(VK_OEM_6) & 0x8000) != 0;
        if (bhop_key_is_down && !bhop_key_was_down)
        {
            GameState::bhop_enabled = !GameState::bhop_enabled;
            printf("[bhop] %s\n", GameState::bhop_enabled ? "enabled" : "disabled");
        }
        bhop_key_was_down = bhop_key_is_down;

        // HOME: toggle whether the aimbot will also lock onto player-slot
        // entities (real players/bots). Off by default - monsters only.
        static bool target_bots_key_was_down = false;
        bool target_bots_key_is_down = (GetAsyncKeyState(VK_HOME) & 0x8000) != 0;
        if (target_bots_key_is_down && !target_bots_key_was_down)
        {
            GameState::aimbot_target_bots = !GameState::aimbot_target_bots;
            printf("[aimbot] target bots/players: %s\n", GameState::aimbot_target_bots ? "enabled" : "disabled (monsters only)");
        }
        target_bots_key_was_down = target_bots_key_is_down;

        // PAGE UP: one-shot dump of nearby entities and how the aimbot
        // classifies them (player/bot, monster, other) to the console.
        static bool dump_key_was_down = false;
        bool dump_key_is_down = (GetAsyncKeyState(VK_PRIOR) & 0x8000) != 0;
        if (dump_key_is_down && !dump_key_was_down)
        {
            cl_entity_t* localPlayer = GetEngineFuncs()->GetLocalPlayer();
            if (localPlayer)
            {
                float* origin = GetEntityOrigin(localPlayer);
                DumpEntityList(origin, 2000.0f);
            }
        }
        dump_key_was_down = dump_key_is_down;

        // DELETE: toggle verbose per-frame aimbot debug logging
        // (rejection reasons). Off by default - noisy.
        static bool debug_key_was_down = false;
        bool debug_key_is_down = (GetAsyncKeyState(VK_DELETE) & 0x8000) != 0;
        if (debug_key_is_down && !debug_key_was_down)
        {
            GameState::aimbot_debug_log = !GameState::aimbot_debug_log;
            printf("[aimbot] debug logging: %s\n", GameState::aimbot_debug_log ? "enabled" : "disabled");
        }
        debug_key_was_down = debug_key_is_down;

        if (GetAsyncKeyState(VK_END) & 1)
            break;
        Sleep(50);
    }

    printf("unloading\n");
    HookManager::DisableAll();
    Logging::Shutdown();
    FreeLibraryAndExitThread(g_hModule, 0);
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        g_hModule = hModule;
        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, MainThread, nullptr, 0, nullptr);
    }
    return TRUE;
}