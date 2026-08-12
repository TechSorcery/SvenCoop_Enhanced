#include "entity_utils.h"
#include "game_interfaces.h"
#include "../shared/game_state.h"
#include "../hooks/hk_playermove.h"
#include <cmath>
#include <cstdio>
#include <cstring>

static constexpr float DEFAULT_EYE_HEIGHT = 28.0f;

bool IsPlayerEntity(cl_entity_t* ent)
{
    return ent->player != 0;
}

// Vanilla Half-Life friendly NPCs that don't go through Sven Co-op's
// "_ally" reskin convention at all - a mapper can place these directly and
// they're friendly by default in the base game. Without this they fall
// through to the movetype fallback below (real AI movement, same as any
// hostile monster) and get misclassified as hostile.
static const char* kFriendlyNpcModels[] = {
    "scientist", "barney",
};

// Sven Co-op appends "_ally" to the model filename for its cooperative
// NPCs that fight alongside players (the "friendly AI" - e.g.
// "models/hgrunt_ally.mdl") - a reliable general signal regardless of
// which specific monster type the ally is based on.
static bool IsAllyModelName(const char* name)
{
    if (!name)
        return false;
    if (strstr(name, "_ally") != nullptr)
        return true;
    for (const char* m : kFriendlyNpcModels)
    {
        if (strstr(name, m))
            return true;
    }
    return false;
}

// Known hostile Sven Co-op / Half-Life monster model filenames. Matched
// as substrings, so path/extension don't matter. Not guaranteed
// exhaustive - check DumpEntityList's model column against what's
// actually on screen and report anything misclassified so this list can
// be corrected.
static const char* kHostileMonsterModels[] = {
    "headcrab", "zombie", "hgrunt", "agrunt", "alien_slave", "islave",
    "bullsquid", "houndeye", "gonome", "babygarg", "gargantua", "garg",
    "apache", "osprey", "controller", "hassassin", "human_assassin",
    "barnacle", "bigmomma", "nihilanth", "tentacle", "pitdrone",
    "shocktrooper", "voltigore", "alien_controller", "alien_grunt",
    "geneworm", "blkopscommando", "shockroach", "abomination",
    // Generic human-enemy naming used by custom content packs (e.g. this
    // Russian-themed pack's "dgf_rsoldier.mdl") - "soldier" is broad but
    // safe, no scenery/prop model names in this codebase's dumps contain it.
    "soldier",
};

static bool IsHostileModelName(const char* name)
{
    // model_t::name starts with "models/" for real studiomodels, "*N" for
    // inline brush models, "sprites/" for sprites - only 'm' paths are
    // worth checking.
    if (!name || name[0] != 'm')
        return false;
    if (IsAllyModelName(name))
        return false;

    // Custom content packs (this server runs one - "models/scmod/...")
    // conventionally organize their monster models under an "npcs/" or
    // "monsters/" folder, e.g. "models/scmod/npcs/rgrunt/turrican.mdl".
    // This generalizes to whatever custom monsters a pack ships without
    // needing to know each one's specific filename.
    if (strstr(name, "/npcs/") || strstr(name, "/monsters/"))
        return true;

    // Real players/bots always use the flat "models/player.mdl". Some
    // custom Sven Co-op content instead builds human NPCs on the vanilla
    // player-skin folder convention, e.g. "models/player/boris/boris.mdl"
    // (the same format used for player skin selection) - since this
    // function is only ever reached after IsPlayerEntity has already
    // returned false, anything under "models/player/" here can only be a
    // server-spawned NPC, never an actual networked player.
    if (strncmp(name, "models/player/", 14) == 0)
        return true;

    for (const char* m : kHostileMonsterModels)
    {
        if (strstr(name, m))
            return true;
    }
    return false;
}

