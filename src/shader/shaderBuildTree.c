#include <dolphin.h>
#include "shader.h"

static struct SHDRShader tempRGBRoot = {
    // type
    { SHADER_TYPE_COMPLEX },
    // channel
    { SHADER_CHANNEL_RGB },
    // referenceCount
    { 0 },
    // TEVStage
    { NULL },
    // shaderParams.constantShader.arg
    { SHADER_ONE }
};

static struct SHDRShader tempARoot = {
    // type
    { SHADER_TYPE_COMPLEX },
    // channel
    { SHADER_CHANNEL_A },
    // referenceCount
    { 0 },
    // TEVStage
    { NULL },
    // shaderParams.constantShader.arg
    { SHADER_ONE }
};

static struct SHDRShader connector = {
    // type
    { SHADER_TYPE_EMPTY },
    // channel
    { SHADER_CHANNEL_RGB },
    // referenceCount
    { 0 },
    // TEVStage
    { NULL },
    // shaderParams.constantShader.arg
    { SHADER_ONE }
};

struct SHDRShader * RGBRoot;

// functions
int BuildTEVTree(struct SHDRShader * rgbShader, struct SHDRShader * aShader, void * compiledTexGen);
static struct SHDRShader * CheckShaderRoot(struct SHDRShader * rgbShader, struct SHDRShader * aShader);
static struct ShaderTEVStage * AllocateStage(struct SHDRShader * shader);
static void LinkParentToChild(struct ShaderTEVStage * parent, u8 index, struct ShaderTEVStage * child);
static void AllocateEmptyStage(struct SHDRShader * shader);
static void AllocateComplexStage(struct ShaderTEVStage * parent, u8 index, struct SHDRShader * shader);
static void AllocateRasterizedStage(struct ShaderTEVStage * parent, u8 index, struct SHDRShader * shader);
static void AllocateTextureStage(struct ShaderTEVStage * parent, u8 index, struct SHDRShader * shader);
static void CollapseShaders(struct ShaderTEVStage * parent, u8 index, struct SHDRShader * shader, u8 numTex, u8 numRas);
static void CollapseConstantShader(struct ShaderTEVStage * parent, u8 index, struct SHDRShader * shader);
static void CollapseComplexInputStage(struct ShaderTEVStage * parent, u8 index, struct SHDRShader * shader);
static void CollapseColorShader(struct ShaderTEVStage * parent, u8 index, struct SHDRShader * shader);
static void CollapseRasterizedShader(struct ShaderTEVStage * parent, u8 index, struct SHDRShader * shader, u8 numRas);
static void CollapseTextureShader(struct ShaderTEVStage * parent, u8 index, struct SHDRShader * shader, u8 numTex);
static enum SHADER_COLOR_TYPE TranslateChannel(enum SHADER_CHANNEL channel);
static void TranslateClampMode(enum SHADER_CLAMP clamp, GXTevClampMode *mode, u8 *clampEnable);
static void CleanUpTree(struct SHDRShader * shader);
static void InitNonAllocatedTevStages(void);
static void AddAlphaParent(struct ShaderTEVStage * stage, struct ShaderTEVStage * parent);
static void CheckAlphaStage(struct ShaderTEVStage * stage);
static int CheckColorInput(enum SHADER_COLOR_TYPE arg);
static void CountTexNRasStages(struct SHDRShader * shader, u8 * numTex, u8 * numRas);

int BuildTEVTree(struct SHDRShader * rgbShader, struct SHDRShader * aShader, void * compiledTexGen) {
    struct SHDRShader * shadeTree;

    shadeTree = CheckShaderRoot(rgbShader, aShader);
    CompileTexGen(shadeTree, compiledTexGen);
    AllocateEmptyStage(shadeTree);
    InitNonAllocatedTevStages();
    CleanUpTree(shadeTree);
    return 1;
}

static struct SHDRShader * CheckShaderRoot(struct SHDRShader * rgbShader, struct SHDRShader * aShader) {
    struct SHDRShader * rgb;
    struct SHDRShader * alpha;

