#include "hk_hud_redraw.h"
#include "../core/hooking.h"
#include "../ui/menu_state.h"
#include "../ui/menu_render.h"
#include "../features/esp/esp.h"
#include <windows.h>
#include <cstdio>

HudRedraw_t Original_HUD_Redraw = nullptr;

int __cdecl Hooked_HUD_Redraw(float time, int intermission)
{
    int result = Original_HUD_Redraw(time, intermission);

    // HUD_Redraw runs every rendered frame, after the 3D scene and the
    // game's own HUD are drawn - the right, idiomatic GoldSrc place to
    // draw a 2D overlay using the engine's own screen-space draw
    // functions (FillRGBA/DrawConsoleString), no raw OpenGL state
    // save/restore needed.

    // ESP is a gameplay overlay, not a menu - runs independently of
    // whether the menu is open.
    ESP_Render();

    if (Menu_IsOpen())
    {
        Menu_Update();
        Menu_Render();
    }

    return result;
}

bool Hook_HudRedraw()
{
    HMODULE hClient = GetModuleHandleA("client.dll");
    if (!hClient)
    {
        printf("[hk_hud_redraw] client.dll not found\n");
        return false;
    }
    void* target = GetProcAddress(hClient, "HUD_Redraw");
    if (!target)
    {
        printf("[hk_hud_redraw] export 'HUD_Redraw' not found\n");
        return false;
    }
    return HookManager::Create("HUD_Redraw", target, (void*)Hooked_HUD_Redraw, (void**)&Original_HUD_Redraw);
}
