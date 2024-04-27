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

enum SHADER_TYPE {
    SHADER_TYPE_TEXTURE = 0,
    SHADER_TYPE_COLOR = 1,
    SHADER_TYPE_RASTERIZED = 2,
    SHADER_TYPE_CONSTANT = 3,
    SHADER_TYPE_COMPLEXINPUT = 4,
    SHADER_TYPE_COMPLEX = 5,
    SHADER_TYPE_EMPTY = 6,
};

enum SHADER_CHANNEL {
    SHADER_CHANNEL_RGB = 0,
    SHADER_CHANNEL_RRR = 1,
    SHADER_CHANNEL_GGG = 2,
    SHADER_CHANNEL_BBB = 3,
    SHADER_CHANNEL_A = 4,
    SHADER_CHANNEL_TRIVIAL = 5,
};

enum SHADER_COLOR_TYPE {
    SHADER_CPREV = 0,
    SHADER_C0 = 1,
    SHADER_C1 = 2,
    SHADER_C2 = 3,
    SHADER_APREV = 4,
    SHADER_A0 = 5,
    SHADER_A1 = 6,
    SHADER_A2 = 7,
    SHADER_TEXC = 8,
    SHADER_TEXA = 9,
    SHADER_RASC = 10,
    SHADER_RASA = 11,
    SHADER_TEXRRR = 12,
    SHADER_TEXGGG = 13,
    SHADER_TEXBBB = 14,
    SHADER_HALF = 15,
    SHADER_QUARTER = 16,
    SHADER_COMPLEXINPUT0_RGB = 25,
    SHADER_COMPLEXINPUT1_RGB = 26,
    SHADER_COMPLEXINPUT2_RGB = 27,
    SHADER_COMPLEXINPUT3_RGB = 28,
    SHADER_COMPLEXINPUT4_RGB = 29,
    SHADER_COMPLEXINPUT5_RGB = 30,
    SHADER_COMPLEXINPUT6_RGB = 31,
    SHADER_COMPLEXINPUT7_RGB = 32,
    SHADER_COMPLEXINPUT0_A = 35,
    SHADER_COMPLEXINPUT1_A = 36,
    SHADER_COMPLEXINPUT2_A = 37,
    SHADER_COMPLEXINPUT3_A = 38,
    SHADER_COMPLEXINPUT4_A = 39,
    SHADER_COMPLEXINPUT5_A = 40,
    SHADER_COMPLEXINPUT6_A = 41,
    SHADER_COMPLEXINPUT7_A = 42,
    SHADER_COLORINPUT0_RGB = 50,
    SHADER_COLORINPUT1_RGB = 51,
    SHADER_COLORINPUT2_RGB = 52,
    SHADER_COLORINPUT3_RGB = 53,
    SHADER_COLORINPUT0_A = 60,
    SHADER_COLORINPUT1_A = 61,
    SHADER_COLORINPUT2_A = 62,
    SHADER_COLORINPUT3_A = 63,
    SHADER_NONTRIVIAL = 100,
    SHADER_ONE = 200,
    SHADER_ZERO = 201,
};

enum SHADER_TEX {
    SHADER_TEX0 = 0,
    SHADER_TEX1 = 1,
    SHADER_TEX2 = 2,
    SHADER_TEX3 = 3,
    SHADER_TEX4 = 4,
    SHADER_TEX5 = 5,
    SHADER_TEX6 = 6,
    SHADER_TEX7 = 7,
    SHADER_TEXNONE = 8,
};

enum SHADER_COLOR {
    SHADER_COLOR0 = 0,
    SHADER_COLOR1 = 1,
    SHADER_COLOR2 = 2,
    SHADER_COLOR3 = 3,
    SHADER_COLOR_NONE = 4,
};

enum SHADER_RAS {
    SHADER_RAS0 = 0,
    SHADER_RAS1 = 1,
    SHADER_RASNONE = 2,
};

enum SHADER_OP {
    SHADER_OP_ADD = 0,
    SHADER_OP_SUB = 1,
};

enum SHADER_CLAMP {
    SHADER_CLAMP_LINEAR_1023 = 0,
    SHADER_CLAMP_LINEAR_255 = 1,
    SHADER_CLAMP_GE_255 = 2,
    SHADER_CLAMP_GE_0 = 3,
    SHADER_CLAMP_EQ_255 = 4,
    SHADER_CLAMP_EQ_0 = 5,
    SHADER_CLAMP_LE_255 = 6,
    SHADER_CLAMP_LE_0 = 7,
};