// Movetype-based detection: CBaseMonster-derived NPCs move themselves via
// MOVETYPE_STEP (ground) or MOVETYPE_FLY (airborne), no matter what custom
// model a content pack gives them - this generalizes to monster models
// this codebase has never seen without needing a name added anywhere.
// Doesn't distinguish hostile from friendly by itself (a mod's ally NPCs
// move the same way), so the "_ally" name check still runs first wherever
// this is used.
//
// MOVETYPE_FLY/STEP alone isn't sufficient though - some non-combat
// scripted/decorative props (e.g. custom-map cosmetic "walking" mascots)
// also use those movetypes for their own scripted motion, which was
// getting picked up as a false positive. Real CBaseMonster NPCs (and
// players) are solid = SOLID_SLIDEBOX; the false positives seen so far
// were SOLID_TRIGGER instead, so gate on that too.
static bool IsAiMovetype(cl_entity_t* ent)
{
    int mt = ent->curstate.movetype;
    if (mt != MOVETYPE_STEP && mt != MOVETYPE_FLY)
        return false;
    return ent->curstate.solid == SOLID_SLIDEBOX;
}

// Non-living scenery/machinery that still manages to pass the movetype+
// solid check above - e.g. a scripted rotating "engine" prop built on a
// CBaseMonster-derived class purely so a mapper could animate/script it,
// with no AI or hostility involved at all. GoldSrc doesn't network
// anything to the client that could tell "real monster" from "monster-
// class scenery" more reliably than this, so - same as the hostile list -
// this is a blacklist that grows as false positives get reported.
static const char* kKnownSceneryModels[] = {
    "rengine", "sat_globe",
};

static bool IsKnownSceneryModel(const char* name)
{
    if (!name)
        return false;
    for (const char* m : kKnownSceneryModels)
    {
        if (strstr(name, m))
            return true;
    }
    return false;
}

bool IsMonsterEntity(cl_entity_t* ent)
{
    if (IsPlayerEntity(ent))
        return false;

    const char* name = GetEntityModelName(ent);
    if (IsAllyModelName(name))
        return false;
    if (IsHostileModelName(name))
        return true;
    if (IsKnownSceneryModel(name))
        return false;
    return IsAiMovetype(ent);
}

bool IsAllyEntity(cl_entity_t* ent)
{
    if (IsPlayerEntity(ent))
        return false;
    return IsAllyModelName(GetEntityModelName(ent));
}

// Specific vanilla HL/Sven Co-op models confirmed to settle at SOLID_BBOX
// instead of ever going SOLID_NOT when killed - their corpses stay solid
// enough to keep blocking movement rather than becoming a non-solid prop.
// Confirmed against weedgirl68/sevin (an unrelated reference cheat) whose
// class_table.cpp hardcodes this exact same model set for this exact same
// reason (IsEntityClassDeadbody: solid==SOLID_BBOX && a per-model
// "dead body" flag on scientist/barney/otis/hgrunt and their reskins/
// variants). The movetype fallback further down already catches these
// generically (their AI movetype still leaves STEP/FLY on death), so this
// isn't adding coverage - it's a faster, independently-verified path that
// doesn't depend on movetype actually transitioning for these specific
// models. Matched as substrings same as the other model lists, so it also
// covers reskins (e.g. "cleansuit_scientist") without listing each one.
static const char* kBboxDeadBodyModels[] = {
    "scientist", "barney", "otis", "hgrunt",
};

static bool IsKnownBboxDeadBodyModel(const char* name)
{
    if (!name)
        return false;
    for (const char* m : kBboxDeadBodyModels)
    {
        if (strstr(name, m))
            return true;
    }
    return false;
}

