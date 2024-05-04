#include <dolphin.h>
#include "shader.h"

void * MtxDataArray[8];
struct TexCoordExp TexExpPool[8];
SHDRRas CurrentShaderResources;
enum SHADER_REG CurrentColorState[4];
enum SHADER_REG CurrentAlphaState[4];
SHDRStage CompiledTEVColorPool[16];
SHDRStage CompiledTEVAlphaPool[16];

u8 CompiledTEVCounter = 0;
u8 CurrentTexExp;
u8 MtxUsed[8];

// functions
void CheckShaderBindings(SHDRInfo *shader);
static void InitExecute(void);
static void CopyTexShaders(SHDRInfo * shader);
void CombineTEVStages(SHDRInfo *shader);
static void CopyTEVStages(SHDRInfo *shader);
static void FindRegisterUsage(enum SHADER_REG *colorRegisters, enum SHADER_REG *alphaRegisters, SHDRStage *stageArray, u8 numStages);
static void AssignConstantColorRegs(enum SHADER_COLOR *colorRemap, SHDRStage *stageArray, u8 numStages);
static void FindParents(u8 *earliest, u8 *latest, SHDRInfo *shader);
static void FindOpenRegisters(u8 pos, enum SHADER_REG *color, enum SHADER_REG *alpha);
static int StageNotEmpty(SHDRStage *stage);
static void FindOutputRegs(u8 earliest, u8 latest, enum SHADER_COLOR_TYPE *regRemap, enum SHADER_REG *startColor, enum SHADER_REG *startAlpha);
static int RemapRegisters(enum SHADER_COLOR_TYPE *regRemap, enum SHADER_REG *tempColorRegUsage, enum SHADER_REG *tempAlphaRegUsage, enum SHADER_REG *colorRegUsage, enum SHADER_REG *alphaRegUsage);
static void PrepareTevPool(u8 earliest, u8 numStages);
static void CopyCompiledStage(SHDRStage *src, SHDRStage *dst);
static void InsertTevStages(u8 pos, SHDRInfo *shader, enum SHADER_TEX *texRemap, enum SHADER_RAS *rasRemap, enum SHADER_COLOR *colorRemap, enum SHADER_COMPLEX *complexRemap, enum SHADER_COLOR_TYPE *regRemap, u8 texOffset);
static void RemapStage(SHDRStage *src, SHDRStage *dst, enum SHADER_TEX * texRemap, enum SHADER_RAS *rasRemap, enum SHADER_COLOR *colorRemap, enum SHADER_COMPLEX *complexRemap, enum SHADER_COLOR_TYPE *regRemap, u8 texOffset);
static void RemapComplexReferences(SHDRInfo *shader, enum SHADER_COLOR_TYPE *regRemap);

void CheckShaderBindings(SHDRInfo *shader) {
    SHDRRas * res;
    SHDRExp * tcExp;
    u8 i;

    res = shader->shaderResources;
    tcExp = shader->texGen;    
    for(i = 0; i < 2; i++) {
        if (res->rasUsed[i] == 1) {
            OSPanic(__FILE__, 0x6F, "Error - rasterized color not bound to any data - CheckShaderBindings");
        }
    }
    for(i = 0; i < 4; i++) {
        if (res->colorUsed[i] == 1) {
            OSPanic(__FILE__, 0x75, "Error - constant color not bound to any data - CheckShaderBindings");
        }
    }
    for(i = 0; i < 8; i++) {
        if (res->textureUsed[i] == 1) {
            OSPanic(__FILE__, 0x7B, "Error - texture not bound to any data - CheckShaderBindings");
        }
        if (res->complexUsed[i] == 1) {
            OSPanic(__FILE__, 0x7E, "Error - complex input not bound to any data - CheckShaderBindings");
        }
        if (tcExp->mtxUsed[i] == 1) {
            OSPanic(__FILE__, 0x81, "Error - mtx input not bound to any data - CheckShaderBindings");
        }
    }
    for(i = 0; i < 8; i++) {
        if (res->complexUsed[i] == 2) {
            CheckShaderBindings(res->complexData[i]);
        }
    }
}

static void InitExecute(void) {
    u8 i;

    CurrentTexExp = 0;
    for(i = 0; i < 8; i++) {
        MtxDataArray[i] = 0;
        MtxUsed[i] = 0;
    }
}

