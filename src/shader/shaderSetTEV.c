#include <dolphin.h>
#include "shader.h"

GXTexCoordID coordIDArray[8] = {
    GX_TEXCOORD0,
    GX_TEXCOORD1,
    GX_TEXCOORD2,
    GX_TEXCOORD3,
    GX_TEXCOORD4,
    GX_TEXCOORD5,
    GX_TEXCOORD6,
    GX_TEXCOORD7
};

GXTexMapID mapIDArray[8] = {
    GX_TEXMAP0,
    GX_TEXMAP1,
    GX_TEXMAP2,
    GX_TEXMAP3,
    GX_TEXMAP4,
    GX_TEXMAP5,
    GX_TEXMAP6,
    GX_TEXMAP7
};

static int RAS0used;
static int RAS1used;

// functions
void SetTEV(SHDRInfo *shader);
static void LoadTextures(SHDRRas *res);
static void LoadColors(SHDRRas *res);
static void ShaderSetTEVOrder(SHDRStage *colorStage, SHDRStage *alphaStage, SHDRRas *res, u8 id);
static void ShaderSetTEVClampMode(SHDRStage *colorStage, u8 id);
static void ShaderSetColorTEVStage(SHDRStage *stage, SHDRRas *res, u8 id);
static void ShaderSetAlphaTEVStage(SHDRStage *stage, SHDRRas *res, u8 id);
static GXTevStageID TranslateStageID(u8 id);
static GXTevColorArg TranslateTEVColorArg(enum SHADER_COLOR_TYPE arg, SHDRRas *res);
static GXTevAlphaArg TranslateTEVAlphaArg(enum SHADER_COLOR_TYPE arg, SHDRRas *res);

void SetTEV(SHDRInfo *shader) {
    u8 i;
    SHDRExp *tcExp;

    tcExp = shader->texGen;
    RAS0used = 0;
    RAS1used = 0;
    LoadTextures(shader->shaderResources);
    LoadColors(shader->shaderResources);
    GXSetNumTevStages(shader->numStages);
    for(i = 0; i < shader->numStages; i++) {
        ShaderSetTEVOrder((void*)((u32)shader->colorStages + (i * 0x34)), (void*)((u32)shader->alphaStages + (i * 0x34)), shader->shaderResources, i);
        ShaderSetTEVClampMode((void*)((u32)shader->colorStages + (i * 0x34)), i);
        ShaderSetColorTEVStage((void*)((u32)shader->colorStages + (i * 0x34)), shader->shaderResources, i);
        ShaderSetAlphaTEVStage((void*)((u32)shader->alphaStages + (i * 0x34)), shader->shaderResources, i);
    }
    if (RAS1used != 0) {
        GXSetNumChans(2);
        return;
    }
    if (RAS0used != 0) {
        GXSetNumChans(1);
        return;
    }
    if (tcExp->numExpressions != 0) {
        GXSetNumChans(0);
        return;
    }
    GXSetNumChans(1);
}

static void LoadTextures(SHDRRas *res) {
    u8 i;

    for(i = 0; i < 8; i++) {
        if (res->textureUsed[i]) {
            GXLoadTexObj(res->textureData[i], Resources.mapIdList[i]);
        }
    }
}

static void LoadColors(SHDRRas *res) {
    u8 i;
    GXColor color;
    GXTevRegID id;

    for(i = 0; i < 4; i++) {
        if (res->regColor[i] != 4 || res->regAlpha[i] != 4) {
            if (res->regAlpha[i] != 4) {
                color.a = res->colorData[res->regAlpha[i]].a;
            }
            if (res->regColor[i] != 4) {
                color.r = res->colorData[res->regColor[i]].r;
                color.g = res->colorData[res->regColor[i]].g;
                color.b = res->colorData[res->regColor[i]].b;
            }
            switch (i) {
                case 0:
                    id = GX_TEVPREV;
                    break;
                case 1:
                    id = GX_TEVREG0;
                    break;
                case 2:
                    id = GX_TEVREG1;
                    break;
                case 3:
                    id = GX_TEVREG2;
                    break;
            }
            GXSetTevColor(id, color);
        }
    }
}

