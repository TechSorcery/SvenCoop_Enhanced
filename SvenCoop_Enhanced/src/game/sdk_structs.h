#pragma once

typedef struct playermove_s
{
    int     player_index;
    int     server;
    int     multiplayer;
    float   time;
    float   frametime;
    float   forward[3], right[3], up[3];
    float   origin[3];
    float   angles[3];
    float   oldangles[3];
    float   velocity[3];
    float   movedir[3];
    float   basevelocity[3];
    float   view_ofs[3];
    float   flDuckTime;
    int     bInDuck;
    int     flTimeStepSound;
    int     iStepLeft;
    float   flFallVelocity;
    float   punchangle[3];
    float   flSwimTime;
    float   flNextPrimaryAttack;
    int     effects;
    int     flags;
} playermove_t;

typedef struct usercmd_s
{
    short   lerp_msec;
    unsigned char msec;
    float   viewangles[3];
    float   forwardmove;
    float   sidemove;
    float   upmove;
    unsigned char lightlevel;
    unsigned short buttons;
    unsigned char impulse;
    unsigned char weaponselect;
    int     impact_index;
    float   impact_position[3];
} usercmd_t;

#define IN_ATTACK   (1 << 0)
#define IN_JUMP     (1 << 1)
#define IN_DUCK     (1 << 2)
#define FL_ONGROUND        (1 << 9)
#define FL_PARTIALGROUND   (1 << 10)

// entity_state_t.solid values (standard GoldSrc SOLID enum)
enum solid_e
{
    SOLID_NOT = 0,      // no interaction with other objects (triggers, some sprites)
    SOLID_TRIGGER = 1,  // touch on edge, but not blocking
    SOLID_BBOX = 2,     // touch on edge, block (most items/props)
    SOLID_SLIDEBOX = 3, // touch on edge, but not onground - players and monsters
    SOLID_BSP = 4,      // bsp clip, touch on edge, block - static/breakable brush entities
};

// entity_state_t.movetype values (standard GoldSrc MOVETYPE enum, from the
// public HL SDK). CBaseMonster-derived NPCs move themselves with
// MOVETYPE_STEP (ground pathing) or MOVETYPE_FLY (airborne) regardless of
// what model/skin they use - this is how the aimbot recognizes custom
// monster models without needing their name in a list. Static props sit at
// MOVETYPE_NONE, dropped items settle at MOVETYPE_TOSS/BOUNCE, doors/trains
// use MOVETYPE_PUSH, and the player uses MOVETYPE_WALK.
enum movetype_e
{
    MOVETYPE_NONE = 0,
    MOVETYPE_ANGLENOCLIP = 1,
    MOVETYPE_ANGLECLIP = 2,
    MOVETYPE_WALK = 3,
    MOVETYPE_STEP = 4,
    MOVETYPE_FLY = 5,
    MOVETYPE_TOSS = 6,
    MOVETYPE_PUSH = 7,
    MOVETYPE_NOCLIP = 8,
    MOVETYPE_FLYMISSILE = 9,
    MOVETYPE_BOUNCE = 10,
    MOVETYPE_BOUNCEMISSILE = 11,
    MOVETYPE_FOLLOW = 12,
    MOVETYPE_PUSHSTEP = 13,
};

typedef void (*PlayerMove_t)(playermove_t* ppmove, int server);
typedef void (*CreateMove_t)(float frametime, usercmd_t* cmd, int active);
typedef int  (*HudRedraw_t)(float time, int intermission);

typedef struct entity_state_s
{
    int     entityType;
    int     number;
    float   msg_time;
    int     messagenum;
    float   origin[3];
    float   angles[3];
    int     modelindex;
    int     sequence;
    float   frame;
    int     colormap;
    short   skin;
    short   solid;
    int     effects;
    float   scale;
    unsigned char eflags;
    int     rendermode;
    int     renderamt;
    unsigned char rendercolor[3];
    int     renderfx;
    int     movetype;
    float   animtime;
    float   framerate;
    int     body;
    unsigned char controller[4];
    unsigned char blending[4];
    float   velocity[3];
    float   mins[3];
    float   maxs[3];
    int     aiment;
    int     owner;
    float   friction;
    float   gravity;
    int     team;
    int     playerclass;
    int     health;
    int     spectator;
    int     weaponmodel;
    int     gaitsequence;
    float   basevelocity[3];
    int     usehull;
    int     oldbuttons;
    int     onground;
    int     iStepLeft;
    float   flFallVelocity;
    float   fov;
    int     weaponanim;
    float   startpos[3];
    float   endpos[3];
    float   impacttime;
    float   starttime;
    int     iuser1, iuser2, iuser3, iuser4;
    float   fuser1, fuser2, fuser3, fuser4;
    float   vuser1[3], vuser2[3], vuser3[3], vuser4[3];
} entity_state_t;

