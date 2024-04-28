#include <dolphin.h>
#include "shader.h"

// functions
static void PrintTEVStage(SHDRStage *stage);
static void PrintTEVArgs(enum SHADER_COLOR_TYPE arg);
static void PrintResources(SHDRRas *res, SHDRExp *tcExp);

void PrintTEVTree(SHDRInfo *shader) {
    SHDRStage *color;
    SHDRStage *alpha;
    SHDRRas *resources;
    SHDRExp *tcExp;
    u8 i;

    color = shader->colorStages;
    alpha = shader->alphaStages;
    resources = shader->shaderResources;
    tcExp = shader->texGen;
    for(i = 0; i < shader->numStages; i++) {
        OSReport("------------------------------------------\n");
        OSReport("Stage number %d \n", i);
        OSReport("------------------------------------------\n\n");
        OSReport("COLOR STAGE: \n");
        PrintTEVStage(&color[i]);
        OSReport("ALPHA STAGE: \n");
        PrintTEVStage(&alpha[i]);
    }
    PrintResources(resources, tcExp);
}

static void PrintTEVStage(SHDRStage *stage) {
    u8 i;

    OSReport("stage ID = %x \n\n", stage);
    for(i = 0; i < 4; i++) {
        OSReport("Shader Argument %d is ", i);
        PrintTEVArgs(stage->TEVArg[i]);
    }
    OSReport("op = %d\n", stage->op);
    OSReport("bias = %d\n", stage->bias);
    OSReport("scale = %d\n", stage->scale);
    OSReport("clamp = %d\n", stage->clamp);
    OSReport("clamp mode = %d\n", stage->mode);
    OSReport("out_reg = ");
    PrintTEVArgs(stage->out_reg);
    OSReport("tex exp index = %d\n", stage->texGenIdx);
    OSReport("tex input = %d\n", stage->texInput);
    OSReport("ras input = %d\n\n\n\n", stage->rasInput);
}

static void PrintTEVArgs(enum SHADER_COLOR_TYPE arg) {
    switch(arg) {
        case SHADER_C0:
            OSReport("SHADER_C0\n");
            return;
        case SHADER_A0:
            OSReport("SHADER_A0\n");
            return;
        case SHADER_C1:
            OSReport("SHADER_C1\n");
            return;
        case SHADER_A1:
            OSReport("SHADER_A1\n");
            return;
        case SHADER_C2:
            OSReport("SHADER_C2\n");
            return;
        case SHADER_A2:
            OSReport("SHADER_A2\n");
            return;
        case SHADER_CPREV:
            OSReport("SHADER_CPREV\n");
            return;
        case SHADER_APREV:
            OSReport("SHADER_APREV\n");
            return;
        case SHADER_TEXC:
            OSReport("SHADER_TEXC\n");
            return;
        case SHADER_TEXA:
            OSReport("SHADER_TEXA\n");
            return;
        case SHADER_RASC:
            OSReport("SHADER_RASC\n");
            return;
        case SHADER_RASA:
            OSReport("SHADER_RASA\n");
            return;
        case SHADER_TEXRRR:
            OSReport("SHADER_TEXRRR\n");
            return;
        case SHADER_TEXGGG:
            OSReport("SHADER_TEXGGG\n");
            return;
        case SHADER_TEXBBB:
            OSReport("SHADER_TEXBBB\n");
            return;
        case SHADER_HALF:
            OSReport("SHADER_HALF\n");
            return;
        case SHADER_QUARTER:
            OSReport("SHADER_QUARTER\n");
            return;
        case SHADER_NONTRIVIAL:
            OSReport("SHADER_NONTRIVIAL\n");
            return;
        case SHADER_ONE:
            OSReport("SHADER_ONE\n");
            return;
        case SHADER_ZERO:
            OSReport("SHADER_ZERO\n");
            return;
        case SHADER_COMPLEXINPUT0_RGB:
        case SHADER_COMPLEXINPUT1_RGB:
        case SHADER_COMPLEXINPUT2_RGB:
        case SHADER_COMPLEXINPUT3_RGB:
        case SHADER_COMPLEXINPUT4_RGB:
        case SHADER_COMPLEXINPUT5_RGB:
        case SHADER_COMPLEXINPUT6_RGB:
        case SHADER_COMPLEXINPUT7_RGB:
            OSReport("complex color input\n");    
            break;
        case SHADER_COMPLEXINPUT0_A:
        case SHADER_COMPLEXINPUT1_A:
        case SHADER_COMPLEXINPUT2_A:
        case SHADER_COMPLEXINPUT3_A:
        case SHADER_COMPLEXINPUT4_A:
        case SHADER_COMPLEXINPUT5_A:
        case SHADER_COMPLEXINPUT6_A:
        case SHADER_COMPLEXINPUT7_A:
            OSReport("complex Alpha input\n");
            break;
        case SHADER_COLORINPUT0_RGB:
            OSReport("constant color input 0\n");
            break;
        case SHADER_COLORINPUT1_RGB:
            OSReport("constant color input 1\n");
            break;
        case SHADER_COLORINPUT2_RGB:
            OSReport("constant color input 2\n");
            break;
        case SHADER_COLORINPUT3_RGB:
            OSReport("constant color input 3\n");
            break;
        case SHADER_COLORINPUT0_A:
            OSReport("constant alpha input 0\n");
            break;
        case SHADER_COLORINPUT1_A:
            OSReport("constant alpha input 1\n");
            break;
        case SHADER_COLORINPUT2_A:
            OSReport("constant alpha input 2\n");
            break;
        case SHADER_COLORINPUT3_A:
            OSReport("constant alpha input 3\n");
            break;
    }
}

static void PrintResources(SHDRRas *res, SHDRExp *tcExp) {
    u8 i;

    OSReport("ras colors used\n");
    for(i = 0; i < 2; i++) {
        if (res->rasUsed[i]) {
            OSReport("%d ", i);
        }
    }
    OSReport("\n");
    OSReport("constant colors used\n");
    for(i = 0; i < 4; i++) {
        if (res->colorUsed[i]) {
            OSReport("%d ", i);
        }
    }
    OSReport("\n");
    OSReport("textures used\n");
    for(i = 0; i < 8; i++) {
        if(res->textureUsed[i]) {
            OSReport("%d ", i);
        }
    }
    OSReport("\n");
    OSReport("complex inputs used\n");
    for(i = 0; i < 8; i++) {
        if (res->complexUsed[i]) {
            OSReport("%d ", i);
        }
    }
    OSReport("\n");
    OSReport("Mtx inputs used\n");
    for(i = 0; i < 8; i++) {
        if (tcExp->mtxUsed[i]) {
            OSReport("%d ", i);
        }
    }
    OSReport("\n");
}
