#pragma once
#include "../game/sdk_structs.h"

extern CreateMove_t Original_CreateMove;
void __cdecl Hooked_CreateMove(float frametime, usercmd_t* cmd, int active);
bool Hook_CreateMove();