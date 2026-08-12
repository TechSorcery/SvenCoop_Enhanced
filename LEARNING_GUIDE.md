# How SvenCoop_Enhanced Works — a guide to the reversing and the architecture

This is a map of the whole project: how the memory structures and functions were identified, how the hooking works, and how each feature fits into the frame-by-frame flow. Each section is a summary — treat it as a table of contents for follow-up conversations, not the final word. Where there's real depth to go into (the aimbot's target-lock logic, the ESP projection math), it's flagged so you know where to come back and dig in.

## Part 1 — What was already known vs. what had to be reversed

The first thing worth understanding is that most of this project *isn't* blind reverse engineering, and knowing which parts are which tells you where to spend your own RE effort versus where to just read documentation.

**Public, documented, zero RE required:** Half-Life's SDK was released by Valve as source code (`cl_dll`, `dlls`, `pm_shared`, etc.), and every GoldSrc mod — Sven Co-op included — is built against it. That means struct layouts like `entity_state_t`, `usercmd_t`, `playermove_t`, and the `cl_enginefunc_t` function table are *documented, public C structs*, not something anyone had to guess from raw bytes. `sdk_structs.h` in this project is essentially a trimmed transcription of those public headers. Likewise, the five functions this project hooks — `CL_CreateMove`, `HUD_PlayerMove`, `HUD_Redraw`, `HUD_PostRunCmd` — are *exported by name* from `client.dll` (`GetProcAddress(hClient, "HUD_Redraw")` just works), because the engine calls into the client DLL through a documented, named interface. No signature scanning needed for any of that — this is the leverage you get from a mod being built on a released SDK instead of a fully closed engine.

**Had to be found empirically:** Two things in this codebase aren't just "read the SDK header":

1. **`GENGFUNCS_OFFSET = 0x1F8998`** (`game_interfaces.cpp`) — the raw byte offset inside `client.dll` where the pointer to the populated `cl_enginefunc_t` table lives. This isn't exported by name; the engine hands the client DLL this table once, early, through an internal interface call, and the DLL stores it in a global. Finding that global's *address* (not its layout — the layout is public) is genuine reverse engineering: the standard approach is a debugger or disassembler (x64dbg/IDA/Ghidra), setting a breakpoint on where the SDK source shows the engine populating `gEngfuncs`, then reading off where that pointer gets written to static storage. Once you have the offset, you don't need a debugger again unless the game updates and shifts it — which is exactly the fragility called out in the code review: there's no version anchor recorded for which build `0x1F8998` was confirmed against.
2. **Raw `cl_entity_t` field offsets** (`GetEntityOrigin` at `+2888`, `GetEntityModel` at `+2964`, `GetEntityMessageNum` at `+700`) — these read *past* the parts of the struct this project's own `sdk_structs.h` bothers to declare. Rather than transcribing the entire multi-hundred-byte `cl_entity_t`/`entity_state_t` layout field-by-field into the header, whoever wrote this jumped straight to "the origin is at this many bytes from the start of the struct," found by the same debugger technique: freeze the game, watch a known-good value (your own player's X/Y/Z) change in memory as you move, and back-calculate the offset from the struct's base pointer. This is a completely valid shortcut, but it's also *why* the earlier `local_state_t`-before-`entity_state_t` ordering bug happened this session — mixing "structs transcribed from the public SDK" with "structs poked at via raw offsets" in the same file makes it easy to lose track of what's actually been verified against what.

The practical lesson for doing this yourself on a new target: always check whether an SDK or leaked source exists for the engine/game you're targeting before reaching for a disassembler. If it does, 80% of "reverse engineering" becomes "find where the public struct you already have lives in memory," which is a much smaller problem than reconstructing an unknown struct from scratch.

## Part 2 — Hooking: how interception actually works, and why these four points

