#include "menu_render.h"
#include "menu_state.h"
#include "../shared/game_state.h"
#include "../game/game_interfaces.h"
#include "../game/entity_utils.h"
#include <cstdio>

void Menu_Render()
{
    cl_enginefunc_t* engine = GetEngineFuncs();
    if (!engine->pfnFillRGBA || !engine->pfnDrawConsoleString || !engine->pfnDrawSetTextColor)
        return;

    const int x = 40;
    const int y = 40;
    const int width = 300;
    const int rowHeight = 18;
    const int headerHeight = 24;
    const int height = headerHeight + ITEM_COUNT * rowHeight + 8;

    // Background + thin border (no line-draw primitive available here,
    // so the border is just four 1px-thick fill rects).
    engine->pfnFillRGBA(x, y, width, height, 10, 10, 10, 200);
    engine->pfnFillRGBA(x, y, width, 1, 90, 90, 90, 255);
    engine->pfnFillRGBA(x, y + height - 1, width, 1, 90, 90, 90, 255);
    engine->pfnFillRGBA(x, y, 1, height, 90, 90, 90, 255);
    engine->pfnFillRGBA(x + width - 1, y, 1, height, 90, 90, 90, 255);

    engine->pfnDrawSetTextColor(1.0f, 1.0f, 1.0f);
    engine->pfnDrawConsoleString(x + 10, y + 6, (char*)"SvenCoop_Enhanced [INSERT to close]");

    int selected = Menu_GetSelectedIndex();
    bool capturing = Menu_IsCapturingKey();

    for (int i = 0; i < ITEM_COUNT; i++)
    {
        int rowY = y + headerHeight + i * rowHeight;
        bool isSelected = (i == selected);

        if (isSelected)
            engine->pfnFillRGBA(x + 4, rowY, width - 8, rowHeight, 60, 60, 90, 180);

        const char* label = "";
        char valueBuf[64] = "";

        switch (static_cast<MenuItemId>(i))
        {
        case ITEM_BHOP_ENABLED:
            label = "Bunnyhop";
            sprintf_s(valueBuf, sizeof(valueBuf), "%s", GameState::bhop_enabled ? "ON" : "OFF");
            break;
        case ITEM_AIMBOT_ENABLED:
            label = "Aimbot";
            sprintf_s(valueBuf, sizeof(valueBuf), "%s", GameState::aimbot_enabled ? "ON" : "OFF");
            break;
        case ITEM_TARGET_BOTS:
            label = "Target Bots/Players";
            sprintf_s(valueBuf, sizeof(valueBuf), "%s", GameState::aimbot_target_bots ? "ON" : "OFF");
            break;
        case ITEM_FOV:
            label = "FOV";
            sprintf_s(valueBuf, sizeof(valueBuf), "%.1f", GameState::aimbot_fov);
            break;
        case ITEM_HITBOX:
            label = "Aim Point";
            sprintf_s(valueBuf, sizeof(valueBuf), "%s", GetHitboxName(GameState::aimbot_hitbox_index));
            break;
        case ITEM_AIM_MODE:
            label = "Aim Mode";
            sprintf_s(valueBuf, sizeof(valueBuf), "%s", GameState::aimbot_hold_mode ? "Hold" : "Toggle");
            break;
        case ITEM_AIM_KEY:
            label = "Aim Key";
            if (capturing && isSelected)
                sprintf_s(valueBuf, sizeof(valueBuf), "press a key...");
            else
                sprintf_s(valueBuf, sizeof(valueBuf), "%s", Menu_KeyName(GameState::aimbot_key));
            break;
        case ITEM_TRIGGERBOT_ENABLED:
            label = "Triggerbot";
            sprintf_s(valueBuf, sizeof(valueBuf), "%s", GameState::triggerbot_enabled ? "ON" : "OFF");
            break;
        case ITEM_NORECOIL:
            label = "No Recoil";
            sprintf_s(valueBuf, sizeof(valueBuf), "%s", GameState::norecoil_enabled ? "ON" : "OFF");
            break;
        case ITEM_DEBUG_LOG:
            label = "Debug Log";
            sprintf_s(valueBuf, sizeof(valueBuf), "%s", GameState::aimbot_debug_log ? "ON" : "OFF");
            break;
        case ITEM_ESP_ENABLED:
            label = "ESP";
            sprintf_s(valueBuf, sizeof(valueBuf), "%s", GameState::esp_enabled ? "ON" : "OFF");
            break;
        case ITEM_ESP_SHOW_MONSTERS:
            label = "  Show Monsters/Allies";
            sprintf_s(valueBuf, sizeof(valueBuf), "%s", GameState::esp_show_monsters ? "ON" : "OFF");
            break;
        case ITEM_ESP_SHOW_PLAYERS:
            label = "  Show Players/Bots";
            sprintf_s(valueBuf, sizeof(valueBuf), "%s", GameState::esp_show_players ? "ON" : "OFF");
            break;
        case ITEM_ESP_SHOW_OTHER:
            label = "  Show Other";
            sprintf_s(valueBuf, sizeof(valueBuf), "%s", GameState::esp_show_other ? "ON" : "OFF");
            break;
        case ITEM_ESP_SHOW_DEAD:
            label = "  Show Dead";
            sprintf_s(valueBuf, sizeof(valueBuf), "%s", GameState::esp_show_dead ? "ON" : "OFF");
            break;
        case ITEM_ESP_SHOW_BOX:
            label = "  Show Box";
            sprintf_s(valueBuf, sizeof(valueBuf), "%s", GameState::esp_show_box ? "ON" : "OFF");
            break;
        case ITEM_ESP_SHOW_NAME:
            label = "  Show Name";
            sprintf_s(valueBuf, sizeof(valueBuf), "%s", GameState::esp_show_name ? "ON" : "OFF");
            break;
        case ITEM_ESP_SHOW_MODEL:
            label = "  Show Model";
            sprintf_s(valueBuf, sizeof(valueBuf), "%s", GameState::esp_show_model ? "ON" : "OFF");
            break;
        case ITEM_ESP_SHOW_DISTANCE:
            label = "  Show Distance";
            sprintf_s(valueBuf, sizeof(valueBuf), "%s", GameState::esp_show_distance ? "ON" : "OFF");
            break;
        default:
            break;
        }

        if (isSelected)
            engine->pfnDrawSetTextColor(1.0f, 1.0f, 0.4f);
        else
            engine->pfnDrawSetTextColor(0.8f, 0.8f, 0.8f);

        engine->pfnDrawConsoleString(x + 12, rowY + 3, (char*)label);
        engine->pfnDrawConsoleString(x + width - 100, rowY + 3, valueBuf);
    }
}
