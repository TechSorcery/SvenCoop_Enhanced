#include "aimbot.h"
#include "../../shared/game_state.h"
#include "../../game/game_interfaces.h"
#include "../../game/entity_utils.h"
#include <windows.h>
#include <cmath>
#include <cstdio>

static constexpr float AIMBOT_RANGE = 2000.0f;
static constexpr float AIMBOT_STRENGTH = .8f;

static float NormalizeAngleDiff(float diff)
{
    while (diff > 180.0f)  diff -= 360.0f;
    while (diff < -180.0f) diff += 360.0f;
    return diff;
}

// Hold mode: aim only while the key is physically down (default).
// Toggle mode: press once to start aiming, press again to stop.
static bool IsAimbotActive()
{
    bool keyDown = (GetAsyncKeyState(GameState::aimbot_key) & 0x8000) != 0;

    if (GameState::aimbot_hold_mode)
        return keyDown;

    static bool wasDown = false;
    static bool toggledOn = false;
    if (keyDown && !wasDown)
        toggledOn = !toggledOn;
    wasDown = keyDown;
    return toggledOn;
}

// The target picked at the start of the current hold, kept for the rest of
// it (hold mode only - see s_LockedTarget usage below). Cleared whenever
// the aimbot goes inactive so the next press picks fresh.
static cl_entity_t* s_LockedTarget = nullptr;

// Consecutive frames the "alive + visible" checks below have failed for
// the current lock. A single bad frame - a monster's movetype transiently
// leaving STEP/FLY during a scripted reaction, or a one-frame LOS trace
// false-block from something briefly crossing the ray - shouldn't drop a
// lock on a target you can plainly see and that's still alive; that was
// causing the lock to flicker/reacquire constantly. Reset to 0 any frame
// both checks pass, and whenever a fresh lock is acquired.
static int s_TargetMissStreak = 0;

// A real death (movetype leaves STEP/FLY and stays gone - see the death
// note below) or a real, sustained LOS loss both still clear this small
// tolerance within a handful of frames - nowhere near perceptible, and
// nowhere near the several-second corpse-settle delay the old solid-only
// death check had.
static constexpr int TARGET_MISS_TOLERANCE = 6;

// Deliberately does NOT re-check FOV - a lock is supposed to tolerate the
// crosshair drifting off the target, same as a "target lock" in other
// games. It DOES re-check LOS and death every frame though (debounced via
// s_TargetMissStreak above) - losing sight of the target or it dying
// should drop the lock and fall through to the re-acquire path in
// Aimbot_OnCreateMove, which picks up whatever's nearest the crosshair
// next, still within the same held press.
static bool IsLockedTargetStillValid(cl_entity_t* target, cl_entity_t* localPlayer, const float* eyeOrigin)
{
    if (!target)
        return false;

    // Live per-frame trace of the locked target's raw state, printed
    // before any early-return so the death transition itself always shows
    // up in the log instead of just the frame before it. Meant to be
    // watched (or scrolled back through afterward) while killing whatever
    // you're locked onto, to see exactly which fields change and when -
    // this is what to grab if a monster is still lingering on ESP/aimbot
    // after dying so it can be diagnosed instead of guessed at further.
    if (GameState::aimbot_debug_log)
    {
        int myMsgNum = GetEntityMessageNum(localPlayer);
        printf("[aimbot] lock-trace idx=%d solid=%d mtype=%d seq=%d frame=%.1f animtime=%.2f msgdiff=%d streak=%d\n",
            target->index, target->curstate.solid, target->curstate.movetype,
            target->curstate.sequence, target->curstate.frame, target->curstate.animtime,
            myMsgNum - GetEntityMessageNum(target), s_TargetMissStreak);
    }

    if (IsEntityDead(target))
        return false;

    int myMsgNum = GetEntityMessageNum(localPlayer);
    if (GetEntityMessageNum(target) + 10 < myMsgNum)
        return false;

    float* targetOrigin = GetEntityOrigin(target);
    float dx = targetOrigin[0] - eyeOrigin[0];
    float dy = targetOrigin[1] - eyeOrigin[1];
    float dz = targetOrigin[2] - eyeOrigin[2];
    float dist = sqrtf(dx * dx + dy * dy + dz * dz);
    if (dist > AIMBOT_RANGE)
        return false;

    // IsEntityDead (curstate.solid == SOLID_NOT) only flips once the corpse
    // finishes its death animation and settles - a few seconds after it
    // actually died. Monster AI movement (MOVETYPE_STEP/FLY) drops the
    // instant the monster dies instead (Killed() switches it to a physics
    // movetype for the death-fall), so this catches it far sooner. Only
    // applies to monster/ally targets - players/bots use MOVETYPE_WALK
    // while alive and would false-trigger this.
    bool aliveMovement = IsPlayerEntity(target)
        || target->curstate.movetype == MOVETYPE_STEP
        || target->curstate.movetype == MOVETYPE_FLY;

    // Same close-range skip as the acquisition scan (see FindBestAimTarget)
    // so this doesn't fight the close-range fix from earlier.
    bool hasLos = true;
    constexpr float LOS_SKIP_DISTANCE = 150.0f;
    if (dist > LOS_SKIP_DISTANCE)
    {
        float aimHeight = GetAimHeightOffset(target);
        float targetPoint[3] = { targetOrigin[0], targetOrigin[1], targetOrigin[2] + aimHeight };
        hasLos = HasLineOfSight((float*)eyeOrigin, targetPoint, target->index);
    }

    if (aliveMovement && hasLos)
    {
        s_TargetMissStreak = 0;
        return true;
    }

    s_TargetMissStreak++;
    return s_TargetMissStreak < TARGET_MISS_TOLERANCE;
}