bool IsEntityDead(cl_entity_t* ent)
{
    if (ent->curstate.solid == SOLID_NOT)
        return true;

    if (IsPlayerEntity(ent))
        return false;

    if (ent->curstate.solid == SOLID_BBOX && IsKnownBboxDeadBodyModel(GetEntityModelName(ent)))
        return true;

    // Fallback for corpses that don't go fully non-solid (see the header
    // comment) - only meaningful for entities actually classified as
    // monster/ally in the first place; a static prop was never "alive" and
    // its movetype means something else entirely. Name-classified entities
    // (scientist, hgrunt, etc) keep classifying correctly after death since
    // that check doesn't depend on movetype, so this still reaches them.
    if (!IsMonsterEntity(ent) && !IsAllyEntity(ent))
        return false;

    int mt = ent->curstate.movetype;
    if (mt != MOVETYPE_STEP && mt != MOVETYPE_FLY)
        return true;

    // Confirmed via DumpEntityList: some monster packs (this "aomdc" pack's
    // twitchers) don't network any corpse state at all when killed - solid
    // and movetype just freeze at their last-alive values because the
    // server stops sending updates for the entity the instant it dies,
    // instead of ever converting it to a corpse. A tight "hasn't been
    // updated in the last couple of ticks" check catches that: a genuinely
    // still-alive, in-PVS AI entity gets networked essentially every tick
    // (packet-entities includes everything in the PVS every frame, just
    // with per-field delta compression - not a per-entity skip), so
    // falling behind even this small a margin is itself a strong "this
    // isn't actually here anymore" signal. Deliberately much tighter than
    // the +10 message margin the scan/dump functions use for their own
    // pre-filtering (generic PVS-exit/network-hiccup tolerance, kept as is
    // - dropping a brand-new target candidate too eagerly is a worse
    // tradeoff than dropping a lock/box on something that just died).
    cl_entity_t* localPlayer = GetEngineFuncs()->GetLocalPlayer();
    if (localPlayer)
    {
        // Tightened from 2 - the 2-tick version cut the delay from ~2-3s
        // down to ~1-2s (confirms the theory), but didn't eliminate it.
        constexpr int STALE_TOLERANCE = 1;
        if (GetEntityMessageNum(ent) + STALE_TOLERANCE < GetEntityMessageNum(localPlayer))
            return true;
    }

    return false;
}

bool GetStableEyeOrigin(cl_entity_t* localPlayer, float outEye[3])
{
    float predictedOrigin[3], predictedViewOfs[3];
    if (GetPredictedOrigin(predictedOrigin, predictedViewOfs))
    {
        outEye[0] = predictedOrigin[0];
        outEye[1] = predictedOrigin[1];
        outEye[2] = predictedOrigin[2] + predictedViewOfs[2];
        return true;
    }

    float* myOrigin = GetEntityOrigin(localPlayer);
    outEye[0] = myOrigin[0];
    outEye[1] = myOrigin[1];
    outEye[2] = myOrigin[2] + DEFAULT_EYE_HEIGHT;
    return false;
}

static const char* kHitboxNames[HITBOX_COUNT] = {
    "Feet", "Legs", "Groin", "Torso", "Chest", "Neck", "Head",
};

static const float kHitboxFractions[HITBOX_COUNT] = {
    0.04f, 0.18f, 0.32f, 0.50f, 0.66f, 0.84f, 0.94f,
};

const char* GetHitboxName(int index)
{
    if (index < 0 || index >= HITBOX_COUNT)
        return "?";
    return kHitboxNames[index];
}

float GetAimHeightOffset(cl_entity_t* ent)
{
    int index = GameState::aimbot_hitbox_index;
    if (index < 0 || index >= HITBOX_COUNT)
        index = 0;

    float* mins = ent->curstate.mins;
    float* maxs = ent->curstate.maxs;
    float bboxHeight = maxs[2] - mins[2];

    // mins/maxs isn't guaranteed to be reliably networked for every
    // entity type in this build (same caution as entity_state_t.health) -
    // if it looks degenerate, fall back to the old fixed offset instead
    // of aiming at the entity's origin/feet.
    if (bboxHeight < 10.0f)
        return 20.0f;

    return mins[2] + bboxHeight * kHitboxFractions[index];
}

// Matched as substrings against the viewmodel path, same convention as the
// monster/scenery lists above. "knife" covers most custom Sven Co-op melee
// weapons, which commonly reuse or riff on that name even when reskinned.
static const char* kMeleeWeaponModels[] = {
    "crowbar", "knife", "machete", "melee", "bat", "wrench", "axe",
    "sword", "claw", "baton", "pipe",
};

