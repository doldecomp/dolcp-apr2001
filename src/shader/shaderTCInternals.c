#include <dolphin.h>
#include <dolphin/gx.h>
#include "shader.h"

SHDRTCResources TCResources = {
    // coord[8]
    {
        GX_TEXCOORD0,
        GX_TEXCOORD1,
        GX_TEXCOORD2,
        GX_TEXCOORD3,
        GX_TEXCOORD4,
        GX_TEXCOORD5,
        GX_TEXCOORD6,
        GX_TEXCOORD7
    },
    // tcShader[8]
    {
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
    },
    // textureShader[8]
    {
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
    }
};

u32 MtxIDArray[8] = {
    0x1E, 0x21, 0x24, 0x27, 0x2A, 0x2D, 0x30, 0x33
};

static struct SHDRTexCoord * TexCoordArray[8];

static u8 numExpressions;

// functions
static void FindUniqueTexCoordExpressions(struct SHDRShader *shader);
static void AddTexCoordShaderToList(struct SHDRTexCoord * shader);
static void CopyExp(struct SHDRTexCoord *shader, struct TexCoordExp *exp);
static GXTexMtxType TranslateTexMtxType(enum SHADER_TG_TYPE type);

void CompileTexGen(struct SHDRShader *shader, SHDRExp **exp) {
    u8 i;

    numExpressions = 0;
    FindUniqueTexCoordExpressions(shader);
    *exp = OSAllocFromHeap(__OSCurrHeap, 0x30);
    if (numExpressions != 0) {
        (*exp)->expressionArray = OSAllocFromHeap(__OSCurrHeap, numExpressions * 0x10);
    } else {
        (*exp)->expressionArray = 0;
    }
    (*exp)->numExpressions = numExpressions;
    for(i = 0; i < 8; i++) {
        (*exp)->mtxArray[i] = 0;
        (*exp)->mtxUsed[i] = 0;
    }
    for(i = 0; i < numExpressions; i++) {
        CopyExp(TexCoordArray[i], &(*exp)->expressionArray[i]);
        if (TexCoordArray[i]->mtxInput != 8 && (*exp)->mtxUsed[TexCoordArray[i]->mtxInput] == 0) {
            (*exp)->mtxUsed[TexCoordArray[i]->mtxInput] = 1;
            (*exp)->mtxArray[TexCoordArray[i]->mtxInput] = OSAllocFromHeap(__OSCurrHeap, 0x30);
        }
    }
}

static void FindUniqueTexCoordExpressions(struct SHDRShader *shader) {
    if (shader) {
        if (shader->type == SHADER_TYPE_TEXTURE) {
            AddTexCoordShaderToList(shader->shaderParams.textureShader.texCoordShader);
            return;
        }
        if (shader->type == SHADER_TYPE_COMPLEX || shader->type == SHADER_TYPE_EMPTY) {
            FindUniqueTexCoordExpressions(shader->shaderParams.complexShader.input1);
            FindUniqueTexCoordExpressions(shader->shaderParams.complexShader.input2);
            FindUniqueTexCoordExpressions(shader->shaderParams.complexShader.input3);
            FindUniqueTexCoordExpressions(shader->shaderParams.complexShader.input4);
        }
    }
}

static void AddTexCoordShaderToList(struct SHDRTexCoord * shader) {
    u8 i;

    for(i = 0; i < numExpressions; i++) {
        if (TexCoordArray[i] == shader) {
            return;
        }
    }
    if (shader->shaderSrc) {
        AddTexCoordShaderToList(shader->shaderSrc);
    }
    ASSERTMSGLINE(0xB3, numExpressions < 8, "Too many texture coordinate shaders - FindUniqueTexCoordExpressions");
    TexCoordArray[numExpressions] = shader;
    shader->texCoordExpIdx = numExpressions;
    numExpressions++;
}

static void CopyExp(struct SHDRTexCoord *shader, struct TexCoordExp *exp) {
    exp->genSrc = shader->genSrc;
    exp->type = shader->type;
    exp->mtxInput = shader->mtxInput;
    if (shader->shaderSrc) {
        exp->srcShaderIdx = shader->shaderSrc->texCoordExpIdx;
        return;
    }
    exp->srcShaderIdx = 0x63;
}

void SetTCGenState(SHDRInfo *shader) {
    SHDRExp *tcExp;
    u8 i;
    struct TexCoordExp *tcShader;
    u32 texMtxIdx;

    tcExp = shader->texGen;
    for(i = 0; i < tcExp->numExpressions; i++) {
        tcShader = &tcExp->expressionArray[i];
        if (tcShader->mtxInput == 8) {
            texMtxIdx = 0x3C;
        } else {
            texMtxIdx = MtxIDArray[tcShader->mtxInput];
            GXLoadTexMtxImm(tcExp->mtxArray[tcShader->mtxInput], texMtxIdx, TranslateTexMtxType(tcShader->type));
        }
        if (tcShader->srcShaderIdx != 0x63) {
            GXSetTexCoordGen(TCResources.coord[i], tcShader->type, tcShader->srcShaderIdx + 0xC, texMtxIdx);
        } else {
            GXSetTexCoordGen(TCResources.coord[i], tcShader->type, tcShader->genSrc, texMtxIdx);
        }
    }
    GXSetNumTexGens(tcExp->numExpressions);
}

static GXTexMtxType TranslateTexMtxType(enum SHADER_TG_TYPE type) {
    switch(type) {
        case SHADER_TG_MTX3x4:
            return GX_MTX3x4;
        case SHADER_TG_MTX2x4:
            return GX_MTX2x4;
        default:
            ASSERTMSGLINE(0x20C, 0, "unknown tex mtx type in TranslateTexMtxType");
            return 0;
    }
}