void Aimbot_OnCreateMove(usercmd_t* cmd)
{
    if (!GameState::aimbot_enabled)
        return;

    if (!IsAimbotActive())
    {
        s_LockedTarget = nullptr; // next press starts a fresh lock
        s_TargetMissStreak = 0;
        return;
    }

    cl_entity_t* localPlayer = GetEngineFuncs()->GetLocalPlayer();
    if (!localPlayer)
        return;

    float eyeOrigin[3];
    GetStableEyeOrigin(localPlayer, eyeOrigin);

    cl_entity_t* target = nullptr;

    if (GameState::aimbot_hold_mode && IsLockedTargetStillValid(s_LockedTarget, localPlayer, eyeOrigin))
    {
        // Stay on whatever was acquired at the start of this hold, even if
        // something else is now closer to the crosshair.
        target = s_LockedTarget;
    }
    else
    {
        // Fresh press (or toggle mode, which always re-picks), or the
        // previous lock stopped being valid - (re)acquire nearest to
        // crosshair. In hold mode this also naturally retries every frame
        // if nothing was found yet, so the lock still engages as soon as
        // something comes into view while the key stays held.
        target = FindBestAimTarget(eyeOrigin, GameState::aimbot_fov, AIMBOT_RANGE, GameState::aimbot_target_bots);
        s_LockedTarget = target;
        s_TargetMissStreak = 0;
    }

    if (!target)
        return;

    float* targetOrigin = GetEntityOrigin(target);
    float aimHeight = GetAimHeightOffset(target);

    float dx = targetOrigin[0] - eyeOrigin[0];
    float dy = targetOrigin[1] - eyeOrigin[1];
    float dz = (targetOrigin[2] + aimHeight) - eyeOrigin[2];

    float dist2D = sqrtf(dx * dx + dy * dy);

    float desiredPitch = -atan2f(dz, dist2D) * 57.2957795f;
    float desiredYaw = atan2f(dy, dx) * 57.2957795f;

    float currentPitch = cmd->viewangles[0];
    float currentYaw = cmd->viewangles[1];

    float pitchDiff = NormalizeAngleDiff(desiredPitch - currentPitch);
    float yawDiff = NormalizeAngleDiff(desiredYaw - currentYaw);

    float newPitch = currentPitch + pitchDiff * AIMBOT_STRENGTH;
    float newYaw = currentYaw + yawDiff * AIMBOT_STRENGTH;

    // keep yaw in a sane range after adding
    while (newYaw > 180.0f)  newYaw -= 360.0f;
    while (newYaw < -180.0f) newYaw += 360.0f;

    // Pitch wasn't clamped at all before - standard engine view-pitch limit
    // is +/-89 (not +/-90, since exactly straight up/down is a gimbal
    // singularity for yaw). Close-range targets in particular can produce
    // a desiredPitch that swings toward vertical since dist2D shrinks
    // faster than height difference does.
    if (newPitch > 89.0f)  newPitch = 89.0f;
    if (newPitch < -89.0f) newPitch = -89.0f;

    cmd->viewangles[0] = newPitch;
    cmd->viewangles[1] = newYaw;

    GetEngineFuncs()->SetViewAngles(cmd->viewangles);
}