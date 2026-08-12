#include "hk_playermove.h"
#include "../core/hooking.h"
#include <windows.h>
#include <cstdio>

PlayerMove_t Original_PlayerMove = nullptr;
static int g_LastFlags = 0;

static float g_PredictedOrigin[3] = { 0.0f, 0.0f, 0.0f };
static float g_PredictedViewOfs[3] = { 0.0f, 0.0f, 0.0f };
static bool g_HasPredictedOrigin = false;

void __cdecl Hooked_PlayerMove(playermove_t* ppmove, int server)
{
    Original_PlayerMove(ppmove, server);
    g_LastFlags = ppmove->flags;

    // ppmove->origin is updated by client-side prediction every frame
    // (not just once per server tick), so it stays smooth while moving.
    g_PredictedOrigin[0] = ppmove->origin[0];
    g_PredictedOrigin[1] = ppmove->origin[1];
    g_PredictedOrigin[2] = ppmove->origin[2];
    g_PredictedViewOfs[0] = ppmove->view_ofs[0];
    g_PredictedViewOfs[1] = ppmove->view_ofs[1];
    g_PredictedViewOfs[2] = ppmove->view_ofs[2];
    g_HasPredictedOrigin = true;
}

bool IsOnGround()
{
    return (g_LastFlags & FL_ONGROUND) || (g_LastFlags & FL_PARTIALGROUND);
}

bool GetPredictedOrigin(float outOrigin[3], float outViewOfs[3])
{
    if (!g_HasPredictedOrigin)
        return false;

    outOrigin[0] = g_PredictedOrigin[0];
    outOrigin[1] = g_PredictedOrigin[1];
    outOrigin[2] = g_PredictedOrigin[2];
    outViewOfs[0] = g_PredictedViewOfs[0];
    outViewOfs[1] = g_PredictedViewOfs[1];
    outViewOfs[2] = g_PredictedViewOfs[2];
    return true;
}

bool Hook_PlayerMove()
{
    HMODULE hClient = GetModuleHandleA("client.dll");
    if (!hClient) return false;

    void* target = GetProcAddress(hClient, "HUD_PlayerMove");
    if (!target)
    {
        printf("[hk_playermove] export 'HUD_PlayerMove' not found\n");
        return false;
    }

    return HookManager::Create("PlayerMove", target, (void*)Hooked_PlayerMove, (void**)&Original_PlayerMove);
}