static void ShaderSetTEVOrder(SHDRStage *colorStage, SHDRStage *alphaStage, SHDRRas *res, u8 id) {
    GXTexCoordID coord;
    GXTexMapID map;
    GXChannelID color;

    if (colorStage->texGenIdx == 0x63) {
        if (alphaStage->texGenIdx == 0x63) {
            coord = 0xFF;
        } else {
            coord = coordIDArray[alphaStage->texGenIdx];
        }
    } else {
        coord = coordIDArray[colorStage->texGenIdx];
    }
    if (colorStage->texInput == 8) {
        if (alphaStage->texInput == 8) {
            map = 0xFF;
        } else {
            map = mapIDArray[alphaStage->texInput];
        }
    } else {
        map = mapIDArray[colorStage->texInput];
    }
    if (colorStage->rasInput == 2) {
        if (alphaStage->rasInput == 2) {
            color = 0xFF;
        } else {
            color = res->rasData[alphaStage->rasInput];
        }
    } else {
        color = res->rasData[colorStage->rasInput];
    }
    if (color == 4) {
        RAS0used = 1;
    } else if (color == 5) {
        RAS1used = 1;
    }
    GXSetTevOrder(TranslateStageID(id), coord, map, color);
}

static void ShaderSetTEVClampMode(SHDRStage *stage, u8 id) {
    
}

static void ShaderSetColorTEVStage(SHDRStage *stage, SHDRRas *res, u8 id) {
    GXTevRegID reg;

    GXSetTevColorIn(TranslateStageID(id), TranslateTEVColorArg(stage->TEVArg[0], res), TranslateTEVColorArg(stage->TEVArg[1], res), TranslateTEVColorArg(stage->TEVArg[2], res), TranslateTEVColorArg(stage->TEVArg[3], res));
    switch(stage->out_reg) {
        case SHADER_CPREV:
            reg = GX_TEVPREV;
            break;
        case SHADER_C0:
            reg = GX_TEVREG0;
            break;
        case SHADER_C1:
            reg = GX_TEVREG1;
            break;
        case SHADER_C2:
            reg = GX_TEVREG2;
            break;
    }
    GXSetTevColorOp(TranslateStageID(id), stage->op, stage->bias, stage->scale, stage->clamp, reg);
}

static void ShaderSetAlphaTEVStage(SHDRStage *stage, SHDRRas *res, u8 id) {
    GXTevRegID reg;
    
    GXSetTevAlphaIn(TranslateStageID(id), TranslateTEVAlphaArg(stage->TEVArg[0], res), TranslateTEVAlphaArg(stage->TEVArg[1], res), TranslateTEVAlphaArg(stage->TEVArg[2], res), TranslateTEVAlphaArg(stage->TEVArg[3], res));
    switch(stage->out_reg) {
        case SHADER_APREV:
            reg = GX_TEVPREV;
            break;
        case SHADER_A0:
            reg = GX_TEVREG0;
            break;
        case SHADER_A1:
            reg = GX_TEVREG1;
            break;
        case SHADER_A2:
            reg = GX_TEVREG2;
            break;
    }
    GXSetTevAlphaOp(TranslateStageID(id), stage->op, stage->bias, stage->scale, stage->clamp, reg);
}

static GXTevStageID TranslateStageID(u8 id) {
    switch (id) {
        case 0:  return GX_TEVSTAGE0;
        case 1:  return GX_TEVSTAGE1;
        case 2:  return GX_TEVSTAGE2;
        case 3:  return GX_TEVSTAGE3;
        case 4:  return GX_TEVSTAGE4;
        case 5:  return GX_TEVSTAGE5;
        case 6:  return GX_TEVSTAGE6;
        case 7:  return GX_TEVSTAGE7;
        case 8:  return GX_TEVSTAGE8;
        case 9:  return GX_TEVSTAGE9;
        case 10: return GX_TEVSTAGE10;
        case 11: return GX_TEVSTAGE11;
        case 12: return GX_TEVSTAGE12;
        case 13: return GX_TEVSTAGE13;
        case 14: return GX_TEVSTAGE14;
        case 15: return GX_TEVSTAGE15;
    }
    return GX_TEVSTAGE0;
}