    rgb = rgbShader;
    alpha = aShader;
    if (rgbShader->type != SHADER_TYPE_COMPLEX) {
        tempRGBRoot.TEVStage = NULL;
        tempRGBRoot.shaderParams.complexShader.input1 = rgbShader;
        tempRGBRoot.shaderParams.complexShader.input2 = ShaderZero;
        tempRGBRoot.shaderParams.complexShader.input3 = ShaderZero;
        tempRGBRoot.shaderParams.complexShader.input4 = ShaderZero;
        tempRGBRoot.shaderParams.complexShader.op = SHADER_OP_ADD;
        tempRGBRoot.shaderParams.complexShader.clamp = SHADER_CLAMP_LINEAR_255;
        tempRGBRoot.shaderParams.complexShader.bias = SHADER_BIAS_ZERO;
        tempRGBRoot.shaderParams.complexShader.scale = SHADER_SCALE_1;
        rgb = &tempRGBRoot;
    }
    if (aShader->type != SHADER_TYPE_COMPLEX) {
        tempARoot.TEVStage = NULL;
        tempARoot.shaderParams.complexShader.input1 = aShader;
        tempARoot.shaderParams.complexShader.input2 = ShaderZero;
        tempARoot.shaderParams.complexShader.input3 = ShaderZero;
        tempARoot.shaderParams.complexShader.input4 = ShaderZero;
        tempARoot.shaderParams.complexShader.op = SHADER_OP_ADD;
        tempARoot.shaderParams.complexShader.clamp = SHADER_CLAMP_LINEAR_255;
        tempARoot.shaderParams.complexShader.bias = SHADER_BIAS_ZERO;
        tempARoot.shaderParams.complexShader.scale = SHADER_SCALE_1;
        alpha = &tempARoot;
    }
    connector.shaderParams.complexShader.input1 = rgb;
    connector.shaderParams.complexShader.input2 = alpha;
    connector.shaderParams.complexShader.input3 = NULL;
    connector.shaderParams.complexShader.input4 = NULL;
    RGBRoot = rgb;
    return &connector;
}

static struct ShaderTEVStage * AllocateStage(struct SHDRShader * shader) {
    struct ShaderTEVStage * currentStage;
    u8 i;

    currentStage = &TEVPool[TEVCounter++];
    shader->TEVStage = currentStage;
    if (shader->channel == SHADER_CHANNEL_A) {
        currentStage->channel = SHADER_TEV_ALPHA;
    } else {
        currentStage->channel = SHADER_TEV_COLOR;
    }
    if (shader->type == SHADER_TYPE_EMPTY) {
        currentStage->type = shader->type;
    } else {
        currentStage->type = SHADER_TYPE_COMPLEX;
    }
    for(i = 0; i < 4; i++) {
        currentStage->TEVArg[i] = SHADER_ZERO;
        currentStage->tevStage[i] = NULL;
    }
    currentStage->op = SHADER_OP_ADD;
    currentStage->bias = SHADER_BIAS_ZERO;
    currentStage->scale = SHADER_SCALE_1;
    TranslateClampMode(0, &currentStage->mode, &currentStage->clamp);
    currentStage->texGenIdx = 0x63;
    currentStage->texInput = SHADER_TEXNONE;
    currentStage->rasInput = SHADER_RASNONE;
    currentStage->numParents = 0;
    currentStage->numNonAllocatedChildren = 0;
    return currentStage;
}

static void LinkParentToChild(struct ShaderTEVStage * parent, u8 index, struct ShaderTEVStage * child) {
    if (parent) {
        parent->tevStage[index] = child;
        if (child->channel == 1) {
            parent->TEVArg[index] = 4;
        } else {
            parent->TEVArg[index] = 0;
        }
        child->parent[child->numParents] = parent;
        child->numParents++;
        ASSERTMSGLINE(0xED, child->numParents < 8, "TEV Stage has too many parents!");
    }
}

static void AllocateEmptyStage(struct SHDRShader * shader) {
    struct ShaderTEVStage * currentStage;
    u8 i;

    currentStage = AllocateStage(shader);
    for(i = 0; i < 4; i++) {
        currentStage->TEVArg[i] = 0;
    }
    CollapseShaders(currentStage, 0, shader->shaderParams.complexShader.input1, 0, 0);
    CollapseShaders(currentStage, 1, shader->shaderParams.complexShader.input2, 0, 0);
}