enum SHADER_BIAS {
    SHADER_BIAS_ZERO = 0,
    SHADER_BIAS_ADDHALF = 1,
    SHADER_BIAS_SUBHALF = 2,
};

enum SHADER_SCALE {
    SHADER_SCALE_1 = 0,
    SHADER_SCALE_2 = 1,
    SHADER_SCALE_4 = 2,
    SHADER_SCALE_DIVIDE_2 = 3,
};

enum SHADER_COMPLEX {
    SHADER_COMPLEX0 = 0,
    SHADER_COMPLEX1 = 1,
    SHADER_COMPLEX2 = 2,
    SHADER_COMPLEX3 = 3,
    SHADER_COMPLEX4 = 4,
    SHADER_COMPLEX5 = 5,
    SHADER_COMPLEX6 = 6,
    SHADER_COMPLEX7 = 7,
};

struct SHDRShader {
    /* 0x00 */ enum SHADER_TYPE type;
    /* 0x04 */ enum SHADER_CHANNEL channel;
    /* 0x08 */ s16 referenceCount;
    /* 0x0C */ void *TEVStage;
    union {
        struct {
            /* 0x00 */ enum SHADER_COLOR_TYPE arg;
        } constantShader;
        struct {
            /* 0x00 */ enum SHADER_TEX tex;
            /* 0x04 */ struct SHDRTexCoord *texCoordShader;
        } textureShader;
        struct {
            /* 0x00 */ enum SHADER_COLOR color;
        } colorShader;
        struct {
            /* 0x00 */ enum SHADER_RAS rasColor;
        } rasterizedShader;
        struct {
            /* 0x00 */ struct SHDRShader * input1;
            /* 0x04 */ struct SHDRShader * input2;
            /* 0x08 */ struct SHDRShader * input3;
            /* 0x0C */ struct SHDRShader * input4;
            /* 0x10 */ enum SHADER_OP op;
            /* 0x14 */ enum SHADER_CLAMP clamp;
            /* 0x18 */ enum SHADER_BIAS bias;
            /* 0x1C */ enum SHADER_SCALE scale;
        } complexShader;
        struct {
            /* 0x00 */ enum SHADER_COMPLEX input;
        } complexInputShader;
    } shaderParams;
};

struct TexCoordExp {
    /* 0x00 */ enum SHADER_TG_SRC genSrc;
    /* 0x04 */ u8 srcShaderIdx;
    /* 0x08 */ enum SHADER_TG_TYPE type;
    /* 0x0C */ enum SHADER_MTX mtxInput;
};

typedef struct {
    /* 0x00 */ struct TexCoordExp * expressionArray;
    /* 0x04 */ u8 numExpressions;
    /* 0x08 */ void * mtxArray[8];
    /* 0x28 */ u8 mtxUsed[8];
} SHDRExp;

typedef struct {
    /* 0x00 */ GXTexCoordID coord[8];
    /* 0x20 */ struct SHDRTexCoord *tcShader[8];
    /* 0x40 */ struct SHDRShader *textureShader[8];
} SHDRResources;

typedef struct {
    /* 0x00 */ void * colorStages;
    /* 0x04 */ void * alphaStages;
    /* 0x08 */ u8 numStages;
    /* 0x0C */ void * shaderResources;
    /* 0x10 */ void * texGen;
} SHDRInfo;

// shaderTCInternals.c
extern SHDRResources TCResources;
extern u32 MtxIDArray[8];

void CompileTexGen(struct SHDRShader *shader, SHDRExp **exp);
void SetTCGenState(SHDRInfo *shader);

// shaderTexCoord.c
struct SHDRTexCoord * SHDRCreateTexCoordExpression(enum SHADER_TG_SRC src, struct SHDRTexCoord *shadSrc, enum SHADER_TG_TYPE texGenType, enum SHADER_MTX mtxInput);
struct SHDRTexCoord * SHDRCreateTCPassThrough(enum SHADER_TG_SRC src);
void SHDRFreeTC(struct SHDRTexCoord * texCoord);

#endif // _DOLPHIN_CP_SHADER_H_
