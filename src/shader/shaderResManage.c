#include <dolphin.h>
#include "shader.h"

// .data
SHDRResources Resources = {
    // regColorStage[4],
    { SHADER_REG_EMPTY, SHADER_REG_EMPTY, SHADER_REG_EMPTY, SHADER_REG_EMPTY },
    // regColor[4],
    { SHADER_COLOR_NONE, SHADER_COLOR_NONE, SHADER_COLOR_NONE, SHADER_COLOR_NONE },
    // colorIdList[4],
    { SHADER_CPREV, SHADER_C0, SHADER_C1, SHADER_C2 },
    // colorSrc[4],
    { NULL, NULL, NULL, NULL },
    // colorLifeTime[4],
    { 0, 0, 0, 0 },
    // regAlphaState[4],
    { SHADER_REG_EMPTY, SHADER_REG_EMPTY, SHADER_REG_EMPTY, SHADER_REG_EMPTY },
    // regAlpha[4],
    { SHADER_COLOR_NONE, SHADER_COLOR_NONE, SHADER_COLOR_NONE, SHADER_COLOR_NONE },
    // alphaIdList[4],
    { SHADER_APREV, SHADER_A0, SHADER_A1, SHADER_A2 },
    // alphaSrc[4],
    { NULL, NULL, NULL, NULL },
    // alphaLifetime[4],
    { 0, 0, 0, 0 },
    // texObj[8],
    { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
    // mapIdList[8],
    { GX_TEXMAP0, GX_TEXMAP1, GX_TEXMAP2, GX_TEXMAP3, GX_TEXMAP4, GX_TEXMAP5, GX_TEXMAP6, GX_TEXMAP7 }
};

// .sbss
u32 TEVCounter; // size: 0x4, address: 0x4
u8 BestNumTEVStages; // size: 0x1, address: 0x0

// .bss
struct ShaderTEVStage * AlphaInstructions[16]; // size: 0x40, address: 0x0
struct ShaderTEVStage * ColorInstructions[16]; // size: 0x40, address: 0x40
struct ShaderTEVStage TEVPool[50]; // size: 0x1838, address: 0x1000
struct ShaderTEVStage BestAlphaInstructions[16]; // size: 0x7C0, address: 0x840
struct ShaderTEVStage BestColorInstructions[16]; // size: 0x7C0, address: 0x80

// functions
void ResetShaderResources(void);
enum SHADER_COLOR_TYPE AllocateColorResource(enum SHADER_COLOR color);
enum SHADER_COLOR_TYPE AllocateAlphaResource(enum SHADER_COLOR color);
GXTexMapID AllocateTextureResource(GXTexObj *tex);
u8 CheckResourceCollision(struct ShaderTEVStage * stage, struct ShaderTEVStage * parent);
u8 CheckResourceAllocation(void);
static u8 FindInstructionLifetime(struct ShaderTEVStage * stage, u8 index);
static u8 AllocateIntermediateRegister(enum SHADER_REG *regState, u8 *regLifetime, u8 time, u8 lifetime);
void InitCompiledResourceInfo(SHDRRas *res);

void ResetShaderResources(void) {
    u8 i;

    for(i = 0; i < 4; i++) {
        Resources.regColorState[i] = SHADER_REG_EMPTY;
        Resources.regColor[i] = SHADER_COLOR_NONE;
        Resources.colorSrc[i] = NULL;
        Resources.regAlphaState[i] = SHADER_REG_EMPTY;
        Resources.regAlpha[i] = SHADER_COLOR_NONE;
        Resources.alphaSrc[i] = NULL;
    }
    for(i = 0; i < 8; i++) {
        Resources.texObj[i] = NULL;
    }
    TEVCounter = 0;
    BestNumTEVStages = 0x63;
    for(i = 0; i < 16; i++) {
        AlphaInstructions[i] = NULL;
        ColorInstructions[i] = NULL;
    }
}

enum SHADER_COLOR_TYPE AllocateColorResource(enum SHADER_COLOR color) {
    u8 i;

    for(i = 3; i; i--) {
        if (Resources.regColorState[i] == SHADER_REG_EMPTY) {
            Resources.regColorState[i] = SHADER_REG_CONSTANTCOLOR;
            Resources.regColor[i] = color;
            return color + SHADER_COLORINPUT0_RGB;
        }
        if (Resources.regColor[i] == color) {
            return color + SHADER_COLORINPUT0_RGB;
        }
    }
    ASSERTMSGLINE(0xB1, 0, "not enough available color registers for this shader - ExecuteShader");
    return color + SHADER_COLORINPUT0_RGB;
}

enum SHADER_COLOR_TYPE AllocateAlphaResource(enum SHADER_COLOR color) {
    u8 i;

    for(i = 3; i; i--) {
        if (Resources.regAlphaState[i] == SHADER_REG_EMPTY) {
            Resources.regAlphaState[i] = SHADER_REG_CONSTANTCOLOR;
            Resources.regAlpha[i] = color;
            return color + SHADER_COLORINPUT0_A;
        }
        if (Resources.regAlpha[i] == color) {
            return color + SHADER_COLORINPUT0_A;
        }
    }
    ASSERTMSGLINE(0xCE, 0, "not enough available alpha registers for this shader - ExecuteShader");
    return color + SHADER_COLORINPUT0_A;
}

GXTexMapID AllocateTextureResource(GXTexObj *tex) {
    u8 i;

    for(i = 0; i < 8; i++) {
        if (Resources.texObj[i] == 0) {
            Resources.texObj[i] = tex;
            return Resources.mapIdList[i];
        }
        if (Resources.texObj[i] == tex) {
            return Resources.mapIdList[i];
        }
    }
    ASSERTMSGLINE(0xE8, 0, "not enough available texture resources for this shader - ExecuteShader");
    return Resources.mapIdList[0];
}

u8 CheckResourceCollision(struct ShaderTEVStage * stage, struct ShaderTEVStage * parent) {
    if ((stage->texGenIdx != 0x63) && (parent->texGenIdx != 0x63) && (parent->texGenIdx != stage->texGenIdx)) {
        return 0;
    }
    if ((stage->texInput != SHADER_TEXNONE) && (parent->texInput != SHADER_TEXNONE) && (parent->texInput != stage->texInput)) {
        return 0;
    }
    if ((stage->rasInput != SHADER_RASNONE) && (parent->rasInput != SHADER_RASNONE) && (parent->rasInput != stage->rasInput)) {
        return 0;
    }
    if (stage->op != parent->op) {
        return 0;
    }
    if (stage->bias != parent->bias) {
        return 0;
    }
    if (stage->scale != parent->scale) {
        return 0;
    }
    if (stage->mode != parent->mode) {
        return 0;
    }
    if ((stage->clamp != parent->clamp) && ((stage->clamp != 0) || (parent->clamp != 1) || (stage->mode != GX_TC_LINEAR))) {
        return 0;
    }
    return 1;
}

u8 CheckResourceAllocation(void) {
    s8 i;
    s8 j;
    u8 outputIndex;
    u8 time;
    u8 cost;

    cost = 0;
    for(i = 0; i < 16; i++) {
        if (ColorInstructions[i] || AlphaInstructions[i]) {
            cost++;
        } else {
            break;
        }
    }
    if (cost < BestNumTEVStages) {
        for(i = 0; i < 4; i++) {
            Resources.colorLifetime[i] = 0x63;
            Resources.alphaLifetime[i] = 0x63;
        }
        for(i = cost - 1; i >= 0; i--) {
            if (ColorInstructions[i]) {
                time = FindInstructionLifetime(ColorInstructions[i], i);
                outputIndex = AllocateIntermediateRegister(Resources.regColorState, Resources.colorLifetime, i, time);
                if (outputIndex != 0x63) {
                    ColorInstructions[i]->out_reg = Resources.colorIdList[outputIndex];
                } else {
                    return 0;
                }
            }
            if (AlphaInstructions[i]) {
                time = FindInstructionLifetime(AlphaInstructions[i], i);
                outputIndex = AllocateIntermediateRegister(Resources.regAlphaState, Resources.alphaLifetime, i, time);
                if (outputIndex != 0x63) {
                    AlphaInstructions[i]->out_reg = Resources.alphaIdList[outputIndex];
                } else {
                    return 0;
                }
            }
        }
        for(i = cost - 1; i >= 0; i--) {
            if (ColorInstructions[i]) {
                for(j = 0; j < 4; j++) {
                    if (ColorInstructions[i]->tevStage[j]) {
                        ColorInstructions[i]->TEVArg[j] = ColorInstructions[i]->tevStage[j]->out_reg;
                    }
                }
            }
            if (AlphaInstructions[i]) {
                for(j = 0; j < 4; j++) {
                    if (AlphaInstructions[i]->tevStage[j]) {
                        AlphaInstructions[i]->TEVArg[j] = AlphaInstructions[i]->tevStage[j]->out_reg;
                    }
                }
            }
        }        
    }
    return cost;
}

static u8 FindInstructionLifetime(struct ShaderTEVStage * stage, u8 index) {
    u8 i;
    u8 j;
    u8 longest;

    longest = 0;
    if (stage->type == SHADER_TYPE_EMPTY) {
        return 0;
    }
    for(i = 0; i < stage->numParents; i++) {
        for(j = index + 1; j < 16; j++) {
            if ((ColorInstructions[j] == stage->parent[i] || AlphaInstructions[j] == stage->parent[i]) && (j > longest)) {
               longest = j; 
            }
        }
    }
    return longest;
}

static u8 AllocateIntermediateRegister(enum SHADER_REG *regState, u8 *regLifetime, u8 time, u8 lifetime) {
    u8 i;

    for(i = 0; i < 4; i++) {
        if (regState[i] == 1) {
            return 0x63;
        }
        if (regLifetime[i] >= lifetime) {
            regLifetime[i] = time;
            return i;
        }
    }
    return 0x63;
}

void InitCompiledResourceInfo(SHDRRas *res) {
    u8 i;

    for(i = 0; i < 2; i++) {
        res->rasUsed[i] = 0;
    }
    for(i = 0; i < 4; i++) {
        res->colorUsed[i] = 0;
    }
    for(i = 0; i < 4; i++) {
        res->regColor[i] = Resources.regColor[i];
        res->regAlpha[i] = Resources.regAlpha[i];
        if (res->regColor[i] != 4) {
            res->colorUsed[res->regColor[i]] = 1;
        }
        if (res->regAlpha[i] != 4) {
            res->colorUsed[res->regAlpha[i]] = 1;
        }
    }
    for(i = 0; i < 8; i++) {
        res->textureUsed[i] = 0;
        res->textureData[i] = 0;
        res->complexUsed[i] = 0;
        res->complexData[i] = 0;
    }
}
