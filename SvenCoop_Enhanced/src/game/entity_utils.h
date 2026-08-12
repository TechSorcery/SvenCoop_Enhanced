#pragma once
#include "sdk_structs.h"

// Player slots only ever occupy indices 1..maxplayers (<=32). Monsters,
// NPCs, and everything else live at higher indices, so the entity scan
// needs to cover the full edict range, not just the player slots, or
// monsters are never seen at all.
constexpr int MAX_AIM_ENTITIES = 1024;

// ent->player is set by the engine for real players AND bots (anything
// occupying a player slot). Monsters/NPCs always have ent->player == 0.
bool IsPlayerEntity(cl_entity_t* ent);

// Classification uses two signals, in order:
//  1) model filename (see GetEntityModelName) against a known-name list -
//     fast path for models already confirmed, no false-positive risk.
//  2) movetype fallback (IsMonsterEntity only) - CBaseMonster-derived NPCs
//     move themselves via MOVETYPE_STEP/MOVETYPE_FLY regardless of what
//     custom model a content pack gives them, so this catches monster
//     models this codebase has never seen without needing their name
//     added anywhere. entity_state_t.health isn't usable for this at all -
//     GoldSrc doesn't network other entities' health to your client, only
//     your own (health reads 0 for everything else, even known-alive
//     entities).
//
// IsMonsterEntity: hostile NPCs worth aiming at by default (headcrab,
// zombie, hgrunt, alien grunt, custom-pack soldiers, etc - named or not).
// IsAllyEntity: friendly/rescuable NPCs and Sven Co-op's cooperative
// "fights alongside you" allies - excluded from default targeting the
// same way bots are, since they're not something you want to shoot. This
// is name-based only (the "_ally" suffix convention) since movetype can't
// tell friend from foe - a content pack whose allies don't follow that
// naming convention could still get auto-targeted; tell me the model name
// if that happens so it can be added to the exclusion list.
bool IsMonsterEntity(cl_entity_t* ent);
bool IsAllyEntity(cl_entity_t* ent);

// entity_state_t.health reads 0 for everything in this build, so it can't
// be used to detect corpses. Four signals, in order:
//  1) Sven Co-op/HL monsters (and players) go non-solid once dead and
//     settled, matching the corpse-detection technique the reference
//     SvenMod cheat used (IsEntityClassCorpse).
//  2) Not reliable for every corpse though - some (e.g. scientist/barney/
//     otis/hgrunt and their reskins) settle at SOLID_BBOX instead of ever
//     going fully SOLID_NOT. Confirmed (not just guessed) against another
//     independent reference cheat's hardcoded model list for exactly this
//     - see kBboxDeadBodyModels in entity_utils.cpp - so this is checked
//     by name directly rather than waiting on signal 3 below.
//  3) For any other model that also settles at SOLID_BBOX without a name
//     match: movetype fallback - a monster/ally under AI control is always
//     MOVETYPE_STEP/FLY, and leaves that (for a physics movetype covering
//     the death-fall) the instant it dies.
//  4) Some custom monster packs (e.g. "aomdc"'s twitchers) don't network a
//     corpse state at all - solid/movetype just freeze at their last-alive
//     values because the server stops updating the entity entirely once
//     it's killed. Falls back to checking whether the entity has actually
//     been updated in the last couple of network ticks - a real, in-PVS AI
//     entity gets a fresh update virtually every tick, so falling stale
//     even briefly is itself a strong "not really here anymore" signal.
bool IsEntityDead(cl_entity_t* ent);

// Single source of truth for "where is my eye right now", shared by every
// feature that aims/traces/projects from the local player's viewpoint
// (aimbot, ESP, triggerbot). Prefers the client-side predicted origin
// (refreshed every frame) over the raw networked cl_entity origin (only
// updated once per server tick - jittery/stepwise while moving).
bool GetStableEyeOrigin(cl_entity_t* localPlayer, float outEye[3]);

// ignoreEntity should be the target's own entity index when tracing toward
// a specific entity (see FindBestAimTarget's use of this), otherwise the
// target's own hitbox can shorten/block the trace at close range. Exposed
// (not just used internally by FindBestAimTarget) so the aimbot's sticky
// hold-lock can re-check LOS on its current target every frame too.
bool HasLineOfSight(float* from, float* to, int ignoreEntity, float* outFraction = nullptr);

// Aim points cyclable with the mouse wheel while the aim key is held (see
// hk_mouse.cpp) or with left/right in the menu's "Aim Point" row. Height
// is a fraction of the entity's networked bounding box (curstate.mins/
// maxs.z); falls back to the old fixed +20 unit offset if that box looks
// unpopulated for a given entity (same caution as the health field).
constexpr int HITBOX_COUNT = 7;
const char* GetHitboxName(int index);
float GetAimHeightOffset(cl_entity_t* ent);

cl_entity_t* FindNearestEntity(float* myOrigin, float maxDistance);

// True if the local player's current viewmodel matches a known melee
// weapon name (crowbar, knife, etc). Same substring-matching convention as
// the monster name lists - report any melee weapon (vanilla or from a
// custom Sven Co-op pack) that isn't being recognized so it can be added.
bool IsLocalPlayerUsingMelee();

// eyeOrigin should be a stable per-frame origin (see GetPredictedOrigin in
// hk_playermove) rather than the raw networked local-player origin, or
// target selection will jitter in lockstep with the aim jitter it's
// meant to avoid. includePlayers gates whether player-slot entities
// (real players + bots) are considered, in addition to monsters.
cl_entity_t* FindBestAimTarget(const float* eyeOrigin, float fovDegrees, float maxDistance, bool includePlayers);

// One-shot console dump of everything in range and how it's classified -
// stand-in for a real entity-list UI until the ImGui overlay exists.
void DumpEntityList(const float* eyeOrigin, float maxDistance);