static void CopyTexShaders(SHDRInfo * shader) {
    SHDRExp * tex;
    SHDRRas * res;
    u8 i;
    u8 j;
    u8 offset;

    tex = shader->texGen;
    res = shader->shaderResources;
    offset = CurrentTexExp;
    for(i = 0; i < tex->numExpressions; i++) {
        if (CurrentTexExp > 7) {
            OSPanic(__FILE__, 0xB1, "too many texture coord expressions - CopyTexShaders");
        }
        TexExpPool[CurrentTexExp].genSrc = tex->expressionArray[i].genSrc;
        TexExpPool[CurrentTexExp].type = tex->expressionArray[i].type;
        if (tex->expressionArray[i].srcShaderIdx == 0x63) {
            TexExpPool[CurrentTexExp].srcShaderIdx = 0x63;
        } else {
            TexExpPool[CurrentTexExp].srcShaderIdx = offset + (tex->expressionArray[i].srcShaderIdx);
        }
        if (tex->expressionArray[i].mtxInput == 8) {
            TexExpPool[CurrentTexExp].mtxInput = 8;
        } else {
            for(j = 0; j < 8; j++) {
                if (MtxDataArray[j] == 0) {
                    MtxDataArray[j] = tex->mtxArray[tex->expressionArray[i].mtxInput];
                    TexExpPool[CurrentTexExp].mtxInput = j;
                    MtxUsed[i] = 2;
                    break;
                }
            }
            if (j == 8) {
                OSPanic(__FILE__, 0xCB, "too many matrices - CopyTexShaders");
            }
        }
        CurrentTexExp++;
    }
}

void CombineTEVStages(SHDRInfo *shader) {
    u8 i;
    SHDRExp compTC;
    SHDRInfo comp;

    for(i = 0; i < 4; i++) {
        CurrentShaderResources.regColor[i] = 4;
        CurrentShaderResources.regAlpha[i] = 4;
        CurrentShaderResources.colorUsed[i] = 0;
    }
    for(i = 0; i < 8; i++) {
        CurrentShaderResources.textureUsed[i] = 0;
        CurrentShaderResources.textureData[i] = 0;
        CurrentShaderResources.complexUsed[i] = 0;
        CurrentShaderResources.complexData[i] = 0;
    }
    for(i = 0; i < 2; i++) {
        CurrentShaderResources.rasUsed[i] = 0;
    }
    for(i = 0; i < 4; i++) {
        CurrentColorState[i] = 0;
        CurrentAlphaState[i] = 0;
    }
    CompiledTEVCounter = 0;
    InitExecute();
    CopyTEVStages(shader);
    compTC.expressionArray = &TexExpPool[0];
    compTC.numExpressions = CurrentTexExp;
    for(i = 0; i < 8; i++) {
        compTC.mtxArray[i] = MtxDataArray[i];
        compTC.mtxUsed[i] = MtxUsed[i];
    }
    comp.colorStages = CompiledTEVColorPool;
    comp.alphaStages = CompiledTEVAlphaPool;
    comp.numStages = CompiledTEVCounter;
    comp.shaderResources = &CurrentShaderResources;
    comp.texGen = &compTC;
    SetTCGenState(&comp);
    SetTEV(&comp);
}

