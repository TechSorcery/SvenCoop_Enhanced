#include "game_state.h"
#include <windows.h>

namespace GameState
{
    bool bhop_enabled = true;
    bool aimbot_enabled = true;
    bool aimbot_target_bots = false;
    bool aimbot_debug_log = false;
    int aimbot_key = VK_XBUTTON1; // mouse4
    float aimbot_fov = 15.0f;
    bool aimbot_hold_mode = true;
    int aimbot_hitbox_index = 3; // Torso

    bool triggerbot_enabled = false;
    bool norecoil_enabled = false;

    bool esp_enabled = false;
    bool esp_show_monsters = true;
    bool esp_show_players = true;
    bool esp_show_other = false;
    bool esp_show_dead = false;
    bool esp_show_box = true;
    bool esp_show_name = true;
    bool esp_show_model = true;
    bool esp_show_distance = true;
}