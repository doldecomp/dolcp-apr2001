#include <dolphin.h>
#include "charPipeline.h"

struct SHDRTexCoord * SHDRCreateTexCoordExpression(enum SHADER_TG_SRC src, struct SHDRTexCoord *shadSrc, enum SHADER_TG_TYPE texGenType, enum SHADER_MTX mtxInput) {
    struct SHDRTexCoord * temp;

    temp = OSAllocFromHeap(__OSCurrHeap, sizeof(struct SHDRTexCoord));
    temp->shaderSrc = shadSrc;
    temp->genSrc = src;
    temp->type = texGenType;
    temp->mtxInput = mtxInput;
    temp->texCoordExpIdx = 0;
    temp->referenceCount = 0;
    return temp;
}

struct SHDRTexCoord * SHDRCreateTCPassThrough(enum SHADER_TG_SRC src) {
    SHDRCreateTexCoordExpression(src, 0, 0, SHADER_IDENTITY);
}

void SHDRFreeTC(struct SHDRTexCoord * texCoord) {
    texCoord->referenceCount--;
    if (texCoord->referenceCount < 1) {
        OSFreeToHeap(__OSCurrHeap, texCoord);
    }
}