bool IsLocalPlayerUsingMelee()
{
    if (!GetEngineFuncs()->GetViewModel)
        return false;

    cl_entity_t* viewModel = GetEngineFuncs()->GetViewModel();
    if (!viewModel)
        return false;

    const char* name = GetEntityModelName(viewModel);
    if (!name || name[0] != 'm') // "models/..." - see IsHostileModelName
        return false;

    for (const char* m : kMeleeWeaponModels)
    {
        if (strstr(name, m))
            return true;
    }
    return false;
}

cl_entity_t* FindNearestEntity(float* myOrigin, float maxDistance)
{
    cl_entity_t* localPlayer = GetEngineFuncs()->GetLocalPlayer();
    if (!localPlayer)
        return nullptr;

    int myMsgNum = GetEntityMessageNum(localPlayer);

    cl_entity_t* best = nullptr;
    float bestDist = maxDistance;

    for (int i = 1; i < MAX_AIM_ENTITIES; i++)
    {
        cl_entity_t* ent = GetEngineFuncs()->GetEntityByIndex(i);
        if (!ent) continue;
        if (ent == localPlayer) continue;
        if (!GetEntityModel(ent)) continue;
        if (GetEntityMessageNum(ent) + 10 < myMsgNum) continue;

        float* entOrigin = GetEntityOrigin(ent);
        float dx = entOrigin[0] - myOrigin[0];
        float dy = entOrigin[1] - myOrigin[1];
        float dz = entOrigin[2] - myOrigin[2];
        float dist = sqrtf(dx * dx + dy * dy + dz * dz);

        if (dist > 1.0f && dist < bestDist)
        {
            best = ent;
            bestDist = dist;
        }
    }

    return best;
}

// ignoreEntity should be the target's own entity index, but PM_TraceLine's
// ignore_pe is documented against the engine's internal pmove physents
// list, not guaranteed to line up with the raw network entity index - so
// rather than lean entirely on that, the traced endpoint is also pulled
// back a bit short of the actual target point. That way the trace doesn't
// need ignore_pe to work perfectly to avoid the target's own hitbox
// blocking its own aim trace; it just never reaches that far in the first
// place. outFraction lets the caller reuse this single trace for debug
// logging instead of tracing twice.
bool HasLineOfSight(float* from, float* to, int ignoreEntity, float* outFraction)
{
    constexpr float PULLBACK = 24.0f;

    float dir[3] = { to[0] - from[0], to[1] - from[1], to[2] - from[2] };
    float len = sqrtf(dir[0] * dir[0] + dir[1] * dir[1] + dir[2] * dir[2]);

    float end[3] = { to[0], to[1], to[2] };
    if (len > PULLBACK)
    {
        float inv = 1.0f / len;
        end[0] = to[0] - dir[0] * inv * PULLBACK;
        end[1] = to[1] - dir[1] * inv * PULLBACK;
        end[2] = to[2] - dir[2] * inv * PULLBACK;
    }

    pmtrace_t* trace = GetEngineFuncs()->PM_TraceLine(from, end, 0, 2, ignoreEntity);
    float fraction = trace ? trace->fraction : 1.0f;
    if (outFraction)
        *outFraction = fraction;
    return fraction >= 0.90f; // still some slack for edge grazes near the pulled-back endpoint
}

