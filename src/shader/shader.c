#include <dolphin.h>
#include "charPipeline.h"

static SHDRStage ModulateColorTEVStage = {
    // TEVArg[4]
    { SHADER_ZERO, SHADER_TEXC, SHADER_RASC, SHADER_ZERO },
    // op
    { GX_TEV_ADD },
    // bias
    { GX_TB_ZERO },
    // scale
    { GX_CS_SCALE_1 },
    // clamp
    { 1 },
    // mode
    { GX_TC_LINEAR },
    // out_reg
    { SHADER_CPREV },
    // rasInput
    { 0 },
    // texGenIdx
    { 0 },
    // texInput
    { 0 }
};
static SHDRStage ModulateAlphaTEVStage = {
    // TEVArg[4]
    { SHADER_ZERO, SHADER_TEXA, SHADER_RASA, SHADER_ZERO },
    // op
    { GX_TEV_ADD },
    // bias
    { GX_TB_ZERO },
    // scale
    { GX_CS_SCALE_1 },
    // clamp
    { 1 },
    // mode
    { GX_TC_LINEAR },
    // out_reg
    { SHADER_APREV },
    // rasInput
    { SHADER_RAS0 },
    // texGenIdx
    { 0 },
    // texInput
    { SHADER_TEX0 }
};
static SHDRRas ModulateResources = {
    // regColor[4]
    { SHADER_COLOR_NONE, SHADER_COLOR_NONE, SHADER_COLOR_NONE, SHADER_COLOR_NONE },
    // regAlpha[4]
    { SHADER_COLOR_NONE, SHADER_COLOR_NONE, SHADER_COLOR_NONE, SHADER_COLOR_NONE },
    // colorUsed[4]
    { 0, 0, 0, 0 },
    // colorData[4]
    { { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    // textureUsed[8]
    { 1, 0, 0, 0, 0, 0, 0, 0 },
    // textureData[8]
    { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
    // rasUsed[2]
    { 1, 0 },
    // rasData[2]
    { GX_COLOR_NULL, GX_COLOR_NULL },
    // complexUsed[8]
    { 0 },
    // complexData[8]
    { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};
static struct TexCoordExp ModulateTexCoord = {
    // genSrc
    { SHADER_TG_TEX0 },
    // srcShaderIdx
    { 0x63 },
    // type
    { SHADER_TG_MTX2x4 },
    // mtxInput
    { SHADER_IDENTITY },
};
static SHDRExp ModulateTexCoordExp = {
    // expressionArray
    { &ModulateTexCoord },
    // numExpressions
    { 1 },
    // mtxArray[8]
    { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
    // mtxUsed
    { 0, 0, 0, 0, 0, 0, 0, 0 },
};
static SHDRInfo ModulateShader = {
    // colorStages
    { &ModulateColorTEVStage },
    // alphaStages
    { &ModulateAlphaTEVStage },
    // numStages
    { 1 },
    // shaderResources
    { &ModulateResources },
    // texGen
    { &ModulateTexCoordExp },
};
static SHDRStage ReplaceColorTEVStage = {
    // TEVArg[4]
    { SHADER_TEXC, SHADER_ZERO, SHADER_ZERO, SHADER_ZERO },
    // op
    { GX_TEV_ADD },
    // bias
    { GX_TB_ZERO },
    // scale
    { GX_CS_SCALE_1 },
    // clamp
    { 1 },
    // mode
    { GX_TC_LINEAR },
    // out_reg
    { SHADER_CPREV },
    // rasInput
    { SHADER_RASNONE },
    // texGenIdx
    { 0 },
    // texInput
    { SHADER_TEX0 }
};
static SHDRStage ReplaceAlphaTEVStage = {
    // TEVArg[4]
    { SHADER_TEXA, SHADER_ZERO, SHADER_ZERO, SHADER_ZERO },
    // op
    { GX_TEV_ADD },
    // bias
    { GX_TB_ZERO },
    // scale
    { GX_CS_SCALE_1 },
    // clamp
    { 1 },
    // mode
    { GX_TC_LINEAR },
    // out_reg
    { SHADER_APREV },
    // rasInput
    { SHADER_RASNONE },
    // texGenIdx
    { 0 },
    // texInput
    { SHADER_TEX0 }
};
static SHDRRas ReplaceResources = {
    // regColor[4]
    { SHADER_COLOR_NONE, SHADER_COLOR_NONE, SHADER_COLOR_NONE, SHADER_COLOR_NONE },
    // regAlpha[4]
    { SHADER_COLOR_NONE, SHADER_COLOR_NONE, SHADER_COLOR_NONE, SHADER_COLOR_NONE },
    // colorUsed[4]
    { 0, 0, 0, 0 },
    // colorData[4]
    { { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    // textureUsed[8]
    { 1, 0, 0, 0, 0, 0, 0, 0 },
    // textureData[8]
    { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
    // rasUsed[2]
    { 0, 0 },
    // rasData[2]
    { GX_COLOR_NULL, GX_COLOR_NULL },
    // complexUsed[8]
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    // complexData[8]
    { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};
static struct TexCoordExp ReplaceTexCoord = {
    // genSrc
    { SHADER_TG_TEX0 },
    // srcShaderIdx
    { 0x63 },
    // type
    { SHADER_TG_MTX2x4 },
    // mtxInput
    { SHADER_IDENTITY },
};
static SHDRExp ReplaceTexCoordExp = {
    // expressionArray
    { &ReplaceTexCoord },
    // numExpressions
    { 1 },
    // mtxArray[8]
    { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
    // mtxUsed
    { 0, 0, 0, 0, 0, 0, 0, 0 },
};
static SHDRInfo ReplaceShader = {
    // colorStages
    { &ReplaceColorTEVStage },
    // alphaStages
    { &ReplaceAlphaTEVStage },
    // numStages
    { 1 },
    // shaderResources
    { &ReplaceResources },
    // texGen
    { &ReplaceTexCoordExp },
};
static SHDRStage PassThruColorTEVStage = {
    // TEVArg[4]
    { SHADER_RASC, SHADER_ZERO, SHADER_ZERO, SHADER_ZERO },
    // op
    { GX_TEV_ADD },
    // bias
    { GX_TB_ZERO },
    // scale
    { GX_CS_SCALE_1 },
    // clamp
    { 1 },
    // mode
    { GX_TC_LINEAR },
    // out_reg
    { SHADER_CPREV },
    // rasInput
    { SHADER_RAS0 },
    // texGenIdx
    { 0 },
    // texInput
    { SHADER_TEXNONE }
};
static SHDRStage PassThruAlphaTEVStage = {
    // TEVArg[4]
    { SHADER_RASA, SHADER_ZERO, SHADER_ZERO, SHADER_ZERO },
    // op
    { GX_TEV_ADD },
    // bias
    { GX_TB_ZERO },
    // scale
    { GX_CS_SCALE_1 },
    // clamp
    { 1 },
    // mode
    { GX_TC_LINEAR },
    // out_reg
    { SHADER_APREV },
    // rasInput
    { SHADER_RAS0 },
    // texGenIdx
    { 0 },
    // texInput
    { SHADER_TEXNONE }
};
static SHDRRas PassThruResources = {
    // regColor[4]
    { SHADER_COLOR_NONE, SHADER_COLOR_NONE, SHADER_COLOR_NONE, SHADER_COLOR_NONE },
    // regAlpha[4]
    { SHADER_COLOR_NONE, SHADER_COLOR_NONE, SHADER_COLOR_NONE, SHADER_COLOR_NONE },
    // colorUsed[4]
    { 0, 0, 0, 0 },
    // colorData[4]
    { { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    // textureUsed[8]
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    // textureData[8]
    { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
    // rasUsed[2]
    { 1, 0 },
    // rasData[2]
    { GX_COLOR_NULL, GX_COLOR_NULL },
    // complexUsed[8]
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    // complexData[8]
    { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};
static SHDRExp PassThruTexCoordExp = {
    // expressionArray
    { NULL },
    // numExpressions
    { 0 },
    // mtxArray[8]
    { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
    // mtxUsed
    { 0, 0, 0, 0, 0, 0, 0, 0 },
};
static SHDRInfo PassThruShader = {
    // colorStages
    { &PassThruColorTEVStage },
    // alphaStages
    { &PassThruAlphaTEVStage },
    // numStages
    { 1 },
    // shaderResources
    { &PassThruResources },
    // texGen
    { &PassThruTexCoordExp },
};
static struct SHDRShader internalShader1 = {
    // type
    { SHADER_TYPE_CONSTANT },
    // channel
    { SHADER_CHANNEL_TRIVIAL },
    // referenceCount
    { 0 },
    // TEVStage
    { NULL },
    // shaderParams.complexShader.arg
    { SHADER_ONE },
};
static struct SHDRShader internalShader2 = {
    // type
    { SHADER_TYPE_CONSTANT },
    // channel
    { SHADER_CHANNEL_TRIVIAL },
    // referenceCount
    { 0 },
    // TEVStage
    { NULL },
    // shaderParams.complexShader.arg
    { SHADER_ZERO },
};
static struct SHDRShader internalShader3 = {
    // type
    { SHADER_TYPE_CONSTANT },
    // channel
    { SHADER_CHANNEL_TRIVIAL },
    // referenceCount
    { 0 },
    // TEVStage
    { NULL },
    // shaderParams.complexShader.arg
    { SHADER_HALF },
};
static struct SHDRShader internalShader4 = {
    // type
    { SHADER_TYPE_CONSTANT },
    // channel
    { SHADER_CHANNEL_TRIVIAL },
    // referenceCount
    { 0 },
    // TEVStage
    { NULL },
    // shaderParams.complexShader.arg
    { SHADER_QUARTER },
};

SHDRInfo *SHDRPassThruShader = &PassThruShader;
SHDRInfo *SHDRReplaceShader = &ReplaceShader;
SHDRInfo *SHDRModulateShader = &ModulateShader;
struct SHDRShader * ShaderOne = &internalShader1;
struct SHDRShader * ShaderZero = &internalShader2;
struct SHDRShader * ShaderHalf = &internalShader3;
struct SHDRShader * ShaderQuarter = &internalShader4;

struct SHDRShader *SHDRCreateTexture(enum SHADER_TEX tex, struct SHDRTexCoord * texCoordShader, enum SHADER_CHANNEL channel) {
    struct SHDRShader * temp;

    temp = OSAllocFromHeap(__OSCurrHeap, 0x30);
    temp->type = 0;
    temp->channel = channel;
    temp->referenceCount = 0;
    temp->TEVStage = 0;
    temp->shaderParams.textureShader.tex = tex;
    temp->shaderParams.textureShader.texCoordShader = texCoordShader;
    texCoordShader->referenceCount+=1;
    return temp;
}

struct SHDRShader *SHDRCreateRasterized(enum SHADER_RAS rasColor, enum SHADER_CHANNEL channel) {
    struct SHDRShader * temp;

    temp = OSAllocFromHeap(__OSCurrHeap, 0x30);
    temp->type = 2;
    temp->channel = channel;
    temp->referenceCount = 0;
    temp->TEVStage = 0;
    temp->shaderParams.rasterizedShader.rasColor = rasColor;
    return temp;    
}

struct SHDRShader *SHDRCreateColor(enum SHADER_COLOR color, enum SHADER_CHANNEL channel) {
    struct SHDRShader * temp;

    temp = OSAllocFromHeap(__OSCurrHeap, 0x30);
    temp->type = 1;
    temp->channel = channel;
    temp->referenceCount = 0;
    temp->TEVStage = 0;
    temp->shaderParams.colorShader.color = color;
    return temp;
}

struct SHDRShader *SHDRCreateComplexInput(enum SHADER_COMPLEX input, enum SHADER_CHANNEL channel) {
    struct SHDRShader * temp;

    temp = OSAllocFromHeap(__OSCurrHeap, 0x30);
    temp->type = 4;
    temp->channel = channel;
    temp->referenceCount = 0;
    temp->TEVStage = 0;
    temp->shaderParams.complexInputShader.input = input;
    return temp;
}

struct SHDRShader *SHDRCreateComplex(struct SHDRShader * input1, struct SHDRShader * input2, struct SHDRShader * input3, struct SHDRShader * input4, enum SHADER_OP op, enum SHADER_CLAMP clamp, enum SHADER_BIAS bias, enum SHADER_SCALE scale, enum SHADER_CHANNEL channel) {
    struct SHDRShader * temp;
    
    ASSERTMSGLINE(0xAC, (channel != SHADER_CHANNEL_A) || (input1->channel >= SHADER_CHANNEL_A && input2->channel >= SHADER_CHANNEL_A && input3->channel >= SHADER_CHANNEL_A && input4->channel >= SHADER_CHANNEL_A), "all input into a complex shader of type SHADER_CHANNEL_A must also be of type SHADER_CHANNEL_A");
    temp = OSAllocFromHeap(__OSCurrHeap, 0x30);
    temp->type = 5;
    temp->channel = channel;
    temp->referenceCount = 0;
    temp->TEVStage = 0;
    temp->shaderParams.complexShader.input1 = input1;
    temp->shaderParams.complexShader.input2 = input2;
    temp->shaderParams.complexShader.input3 = input3;
    temp->shaderParams.complexShader.input4 = input4;
    temp->shaderParams.complexShader.op = op;
    temp->shaderParams.complexShader.clamp = clamp;
    temp->shaderParams.complexShader.bias = bias;
    temp->shaderParams.complexShader.scale = scale;
    if (input1->type != 3) {
        input1->referenceCount++;
    }
    if (input2->type != 3) {
        input2->referenceCount++;
    }
    if (input3->type != 3) {
        input3->referenceCount++;
    }
    if (input4->type != 3) {
        input4->referenceCount++;
    }
    return temp;
}

#pragma push
#pragma dont_inline on
// dont recursively inline
void SHDRFree(struct SHDRShader * shader) {
    if (shader && (shader->type != SHADER_TYPE_CONSTANT)) {
        shader->referenceCount--;
        if (shader->referenceCount < 1) {
            if (shader->type == SHADER_TYPE_COMPLEX) {
                SHDRFree(shader->shaderParams.complexShader.input1);
                SHDRFree(shader->shaderParams.complexShader.input2);
                SHDRFree(shader->shaderParams.complexShader.input3);
                SHDRFree(shader->shaderParams.complexShader.input4);
            }
            if ((shader->type == SHADER_TYPE_TEXTURE) && (shader->referenceCount < 1)) {
                SHDRFreeTC(shader->shaderParams.textureShader.texCoordShader);
            }
            OSFreeToHeap(__OSCurrHeap, shader);
        }
    }
}
#pragma pop

SHDRInfo *SHDRCompile(struct SHDRShader * rgbShader, struct SHDRShader * aShader) {
    SHDRInfo *compiledShader;

    if (!rgbShader) {
        rgbShader = ShaderOne;
    }
    if (!aShader) {
        aShader = ShaderOne;
    }
    ASSERTMSGLINE(0xEE, rgbShader->channel != SHADER_CHANNEL_A, "rgbShader needs to be of color type");
    ASSERTMSGLINE(0xEF, aShader->channel == 4 || aShader->type == 3, "aShader needs to be of alpha type");
    if (rgbShader->type != 3) {
        rgbShader->referenceCount++;
    }
    if (aShader->type != 3) {
        aShader->referenceCount++;
    }
    ResetShaderResources();
    compiledShader = OSAllocFromHeap(__OSCurrHeap, 0x14);
    compiledShader->texGen = NULL;
    compiledShader->colorStages = NULL;
    compiledShader->alphaStages = NULL;
    if (BuildTEVTree(rgbShader, aShader, &compiledShader->texGen) == 0) {
        if (compiledShader->texGen) {
            OSFreeToHeap(__OSCurrHeap, compiledShader->texGen);
        }
        OSFreeToHeap(__OSCurrHeap, compiledShader);
        return NULL;
    }
    PruneTEVTree();
    if (BestNumTEVStages == 0x63) {
        if (compiledShader->texGen) {
            OSFreeToHeap(__OSCurrHeap, compiledShader->texGen);
        }
        OSFreeToHeap(__OSCurrHeap, compiledShader);
        return NULL;
    }
    compiledShader->colorStages = OSAllocFromHeap(__OSCurrHeap, BestNumTEVStages * 0x34);
    compiledShader->alphaStages = OSAllocFromHeap(__OSCurrHeap, BestNumTEVStages * 0x34);
    compiledShader->numStages = BestNumTEVStages;
    compiledShader->shaderResources = OSAllocFromHeap(__OSCurrHeap, 0x90);
    InitCompiledResourceInfo(compiledShader->shaderResources);
    CopyCompiledTEVStages(compiledShader->colorStages, BestColorInstructions, BestNumTEVStages, compiledShader->shaderResources);
    CopyCompiledTEVStages(compiledShader->alphaStages, BestAlphaInstructions, BestNumTEVStages, compiledShader->shaderResources);
    return compiledShader;
}

void SHDRExecute(SHDRInfo *shader) {
    CheckShaderBindings(shader);
    CombineTEVStages(shader);
}

void SHDRBindTexture(SHDRInfo *shader, enum SHADER_TEX tex, GXTexObj *texObj) {
    SHDRRas *resources;

    resources = shader->shaderResources;
    if (!resources->textureUsed[tex]) {
        OSPanic(__FILE__, 0x142, "Error, trying to bind a texture to an unused texture input - SHDRBindTexture");
    }
    if (texObj) {
        resources->textureUsed[tex] = 2;
        resources->textureData[tex] = texObj;
        return;
    }
    resources->textureUsed[tex] = 1;
    resources->textureData[tex] = NULL;
}

void SHDRBindRasterized(SHDRInfo *shader, enum SHADER_RAS rasColor, GXChannelID channel) {
    SHDRRas *resources;

    resources = shader->shaderResources;
    if (!resources->rasUsed[rasColor]) {
        OSPanic(__FILE__, 0x156, "Error, trying to bind a rasterized color to an unused rasterized input - SHDRBindRasterized");
    }
    resources->rasUsed[rasColor] = 2;
    resources->rasData[rasColor] = channel;
}

void SHDRBindColor(SHDRInfo *shader, enum SHADER_COLOR colorInput, GXColor color) {
    SHDRRas *resources;

    resources = shader->shaderResources;
    if (!resources->colorUsed[colorInput]) {
        OSPanic(__FILE__, 0x162, "Error, trying to bind a color to an unused rasterized input - SHDRBindColor");
    }
    resources->colorUsed[colorInput] = 2;
    resources->colorData[colorInput] = color;
}

void SHDRBindComplexInput(SHDRInfo *shader, enum SHADER_COMPLEX input, SHDRInfo *inputShader) {
    SHDRRas *resources;

    resources = shader->shaderResources;
    if (!resources->complexUsed[input]) {
        OSPanic(__FILE__, 0x16E, "Error, trying to bind a complex shader to an unused complex input - SHDRBindComplexInput");
    }
    if (inputShader) {
        resources->complexUsed[input] = 2;
        resources->complexData[input] = inputShader;
        return;
    }
    resources->complexUsed[input] = 1;
    resources->complexData[input] = NULL;
}

void SHDRBindTexGenMtx(SHDRInfo *shader, enum SHADER_MTX input, MtxPtr mtxData) {
    SHDRExp *tcExp;
    void *temp;

    tcExp = shader->texGen;
    if (!tcExp->mtxUsed[input]) {
        OSPanic(__FILE__, 0x183, "Error, trying to bind a mtx to an unused mtx input - SHDRBindTexGenMtx");
    }
    if (tcExp->mtxUsed[input] != 0) {
        tcExp->mtxUsed[input] = 2;
        temp = tcExp->mtxArray[input];
        MTXCopy(mtxData, temp);
    }
}
