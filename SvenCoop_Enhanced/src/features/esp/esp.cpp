#include "esp.h"
#include "../../shared/game_state.h"
#include "../../game/game_interfaces.h"
#include "../../game/entity_utils.h"
#include <cmath>
#include <cstdio>

static constexpr float DEG2RAD = 0.01745329252f;
static constexpr float ESP_DEFAULT_FOV = 90.0f; // used if the "fov" cvar can't be read

// The "fov" cvar is always defined for a 4:3 baseline - on anything wider
// (basically every modern resolution), the engine widens the actual
// rendered horizontal FOV to match the real aspect ratio using this exact
// correction before drawing the 3D scene. Skipping this made the ESP
// projection assume a narrower view cone than what's actually on screen,
// which matches exactly what was reported: barely any error at screen
// center, growing the further a point is toward the left/right edge.
static float ScaleFovByAspect(float fovDegrees, float aspectRatio)
{
    constexpr float BASE_ASPECT = 4.0f / 3.0f;
    float halfRad = fovDegrees * 0.5f * DEG2RAD;
    float t = tanf(halfRad) * (aspectRatio / BASE_ASPECT);
    return atanf(t) * 2.0f / DEG2RAD;
}

// Standard GoldSrc perspective projection - similar-triangles projection
// of the vector to worldPoint (in eye-relative forward/right/up space)
// onto the screen plane. rfl ("range from lens", precomputed once per
// frame) is (screenWidth/2) / tan(hfov/2), the standard focal-length
// analogue for this projection. Returns false if the point is behind the
// camera (nothing sensible to draw).
static bool WorldToScreen(const float* worldPoint, const float* forward, const float* right, const float* up,
    const float* eyeOrigin, float rfl, int screenW, int screenH, float* outX, float* outY)
{
    float toTarget[3] = {
        worldPoint[0] - eyeOrigin[0],
        worldPoint[1] - eyeOrigin[1],
        worldPoint[2] - eyeOrigin[2],
    };

    float forwardDot = toTarget[0] * forward[0] + toTarget[1] * forward[1] + toTarget[2] * forward[2];
    if (forwardDot < 1.0f) // behind, or basically on top of, the camera
        return false;

    float rightDot = toTarget[0] * right[0] + toTarget[1] * right[1] + toTarget[2] * right[2];
    float upDot = toTarget[0] * up[0] + toTarget[1] * up[1] + toTarget[2] * up[2];

    *outX = (screenW * 0.5f) + (rightDot * rfl / forwardDot);
    *outY = (screenH * 0.5f) - (upDot * rfl / forwardDot);
    return true;
}