**The mechanism.** This project uses [MinHook](https://github.com/TsudaKageyu/minhook), a standard inline/trampoline hooking library, wired up through `core/hooking.cpp`'s thin `HookManager` wrapper. The concept: MinHook overwrites the first several bytes of the target function with a jump instruction to your replacement function (the "detour"), and saves the original overwritten bytes into a small allocated stub (the "trampoline") so you can still call the *original* behavior on demand. Every hook in this project follows the exact same shape because of this: `Hooked_X` runs, calls `Original_X(...)` (the trampoline) either first or last depending on whether it needs to see the result, and does its own work around that call. This is why, e.g., `Hooked_HUD_Redraw` calls `Original_HUD_Redraw` *first* and draws ESP/menu *after* — the 3D scene and the game's own HUD need to exist on screen before you draw on top of them.

**Why these specific four functions, and not others.** Each hook point corresponds to a distinct moment in a single rendered frame's lifecycle, and the choice of *where* to hook is really a choice of "what data is available, and what can I still change, at this exact moment":

- **`CL_CreateMove`** runs once per frame, *before* the resulting `usercmd_t` (view angles, movement, buttons) is sent off to be predicted and eventually networked to the server. This is the only point where altering `cmd->viewangles` or `cmd->buttons` actually changes what the server sees — which is why the aimbot, triggerbot, and bhop logic all live here. Miss this window and you're just changing a copy of data nobody reads.
- **`HUD_PlayerMove`** runs during client-side prediction (movement simulation the client runs locally to hide network latency). This project doesn't change anything here — it only *reads* `ppmove->origin`, because prediction updates every frame rather than once per server tick, giving a much smoother position to aim/project from than the raw networked entity origin would (see `GetPredictedOrigin`/`GetStableEyeOrigin`).
- **`HUD_Redraw`** runs once per actually-rendered frame, after the 3D scene and the game's native HUD are drawn. This is the idiomatic place for any 2D screen-space overlay in a GoldSrc client DLL — ESP boxes and the menu both draw here, using the engine's own `pfnFillRGBA`/`pfnDrawConsoleString` primitives rather than raw OpenGL calls (which would need manual state save/restore to avoid corrupting the engine's own rendering).
- **`HUD_PostRunCmd`** runs after a `usercmd` has been through prediction, and hands you the resulting `local_state_t` — including `punchangle`, the value that actually drives camera kick from recoil on the next frame. This is the only point where zeroing that value has any effect, which is why no-recoil lives specifically here and nowhere else.

Note the project also draws a line between *fatal* and *optional* hooks (see `dllmain.cpp`'s `MainThread`): the four exported functions above are fatal to lose (the DLL unloads itself if any fails), but the mouse-wheel hitbox-cycling hook — which subclasses the raw window procedure via `SetWindowLongPtrA` rather than hooking an official export — is allowed to fail without taking anything else down, because it's cosmetic and because subclassing a window procedure is a fundamentally less reliable technique than hooking a documented export.

## Part 3 — The shared foundation: `entity_utils.cpp` as a case study in iterative RE

This file is worth studying on its own, separate from any single feature, because it's the clearest example in the project of *evidence-driven* reverse engineering rather than guessing.

The core problem it solves — "is this entity a hostile monster, an ally, a player, or dead?" — sounds like it should be answerable from `entity_state_t.health`, but isn't: GoldSrc only networks health for *your own* player, not for anything else you can see. Every classification signal in this file exists because that one obvious approach was ruled out empirically, and each fallback exists because the previous one had a specific, discovered failure case:

- **Model name matching** is the first-choice signal (fast, reliable, no false-positive risk) — but it only works for models someone's already seen and added to a list.
- **Movetype** (`MOVETYPE_STEP`/`MOVETYPE_FLY` = AI-controlled) generalizes to unknown models, since any CBaseMonster-derived NPC moves itself that way regardless of skin — but it over-fires on non-combat scripted props that reuse the same movetype for their own animation, which is why it's gated on `solid == SOLID_SLIDEBOX` too (a false positive found via a specific decorative map prop).
- **Death detection** has four layered signals for the same reason, each added after a specific monster pack broke the previous one: `solid == SOLID_NOT` as the fast path, a hardcoded model list for the (now externally-confirmed) case of corpses that settle at `SOLID_BBOX` instead, a `movetype`-leaving-STEP/FLY fallback for everything else in that category, and a message-staleness fallback for the one pack found to freeze its entity state entirely on death rather than transition it at all.

The methodology worth taking away, independent of GoldSrc specifics: when a heuristic doesn't hold universally, don't throw it out — find *why* it failed for that specific case, add the narrowest fallback that covers exactly that case, and write down the failure that justified it. That's what makes this file readable six months later instead of a pile of unexplained special cases. The live per-frame debug trace built later in this project (`aimbot_debug_log`, `DumpEntityList`) is the same instinct applied to *runtime* diagnosis: get a ground-truth trace of what the game is actually doing, rather than guessing from a single symptom report.

## Part 4 — One frame, start to finish

Roughly, in order, everything that happens once per rendered frame:

1. **`CL_CreateMove` fires.** If the menu is open, view angles get frozen and every gameplay feature is skipped for this frame. Otherwise: `Bhop_OnCreateMove` clears the jump button if you're airborne (letting the engine's own timing decide when the next jump actually fires, rather than trying to reimplement jump timing), `Aimbot_OnCreateMove` may override `cmd->viewangles`, and `Triggerbot_OnCreateMove` may force `cmd->buttons |= IN_ATTACK`.
2. **The engine runs prediction**, which internally calls `HUD_PlayerMove` (possibly more than once, replaying past commands) — this project just watches and caches the smoothed predicted origin as it goes by.
3. **`HUD_PostRunCmd` fires** with the predicted result; no-recoil zeroes `punchangle` here if enabled.
4. **The engine renders the 3D scene** using the (possibly aimbot-modified) view angles from step 1.
5. **`HUD_Redraw` fires** after that render: ESP draws its boxes/text over whatever was just rendered, then the menu draws on top of that if it's open.

Everything in `features/` and `game/entity_utils.cpp` is *called from* one of these five points — none of it runs independently or on its own timer. That single-threaded, single-frame-loop structure is also why there's so little synchronization/locking anywhere in the project: everything genuinely does run sequentially, in this exact order, once per frame, on the game's own thread.

## Part 5 — Aimbot (summary — ask to go deeper)

Pipeline, roughly: `FindBestAimTarget` scans every entity in range once, rejecting by type (unclassified/ally/dead/stale/too far/outside the FOV cone/no line of sight, in that order, each with its own debug-log line), and keeps whichever surviving candidate has the smallest angle off your crosshair. Once acquired in Hold mode, the target is "sticky" — held across frames via `s_LockedTarget` rather than re-picked every frame — and only re-evaluated through `IsLockedTargetStillValid`, which itself only drops the lock after several consecutive bad frames (`s_TargetMissStreak`/`TARGET_MISS_TOLERANCE`) rather than on the first one, specifically to avoid the "flickers between targets" bug found earlier in this project. The actual aim movement is `atan2`-based angle math (desired pitch/yaw from the vector to the target) blended toward the current view angle by a fixed `AIMBOT_STRENGTH` fraction per frame rather than snapped instantly, then pitch-clamped to the engine's real ±89° limit.

Worth going deeper on later: exactly how the FOV-cone/angle math works, why `HasLineOfSight` pulls its trace endpoint back 24 units, or the full reasoning behind the miss-streak debounce.

## Part 6 — ESP (summary — ask to go deeper)

`WorldToScreen` is a standard perspective projection: take the vector from your eye to the target, decompose it into forward/right/up components using the camera's own basis vectors (from `pfnAngleVectors`), then scale the right/up components by `rfl` (a focal-length equivalent, `screenWidth/2 / tan(halfFOV)`) divided by the forward component — the further away something is, the more its screen-space offset shrinks, which is exactly the "similar triangles" perspective effect. The box itself comes from projecting the top and bottom of the entity's *networked* bounding box (`curstate.mins`/`maxs`) rather than using a fixed pixel size, so boxes scale correctly with distance. The FOV-vs-aspect-ratio correction fixed this session (`ScaleFovByAspect`) exists because the `"fov"` cvar is always the 4:3-baseline value; the engine internally widens the true rendered FOV to match your actual aspect ratio before drawing the 3D scene, and the projection math has to apply that same widening or its assumed view cone doesn't match what's actually on screen.

Worth going deeper on later: the full derivation of the perspective-projection formula, or how the hitbox/aim-point offset system (`GetAimHeightOffset`, the 7-position cycle) works.

## Part 7 — Triggerbot, bhop, no-recoil (brief — these are small enough that the code comments are close to the full story)

Triggerbot reuses `FindBestAimTarget` with a very tight FOV cone (2°) instead of writing a separate center-screen raytrace, specifically to avoid a documented unreliability in `PM_TraceLine`'s `ignore_pe` parameter (its physents-list index isn't guaranteed to line up with the raw network entity index). Bhop just strips the jump button when you're airborne, leaving the engine's own ground-detection and timing to decide when the *next* press actually triggers a jump — no custom timing logic at all. No-recoil is the single line in Part 2 above (zeroing `punchangle` in `HUD_PostRunCmd`); it doesn't touch anything server-authoritative like actual bullet spread, which is why no-spread was ruled out entirely — that's rolled server-side and never reaches the client.

## Part 8 — Menu (brief)

Clean state/render split: `menu_state.cpp` owns input handling and the `GameState` mutations, `menu_render.cpp` only reads `GameState` and draws — they only share the `MenuItemId` enum. Key capture (rebinding the aim key) works by priming an edge-detection table with every key that's *currently* held right when capture mode starts, so the same keypress that opened capture mode doesn't immediately get captured as the new binding.

## Where to go deeper from here

Good next sessions, roughly in order of how much new ground each covers:
- The aimbot's angle math and sticky-lock state machine in full detail
- The ESP projection math derived step by step, including the aspect-ratio correction
- How you'd actually find `GENGFUNCS_OFFSET` yourself in a debugger, from scratch, on a fresh game update
- `entity_utils.cpp`'s full classification decision tree, including every false positive that shaped it
- How MinHook's trampoline generation actually works under the hood (worth reading `ext/minhook/src/hook.c` and `trampoline.c` directly)
- The manual-mapping/reflective-injection technique you asked about earlier — as a subject to *understand* (how anti-cheat module enumeration and `LdrLoadDll` hooking work, and what manual mapping does to evade them) rather than to implement against a live multiplayer game