cl_entity_t* FindBestAimTarget(const float* eyeOrigin, float fovDegrees, float maxDistance, bool includePlayers)
{
    cl_entity_t* localPlayer = GetEngineFuncs()->GetLocalPlayer();
    if (!localPlayer)
        return nullptr;

    const bool debugLog = GameState::aimbot_debug_log;

    float viewAngles[3];
    GetEngineFuncs()->GetViewAngles(viewAngles);

    float forward[3], right[3], up[3];
    GetEngineFuncs()->pfnAngleVectors(viewAngles, forward, right, up);

    int myMsgNum = GetEntityMessageNum(localPlayer);

    cl_entity_t* best = nullptr;
    float bestAngle = fovDegrees;

    int playerTypeSeen = 0;
    int playerTypePastFilter = 0;

    if (debugLog)
        printf("[aimbot] scan start: includePlayers=%d fov=%.1f\n", includePlayers, fovDegrees);

    for (int i = 1; i < MAX_AIM_ENTITIES; i++)
    {
        cl_entity_t* ent = GetEngineFuncs()->GetEntityByIndex(i);
        if (!ent) continue;
        if (ent == localPlayer) continue;

        void* model = GetEntityModel(ent);
        if (!model)
            continue;

        // Only ever target hostile monsters/NPCs by default; player-slot
        // entities (real players + bots) are opt-in via includePlayers,
        // and cooperative allies are never auto-targeted at all.
        bool isPlayerEnt = IsPlayerEntity(ent);
        if (isPlayerEnt)
            playerTypeSeen++;

        if (isPlayerEnt)
        {
            if (!includePlayers)
            {
                if (debugLog) printf("[aimbot] idx=%d REJECT player-type, includePlayers=false\n", i);
                continue;
            }
        }
        else
        {
            if (IsAllyEntity(ent))
            {
                if (debugLog) printf("[aimbot] idx=%d REJECT ally (%s)\n", i, GetEntityModelName(ent));
                continue;
            }
            if (!IsMonsterEntity(ent))
            {
                if (debugLog) printf("[aimbot] idx=%d REJECT unclassified movetype=%d (%s)\n", i, ent->curstate.movetype, GetEntityModelName(ent));
                continue;
            }
        }

        if (IsEntityDead(ent))
        {
            if (debugLog) printf("[aimbot] idx=%d REJECT dead (solid=%d)\n", i, ent->curstate.solid);
            continue;
        }

        if (isPlayerEnt)
            playerTypePastFilter++;

        int entMsgNum = GetEntityMessageNum(ent);
        if (entMsgNum + 10 < myMsgNum)
        {
            if (debugLog) printf("[aimbot] idx=%d REJECT stale msgnum=%d mine=%d\n", i, entMsgNum, myMsgNum);
            continue;
        }

        float* entOrigin = GetEntityOrigin(ent);
        float aimHeight = GetAimHeightOffset(ent);

        float dx = entOrigin[0] - eyeOrigin[0];
        float dy = entOrigin[1] - eyeOrigin[1];
        float dz = (entOrigin[2] + aimHeight) - eyeOrigin[2];

        float dist = sqrtf(dx * dx + dy * dy + dz * dz);
        if (dist < 1.0f || dist > maxDistance)
        {
            if (debugLog) printf("[aimbot] idx=%d REJECT dist=%.1f\n", i, dist);
            continue;
        }

        float dirX = dx / dist, dirY = dy / dist, dirZ = dz / dist;
        float dot = dirX * forward[0] + dirY * forward[1] + dirZ * forward[2];
        if (dot < -1.0f) dot = -1.0f;
        if (dot > 1.0f) dot = 1.0f;

        float angle = acosf(dot) * 57.2957795f;
        if (angle > bestAngle)
        {
            if (debugLog) printf("[aimbot] idx=%d REJECT angle=%.1f (fov=%.1f)\n", i, angle, bestAngle);
            continue;
        }

        float targetPoint[3] = { entOrigin[0], entOrigin[1], entOrigin[2] + aimHeight };

        // Skip the LOS trace entirely at close range. Passing ent->index as
        // ignore_pe should exclude the target's own hitbox from the trace,
        // but PM_TraceLine's ignore_pe is documented against the engine's
        // internal pmove physents list, not necessarily the raw network
        // entity index - they usually line up but aren't guaranteed to,
        // and a thick hull-2 trace whose start and end are only a few
        // units apart is exactly where that mismatch would bite (the
        // overlap that gets missed at long range becomes the whole trace
        // up close). There's no realistic case where something is fully
        // touching-range close and still has a wall between you and it.
        constexpr float LOS_SKIP_DISTANCE = 150.0f;
        float fraction = 1.0f;
        if (dist > LOS_SKIP_DISTANCE && !HasLineOfSight((float*)eyeOrigin, targetPoint, ent->index, &fraction))
        {
            if (debugLog) printf("[aimbot] idx=%d REJECT los fraction=%.3f\n", i, fraction);
            continue;
        }

        if (debugLog) printf("[aimbot] idx=%d ACCEPT %s (%s) angle=%.1f dist=%.1f fraction=%.3f\n", i, isPlayerEnt ? "player" : "monster", GetEntityModelName(ent), angle, dist, fraction);

        best = ent;
        bestAngle = angle;
    }

    if (debugLog)
        printf("[aimbot] scan done: playerTypeSeen=%d playerTypePastFilter=%d best=%s\n",
            playerTypeSeen, playerTypePastFilter, best ? GetEntityModelName(best) : "none");

    return best;
}

