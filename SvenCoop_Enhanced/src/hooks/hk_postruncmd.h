#pragma once
#include "../game/sdk_structs.h"

extern PostRunCmd_t Original_PostRunCmd;
void __cdecl Hooked_PostRunCmd(local_state_t* from, local_state_t* to, usercmd_t* cmd, int runfuncs, double time, unsigned int random_seed);
bool Hook_PostRunCmd();
