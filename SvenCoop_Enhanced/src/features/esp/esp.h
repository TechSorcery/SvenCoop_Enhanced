#pragma once

// Draws boxes + debug text over entities in range, gated by GameState::
// esp_* toggles (see game_state.h and the menu's ESP section). Called
// every frame from the HUD_Redraw hook, independent of both the aimbot
// and the menu.
void ESP_Render();
