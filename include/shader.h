#ifndef _DOLPHIN_CP_SHADER_H_
#define _DOLPHIN_CP_SHADER_H_

enum SHADER_TG_SRC {
    SHADER_TG_POS = 0,
    SHADER_TG_NRM = 1,
    SHADER_TG_BINRM = 2,
    SHADER_TG_TANGENT = 3,
    SHADER_TG_TEX0 = 4,
    SHADER_TG_TEX1 = 5,
    SHADER_TG_TEX2 = 6,
    SHADER_TG_TEX3 = 7,
    SHADER_TG_TEX4 = 8,
    SHADER_TG_TEX5 = 9,
    SHADER_TG_TEX6 = 10,
    SHADER_TG_TEX7 = 11,
    SHADER_TG_COLOR0 = 19,
    SHADER_TG_COLOR1 = 20,
};

enum SHADER_TG_TYPE {
    SHADER_TG_MTX3x4 = 0,
    SHADER_TG_MTX2x4 = 1,
    SHADER_TG_BUMP0 = 2,
    SHADER_TG_BUMP1 = 3,
    SHADER_TG_BUMP2 = 4,
    SHADER_TG_BUMP3 = 5,
    SHADER_TG_BUMP4 = 6,
    SHADER_TG_BUMP5 = 7,
    SHADER_TG_BUMP6 = 8,
    SHADER_TG_BUMP7 = 9,
    SHADER_TG_SRTG = 10,
};

enum SHADER_MTX {
    SHADER_MTX0 = 0,
    SHADER_MTX1 = 1,
    SHADER_MTX2 = 2,
    SHADER_MTX3 = 3,
    SHADER_MTX4 = 4,
    SHADER_MTX5 = 5,
    SHADER_MTX6 = 6,
    SHADER_MTX7 = 7,
    SHADER_IDENTITY = 8,
};

struct SHDRTexCoord {
    /* 0x00 */ s16 referenceCount;
    /* 0x04 */ enum SHADER_TG_SRC genSrc;
    /* 0x08 */ struct SHDRTexCoord *shaderSrc;
    /* 0x0C */ enum SHADER_TG_TYPE type;
    /* 0x10 */ enum SHADER_MTX mtxInput;
    /* 0x14 */ u8 texCoordExpIdx;
};

// shaderTexCoord.c
struct SHDRTexCoord * SHDRCreateTexCoordExpression(enum SHADER_TG_SRC src, struct SHDRTexCoord *shadSrc, enum SHADER_TG_TYPE texGenType, enum SHADER_MTX mtxInput);
struct SHDRTexCoord * SHDRCreateTCPassThrough(enum SHADER_TG_SRC src);
void SHDRFreeTC(struct SHDRTexCoord * texCoord);

#endif // _DOLPHIN_CP_SHADER_H_
