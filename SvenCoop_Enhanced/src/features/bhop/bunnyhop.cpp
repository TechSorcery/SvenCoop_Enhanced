#include "bunnyhop.h"
#include "../../shared/game_state.h"
#include "../../hooks/hk_playermove.h"

void Bhop_OnCreateMove(usercmd_t* cmd)
{
    if (!GameState::bhop_enabled)
        return;

    if ((cmd->buttons & IN_JUMP) && !IsOnGround())
        cmd->buttons &= ~IN_JUMP;
}