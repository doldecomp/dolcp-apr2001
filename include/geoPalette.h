#ifndef _DOLPHIN_CP_GEOPALETTE_H_
#define _DOLPHIN_CP_GEOPALETTE_H_

#include "unktypes.h"

typedef void * (*unkCB)(void *, struct DODisplayObj *, u32, void *, int, char *);

struct DODisplayObj {
    /* 0x00 */ PositionData *positionData;
    /* 0x04 */ ColorData *colorData;
    /* 0x08 */ TextureData *textureData;
    /* 0x0C */ LightingData *lightingData;
    /* 0x10 */ DisplayData *displayData;
    /* 0x14 */ u8 numTextureChannels;
    /* 0x15 */ u8 pad8;
    /* 0x16 */ u16 pad16;
    /* 0x18 */ int visibility;
    /* 0x1C */ Mtx worldMatrix;
    /* 0x4C */ unkCB shaderFunc;
    /* 0x50 */ void *shaderData;
};

extern u8 normalTableQuantizeInfo;
extern u8 normalTableNumComponents;
extern u16 normalTableNumNormals;

extern f32 normalTable[252][3];

#endif // _DOLPHIN_CP_GEOPALETTE_H_
