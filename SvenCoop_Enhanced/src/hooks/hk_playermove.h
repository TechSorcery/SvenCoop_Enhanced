#pragma once
#include "../game/sdk_structs.h"

extern PlayerMove_t Original_PlayerMove;
void __cdecl Hooked_PlayerMove(playermove_t* ppmove, int server);
bool Hook_PlayerMove();
bool IsOnGround();

// Client-side predicted local player origin/eye offset, refreshed every
// frame during prediction (unlike the networked cl_entity origin, which
// only steps forward once per server tick and looks jittery when used
// for aiming while the player is moving).
// Returns false if prediction hasn't run yet.
bool GetPredictedOrigin(float outOrigin[3], float outViewOfs[3]);