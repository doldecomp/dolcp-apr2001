#include <dolphin.h>
#include "charPipeline.h"

// functions
void LITAlloc(Light **light);
void LITFree(Light **light);
void LITInitAttn(Light *light, f32 a0, f32 a1, f32 a2, f32 k0, f32 k1, f32 k2);
void LITInitSpot(Light *light, f32 cutoff, GXSpotFn spot_func);
void LITInitDistAttn(Light *light, f32 ref_distance, f32 ref_brightness, GXDistAttnFn dist_func);
void LITInitPos(Light *light, f32 x, f32 y, f32 z);
void LITInitDir(Light *light, f32 nx, f32 ny, f32 nz);
void LITInitColor(Light *light, GXColor color);
void LITXForm(Light *light, Mtx44Ptr view);
void LITAttach(Light *light, char *parent, enum PARENT_TYPE type);
void LITDetach(Light *light);
Control *LITGetControl(Light *light);
void LITSetAnimSequence(Light *light, ANIMBank *animBank, char *seqName, f32 time);
void LITTick(Light *light);

void LITAlloc(Light **light) {
    *light = OSAllocFromHeap(__OSCurrHeap, 0xB0);
    (*light)->position.x = (*light)->position.y = (*light)->position.z = 0.0f;
    (*light)->direction.x = (*light)->direction.y = 0.0f;
    (*light)->direction.z = 1.0f;
    (*light)->parent = NULL;
    (*light)->animPipe = NULL;
    (*light)->control.type = 0;
}

void LITFree(Light **light) {
    OSFreeToHeap(__OSCurrHeap, *light);
    *light = NULL;
}

void LITInitAttn(Light *light, f32 a0, f32 a1, f32 a2, f32 k0, f32 k1, f32 k2) {
    GXInitLightAttn(&light->lt_obj, a0, a1, a2, k0, k1, k2);
}

void LITInitSpot(Light *light, f32 cutoff, GXSpotFn spot_func) {
    GXInitLightSpot(&light->lt_obj, cutoff, spot_func);
}

void LITInitDistAttn(Light *light, f32 ref_distance, f32 ref_brightness, GXDistAttnFn dist_func) {
    GXInitLightDistAttn(&light->lt_obj, ref_distance, ref_brightness, dist_func);
}

void LITInitPos(Light *light, f32 x, f32 y, f32 z) {
    light->position.x = x;
    light->position.y = y;
    light->position.z = z;
}

void LITInitDir(Light *light, f32 nx, f32 ny, f32 nz) {
    light->direction.x = nx;
    light->direction.y = ny;
    light->direction.z = nz;
}

void LITInitColor(Light *light, GXColor color) {
    light->color = color;
}

void LITXForm(Light *light, Mtx44Ptr view) {
    Mtx m;

    CTRLBuildMatrix(&light->control, m);
    if (light->parent) {
        MTXConcat(light->parent, m, m);
    }
    MTXConcat(view, m, m);
    MTXMultVec(m, &light->position, &light->worldPosition);
    MTXInverse(m, m);
    MTXTranspose(m, m);
    MTXMultVec(m, &light->direction, &light->worldDirection);
}

void LITAttach(Light *light, char *parent, enum PARENT_TYPE type) {
    switch (type) {
        case PARENT_BONE:
            light->parent = (void*)((u32*)parent)[0x84/4];
            return;
        case PARENT_DISP_OBJ:
            light->parent = (void*)((char*)parent + 0x1C);
            return;
        case PARENT_MTX:
            light->parent = (void*)parent;
            return;
    }
}

void LITDetach(Light *light) {
    light->parent = NULL;
}

Control *LITGetControl(Light *light) {
    return &light->control;
}

void LITSetAnimSequence(Light *light, ANIMBank *animBank, char *seqName, f32 time) {
    ANIMSequences *seq;
    struct ANIMAnimTrack * track;

    seq = ANIMGetSequence(animBank, seqName, 0);
    track = seq->trackArray;
    if (!light->animPipe) {
        light->animPipe = OSAllocFromHeap(__OSCurrHeap, 0x14);
        light->animPipe->time = 0.0f;
        light->animPipe->speed = 1.0f;
        light->animPipe->currentTrack = 0;
        light->animPipe->control = 0;
    }
    ANIMBind(light->animPipe, &light->control, track, time);
}

void LITTick(Light *light) {
    ANIMTick(light->animPipe);
}