static void AllocateComplexStage(struct ShaderTEVStage * parent, u8 index, struct SHDRShader * shader) {
    struct ShaderTEVStage * currentStage;
    u8 numTextureChildren;
    u8 numRasterizedChildren;

    numTextureChildren = 0;
    numRasterizedChildren = 0;
    if (shader->TEVStage) {
        currentStage = shader->TEVStage;
    } else {
        currentStage = AllocateStage(shader);
        currentStage->op = shader->shaderParams.complexShader.op;
        currentStage->bias = shader->shaderParams.complexShader.bias;
        currentStage->scale = shader->shaderParams.complexShader.scale;
        TranslateClampMode(shader->shaderParams.complexShader.clamp, &currentStage->mode, &currentStage->clamp);
        CountTexNRasStages(shader, &numTextureChildren, &numRasterizedChildren);
        CollapseShaders(currentStage, 0, shader->shaderParams.complexShader.input1, numTextureChildren, numRasterizedChildren);
        CollapseShaders(currentStage, 1, shader->shaderParams.complexShader.input2, numTextureChildren, numRasterizedChildren);
        CollapseShaders(currentStage, 2, shader->shaderParams.complexShader.input3, numTextureChildren, numRasterizedChildren);
        CollapseShaders(currentStage, 3, shader->shaderParams.complexShader.input4, numTextureChildren, numRasterizedChildren);
        CheckAlphaStage(currentStage);
    }
    LinkParentToChild(parent, index, currentStage);
}

static void AllocateRasterizedStage(struct ShaderTEVStage * parent, u8 index, struct SHDRShader * shader) {
    struct ShaderTEVStage * currentStage;

    if (shader->TEVStage) {
        currentStage = shader->TEVStage;
    } else {
        currentStage = AllocateStage(shader);
        if (shader->channel == SHADER_CHANNEL_RGB) {
            currentStage->TEVArg[0] = SHADER_RASC;
        } else {
            currentStage->TEVArg[0] = SHADER_RASA;
        }
        currentStage->rasInput = shader->shaderParams.rasterizedShader.rasColor;
    }
    LinkParentToChild(parent, index, currentStage);
}

static void AllocateTextureStage(struct ShaderTEVStage * parent, u8 index, struct SHDRShader * shader) {
    struct ShaderTEVStage * currentStage;

    if (shader->TEVStage) {
        currentStage = shader->TEVStage;
    } else {
        currentStage = AllocateStage(shader);
        currentStage->TEVArg[0] = TranslateChannel(shader->channel);
        currentStage->texGenIdx = shader->shaderParams.textureShader.texCoordShader->texCoordExpIdx;
        currentStage->texInput = shader->shaderParams.textureShader.tex;
    }
    LinkParentToChild(parent, index, currentStage);
}

static void CollapseShaders(struct ShaderTEVStage * parent, u8 index, struct SHDRShader * shader, u8 numTex, u8 numRas) {
    switch (shader->type) {
        case SHADER_TYPE_TEXTURE:
            CollapseTextureShader(parent, index, shader, numTex);
            return;
        case SHADER_TYPE_COLOR:
            CollapseColorShader(parent, index, shader);
            return;
        case SHADER_TYPE_RASTERIZED:
            CollapseRasterizedShader(parent, index, shader, numRas);
            return;
        case SHADER_TYPE_CONSTANT:
            CollapseConstantShader(parent, index, shader);
            return;
        case SHADER_TYPE_COMPLEXINPUT:
            CollapseComplexInputStage(parent, index, shader);
            return;
        case SHADER_TYPE_COMPLEX:
            AllocateComplexStage(parent, index, shader);
            return;
    }
}

static void CollapseConstantShader(struct ShaderTEVStage * parent, u8 index, struct SHDRShader * shader) {
    parent->tevStage[index] = NULL;
    parent->TEVArg[index] = shader->shaderParams.constantShader.arg;
}

static void CollapseComplexInputStage(struct ShaderTEVStage * parent, u8 index, struct SHDRShader * shader) {
    parent->tevStage[index] = NULL;
    if (shader->channel == 0) {
        parent->TEVArg[index] = shader->shaderParams.complexInputShader.input + 0x19;
        return;
    }
    parent->TEVArg[index] = shader->shaderParams.complexInputShader.input + 0x23;
}

static void CollapseColorShader(struct ShaderTEVStage * parent, u8 index, struct SHDRShader * shader) {
    if (shader->channel == 0) {
        parent->TEVArg[index] = AllocateColorResource(shader->shaderParams.colorShader.color);
    } else {
        parent->TEVArg[index] = AllocateAlphaResource(shader->shaderParams.colorShader.color);
    }
    parent->tevStage[index] = NULL;
}