static void CopyTEVStages(SHDRInfo *shader) {
    enum SHADER_TEX texRemap[8];
    enum SHADER_RAS rasRemap[2];
    enum SHADER_COLOR colorRemap[4];
    enum SHADER_COMPLEX complexRemap[8];
    enum SHADER_COLOR_TYPE regRemap[8];
    enum SHADER_REG colorRegUsage[4] = { SHADER_REG_EMPTY, SHADER_REG_EMPTY, SHADER_REG_EMPTY, SHADER_REG_EMPTY };
    enum SHADER_REG alphaRegUsage[4] = { SHADER_REG_EMPTY, SHADER_REG_EMPTY, SHADER_REG_EMPTY, SHADER_REG_EMPTY };
    enum SHADER_REG tempColorRegUsage[4] = { SHADER_REG_EMPTY, SHADER_REG_EMPTY, SHADER_REG_EMPTY, SHADER_REG_EMPTY };
    enum SHADER_REG tempAlphaRegUsage[4] = { SHADER_REG_EMPTY, SHADER_REG_EMPTY, SHADER_REG_EMPTY, SHADER_REG_EMPTY };
    SHDRRas * res;
    s8 i;
    s8 j;
    u8 earliest;
    u8 latest;
    u8 texOffset;

    res = shader->shaderResources;
    texOffset = CurrentTexExp;
    for(i = 0; i < 8; i++) {
        if (res->textureUsed[i]) {
            for(j = 0; j < 8; j++) {
                if (CurrentShaderResources.textureUsed[j]) {
                    if (CurrentShaderResources.textureData[j] == res->textureData[i]) {
                        texRemap[i] = j;
                        break;
                    }
                } else {
                    texRemap[i] = j;
                    CurrentShaderResources.textureData[j] = res->textureData[i];
                    CurrentShaderResources.textureUsed[j] = 2;
                    break;
                }
            }
            if (j == 8) {
                OSPanic(__FILE__, 0x14E, "Cannot find empty texture - CopyTEVStages");
            }
        }
    }
    for(i = 0; i < 8; i++) {
        if (res->complexUsed[i]) {
            for(j = 0; j < 8; j++) {
                if (CurrentShaderResources.complexUsed[j]) {
                    if (CurrentShaderResources.complexData[j] == res->complexData[i]) {
                        complexRemap[i] = j;
                        break;
                    }
                } else {
                    complexRemap[i] = j;
                    CurrentShaderResources.complexData[j] = res->complexData[i];
                    CurrentShaderResources.complexUsed[j] = 2;
                    break;
                }
            }
            if (j == 8) {
                OSPanic(__FILE__, 0x16C, "Cannot find empty complex input - CopyTEVStages");
            }
        }
    }
    for(i = 0; i < 2; i++) {
        if (res->rasUsed[i]) {
            for(j = 0; j < 2; j++) {
                if (CurrentShaderResources.rasUsed[j]) {
                    if (CurrentShaderResources.rasData[j] == res->rasData[i]) {
                        rasRemap[i] = j;
                        break;
                    }
                } else {
                    rasRemap[i] = j;
                    CurrentShaderResources.rasData[j] = res->rasData[i];
                    CurrentShaderResources.rasUsed[j] = 2;
                    break;
                }
            }
            if (j == 2) {
                OSPanic(__FILE__, 0x189, "Cannot find empty ras color - CopyTEVStages");
            }
        }
    }
    for(i = 0; i < 4; i++) {
        if (res->colorUsed[i]) {
            for(j = 0; j < 4; j++) {
                if (CurrentShaderResources.colorUsed[j]) {
                    if ((res->colorData[i].r == CurrentShaderResources.colorData[j].r)
                     && (res->colorData[i].g == CurrentShaderResources.colorData[j].g)
                     && (res->colorData[i].b == CurrentShaderResources.colorData[j].b)
                     && (res->colorData[i].a == CurrentShaderResources.colorData[j].a)) {
                        colorRemap[i] = j;
                        break;
                    }
                } else {
                    colorRemap[i] = j;
                    CurrentShaderResources.colorData[j] = res->colorData[i];
                    CurrentShaderResources.colorUsed[j] = 2;
                    break;
                }
            }
            if (j == 4) {
                OSPanic(__FILE__, 0x1A9, "Cannot find empty color - CopyTEVStages");
            }
        }
    }
    AssignConstantColorRegs(colorRemap, shader->colorStages, shader->numStages);
    AssignConstantColorRegs(colorRemap, shader->alphaStages, shader->numStages);
    FindRegisterUsage(colorRegUsage, alphaRegUsage, shader->colorStages, shader->numStages);
    FindRegisterUsage(colorRegUsage, alphaRegUsage, shader->alphaStages, shader->numStages);
    FindParents(&earliest, &latest, shader);
    while(1) {
        FindOpenRegisters(earliest, tempColorRegUsage, tempAlphaRegUsage);
        FindOutputRegs(earliest, latest, regRemap, tempColorRegUsage, tempAlphaRegUsage);
        if (RemapRegisters(regRemap, tempColorRegUsage, tempAlphaRegUsage, colorRegUsage, alphaRegUsage)) {
            break;
        }
        if (earliest == 0) {
            OSPanic(__FILE__, 0x1C6, "cannot insert shader - CopyTEVStages");
        }
        earliest-=1;
    }
    CopyTexShaders(shader);
    if (earliest != 0x63) {
        RemapComplexReferences(shader, regRemap);
    }
    PrepareTevPool(earliest, shader->numStages);
    if (earliest == 0x63) {
        earliest = 0;
    }
    InsertTevStages(earliest, shader, texRemap, rasRemap, colorRemap, complexRemap, regRemap, texOffset);
    for(i = 0; i < 8; i++) {
        if (res->complexUsed[i]) {
            CopyTEVStages(res->complexData[i]);
        }
    }
}