constexpr int MAX_PHYSINFO_STRING = 256;

// Public HL SDK struct (cdll_dll.h) - per-frame local-player state used
// for client-side prediction. punchangle is the weapon recoil/view-kick
// vector applied to the camera; zeroing it in HUD_PostRunCmd's "to" state
// is the standard no-recoil technique (see hk_postruncmd.cpp).
typedef struct clientdata_s
{
    float   origin[3];
    float   velocity[3];

    int     viewmodel;
    float   punchangle[3];
    int     flags;
    int     waterlevel;
    int     watertype;
    float   view_ofs[3];
    float   health;

    int     bInDuck;
    int     weapons;

    int     flTimeStepSound;
    int     flDuckTime;
    int     flSwimTime;
    int     waterjumptime;

    float   maxspeed;

    float   fov;
    int     weaponanim;

    int     m_iId;
    int     ammo_shells;
    int     ammo_nails;
    int     ammo_cells;
    int     ammo_rockets;
    float   m_flNextAttack;

    int     tfstate;

    int     pushmsec;

    int     deadflag;
    char    physinfo[MAX_PHYSINFO_STRING];

    int     iuser1, iuser2, iuser3, iuser4;
    float   fuser1, fuser2, fuser3, fuser4;
    float   vuser1[3], vuser2[3], vuser3[3], vuser4[3];
} clientdata_t;

// Public HL SDK struct (cdll_dll.h) - per-weapon prediction data, part of
// local_state_t below. Not used by the no-recoil hook directly, but
// local_state_t's layout isn't valid without it.
typedef struct weapon_data_s
{
    int     m_iId;
    int     m_iClip;

    float   m_flNextPrimaryAttack;
    float   m_flNextSecondaryAttack;
    float   m_flTimeWeaponIdle;

    int     m_fInReload;
    int     m_fInSpecialReload;
    float   m_flNextReload;
    float   m_flPumpTime;
    float   m_fReloadTime;

    float   m_fAimedDamage;
    float   m_fNextAimBonus;
    int     m_fInZoom;
    int     m_iWeaponState;

    int     iuser1, iuser2, iuser3, iuser4;
    float   fuser1, fuser2, fuser3, fuser4;
} weapon_data_t;

// Public HL SDK struct (entity_state.h) - the "before" and "after" state
// HUD_PostRunCmd receives for each processed usercmd during prediction.
// Must come after entity_state_t (used by value here) is fully defined.
typedef struct local_state_s
{
    entity_state_t playerstate;
    clientdata_t   client;
    weapon_data_t  weapondata[64];
} local_state_t;

// HUD_PostRunCmd is called after each usercmd is run (both during
// prediction replay and the final authoritative run) - a standard cl_dll
// export, same convention as HUD_CreateMove/HUD_Redraw/HUD_PlayerMove
// already hooked elsewhere in this codebase.
typedef void (*PostRunCmd_t)(local_state_t* from, local_state_t* to, usercmd_t* cmd, int runfuncs, double time, unsigned int random_seed);

typedef struct cl_entity_s
{
    int             index;
    int             player;
    entity_state_t  baseline;
    entity_state_t  curstate;
    entity_state_t  prevstate;
} cl_entity_t;

typedef struct pmtrace_s
{
    int allsolid;
    int startsolid;
    int inopen, inwater;
    float fraction;
    float endpos[3];
    float plane_normal[3];
    float plane_dist;
    int ent;
    float deltavelocity[3];
    int hitgroup;
} pmtrace_t;

