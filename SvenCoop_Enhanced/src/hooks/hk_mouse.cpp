#include "hk_mouse.h"
#include "../shared/game_state.h"
#include "../game/entity_utils.h"
#include <windows.h>
#include <cstdio>

static WNDPROC s_OriginalWndProc = nullptr;

// Wheel is only consumed as a hitbox-cycle input while the aim key is
// physically held - otherwise it's left alone so it still cycles weapons
// like normal Half-Life behavior. Windows delivers WM_MOUSEWHEEL delta as
// a signed 16-bit value in the high word of wParam (WHEEL_DELTA = 120 per
// notch).
static LRESULT CALLBACK Hooked_WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_MOUSEWHEEL)
    {
        bool aimKeyHeld = (GetAsyncKeyState(GameState::aimbot_key) & 0x8000) != 0;
        if (aimKeyHeld)
        {
            short delta = (short)HIWORD(wParam);
            int dir = (delta > 0) ? 1 : -1;

            int idx = GameState::aimbot_hitbox_index + dir;
            if (idx < 0) idx = HITBOX_COUNT - 1;
            if (idx >= HITBOX_COUNT) idx = 0;
            GameState::aimbot_hitbox_index = idx;

            printf("[aimbot] aim point: %s\n", GetHitboxName(idx));

            // Consumed - don't forward to the game (would otherwise also
            // switch weapons while the player is trying to pick a hitbox).
            return 0;
        }
    }

    return CallWindowProcA(s_OriginalWndProc, hwnd, msg, wParam, lParam);
}

bool Hook_MouseWheel()
{
    // "Valve001" is GoldSrc's documented window class name.
    HWND hwnd = FindWindowA("Valve001", nullptr);
    if (!hwnd)
    {
        printf("[hk_mouse] Valve001 window not found\n");
        return false;
    }

    SetLastError(0);
    LONG_PTR prev = SetWindowLongPtrA(hwnd, GWLP_WNDPROC, (LONG_PTR)Hooked_WndProc);
    if (!prev)
    {
        printf("[hk_mouse] SetWindowLongPtrA failed (err=%lu)\n", GetLastError());
        return false;
    }

    s_OriginalWndProc = (WNDPROC)prev;
    return true;
}
