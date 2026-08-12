#pragma once
#include "sdk_structs.h"

cl_enginefunc_t* GetEngineFuncs();

inline float* GetEntityOrigin(cl_entity_t* ent)
{
    return (float*)((char*)ent + 2888);
}

inline void* GetEntityModel(cl_entity_t* ent)
{
    return *(void**)((char*)ent + 2964);
}

inline float* GetEntityCurOrigin(cl_entity_t* ent)
{
    return (float*)((char*)ent + 704);
}

inline int GetEntityMessageNum(cl_entity_t* ent)
{
    return *(int*)((char*)ent + 700);
}

// model_t (com_model.h in the HL SDK) starts with `char name[64]` (the
// relative model path, e.g. "models/hgrunt.mdl" or "sprites/foo.spr" or
// "*42" for inline brush models) - so the model pointer we already read
// via GetEntityModel can be treated directly as that string. This is a
// public, documented struct layout (unlike entity_state_t's health/iuser
// fields, which this mod doesn't seem to populate the standard way), so
// it's a far more reliable way to identify what an entity actually is.
inline const char* GetEntityModelName(cl_entity_t* ent)
{
    void* model = GetEntityModel(ent);
    if (!model)
        return "";
    return (const char*)model;
}