static void FindRegisterUsage(enum SHADER_REG *colorRegisters, enum SHADER_REG *alphaRegisters, SHDRStage *stageArray, u8 numStages) {
    u8 i;
    u8 j;

    for(i = 0; i < numStages; i++) {
        for(j = 0; j < 4; j++) {
            if (stageArray[i].TEVArg[j] >= 0 && stageArray[i].TEVArg[j] <= 3) {
                colorRegisters[stageArray[i].TEVArg[j]] = 2;
            } else if (stageArray[i].TEVArg[j] >= 4 && stageArray[i].TEVArg[j] <= 7) {
                alphaRegisters[(int)(stageArray[i].TEVArg[j] - 4)] = 2;
            }
        }
        if (stageArray[i].out_reg >= 0 && stageArray[i].out_reg <= 3) {
            colorRegisters[stageArray[i].out_reg] = 2;
        } else if (stageArray[i].out_reg >= 4 && stageArray[i].out_reg <= 7) {
            alphaRegisters[(int)(stageArray[i].out_reg - 4)] = 2;
        }
    }
}

static void AssignConstantColorRegs(enum SHADER_COLOR *colorRemap, SHDRStage *stageArray, u8 numStages) {
    s8 i;
    s8 j;
    s8 k;

    for(i = 0; i < numStages; i++) {
        for(j = 0; j < 4; j++) {
            if(stageArray[i].TEVArg[j] >= 0x32 && stageArray[i].TEVArg[j] <= 0x35) {
                for(k = 3; k > -1; k--) {
                    if (CurrentColorState[k] == 2) {
                        OSPanic(__FILE__, 0x21D, "cannot allocate color register for constant color - AssignConstantColorRegs");
                    } else if (CurrentColorState[k] == 1) {
                        if (CurrentShaderResources.regColor[k] == colorRemap[(int)(stageArray[i].TEVArg[j] - 0x32)]) {
                            break;
                        }
                    } else {
                        CurrentColorState[k] = 1;
                        CurrentShaderResources.regColor[k] = colorRemap[(int)(stageArray[i].TEVArg[j] - 0x32)];
                        break;
                    }
                }
                if (k == -1) {
                    OSPanic(__FILE__, 0x22B, "out of registers - AssignConstantColorRegs");
                }
            } else if(stageArray[i].TEVArg[j] >= 0x3C && stageArray[i].TEVArg[j] <= 0x3F) {
                for(k = 3; k > -1; k--) {
                    if (CurrentAlphaState[k] == 2) {
                        OSPanic(__FILE__, 0x234, "cannot allocate alpha register for constant color - AssignConstantColorRegs");
                    } else if (CurrentAlphaState[k] == 1) {
                        if (CurrentShaderResources.regAlpha[k] == colorRemap[(int)(stageArray[i].TEVArg[j] - 0x3C)]) {
                            break;
                        }
                    } else {
                        CurrentAlphaState[k] = 1;
                        CurrentShaderResources.regAlpha[k] = colorRemap[(int)(stageArray[i].TEVArg[j] - 0x3C)];
                        break;
                    }
                }
                if (k == -1) {
                    OSPanic(__FILE__, 0x242, "out of registers - AssignConstantColorRegs");
                }
            }
        }
    }
}

