#pragma once

namespace GameState
{
    extern bool bhop_enabled;
    extern bool aimbot_enabled;

    // When false (default), the aimbot only locks onto monsters/NPCs and
    // ignores player-slot entities (real players and bots). When true, it
    // will also consider player-slot entities as valid targets.
    extern bool aimbot_target_bots;

    // Verbose per-entity accept/reject logging in the target scan. Off by
    // default - with the full entity range scanned every frame this would
    // otherwise spam the console and hurt performance.
    extern bool aimbot_debug_log;

    // Virtual-key code (or VK_XBUTTON1/2 for mouse4/5) that triggers the
    // aimbot. Default is mouse4 (VK_XBUTTON1). Rebindable via the menu.
    extern int aimbot_key;

    // Field of view cone (degrees) the aimbot will consider targets
    // within, centered on the crosshair. Adjustable via the menu.
    extern float aimbot_fov;

    // true = must hold aimbot_key down to aim (default). false = press
    // once to toggle aiming on, press again to toggle off.
    extern bool aimbot_hold_mode;

    // Index into the aim-point list (Feet/Legs/Groin/Torso/Chest/Neck/Head
    // - see GetHitboxName in entity_utils.h). Cycled with the mouse wheel
    // while aimbot_key is held (see hk_mouse.cpp), or with left/right on
    // the menu's "Aim Point" row. Default 3 = Torso.
    extern int aimbot_hitbox_index;

    // Auto-fires (holds down IN_ATTACK) whenever a valid hostile target is
    // under the crosshair, in LOS, and alive - see features/triggerbot.
    // Independent of the aimbot's key/hold state, purely a menu toggle.
    extern bool triggerbot_enabled;

    // Zeroes the client's view-punch (weapon recoil kick) every frame via
    // the HUD_PostRunCmd hook - see hooks/hk_postruncmd.cpp.
    extern bool norecoil_enabled;

    // ESP overlay (see features/esp/esp.cpp) - boxes + debug text drawn
    // over entities in range every frame, independent of the aimbot.
    extern bool esp_enabled;
    extern bool esp_show_monsters; // includes allies too (drawn green instead of red)
    extern bool esp_show_players;  // real players + bots
    extern bool esp_show_other;    // everything else (props/weapons/decor) - very noisy, off by default
    extern bool esp_show_dead;     // corpses - off by default, mostly clutter
    extern bool esp_show_box;
    extern bool esp_show_name;     // type + entity index, e.g. "MONSTER #158"
    extern bool esp_show_model;
    extern bool esp_show_distance;
}