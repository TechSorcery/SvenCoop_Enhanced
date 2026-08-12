#pragma once
#include "../game/sdk_structs.h"

extern HudRedraw_t Original_HUD_Redraw;
int __cdecl Hooked_HUD_Redraw(float time, int intermission);
bool Hook_HudRedraw();