static void FindParents(u8 *earliest, u8 *latest, SHDRInfo *shader) {
    u8 i;
    u8 j;
    enum SHADER_COMPLEX inputID;

    *earliest = *latest = 0x63;
    if (CompiledTEVCounter != 0) {
        for(i = 0; i < 8; i++) {
            if (CurrentShaderResources.complexData[i] == shader) {
                inputID = i;
                break;
            }
        }
        if (i == 8) {
            OSPanic(__FILE__, 0x25D, "cannot find complex input in list - FindParents");
        }
        for(i = 0; i < CompiledTEVCounter; i++) {
            for(j = 0; j < 4; j++) {
                if ((inputID == (u32)(CompiledTEVColorPool[i].TEVArg[j] - 0x19))
                 || (inputID == (u32)(CompiledTEVColorPool[i].TEVArg[j] - 0x23))
                 || (inputID == (u32)(CompiledTEVAlphaPool[i].TEVArg[j] - 0x19))
                 || (inputID == (u32)(CompiledTEVAlphaPool[i].TEVArg[j] - 0x23))) {
                    if (*earliest == 0x63) {
                        *earliest = i;
                    }
                    *latest = i;
                }
            }
        }
        if (*earliest == 0x63) {
            OSPanic(__FILE__, 0x273, "child not found in list - FindParents");
        }
    }
}

static void FindOpenRegisters(u8 pos, enum SHADER_REG *color, enum SHADER_REG *alpha) {
    u8 i;
    u8 j;
    u8 colorAccess[4] = { 0x63, 0x63, 0x63, 0x63 };
    u8 alphaAccess[4] = { 0x63, 0x63, 0x63, 0x63 };

    for(i = 0; i < 4; i++) {
        if (CurrentColorState[i] == 1) {
            colorAccess[i] = 0;
        }
        if (CurrentAlphaState[i] == 1) {
            alphaAccess[i] = 0;
        }
    }
    if (pos != 0x63) {
        for(i = pos; i < CompiledTEVCounter; i++) {
            if (StageNotEmpty(&CompiledTEVColorPool[i]) != 0) {
                for(j = 0; j < 4; j++) {
                    if (CompiledTEVColorPool[i].TEVArg[j] >= 0 && CompiledTEVColorPool[i].TEVArg[j] <= 3) {
                        if (colorAccess[CompiledTEVColorPool[i].TEVArg[j]] == 0x63) {
                            colorAccess[CompiledTEVColorPool[i].TEVArg[j]] = 0;
                        }
                    } else if (CompiledTEVColorPool[i].TEVArg[j] >= 4 && CompiledTEVColorPool[i].TEVArg[j] <= 7) {
                        if (colorAccess[CompiledTEVColorPool[i].TEVArg[j] - 4] == 0x63) {
                            colorAccess[CompiledTEVColorPool[i].TEVArg[j] - 4] = 0;
                        }
                    }
                }
                if (colorAccess[CompiledTEVColorPool[i].out_reg] == 0x63) {
                    colorAccess[CompiledTEVColorPool[i].out_reg] = 1;
                }
            }
            if (StageNotEmpty(&CompiledTEVAlphaPool[i]) != 0) {
                for(j = 0; j < 4; j++) {
                    if (CompiledTEVAlphaPool[i].TEVArg[j] >= 4 && CompiledTEVAlphaPool[i].TEVArg[j] <= 7) {
                        if (alphaAccess[CompiledTEVAlphaPool[i].TEVArg[j] - 4] == 0x63) {
                            alphaAccess[CompiledTEVAlphaPool[i].TEVArg[j] - 4] = 0;
                        }
                    }
                }
                if (alphaAccess[CompiledTEVAlphaPool[i].out_reg] == 0x63) {
                    alphaAccess[CompiledTEVAlphaPool[i].out_reg] = 1;
                }
            }
        }
        if (colorAccess[0] == 0x63) {
            colorAccess[0] = 0;
        }
        if (alphaAccess[0] == 0x63) {
            alphaAccess[0] = 0;
        }
    }
    for(i = 0; i < 4; i++) {
        if (colorAccess[i] != 0) {
            color[i] = 0;
        } else {
            color[i] = 2;
        }
        if (alphaAccess[i] != 0) {
            alpha[i] = 0;
        } else {
            alpha[i] = 2;
        }
    }
}

static int StageNotEmpty(SHDRStage *stage) {
    if (stage->TEVArg[1] == 0xC9 && stage->TEVArg[2] == 0xC9 && stage->TEVArg[3] == 0xC9) {
        return 0;
    }
    return 1;
}

