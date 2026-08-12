#pragma once

// Draws the menu box via the engine's own 2D screen-space HUD drawing
// functions (FillRGBA/DrawConsoleString) - called from the HUD_Redraw
// hook while the menu is open.
void Menu_Render();