static GXTevColorArg TranslateTEVColorArg(enum SHADER_COLOR_TYPE arg, SHDRRas *res) {
    u8 i;

    switch(arg) {
        case SHADER_CPREV:
	        return GX_CC_CPREV;
        case SHADER_C0:
	        return GX_CC_C0;
        case SHADER_C1:
	        return GX_CC_C1;
        case SHADER_C2:
	        return GX_CC_C2;
        case SHADER_APREV:
	        return GX_CC_APREV;
        case SHADER_A0:
	        return GX_CC_A0;
        case SHADER_A1:
	        return GX_CC_A1;
        case SHADER_A2:
	        return GX_CC_A2;
        case SHADER_TEXC:
	        return GX_CC_TEXC;
        case SHADER_TEXA:
	        return GX_CC_TEXA;
        case SHADER_RASC:
	        return GX_CC_RASC;
        case SHADER_RASA:
	        return GX_CC_RASA;
        case SHADER_TEXRRR:
	        return GX_CC_TEXRRR;
        case SHADER_TEXGGG:
	        return GX_CC_TEXGGG;
        case SHADER_TEXBBB:
	        return GX_CC_TEXBBB;
        case SHADER_HALF:
	        return GX_CC_HALF;
        case SHADER_QUARTER:
	        return GX_CC_QUARTER;
        case SHADER_ONE:
	        return GX_CC_ONE;  
        case SHADER_ZERO:
	        return GX_CC_ZERO;
        case SHADER_COLORINPUT0_RGB:
        case SHADER_COLORINPUT1_RGB:
        case SHADER_COLORINPUT2_RGB:
        case SHADER_COLORINPUT3_RGB: 
            for(i = 0; i < 4; i++) {
                if (res->regColor[i] == (arg - 50)) {
                    return TranslateTEVColorArg(i, res);
                }
            }
            OSPanic(__FILE__, 0x1EA, "constant color not found - TranslateTEVColorArg");
            break;
        case SHADER_COLORINPUT0_A:
        case SHADER_COLORINPUT1_A:
        case SHADER_COLORINPUT2_A:
        case SHADER_COLORINPUT3_A:
            for(i = 0; i < 4; i++) {
                if (res->regAlpha[i] == (arg - 60)) {
                    return TranslateTEVColorArg(i + 4, res);
                }
            }
            OSPanic(__FILE__, 0x1F7, "constant alpha not found - TranslateTEVColorArg");
            break;
        default:
            ASSERTMSGLINE(0x1FB, 0, "unknown color arg type in shader compilation");
            break;
    }
    return GX_CC_ZERO;
}

static GXTevAlphaArg TranslateTEVAlphaArg(enum SHADER_COLOR_TYPE arg, SHDRRas *res) {
    u8 i;

    switch(arg) {
        case SHADER_APREV:
            return GX_CA_APREV;
        case SHADER_A0:
            return GX_CA_A0;
        case SHADER_A1:
            return GX_CA_A1;
        case SHADER_A2:
            return GX_CA_A2;
        case SHADER_TEXA:
            return GX_CA_TEXA;
        case SHADER_RASA:
            return GX_CA_RASA;
        case SHADER_ONE:
            return GX_CA_ONE;  
        case SHADER_ZERO:
            return GX_CA_ZERO;
        case SHADER_COLORINPUT0_A:
        case SHADER_COLORINPUT1_A:
        case SHADER_COLORINPUT2_A:
        case SHADER_COLORINPUT3_A:
            for(i = 0; i < 4; i++) {
                if (res->regAlpha[i] == (arg - 60)) {
                    return TranslateTEVAlphaArg(i + 4, res);
                }
            }
            OSPanic(__FILE__, 0x223, "constant alpha not found - TranslateTEVAlphaArg");
            break;
        default:
            ASSERTMSGLINE(0x227, 0, "unknown alpha arg type in shader compilation");
            break;
    }
    return GX_CA_ZERO;
}