static void FindOutputRegs(u8 earliest, u8 latest, enum SHADER_COLOR_TYPE *regRemap, enum SHADER_REG *startColor, enum SHADER_REG *startAlpha) {
    u8 colorAccess[4] = { 0x63, 0x63, 0x63, 0x63 };
    u8 alphaAccess[4] = { 0x63, 0x63, 0x63, 0x63 };
    u8 i;
    u8 j;

    for(i = 0; i <= 4; i++) {
        if (startColor[i]) {
            colorAccess[i] = 0;
        }
        if (startAlpha[i]) {
            alphaAccess[i] = 0;
        }
    }
    if (earliest != 0x63) {
        for(i = earliest; i <= latest; i++) {
            if (StageNotEmpty(&CompiledTEVColorPool[i])) {
                for(j = 0; j < 4; j++) {
                    if (CompiledTEVColorPool[i].TEVArg[j] >= 0 && CompiledTEVColorPool[i].TEVArg[j] <= 3) {
                        colorAccess[CompiledTEVColorPool[i].TEVArg[j]] = 0;
                    } else if (CompiledTEVColorPool[i].TEVArg[j] >= 4 && CompiledTEVColorPool[i].TEVArg[j] <= 7) {
                        colorAccess[CompiledTEVColorPool[i].TEVArg[j] - 4] = 0;
                    }
                }
                colorAccess[CompiledTEVColorPool[i].out_reg] = 0;
            }
            if (StageNotEmpty(&CompiledTEVAlphaPool[i])) {
                for(j = 0; j < 4; j++) {
                    if (CompiledTEVAlphaPool[i].TEVArg[j] >= 4 && CompiledTEVAlphaPool[i].TEVArg[j] <= 7) {
                        alphaAccess[CompiledTEVAlphaPool[i].TEVArg[j] - 4] = 0;
                    }
                }
                alphaAccess[CompiledTEVAlphaPool[i].out_reg] = 0;
            }
        }
    }
    for(i = 0; i <= 4; i++) {
        if (colorAccess[i] == 0x63) {
            regRemap[0] = i;
            startColor[i] = 2;
            break;
        }
    }
    if (i == 4) {
        OSPanic(__FILE__, 0x324, "Cannot find color output register");
    }
    for(i = 0; i <= 4; i++) {
        if (alphaAccess[i] == 0x63) {
            regRemap[4] = i + 4;
            startAlpha[i] = 2;
            break;
        }
    }
    if (i == 4) {
        OSPanic(__FILE__, 0x330, "Cannot find alpha output register");
    }
}

static int RemapRegisters(enum SHADER_COLOR_TYPE *regRemap, enum SHADER_REG *tempColorRegUsage, enum SHADER_REG *tempAlphaRegUsage, enum SHADER_REG *colorRegUsage, enum SHADER_REG *alphaRegUsage) {
    u8 i;
    u8 j;

    for(i = 1; i < 4; i++) {
        if (colorRegUsage[i] == 2) {
            for(j = 0; j < 4; j++) {
                if (tempColorRegUsage[j] == 0) {
                    regRemap[i] = j;
                    tempColorRegUsage[j] = 2;
                    break;
                }
            }
            if (j == 4) {
                return 0;
            }
        }
        if (alphaRegUsage[i] == 2) {
            for(j = 0; j < 4; j++) {
                if (tempAlphaRegUsage[j] == 0) {
                    regRemap[i + 4] = j + 4;
                    tempAlphaRegUsage[j] = 2;
                    break;
                }
            }
            if (j == 4) {
                return 0;
            }
        }
    }
    return 1;
}

static void PrepareTevPool(u8 earliest, u8 numStages) {
    s8 i;

    if ((numStages + CompiledTEVCounter) > 16) {
        OSPanic(__FILE__, 0x363, "too many TEV stages - PrepareTevPool");
    }
    if (earliest != 0x63) {
        for(i = CompiledTEVCounter - 1; i >= (s8)earliest; i--) {
            CopyCompiledStage(&CompiledTEVColorPool[i], &CompiledTEVColorPool[i + numStages]);
            CopyCompiledStage(&CompiledTEVAlphaPool[i], &CompiledTEVAlphaPool[i + numStages]);
        }
    }
    CompiledTEVCounter += numStages;
}

static void CopyCompiledStage(SHDRStage *src, SHDRStage *dst) {
    u8 i;

    for(i = 0; i < 4; i++) {
        dst->TEVArg[i] = src->TEVArg[i];
    }
    dst->op = src->op;
    dst->bias = src->bias;
    dst->scale = src->scale;
    dst->clamp = src->clamp;
    dst->mode = src->mode;
    dst->out_reg = src->out_reg;
    dst->rasInput = src->rasInput;
    dst->texGenIdx = src->texGenIdx;
    dst->texInput = src->texInput;
}

