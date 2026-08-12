#pragma once

// Rows shown in the menu, in display order. Kept as an enum so
// menu_state.cpp (input handling) and menu_render.cpp (drawing) stay in
// sync on what row index means what.
enum MenuItemId
{
    ITEM_BHOP_ENABLED,
    ITEM_AIMBOT_ENABLED,
    ITEM_TARGET_BOTS,
    ITEM_FOV,
    ITEM_HITBOX,
    ITEM_AIM_MODE,
    ITEM_AIM_KEY,
    ITEM_TRIGGERBOT_ENABLED,
    ITEM_NORECOIL,
    ITEM_DEBUG_LOG,
    ITEM_ESP_ENABLED,
    ITEM_ESP_SHOW_MONSTERS,
    ITEM_ESP_SHOW_PLAYERS,
    ITEM_ESP_SHOW_OTHER,
    ITEM_ESP_SHOW_DEAD,
    ITEM_ESP_SHOW_BOX,
    ITEM_ESP_SHOW_NAME,
    ITEM_ESP_SHOW_MODEL,
    ITEM_ESP_SHOW_DISTANCE,
    ITEM_COUNT
};

bool Menu_IsOpen();

// Opens/closes the menu. Called from the INSERT keybind poll in
// dllmain's main loop.
void Menu_Toggle();

// Polls navigation/value-adjust/key-capture input. Called every frame
// from the HUD_Redraw hook while the menu is open (not from the slower
// background poll loop) so arrow-key navigation feels responsive.
void Menu_Update();

int Menu_GetSelectedIndex();
bool Menu_IsCapturingKey();

// Human-readable name for a virtual-key code, including the mouse
// buttons (which GetKeyNameTextA doesn't handle). Returns a pointer to a
// static buffer - copy it if you need it to outlive the next call.
const char* Menu_KeyName(int vk);
