#include "triggerbot.h"
#include "../../shared/game_state.h"
#include "../../game/game_interfaces.h"
#include "../../game/entity_utils.h"
#include <windows.h>

static constexpr float TRIGGERBOT_RANGE = 2000.0f;

// Stock HL's crowbar traces its swing 32 units from the eye; Sven Co-op's
// various melee weapons are all in the same ballpark. This is a bit more
// generous than that to account for reach differing slightly between
// custom melee weapons - if it's firing before the swing would actually
// land (or not firing when it should), tell me and this can be tuned.
static constexpr float TRIGGERBOT_MELEE_RANGE = 48.0f;

// Tight cone approximating "directly under the crosshair" - reuses the
// same acquisition scan the aimbot uses (FindBestAimTarget: angle + LOS,
// both already validated there), just with a much smaller FOV instead of a
// new raycast-based check. A literal center-screen raycast would need to
// trust PM_TraceLine's returned hit-entity index lining up with the real
// network entity index, which - same caveat as the ignore_pe notes
// elsewhere in this codebase - isn't guaranteed, so this sticks to the
// already-proven approach instead.
static constexpr float TRIGGERBOT_FOV = 2.0f;

void Triggerbot_OnCreateMove(usercmd_t* cmd)
{
    if (!GameState::triggerbot_enabled)
        return;

    // Follows the aimbot's own key/mode instead of running unconditionally:
    // in Hold mode there's an obvious "am I actively aiming right now"
    // signal (the key), so only fire while it's held, same as the aimbot
    // itself only aims while held. In Toggle mode there's no held state to
    // key off - the aimbot's toggle is a separate on/off - so the
    // triggerbot just runs continuously whenever its own menu toggle is on.
    if (GameState::aimbot_hold_mode)
    {
        bool keyDown = (GetAsyncKeyState(GameState::aimbot_key) & 0x8000) != 0;
        if (!keyDown)
            return;
    }

    cl_entity_t* localPlayer = GetEngineFuncs()->GetLocalPlayer();
    if (!localPlayer)
        return;

    float eyeOrigin[3];
    GetStableEyeOrigin(localPlayer, eyeOrigin);

    // Firing full-auto at a target you can't actually reach with a melee
    // weapon just wastes the swing (and telegraphs you're not actually
    // aiming) - cap the range to the weapon's reach instead of the normal
    // engagement distance whenever a melee weapon is equipped.
    float range = IsLocalPlayerUsingMelee() ? TRIGGERBOT_MELEE_RANGE : TRIGGERBOT_RANGE;

    // Shares the aimbot's target_bots setting - if the aimbot is set to
    // ignore player-slot entities, the triggerbot does too, and vice
    // versa, rather than adding a second, easy-to-forget-about toggle for
    // the same underlying question ("should this ever act on players?").
    cl_entity_t* target = FindBestAimTarget(eyeOrigin, TRIGGERBOT_FOV, range, GameState::aimbot_target_bots);
    if (!target)
        return;

    cmd->buttons |= IN_ATTACK;
}
