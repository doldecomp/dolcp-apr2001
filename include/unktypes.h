#ifndef _DOLPHIN_CP_UNKTYPES_H_
#define _DOLPHIN_CP_UNKTYPES_H_

// uncategorized types go in here.

#include <dolphin.h>
#include <dolphin/gx.h>

typedef struct {
    /* 0x00 */ char *positionArray;
    /* 0x04 */ u16 numPositions;
    /* 0x06 */ u8 quantizeInfo;
    /* 0x07 */ u8 compCount;
} PositionData;

typedef struct {
    /* 0x00 */ char * colorArray;
    /* 0x04 */ u16 numColors;
    /* 0x06 */ u8 quantizeInfo;
    /* 0x07 */ u8 compCount;
} ColorData;

typedef struct {
    /* 0x00 */ u16 height;
    /* 0x02 */ u16 width;
    /* 0x04 */ u32 format;
    /* 0x08 */ char * data;
    /* 0x0C */ GXTexWrapMode wrapS;
    /* 0x10 */ GXTexWrapMode wrapT;
    /* 0x14 */ GXTexFilter minFilter;
    /* 0x18 */ GXTexFilter magFilter;
    /* 0x1C */ f32 LODBias;
    /* 0x20 */ u8 edgeLODEnable;
    /* 0x21 */ u8 minLOD;
    /* 0x22 */ u8 maxLOD;
    /* 0x23 */ u8 unpacked;
} TextureHeader;

typedef struct {
    /* 0x00 */ u16 numEntries;
    /* 0x02 */ u8 unpacked;
    /* 0x03 */ u8 pad8;
    /* 0x04 */ GXTlutFmt format;
    /* 0x08 */ char * data;
} CLUTHeader;

typedef struct {
    /* 0x00 */ TextureHeader * textureHeader;
    /* 0x04 */ CLUTHeader * CLUTHeader;
} DescriptorArray;

typedef struct {
    /* 0x00 */ u32 versionNumber;
    /* 0x04 */ u32 numDescriptors;
    /* 0x08 */ DescriptorArray * descriptorArray;
} TexturePalette;

typedef struct {
    /* 0x00 */ char * textureCoordArray;
    /* 0x04 */ u16 numTextureCoords;
    /* 0x06 */ u8 quantizeInfo;
    /* 0x07 */ u8 compCount;
    /* 0x08 */ char * texturePaletteName;
    /* 0x0C */ TexturePalette * texturePalette;
} TextureData;

typedef struct {
    /* 0x00 */ char * normalArray;
    /* 0x04 */ u16 numNormals;
    /* 0x06 */ u8 quantizeInfo;
    /* 0x07 */ u8 compCount;
    /* 0x08 */ f32 ambientPercentage;
} LightingData;

typedef struct {
    /* 0x00 */ u8 id;
    /* 0x01 */ u8 pad8;
    /* 0x02 */ u16 pad16;
    /* 0x04 */ u32 setting;
    /* 0x08 */ char * primitiveList;
    /* 0x0C */ u32 listSize;
} DisplayStateList;

typedef struct {
    /* 0x00 */ char * primitiveBank;
    /* 0x04 */ DisplayStateList * displayStateList;
    /* 0x08 */ u16 numStateEntries;
    /* 0x0A */ u16 pad16;
} DisplayData;

typedef struct {
    /* 0x00 */ f32 time;
    /* 0x04 */ char * setting;
    /* 0x08 */ char * interpolation;
} KeyFrame;

typedef struct {
    /* 0x00 */ char * Prev;
    /* 0x04 */ char * Next;
    /* 0x08 */ char * Parent;
    /* 0x0C */ char * Children;
} Branch;

typedef struct {
    /* 0x00 */ Vec s;
    /* 0x0C */ Quaternion r;
    /* 0x1C */ Vec t;
} Srt;

typedef union {
    /* 0x00 */ Srt srt;
    /* 0x00 */ struct {
        /* 0x00 */ Mtx m;
    } mtx;
} ControlParams;

typedef struct {
    /* 0x00 */ u8 type;
    /* 0x01 */ u8 pad8;
    /* 0x02 */ u16 pad16;
    /* 0x04 */ ControlParams controlParams;
} Control;

#endif // _DOLPHIN_CP_UNKTYPES_H_
