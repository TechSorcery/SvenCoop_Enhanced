#pragma once

// Subclasses the game window to catch WM_MOUSEWHEEL for hitbox cycling
// (see hk_mouse.cpp). Not fatal if it fails to attach - the aimbot still
// works with whatever aim point is set, it just can't be cycled live with
// the wheel (still adjustable via the menu's "Aim Point" row).
bool Hook_MouseWheel();