void DumpEntityList(const float* eyeOrigin, float maxDistance)
{
    cl_entity_t* localPlayer = GetEngineFuncs()->GetLocalPlayer();
    if (!localPlayer)
    {
        printf("[entlist] no local player\n");
        return;
    }

    int myMsgNum = GetEntityMessageNum(localPlayer);
    int shown = 0;

    // Classification here is by model filename (see IsMonsterEntity /
    // IsAllyEntity) - check the "model" column against what you're
    // actually looking at in-game. If something shows up as "other" that
    // should be a monster, or "monster" that should be an ally, tell me
    // the model name shown here and the kHostileMonsterModels /
    // IsAllyModelName lists in entity_utils.cpp can be corrected.
    // msgdiff = how many messages behind mine this entity's last update is
    // (0 = updated this tick, right in step with me). This is what
    // IsEntityDead's staleness check (see entity_utils.cpp) compares
    // against STALE_TOLERANCE - if a known-alive entity is regularly
    // sitting above that tolerance even while clearly still alive on
    // screen, the tolerance is too tight and dropping legitimate targets;
    // tell me the numbers you're seeing.
    // seq/frame = the currently-playing animation sequence index and its
    // playback position - a death animation starting is a sequence change,
    // and it happens the instant the server decides the monster died
    // (long before solid/movetype necessarily follow), so watching this
    // column (or the aimbot's per-frame lock-trace log, which also prints
    // it) across a kill is the fastest way to see whether a death is
    // detectable earlier than solid/movetype currently allow.
    printf("[entlist] idx  type        dist   solid dead  mtype msgdiff seq   frame  model\n");
    for (int i = 1; i < MAX_AIM_ENTITIES; i++)
    {
        cl_entity_t* ent = GetEngineFuncs()->GetEntityByIndex(i);
        if (!ent) continue;
        if (ent == localPlayer) continue;
        if (!GetEntityModel(ent)) continue;
        if (GetEntityMessageNum(ent) + 10 < myMsgNum) continue;

        float* entOrigin = GetEntityOrigin(ent);
        float dx = entOrigin[0] - eyeOrigin[0];
        float dy = entOrigin[1] - eyeOrigin[1];
        float dz = entOrigin[2] - eyeOrigin[2];
        float dist = sqrtf(dx * dx + dy * dy + dz * dz);
        if (dist > maxDistance) continue;

        const char* type = IsPlayerEntity(ent) ? "player/bot"
            : IsAllyEntity(ent) ? "ally"
            : IsMonsterEntity(ent) ? "monster"
            : "other";
        int msgDiff = myMsgNum - GetEntityMessageNum(ent);
        printf("[entlist] %-4d %-11s %-6.0f %-5d %-5s %-5d %-7d %-5d %-6.1f %s\n", i, type, dist,
            ent->curstate.solid, IsEntityDead(ent) ? "yes" : "no", ent->curstate.movetype, msgDiff,
            ent->curstate.sequence, ent->curstate.frame, GetEntityModelName(ent));
        shown++;
    }
    printf("[entlist] %d entities in range\n", shown);
}