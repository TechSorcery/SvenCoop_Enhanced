#pragma once
#include "../../game/sdk_structs.h"

// Auto-fires whenever a valid hostile target is under the crosshair, in
// LOS, and alive - gated purely by GameState::triggerbot_enabled (menu
// toggle), no hotkey involved. Called every frame from Hooked_CreateMove,
// same as the aimbot and bhop.
void Triggerbot_OnCreateMove(usercmd_t* cmd);
