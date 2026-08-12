#include "menu_state.h"
#include "../shared/game_state.h"
#include "../game/entity_utils.h"
#include <windows.h>
#include <cstdio>

static bool s_MenuOpen = false;
static int s_SelectedIndex = 0;
static bool s_CapturingKey = false;

// Separate edge-tracking arrays for menu navigation vs. key-capture mode,
// so they don't interfere with each other.
static bool s_NavWasDown[256] = {};
static bool s_CaptureWasDown[256] = {};

bool Menu_IsOpen()
{
    return s_MenuOpen;
}

void Menu_Toggle()
{
    s_MenuOpen = !s_MenuOpen;
    s_CapturingKey = false;
}

int Menu_GetSelectedIndex()
{
    return s_SelectedIndex;
}

bool Menu_IsCapturingKey()
{
    return s_CapturingKey;
}

static bool NavKeyPressedEdge(int vk)
{
    bool isDown = (GetAsyncKeyState(vk) & 0x8000) != 0;
    bool pressed = isDown && !s_NavWasDown[vk];
    s_NavWasDown[vk] = isDown;
    return pressed;
}

// Prime the capture-tracking table with whatever's currently held (most
// importantly VK_RETURN, since that's what triggers capture mode) so
// that same keypress doesn't immediately get captured as the new bind.
static void BeginKeyCapture()
{
    s_CapturingKey = true;
    for (int vk = 0; vk < 256; vk++)
        s_CaptureWasDown[vk] = (GetAsyncKeyState(vk) & 0x8000) != 0;
}

// Returns the first newly-pressed virtual-key code, or 0 if none.
static int PollKeyCapture()
{
    for (int vk = 1; vk < 254; vk++)
    {
        bool isDown = (GetAsyncKeyState(vk) & 0x8000) != 0;
        bool pressed = isDown && !s_CaptureWasDown[vk];
        s_CaptureWasDown[vk] = isDown;
        if (pressed)
            return vk;
    }
    return 0;
}