typedef void (*pfnAngleVectors_t)(const float* angles, float* forward, float* right, float* up);
typedef void (*pfnGetViewAngles_t)(float* angles);
typedef void (*pfnSetViewAngles_t)(float* angles);
typedef cl_entity_t* (*pfnGetLocalPlayer_t)();
typedef cl_entity_t* (*pfnGetEntityByIndex_t)(int idx);
typedef cl_entity_t* (*pfnGetViewModel_t)();
typedef pmtrace_t* (*pfnPMTraceLine_t)(float* start, float* end, int flags, int usehull, int ignore_pe);
typedef int (*pfnGetMaxClients_t)();

// Public HL SDK struct (cdll_int.h) - iSize must be set to sizeof(SCREENINFO)
// before calling pfnGetScreenInfo, the engine uses it as a version check.
typedef struct SCREENINFO_s
{
    int   iSize;
    int   iWidth;
    int   iHeight;
    int   iFlags;
    int   iCharHeight;
    short charWidths[256];
} SCREENINFO;

typedef int   (*pfnGetScreenInfo_t)(SCREENINFO* pscrinfo);
typedef float (*pfnGetCvarFloat_t)(char* szName);

// 2D screen-space HUD drawing primitives - safe to call from a HUD_Redraw
// hook without any OpenGL state save/restore dance, unlike raw GL calls.
typedef void (*pfnFillRGBA_t)(int x, int y, int width, int height, int r, int g, int b, int a);
typedef int  (*pfnDrawConsoleString_t)(int x, int y, char* string);
typedef void (*pfnDrawSetTextColor_t)(float r, float g, float b);
typedef void (*pfnDrawConsoleStringLen_t)(const char* string, int* width, int* height);

typedef struct cl_enginefunc_s
{
    void* pfnSPR_Load; void* pfnSPR_Frames; void* pfnSPR_Height; void* pfnSPR_Width;
    void* pfnSPR_Set; void* pfnSPR_Draw; void* pfnSPR_DrawHoles; void* pfnSPR_DrawAdditive;
    void* pfnSPR_EnableScissor; void* pfnSPR_DisableScissor; void* pfnSPR_GetList;
    pfnFillRGBA_t pfnFillRGBA; pfnGetScreenInfo_t pfnGetScreenInfo; void* pfnSetCrosshair;
    void* pfnRegisterVariable; pfnGetCvarFloat_t pfnGetCvarFloat; void* pfnGetCvarString;
    void* pfnAddCommand; void* pfnHookUserMsg; void* pfnServerCmd; void* pfnClientCmd;
    void* pfnGetPlayerInfo;
    void* pfnPlaySoundByName;
    void* pfnPlaySoundByIndex;
    pfnAngleVectors_t pfnAngleVectors;
    void* pfnTextMessageGet; void* pfnDrawCharacter; pfnDrawConsoleString_t pfnDrawConsoleString;
    pfnDrawSetTextColor_t pfnDrawSetTextColor; pfnDrawConsoleStringLen_t pfnDrawConsoleStringLen; void* pfnConsolePrint;
    void* pfnCenterPrint; void* GetWindowCenterX; void* GetWindowCenterY;
    pfnGetViewAngles_t GetViewAngles;
    pfnSetViewAngles_t SetViewAngles;
    pfnGetMaxClients_t GetMaxClients;
    void* Cvar_SetValue; void* Cmd_Argc; void* Cmd_Argv;
    void* Con_Printf; void* Con_DPrintf; void* Con_NPrintf; void* Con_NXPrintf;
    void* PhysInfo_ValueForKey; void* ServerInfo_ValueForKey; void* GetClientMaxspeed;
    void* CheckParm; void* Key_Event; void* GetMousePosition; void* IsNoClipping;
    pfnGetLocalPlayer_t GetLocalPlayer;
    pfnGetViewModel_t GetViewModel;
    pfnGetEntityByIndex_t GetEntityByIndex;
    void* GetClientTime; void* V_CalcShake; void* V_ApplyShake;
    void* PM_PointContents; void* PM_WaterEntity;
    pfnPMTraceLine_t PM_TraceLine;
} cl_enginefunc_t;