static void CollapseRasterizedShader(struct ShaderTEVStage * parent, u8 index, struct SHDRShader * shader, u8 numRas) {
    enum SHADER_RAS rasInput;
    enum SHADER_COLOR_TYPE arg;

    rasInput = shader->shaderParams.rasterizedShader.rasColor;
    if ((numRas <= 1) && (parent->rasInput == 2 || parent->rasInput == rasInput)) {
        if (shader->channel == 0) {
            arg = 0xA;
        } else {
            arg = 0xB;
        }
        parent->rasInput = rasInput;
        parent->tevStage[index] = NULL;
        parent->TEVArg[index] = arg;
        return;
    }
    AllocateRasterizedStage(parent, index, shader);
}

static void CollapseTextureShader(struct ShaderTEVStage * parent, u8 index, struct SHDRShader * shader, u8 numTex) {
    enum SHADER_TEX texInput;

    texInput = shader->shaderParams.textureShader.tex;
    if ((numTex <= 1) 
        && ((parent->texInput == 8 && parent->texGenIdx == 0x63) 
        || (parent->texInput == texInput 
        && parent->texGenIdx == shader->shaderParams.textureShader.texCoordShader->texCoordExpIdx))) {
        parent->texInput = texInput;
        parent->texGenIdx = shader->shaderParams.textureShader.texCoordShader->texCoordExpIdx;
        parent->tevStage[index] = NULL;
        parent->TEVArg[index] = TranslateChannel(shader->channel);
        return;
    }
    AllocateTextureStage(parent, index, shader);
}

static enum SHADER_COLOR_TYPE TranslateChannel(enum SHADER_CHANNEL channel) {
    switch (channel) {
        case SHADER_CHANNEL_RGB:
            return SHADER_TEXC;
        case SHADER_CHANNEL_A:
            return SHADER_TEXA;
        case SHADER_CHANNEL_RRR:
            return SHADER_TEXRRR;
        case SHADER_CHANNEL_GGG:
            return SHADER_TEXGGG;
        case SHADER_CHANNEL_BBB:
            return SHADER_TEXBBB;
        default:
            return SHADER_TEXC;
    }    
}

static void TranslateClampMode(enum SHADER_CLAMP clamp, GXTevClampMode *mode, u8 *clampEnable) {
    switch(clamp) {
        case SHADER_CLAMP_LINEAR_1023:
            *mode = GX_TC_LINEAR;
            *clampEnable = 0;
            return;
        case SHADER_CLAMP_LINEAR_255:
            *mode = GX_TC_LINEAR;
            *clampEnable = 1;
            return;
        case SHADER_CLAMP_GE_255:
            *mode = GX_TC_GE;
            *clampEnable = 0;
            return;
        case SHADER_CLAMP_GE_0:
            *mode = GX_TC_GE;
            *clampEnable = 1;
            return;
        case SHADER_CLAMP_EQ_255:
            *mode = GX_TC_EQ;
            *clampEnable = 0;
            return;
        case SHADER_CLAMP_EQ_0:
            *mode = GX_TC_EQ;
            *clampEnable = 1;
            return;
        case SHADER_CLAMP_LE_255:
            *mode = GX_TC_LE;
            *clampEnable = 0;
            return;
        case SHADER_CLAMP_LE_0:
            *mode = GX_TC_LE;
            *clampEnable = 1;
            return;
    }
}

static void CleanUpTree(struct SHDRShader * shader) {
    if (shader) {
        shader->TEVStage = NULL;
        if (shader->type == SHADER_TYPE_COMPLEX || shader->type == SHADER_TYPE_EMPTY) {
            CleanUpTree(shader->shaderParams.complexShader.input1);
            CleanUpTree(shader->shaderParams.complexShader.input2);
            CleanUpTree(shader->shaderParams.complexShader.input3);
            CleanUpTree(shader->shaderParams.complexShader.input4);
        }
    }
}