static void InsertTevStages(u8 pos, SHDRInfo *shader, enum SHADER_TEX *texRemap, enum SHADER_RAS *rasRemap, enum SHADER_COLOR *colorRemap, enum SHADER_COMPLEX *complexRemap, enum SHADER_COLOR_TYPE *regRemap, u8 texOffset) {
    SHDRStage *colorStages;
    SHDRStage *alphaStages;
    u8 i;

    colorStages = shader->colorStages;
    alphaStages = shader->alphaStages;
    for(i = 0; i < shader->numStages; i++) {
        RemapStage(&colorStages[i], &CompiledTEVColorPool[i + pos], texRemap, rasRemap, colorRemap, complexRemap, regRemap, texOffset);
        RemapStage(&alphaStages[i], &CompiledTEVAlphaPool[i + pos], texRemap, rasRemap, colorRemap, complexRemap, regRemap, texOffset);
    }
}

static void RemapStage(SHDRStage *src, SHDRStage *dst, enum SHADER_TEX * texRemap, enum SHADER_RAS *rasRemap, enum SHADER_COLOR *colorRemap, enum SHADER_COMPLEX *complexRemap, enum SHADER_COLOR_TYPE *regRemap, u8 texOffset) {
    u8 i;

    for(i = 0; i < 4; i++) {
        if (src->TEVArg[i] <= 7) {
            dst->TEVArg[i] = regRemap[src->TEVArg[i]];
        } else if (src->TEVArg[i] >= 0x19 && src->TEVArg[i] <= 0x2A) {
            if (src->TEVArg[i] >= 0x23) {
                dst->TEVArg[i] = complexRemap[(int)(src->TEVArg[i] - 0x23)] + 0x23;
            } else {
                dst->TEVArg[i] = complexRemap[(int)(src->TEVArg[i] - 0x19)] + 0x19;
            }
        } else if (src->TEVArg[i] >= 0x32 && src->TEVArg[i] <= 0x3F) {
            if (src->TEVArg[i] >= 0x3C) {
                dst->TEVArg[i] = colorRemap[(int)(src->TEVArg[i] - 0x3C)] + 0x3C;
            } else {
                dst->TEVArg[i] = colorRemap[(int)(src->TEVArg[i] - 0x32)] + 0x32;
            }
        } else {
            dst->TEVArg[i] = src->TEVArg[i];
        }
    }
    dst->op = src->op;
    dst->bias = src->bias;
    dst->scale = src->scale;
    dst->clamp = src->clamp;
    dst->mode = src->mode;
    dst->out_reg = regRemap[src->out_reg];
    if (src->rasInput == 2) {
        dst->rasInput = 2;
    } else {
        dst->rasInput = rasRemap[src->rasInput];
    }
    if (src->texGenIdx == 0x63) {
        dst->texGenIdx = 0x63;
    } else {
        dst->texGenIdx = (src->texGenIdx + texOffset);
    }
    if (src->texInput == 8) {
        dst->texInput = 8;
        return;
    }
    dst->texInput = texRemap[src->texInput];
}

static void RemapComplexReferences(SHDRInfo *shader, enum SHADER_COLOR_TYPE *regRemap) {
    u8 i;
    u8 j;
    enum SHADER_COMPLEX input;

    for(i = 0; i < 8; i++) {
        if (CurrentShaderResources.complexData[i] == shader) {
            input = i;
            break;
        }
    }
    if (i == 8) {
        OSPanic(__FILE__, 0x3FE, "Complex input not found - RemapComplexReferences");
    }
    for(i = 0; i < CompiledTEVCounter; i++) {
        for(j = 0; j < 4; j++) {
            if ((int)((u32)input + 0x19) == CompiledTEVColorPool[i].TEVArg[j]) {
                CompiledTEVColorPool[i].TEVArg[j] = regRemap[0];
            } else if ((int)((u32)input + 0x23) == CompiledTEVColorPool[i].TEVArg[j]) {
                CompiledTEVColorPool[i].TEVArg[j] = regRemap[4];
            }
            if ((int)((u32)input + 0x23) == CompiledTEVAlphaPool[i].TEVArg[j]) {
                CompiledTEVAlphaPool[i].TEVArg[j] = regRemap[4];
            }
        }
    }
}