void ESP_Render()
{
    if (!GameState::esp_enabled)
        return;

    cl_enginefunc_t* engine = GetEngineFuncs();
    if (!engine->pfnFillRGBA || !engine->pfnDrawConsoleString || !engine->pfnDrawSetTextColor)
        return;

    cl_entity_t* localPlayer = engine->GetLocalPlayer();
    if (!localPlayer)
        return;

    float eyeOrigin[3];
    GetStableEyeOrigin(localPlayer, eyeOrigin);

    float viewAngles[3];
    engine->GetViewAngles(viewAngles);

    float forward[3], right[3], up[3];
    engine->pfnAngleVectors(viewAngles, forward, right, up);

    int screenW = 1920, screenH = 1080; // sane fallback if GetScreenInfo is unavailable
    if (engine->pfnGetScreenInfo)
    {
        SCREENINFO info = {};
        info.iSize = sizeof(SCREENINFO);
        if (engine->pfnGetScreenInfo(&info) >= 0 && info.iWidth > 0 && info.iHeight > 0)
        {
            screenW = info.iWidth;
            screenH = info.iHeight;
        }
    }

    float fov = ESP_DEFAULT_FOV;
    if (engine->pfnGetCvarFloat)
    {
        float cvarFov = engine->pfnGetCvarFloat((char*)"fov");
        if (cvarFov > 1.0f)
            fov = cvarFov;
    }
    if (screenH > 0)
        fov = ScaleFovByAspect(fov, (float)screenW / (float)screenH);
    float rfl = (screenW * 0.5f) / tanf(fov * 0.5f * DEG2RAD);

    int myMsgNum = GetEntityMessageNum(localPlayer);

    for (int i = 1; i < MAX_AIM_ENTITIES; i++)
    {
        cl_entity_t* ent = engine->GetEntityByIndex(i);
        if (!ent) continue;
        if (ent == localPlayer) continue;
        if (!GetEntityModel(ent)) continue;
        if (GetEntityMessageNum(ent) + 10 < myMsgNum) continue;

        bool isPlayerEnt = IsPlayerEntity(ent);
        bool isAlly = !isPlayerEnt && IsAllyEntity(ent);
        bool isMonster = !isPlayerEnt && !isAlly && IsMonsterEntity(ent);
        bool isOther = !isPlayerEnt && !isAlly && !isMonster;

        if (isPlayerEnt && !GameState::esp_show_players) continue;
        if ((isMonster || isAlly) && !GameState::esp_show_monsters) continue;
        if (isOther && !GameState::esp_show_other) continue;

        bool dead = IsEntityDead(ent);
        if (dead && !GameState::esp_show_dead) continue;

        float* origin = GetEntityOrigin(ent);
        float dx = origin[0] - eyeOrigin[0];
        float dy = origin[1] - eyeOrigin[1];
        float dz = origin[2] - eyeOrigin[2];
        float dist = sqrtf(dx * dx + dy * dy + dz * dz);

        // Project the top and bottom of the entity's networked bbox to get
        // an on-screen box that actually matches its size at this
        // distance, rather than a fixed pixel size - same mins/maxs data
        // (and the same "looks unpopulated, fall back" caution) as the
        // aimbot's hitbox-height calc in entity_utils.cpp.
        float* mins = ent->curstate.mins;
        float* maxs = ent->curstate.maxs;
        float bboxHeight = maxs[2] - mins[2];
        if (bboxHeight < 10.0f)
            bboxHeight = 72.0f;

        float topPoint[3] = { origin[0], origin[1], origin[2] + mins[2] + bboxHeight };
        float botPoint[3] = { origin[0], origin[1], origin[2] + mins[2] };

        float topX, topY, botX, botY;
        if (!WorldToScreen(topPoint, forward, right, up, eyeOrigin, rfl, screenW, screenH, &topX, &topY))
            continue;
        if (!WorldToScreen(botPoint, forward, right, up, eyeOrigin, rfl, screenW, screenH, &botX, &botY))
            continue;

        float boxHeight = botY - topY;
        if (boxHeight < 4.0f) // too far away to be worth drawing
            continue;
        float boxWidth = boxHeight * 0.5f;
        float boxX = topX - boxWidth * 0.5f;
        float boxY = topY;

        if (boxX + boxWidth < 0 || boxX > screenW || boxY + boxHeight < 0 || boxY > screenH)
            continue; // fully offscreen

        int r, g, b;
        if (isPlayerEnt)     { r = 255; g = 255; b = 60; }
        else if (isAlly)     { r = 60;  g = 255; b = 60; }
        else if (isMonster)  { r = 255; g = 60;  b = 60; }
        else                 { r = 160; g = 160; b = 160; }
        if (dead) { r /= 2; g /= 2; b /= 2; }

        if (GameState::esp_show_box)
        {
            int bx = (int)boxX, by = (int)boxY, bw = (int)boxWidth, bh = (int)boxHeight;
            if (bw < 1) bw = 1;
            if (bh < 1) bh = 1;
            engine->pfnFillRGBA(bx, by, bw, 1, r, g, b, 255);
            engine->pfnFillRGBA(bx, by + bh, bw, 1, r, g, b, 255);
            engine->pfnFillRGBA(bx, by, 1, bh, r, g, b, 255);
            engine->pfnFillRGBA(bx + bw, by, 1, bh, r, g, b, 255);
        }

        char lines[4][160];
        int lineCount = 0;

        if (GameState::esp_show_name && lineCount < 4)
        {
            const char* typeStr = isPlayerEnt ? "PLAYER" : isAlly ? "ALLY" : isMonster ? "MONSTER" : "OTHER";
            sprintf_s(lines[lineCount], sizeof(lines[lineCount]), "%s #%d%s", typeStr, i, dead ? " (dead)" : "");
            lineCount++;
        }
        if (GameState::esp_show_model && lineCount < 4)
        {
            sprintf_s(lines[lineCount], sizeof(lines[lineCount]), "%s", GetEntityModelName(ent));
            lineCount++;
        }
        if (GameState::esp_show_distance && lineCount < 4)
        {
            sprintf_s(lines[lineCount], sizeof(lines[lineCount]), "%.0fu", dist);
            lineCount++;
        }

        if (lineCount > 0)
        {
            engine->pfnDrawSetTextColor(r / 255.0f, g / 255.0f, b / 255.0f);
            const int lineHeight = 10;
            for (int line = 0; line < lineCount; line++)
            {
                int ty = (int)boxY - (lineCount - line) * lineHeight;
                engine->pfnDrawConsoleString((int)boxX, ty, lines[line]);
            }
        }
    }
}