static void InitNonAllocatedTevStages(void) {
    u8 i;
    u8 j;
    struct ShaderTEVStage * currentStage;

    for(i = TEVCounter; i < 50; i++) {
        currentStage = &TEVPool[i];
        currentStage->channel = 0;
        currentStage->type = 6;
        for(j = 0; j < 4; j++) {
            currentStage->TEVArg[j] = 0xC9;
            currentStage->tevStage[j] = NULL;
        }
        currentStage->op = 0;
        currentStage->bias = 0;
        currentStage->scale = 0;
        TranslateClampMode(1, &currentStage->mode, &currentStage->clamp);
        currentStage->texGenIdx = 0x63;
        currentStage->texInput = 8;
        currentStage->rasInput = 2;
        currentStage->numParents = 0;
    }
}

static void AddAlphaParent(struct ShaderTEVStage * stage, struct ShaderTEVStage * parent) {
    u8 i;

    if (stage->channel == 1) {
        stage->parent[stage->numParents] = parent;
        stage->numParents++;
        ASSERTMSGLINE(0x248, stage->numParents < 8, "TEV Stage has too many parents!");
    } else if (stage->type == 5) {
        for(i = 0; i < 4; i++) {
            if (stage->tevStage[i]) {
                AddAlphaParent(stage->tevStage[i], parent);
            }
        }
    }
}

static void CheckAlphaStage(struct ShaderTEVStage * stage) {
    u32 i;

    for(i = 0; i < 4; i++) {
        if (CheckColorInput(stage->TEVArg[i]) != 0) {
            return;
        }
    }
    if (RGBRoot->TEVStage != stage) {
        stage->channel = 1;
    }
}

static int CheckColorInput(enum SHADER_COLOR_TYPE arg) {
    switch(arg) {
        case SHADER_CPREV:
        case SHADER_C0:
        case SHADER_C1:
        case SHADER_C2:
        case SHADER_TEXC:
        case SHADER_RASC:
        case SHADER_TEXRRR:
        case SHADER_TEXGGG:
        case SHADER_TEXBBB:
        case SHADER_HALF:
        case SHADER_QUARTER:
        case SHADER_COMPLEXINPUT0_RGB:
        case SHADER_COMPLEXINPUT1_RGB:
        case SHADER_COMPLEXINPUT2_RGB:
        case SHADER_COMPLEXINPUT3_RGB:
        case SHADER_COMPLEXINPUT4_RGB:
        case SHADER_COMPLEXINPUT5_RGB:
        case SHADER_COMPLEXINPUT6_RGB:
        case SHADER_COMPLEXINPUT7_RGB:
        case SHADER_COLORINPUT0_RGB:
        case SHADER_COLORINPUT1_RGB:
        case SHADER_COLORINPUT2_RGB:
        case SHADER_COLORINPUT3_RGB:
            return 1;
        default:
            return 0;
    }
}

static void CountTexNRasStages(struct SHDRShader * shader, u8 * numTex, u8 * numRas) {
    if (shader->shaderParams.complexShader.input1->type == SHADER_TYPE_TEXTURE) {
        *numTex += 1;
    } else if (shader->shaderParams.complexShader.input1->type == SHADER_TYPE_RASTERIZED) {
        *numRas += 1;
    } else if (shader->shaderParams.complexShader.input1->type == SHADER_TYPE_COMPLEX) {
        *numRas += 1;
        *numTex += 1;
    }
    if (shader->shaderParams.complexShader.input2->type == SHADER_TYPE_TEXTURE) {
        *numTex += 1;
    } else if (shader->shaderParams.complexShader.input2->type == SHADER_TYPE_RASTERIZED) {
        *numRas += 1;
    } else if (shader->shaderParams.complexShader.input2->type == SHADER_TYPE_COMPLEX) {
        *numRas += 1;
        *numTex += 1;
    }
    if (shader->shaderParams.complexShader.input3->type == SHADER_TYPE_TEXTURE) {
        *numTex += 1;
    } else if (shader->shaderParams.complexShader.input3->type == SHADER_TYPE_RASTERIZED) {
        *numRas += 1;
    } else if (shader->shaderParams.complexShader.input3->type == SHADER_TYPE_COMPLEX) {
        *numRas += 1;
        *numTex += 1;
    }
    if (shader->shaderParams.complexShader.input4->type == SHADER_TYPE_TEXTURE) {
        *numTex += 1;
        return;
    }
    if (shader->shaderParams.complexShader.input4->type == SHADER_TYPE_RASTERIZED) {
        *numRas += 1;
        return;
    }
    if (shader->shaderParams.complexShader.input4->type == SHADER_TYPE_COMPLEX) {
        *numRas += 1;
        *numTex += 1;
    }
}