void Menu_Update()
{
    if (s_CapturingKey)
    {
        int vk = PollKeyCapture();
        if (vk != 0)
        {
            if (vk != VK_ESCAPE) // ESC cancels without changing the bind
                GameState::aimbot_key = vk;
            s_CapturingKey = false;
        }
        return;
    }

    if (NavKeyPressedEdge(VK_UP))
        s_SelectedIndex = (s_SelectedIndex - 1 + ITEM_COUNT) % ITEM_COUNT;
    if (NavKeyPressedEdge(VK_DOWN))
        s_SelectedIndex = (s_SelectedIndex + 1) % ITEM_COUNT;

    bool leftHeld = (GetAsyncKeyState(VK_LEFT) & 0x8000) != 0;
    bool rightHeld = (GetAsyncKeyState(VK_RIGHT) & 0x8000) != 0;
    bool leftEdge = NavKeyPressedEdge(VK_LEFT);
    bool rightEdge = NavKeyPressedEdge(VK_RIGHT);

    switch (static_cast<MenuItemId>(s_SelectedIndex))
    {
    case ITEM_BHOP_ENABLED:
        if (leftEdge || rightEdge)
            GameState::bhop_enabled = !GameState::bhop_enabled;
        break;

    case ITEM_AIMBOT_ENABLED:
        if (leftEdge || rightEdge)
            GameState::aimbot_enabled = !GameState::aimbot_enabled;
        break;

    case ITEM_TARGET_BOTS:
        if (leftEdge || rightEdge)
            GameState::aimbot_target_bots = !GameState::aimbot_target_bots;
        break;

    case ITEM_FOV:
        // Continuous while held (not edge-triggered) for a smoother feel.
        if (rightHeld) GameState::aimbot_fov += 0.25f;
        if (leftHeld)  GameState::aimbot_fov -= 0.25f;
        if (GameState::aimbot_fov < 1.0f)   GameState::aimbot_fov = 1.0f;
        // 360 rather than 180: the angle between two vectors physically
        // maxes out at 180 anyway, so anything >= 180 already means
        // "unrestricted" - the extra headroom just makes that state
        // reachable/visible on the FOV slider instead of looking capped.
        if (GameState::aimbot_fov > 360.0f) GameState::aimbot_fov = 360.0f;
        break;

    case ITEM_HITBOX:
        if (leftEdge)
            GameState::aimbot_hitbox_index = (GameState::aimbot_hitbox_index - 1 + HITBOX_COUNT) % HITBOX_COUNT;
        if (rightEdge)
            GameState::aimbot_hitbox_index = (GameState::aimbot_hitbox_index + 1) % HITBOX_COUNT;
        break;

    case ITEM_AIM_MODE:
        if (leftEdge || rightEdge)
            GameState::aimbot_hold_mode = !GameState::aimbot_hold_mode;
        break;

    case ITEM_AIM_KEY:
        if (NavKeyPressedEdge(VK_RETURN))
            BeginKeyCapture();
        break;

    case ITEM_TRIGGERBOT_ENABLED:
        if (leftEdge || rightEdge)
            GameState::triggerbot_enabled = !GameState::triggerbot_enabled;
        break;

    case ITEM_NORECOIL:
        if (leftEdge || rightEdge)
            GameState::norecoil_enabled = !GameState::norecoil_enabled;
        break;

    case ITEM_DEBUG_LOG:
        if (leftEdge || rightEdge)
            GameState::aimbot_debug_log = !GameState::aimbot_debug_log;
        break;

    case ITEM_ESP_ENABLED:
        if (leftEdge || rightEdge)
            GameState::esp_enabled = !GameState::esp_enabled;
        break;

    case ITEM_ESP_SHOW_MONSTERS:
        if (leftEdge || rightEdge)
            GameState::esp_show_monsters = !GameState::esp_show_monsters;
        break;

    case ITEM_ESP_SHOW_PLAYERS:
        if (leftEdge || rightEdge)
            GameState::esp_show_players = !GameState::esp_show_players;
        break;

    case ITEM_ESP_SHOW_OTHER:
        if (leftEdge || rightEdge)
            GameState::esp_show_other = !GameState::esp_show_other;
        break;

    case ITEM_ESP_SHOW_DEAD:
        if (leftEdge || rightEdge)
            GameState::esp_show_dead = !GameState::esp_show_dead;
        break;

    case ITEM_ESP_SHOW_BOX:
        if (leftEdge || rightEdge)
            GameState::esp_show_box = !GameState::esp_show_box;
        break;

    case ITEM_ESP_SHOW_NAME:
        if (leftEdge || rightEdge)
            GameState::esp_show_name = !GameState::esp_show_name;
        break;

    case ITEM_ESP_SHOW_MODEL:
        if (leftEdge || rightEdge)
            GameState::esp_show_model = !GameState::esp_show_model;
        break;

    case ITEM_ESP_SHOW_DISTANCE:
        if (leftEdge || rightEdge)
            GameState::esp_show_distance = !GameState::esp_show_distance;
        break;

    default:
        break;
    }
}

const char* Menu_KeyName(int vk)
{
    static char buf[32];

    switch (vk)
    {
    case VK_LBUTTON:  return "MOUSE1";
    case VK_RBUTTON:  return "MOUSE2";
    case VK_MBUTTON:  return "MOUSE3";
    case VK_XBUTTON1: return "MOUSE4";
    case VK_XBUTTON2: return "MOUSE5";
    }

    UINT scanCode = MapVirtualKeyA((UINT)vk, MAPVK_VK_TO_VSC);
    LONG lParam = (LONG)(scanCode << 16);

    switch (vk)
    {
    case VK_LEFT: case VK_UP: case VK_RIGHT: case VK_DOWN:
    case VK_PRIOR: case VK_NEXT: case VK_END: case VK_HOME:
    case VK_INSERT: case VK_DELETE: case VK_DIVIDE: case VK_NUMLOCK:
        lParam |= (1 << 24); // extended-key bit
        break;
    }

    if (scanCode != 0 && GetKeyNameTextA(lParam, buf, sizeof(buf)) > 0)
        return buf;

    sprintf_s(buf, sizeof(buf), "VK_0x%02X", vk);
    return buf;
}
