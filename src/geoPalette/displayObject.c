#include <dolphin.h>
#include "geoPalette.h"
#include "shader.h"
#include "light.h"
#include "charPipeline/texPalette.h"
#include "charPipeline/fileCache.h"

// mapped to DOTextureState enums
static u32 TextureStateOverrides[11] = {
    0xFFFFFFFF,
    0xFFFFFFFF,
    0xFFFFFFFF,
    0xFFFFFFFF,
    0xFFFFFFFF,
    0xFFFFFFFF,
    0xFFFFFFFF,
    0xFFFFFFFF,
    0xFFFFFFFF,
    0xFFFFFFFF,
    0xFFFFFFFF
};

static GXTexObj DOTexObj[1];
static GXTlutObj DOTlut[1];

static MtxPtr SkinForwardArray;
static MtxPtr SkinInverseArray;
static GXTevMode LastShader;
static u8 ShaderSet;
static int ColorChanUsed;
static GXTexObj *CurrentTexObj;
static f32 OverrideMaxLOD;
static f32 OverrideMinLOD;
static f32 OverrideLODBias;
u32 HackTevState;
int CPEnableMultiTexture;

// functions
void DORelease(struct DODisplayObj * * dispObj);
Mtx *DOGetWorldMatrix(struct DODisplayObj * dispObj);
void DOHide(struct DODisplayObj * dispObj);
void DORender(struct DODisplayObj * dispObj, MtxPtr camera, u8 numLights, ...);
void DOVARender(struct DODisplayObj * dispObj, MtxPtr camera, u8 numLights, va_list *list);
void DORenderSkin(struct DODisplayObj *dispObj, MtxPtr camera, MtxPtr mtxArray, MtxPtr invTransposeMtxArray, u8 numLights, ...);
void DOVARenderSkin(struct DODisplayObj *dispObj, MtxPtr camera, MtxPtr mtxArray, MtxPtr invTransposeMtxArray, u8 numLights, va_list *list);
static void SetState(DisplayStateList *state, struct DODisplayObj * dispObj, MtxPtr camera);
void DOSetWorldMatrix(struct DODisplayObj * dispObj, MtxPtr m);
void DOShow(struct DODisplayObj * dispObj);
f32 DOSetAmbientPercentage(struct DODisplayObj * dispObj, f32 percent);
static void TranslateVCD(u32 VCD);
static void LoadLights(u8 numLights, va_list *list, f32 scale, GXColorSrc colorSrc);
static void DisableLights(GXColorSrc colorSrc);
static GXCompCnt TranslateCmpCount(GXAttr attr, u8 numComponents);
static void GetColorFromQuant(void * src, u8 srcCode, u8 * r, u8 * g, u8 * b, u8 * a);
static u16 DisplayDataSize(u8 quantFlags);
static u16 DisplayDataSizeColor(u8 quantFlags);
static void SetDispObjShader(struct DODisplayObj * dispObj, u32 setting);
void DOSetEffectsShader(struct DODisplayObj *dispObj, char *shaderFunc, char *data);
void DOOverrideTextureState(enum DOTextureState state, enum DOTextureSetting setting, f32 LODValue);

void DORelease(struct DODisplayObj * * dispObj) {
    if ((*dispObj)->positionData != 0) {
        OSFreeToHeap(__OSCurrHeap, (*dispObj)->positionData);
    }
    if ((*dispObj)->colorData != 0) {
        OSFreeToHeap(__OSCurrHeap, (*dispObj)->colorData);
    }
    if ((*dispObj)->textureData != 0) {
        OSFreeToHeap(__OSCurrHeap, (*dispObj)->textureData);
    }
    if ((*dispObj)->lightingData != 0) {
        OSFreeToHeap(__OSCurrHeap, (*dispObj)->lightingData);
    }
    if ((*dispObj)->displayData != 0) {
        OSFreeToHeap(__OSCurrHeap, (*dispObj)->displayData);
    }
    OSFreeToHeap(__OSCurrHeap, *dispObj);
    *dispObj = NULL;
}

Mtx *DOGetWorldMatrix(struct DODisplayObj * dispObj) {
    return &dispObj->worldMatrix;
}

void DOHide(struct DODisplayObj * dispObj) {
    dispObj->visibility = 0;
}

void DORender(struct DODisplayObj * dispObj, MtxPtr camera, u8 numLights, ...) {
    va_list ptr;

    va_start(ptr, numLights);
    DOVARender(dispObj, camera, numLights, &ptr);
    va_end(ptr);
}

void DOVARender(struct DODisplayObj * dispObj, MtxPtr camera, u8 numLights, va_list *list) {
    u32 i;
    DisplayStateList *cursor;
    GXColor matColor;
    GXColorSrc colorSrc;
    Mtx mv;
    Mtx tempMtx;

    colorSrc = GX_SRC_VTX;
    if (dispObj->visibility != 0) {
        if (SkinForwardArray == 0) {
            MTXConcat(camera, dispObj->worldMatrix, mv);
            GXLoadPosMtxImm(mv, 0);
            GXSetCurrentMtx(0);
        }
        ASSERTMSGLINE(0xD8, dispObj->positionData, "DOVARender: Must provide position data");
        ASSERTMSGLINE(0xDA, dispObj->colorData || dispObj->textureData, "DOVARender: Must either supply color or texture coordinates.\n");
        GXSetArray(9, dispObj->positionData->positionArray, dispObj->positionData->compCount * DisplayDataSize(dispObj->positionData->quantizeInfo));
        GXSetVtxAttrFmt(0, 9, TranslateCmpCount(9, dispObj->positionData->compCount), dispObj->positionData->quantizeInfo >> 4, dispObj->positionData->quantizeInfo & 0xF);
        if (dispObj->colorData != 0) {
            if (dispObj->colorData->numColors == 1) {
                colorSrc = 0;
                GetColorFromQuant(dispObj->colorData->colorArray, dispObj->colorData->quantizeInfo, &matColor.r, &matColor.g, &matColor.b, &matColor.a);
                GXSetChanMatColor(4, matColor);
            } else {
                GXSetArray(0xB, dispObj->colorData->colorArray, DisplayDataSizeColor(dispObj->colorData->quantizeInfo));
                GXSetVtxAttrFmt(0, 0xB, TranslateCmpCount(0xB, dispObj->colorData->compCount), dispObj->colorData->quantizeInfo >> 4, 0);
            }
            GXSetNumChans(1);
            ColorChanUsed = 1;
        } else if (CPEnableMultiTexture != 0) {
            colorSrc = 0;
            matColor.r = matColor.g = matColor.b = matColor.a = 0xFF;
            GXSetChanMatColor(4, matColor);
            GXSetNumChans(1);
            ColorChanUsed = 1;
        } else {
            GXSetNumChans(0);
            ColorChanUsed = 0;
        }
        if (dispObj->textureData != 0U) {
            for(i = 0; i < dispObj->numTextureChannels; i++) {
                GXSetArray(i + GX_VA_TEX0, dispObj->textureData[i].textureCoordArray, dispObj->textureData[i].compCount * DisplayDataSize(dispObj->textureData[i].quantizeInfo));
                GXSetVtxAttrFmt(0, i + GX_VA_TEX0, TranslateCmpCount(i + GX_VA_TEX0, dispObj->textureData[i].compCount), dispObj->textureData[i].quantizeInfo >> 4, dispObj->textureData[i].quantizeInfo & 0xF);
                GXSetTexCoordGen(i, 1, i + 4, 0x3C);
            }
        }
        if ((dispObj->lightingData != 0) && (dispObj->colorData != 0) && (numLights & 0xFF)) {
            if (dispObj->lightingData->normalArray != 0) {
                GXSetArray(0xA, dispObj->lightingData->normalArray, dispObj->lightingData->compCount * DisplayDataSize(dispObj->lightingData->quantizeInfo));
                GXSetVtxAttrFmt(0, 0xA, TranslateCmpCount(0xA, dispObj->lightingData->compCount), dispObj->lightingData->quantizeInfo >> 4, dispObj->lightingData->quantizeInfo & 0xF);
            } else {
                GXSetArray(0xA, &normalTable, normalTableNumComponents * DisplayDataSize(normalTableQuantizeInfo));
                GXSetVtxAttrFmt(0, 0xA, TranslateCmpCount(0xA, normalTableNumComponents), (s32) normalTableQuantizeInfo >> 4, normalTableQuantizeInfo & 0xF);
            }
            if ((u32) SkinForwardArray == 0U) {
                MTXInverse(mv, tempMtx);
                MTXTranspose(tempMtx, mv);
                GXLoadNrmMtxImm(mv, 0);
            }
            LoadLights(numLights, list, (0.01f * dispObj->lightingData->ambientPercentage), colorSrc);
        } else {
            DisableLights(colorSrc);
        }
        cursor = dispObj->displayData->displayStateList;
        for(i = 0; i < dispObj->displayData->numStateEntries; i++) {
            SetState(&cursor[i], dispObj, camera);
            if (cursor[i].primitiveList != 0U) {
                if (ShaderSet == 0) {
                    SetDispObjShader(dispObj, LastShader);
                }
                GXCallDisplayList(cursor[i].primitiveList, cursor[i].listSize);
            }
        }
    }
}

void DORenderSkin(struct DODisplayObj *dispObj, MtxPtr camera, MtxPtr mtxArray, MtxPtr invTransposeMtxArray, u8 numLights, ...) {
    va_list ptr;

    va_start(ptr, numLights);
    DOVARenderSkin(dispObj, camera, mtxArray, invTransposeMtxArray, numLights, &ptr);
    va_end(ptr);
}

void DOVARenderSkin(struct DODisplayObj *dispObj, MtxPtr camera, MtxPtr mtxArray, MtxPtr invTransposeMtxArray, u8 numLights, va_list *list) {
    ASSERTLINE(0x16F, mtxArray);
    SkinForwardArray = mtxArray;
    SkinInverseArray = invTransposeMtxArray;
    DOVARender(dispObj, camera, numLights, list);
    SkinForwardArray = 0;
    SkinInverseArray = 0;
}

static void SetState(DisplayStateList *state, struct DODisplayObj * dispObj, MtxPtr camera) {
    TEXDescriptorPtr tex;
    u32 pnMtxIdx;
    Mtx tempMtx;
    u32 wrapS;
    u32 wrapT;
    u32 minFilt;
    u32 magFilt;
    u32 maxAniso;
    u8 mipmap;
    u8 biasClamp;
    u8 edgeLOD;
    f32 minLOD;
    f32 maxLOD;
    f32 biasLOD;
    u32 channel;

    switch(state->id) {
        case 1:
            channel = (state->setting >> 0xDU) & 7;
            tex = TEXGet(dispObj->textureData[channel].texturePalette, state->setting & 0x1FFF);
            if (TextureStateOverrides[0] != -1) {
                wrapS = TextureStateOverrides[0];
            } else {
                wrapS = (state->setting >> 0x10U) & 0xF;
                switch (wrapS) {                      
                case 0:                                 
                    wrapS = 0;
                    break;
                case 1:                                 
                    wrapS = 1;
                    break;
                case 2:                                 
                    wrapS = 2;
                    break;
                }
            }
            if (TextureStateOverrides[1] != -1) {
                wrapT = TextureStateOverrides[1];
            } else {
                wrapT = (state->setting >> 0x14U) & 0xF;
                switch (wrapT) {                      
                case 0:                                 
                    wrapT = 0;
                    break;
                case 1:                                 
                    wrapT = 1;
                    break;
                case 2:                                 
                    wrapT = 2;
                    break;
                }
            }
            if (TextureStateOverrides[3] != -1) {
                minFilt = TextureStateOverrides[3];
            } else {
                minFilt = (state->setting >> 0x18U) & 0xF;
                switch (minFilt) {                      
                case 0:                                 
                    minFilt = 0;
                    break;
                case 1:                                 
                    minFilt = 1;
                    break;
                case 2:                                 
                    minFilt = 2;
                    break;
                case 3:                                 
                    minFilt = 3;
                    break;
                case 4:                                 
                    minFilt = 4;
                    break;
                case 5:                                 
                    minFilt = 5;
                    break;
                }
            }
            if (TextureStateOverrides[4] != -1) {
                magFilt = TextureStateOverrides[4];
            } else {
                magFilt = state->setting >> 0x1CU;
                switch (magFilt) {                
                case 0:                                 
                    magFilt = 0;
                    break;
                case 1:                                 
                    magFilt = 1;
                    break;
                case 2:                                 
                    magFilt = 2;
                    break;
                case 3:                                 
                    magFilt = 3;
                    break;
                case 4:                                 
                    magFilt = 4;
                    break;
                case 5:                                 
                    magFilt = 5;
                    break;
                }
            }
            if (TextureStateOverrides[2] != -1) {
                mipmap = TextureStateOverrides[2];
            } else if (tex->textureHeader->minLOD == tex->textureHeader->maxLOD) {
                mipmap = 0;
            } else {
                mipmap = 1;
            }
            if (TextureStateOverrides[5] != -1) {
                minLOD = OverrideMinLOD;
            } else {
                minLOD = tex->textureHeader->minLOD;
            }
            if (TextureStateOverrides[6] != -1) {
                maxLOD = OverrideMaxLOD;
            } else {
                maxLOD = tex->textureHeader->maxLOD;
            }
            if (TextureStateOverrides[7] != -1) {
                biasLOD = OverrideLODBias;
            } else {
                biasLOD = tex->textureHeader->LODBias;
            }
            if (TextureStateOverrides[8] != -1) {
                biasClamp = TextureStateOverrides[7];
            } else {
                biasClamp = 0;
            }
            if (TextureStateOverrides[9] != -1) {
                edgeLOD = TextureStateOverrides[9];
            } else {
                edgeLOD = tex->textureHeader->edgeLODEnable;
            }
            if (TextureStateOverrides[10] != -1) {
                maxAniso = TextureStateOverrides[10];
            } else {
                maxAniso = 0;
            }
            if (tex->CLUTHeader != 0) {
                GXInitTexObjCI((GXTexObj*)&DOTexObj[channel].dummy[0], tex->textureHeader->data, tex->textureHeader->width, tex->textureHeader->height, tex->textureHeader->format, wrapS, wrapT, mipmap, 0);
                GXInitTlutObj((GXTlutObj*)&DOTlut[channel].dummy[0], tex->CLUTHeader->data, tex->CLUTHeader->format, tex->CLUTHeader->numEntries);
                GXLoadTlut((GXTlutObj*)&DOTlut[channel].dummy[0], 0);
            } else {
                GXInitTexObj((GXTexObj*)&DOTexObj[channel].dummy, tex->textureHeader->data, tex->textureHeader->width, tex->textureHeader->height, tex->textureHeader->format, wrapS, wrapT, mipmap);
            }
            GXInitTexObjLOD((GXTexObj*)&DOTexObj[channel].dummy, minFilt, magFilt, minLOD, maxLOD, biasLOD, biasClamp, edgeLOD, maxAniso);
            GXLoadTexObj((GXTexObj*)&DOTexObj[channel].dummy, channel);
            ShaderSet = 0;
            return;
        case 3:
            TranslateVCD(state->setting);
            return;
        case 4:
            SetDispObjShader(dispObj, state->setting);
            return;
        case 5:
            pnMtxIdx = ((u16)state->setting) * 3;
            ASSERTMSGLINE(0x228, SkinForwardArray, "Display object is stitched but no stitching matrices set through DORenderSkin or DOVARenderSkin.");
            MTXConcat(camera, (void*)&SkinForwardArray[(state->setting >> 0x10) * 3][0], tempMtx);
            GXLoadPosMtxImm(tempMtx, pnMtxIdx);
            if (dispObj->lightingData != 0U) {
                MTXConcat(camera, (void*)&SkinInverseArray[(state->setting >> 0x10) * 3][0], tempMtx);
                GXLoadNrmMtxImm(tempMtx, pnMtxIdx);
            }
            return;
        default:
            ASSERTMSGLINE(0x234, 0, "Unknown state setting for Display Object");
            break;
    }
}

void DOSetWorldMatrix(struct DODisplayObj * dispObj, MtxPtr m) {
    MTXCopy(m, dispObj->worldMatrix);
}

void DOShow(struct DODisplayObj * dispObj) {
    dispObj->visibility = 1;
}

f32 DOSetAmbientPercentage(struct DODisplayObj * dispObj, f32 percent) {
    f32 temp;

    temp = 0.0f;
    if (dispObj->lightingData) {
        temp = dispObj->lightingData->ambientPercentage;
        dispObj->lightingData->ambientPercentage = percent;
    }
    return temp;
}

static void TranslateVCD(u32 VCD) {
    GXAttrType type;
    u32 i;
    u32 shift;
    GXAttr attr;
    GXVtxDescList attrList[27];

    GXGetVtxDescv(attrList);

    for(i = 0; i < 27; i++) {
        if (attrList[i].attr == 0xFF) {
            if (i) {
                return;
            } else {
                break;
            }
        }
        if (attrList[i].attr == 0) {
            shift = 0;
        } else {
            shift = (attrList[i].attr - 8) * 2;
        }
        if (attrList[i].type != ((VCD >> shift) & 3)) {
            break;
        }
    }
    GXClearVtxDesc();
    i = 0;
    type = VCD & 3;
    if (type != 0) {
        attrList[i].attr = 0;
        attrList[i].type = type;
        i++;
    }
    attr = 9;
    shift = 2;
    while(attr <= 0x14) {
        type = (VCD >> shift) & 3;
        if (type) {
            attrList[i].attr = attr;
            attrList[i].type = type;
            i++;
        }
        attr+=1;
        shift+=2;
    }
    attrList[i].attr = 0xFF;
    GXSetVtxDescv(attrList);
}

static void LoadLights(u8 numLights, va_list *list, f32 scale, GXColorSrc colorSrc) {
    Light *light;
    GXLightID lightNum;
    GXLightID lightMask;
    GXColor color;
    u32 i;

    lightMask = 0;
    ASSERTMSGLINE(0x29F, numLights <= 8, "LoadLights: Maximum number of lights is 8");
    ASSERTMSGLINE(0x2A0, (scale >= 0.0f) && (scale <= 1.0f), "LoadLights: Scale needs to be normalized");
    ASSERTMSGLINE(0x2A1, (colorSrc == 1) || (colorSrc == 0), "LoadLights: Color source incorrect");
    color.r = color.g = color.b = (scale * 255.0f);
    color.a = 0xFF;
    GXSetChanAmbColor(4, color);
    scale = 1.0f - scale;
    for(i = 0; i < numLights; i++) {
        light = va_arg(*list, Light*);
        color.r = light->color.r * scale;
        color.g = light->color.g * scale;
        color.b = light->color.b * scale;
        GXInitLightColor(&light->lt_obj, color);
        GXInitLightPos(&light->lt_obj, light->worldPosition.x, light->worldPosition.y, light->worldPosition.z);
        GXInitLightDir(&light->lt_obj, light->worldDirection.x, light->worldDirection.y, light->worldDirection.z);
        lightNum = 1 << i;
        GXLoadLightObjImm(&light->lt_obj, lightNum);
        lightMask |= lightNum;
    }
    GXSetChanCtrl(4, 1, 0, colorSrc, lightMask, 2, 1);
}

static void DisableLights(GXColorSrc colorSrc) {
    GXSetChanCtrl(4, 0, 0, colorSrc, 0, 0, 2);
}

static GXCompCnt TranslateCmpCount(GXAttr attr, u8 numComponents) {
    switch(attr) {
        case 9: {
            if (numComponents == 2) {
                return 0;
            } else if (numComponents == 3 || numComponents == 6) {
                return 1;
            }
            break;
        }
        case 10: {
            if ((numComponents == 3) || (numComponents == 6)) {
                return 0;
            }
            break;
        }
        case 11:
        case 12: {
            if (numComponents == 3) {
                return 0;
            }
            if (numComponents == 4) {
                return 1;
            }
            break;
        }
        case 13:
        case 14:
        case 15:
        case 16:
        case 17:
        case 18:
        case 19:
        case 20: {
            if (numComponents == 1) {
                return 0;
            }
            if (numComponents == 2) {
                return 1;
            }
        }
    }
    ASSERTMSGLINE(0x2F8, 0, "TranslateCmpCount: Unknown component count value");
    return 0;
}

static void GetColorFromQuant(void * src, u8 srcCode, u8 * r, u8 * g, u8 * b, u8 * a) {
    u8 quant = srcCode >> 4;

    ASSERTMSGLINE(0x301, src && r && g && b && a, "GetColorFromQuant: One argument is NULL");
    
    switch(quant) {
        case 0:
            *r = (int)((*(u16*)src >> 8U) & 0xF8);
            *g = (int)((*(u16*)src >> 3U) & 0xFC);
            *b = (int)((*(u16*)src * 8) & 0xF8);
            *a = 0xFF;
            return;
        case 3:
            *r = (int)((*(u16*)src >> 8U) & 0xF0);
            *g = (int)((*(u16*)src >> 4U) & 0xF0);
            *b = (int)((*(u16*)src) & 0xF0);
            *a = (int)((*(u16*)src * 0x10) & 0xF0);
            return;
        case 5:
            *r = (int)(((*(u32*)src & 0xFF000000) >> 24));
            *g = (int)(((*(u32*)src & 0xFF0000) >> 16));
            *b = (int)(((*(u32*)src & 0xFF00) >> 8));
            *a = (int)((*(u32*)src >> 0));
            return;
        case 1:
        case 2:
            *r = (int)(((*(u32*)src & 0xFF000000) >> 24));
            *g = (int)(((*(u32*)src & 0xFF0000) >> 16));
            *b = (int)(((*(u32*)src & 0xFF00) >> 8));
            *a = 0xFF;
            return;
        case 4:
            *r = (int)((*(u16*)src & (0x3F << 18)) >> 0x10U);
            *g = (int)((*(u16*)src & (0x3F << 12)) >> 0xAU);
            *b = (int)((*(u16*)src & (0x3F << 6)) >> 4U);
            *a = (int)((*(u16*)src & (0x3F << 0)) << 2);
            return;
    }
    ASSERTMSGLINE(0x32A, 0, "GetColorFromQuant: Unknown color type");
}

static u16 DisplayDataSize(u8 quantFlags) {
    u8 flag;

    flag = quantFlags >> 4;
    switch(flag) {
        case 0:
        case 1:
            return 1;
        case 2:
        case 3:
            return 2;
        case 4:
            return 4;
    }
    ASSERTMSGLINE(0x33D, 0, "DisplayDataSize: Unknown quantization type");
    return 0;
}

static u16 DisplayDataSizeColor(u8 quantFlags) {
    u8 flag = quantFlags >> 4;

    switch(flag) {
        case 3:
        case 0:
            return 2;
        case 1:
        case 4:
            return 3;
        case 5:
        case 2:
            return 4;
    }
    ASSERTMSGLINE(0x350, 0, "DisplayDataSizeColor: Unknown quantization type");
    return 0;
}

static void SetDispObjShader(struct DODisplayObj * dispObj, u32 setting) {
    SHDRInfo *shader;
    GXTevMode tevMode;
    GXChannelID channelID;
    u8 numTevStages;
    u8 numTexGens;
    u8 channel;

    tevMode = 0xF;
    numTevStages = 0;
    numTexGens = 0;
    if (dispObj->shaderFunc == 0) {
        for(channel = 0; (channel < 1) && (tevMode != 4); channel++) {
            if (CPEnableMultiTexture != 0) {
                tevMode = setting & (0xF << ((channel << 2)));
                if ((tevMode == 0xF) || ((HackTevState != 0U) && (channel != 0))) {
                    break;
                }
                if ((HackTevState == 1U) && (channel == 0)) {
                    tevMode = 4;
                } else if ((HackTevState == 2U) && (channel == 0)) {
                    if (tevMode == 0) {
                        tevMode = 3;
                    }
                } else if ((HackTevState == 3U) && (channel == 0)) {
                    tevMode = setting & 0xF0;
                    if (tevMode == 0xF) {
                        tevMode = 4;
                    }
                    if (tevMode == 0) {
                        tevMode = 3;
                    }
                    switch (tevMode) {
                        case 3:
                            GXSetTevOrder(0U, 1U, 1U, 0xFF);
                            GXSetTevOp(0U, 3);
                            numTexGens += 1;
                            numTexGens += 1;
                            numTevStages += 1;
                            break;
                        default:
                            GXSetTevOrder(0U, 0xFFU, 0xFFU, 4);
                            GXSetTevOp(0U, 4);
                            numTevStages += 1;
                    }
                    break;
                }
            } else {
                tevMode = setting & 0xF;
            }
            switch (tevMode) {
                case 0:
                case 1:
                    if (channel != 0) {
                        channelID = 0xFF;
                    } else {
                        channelID = 4;
                    }
                    GXSetTevOrder(channel, channel, channel, channelID);
                    GXSetTevOp(channel, tevMode);
                    numTexGens += 1;
                    numTevStages += 1;
                    break;
                case 3:                             
                    GXSetTevOrder(channel, channel, channel, 0xFF);
                    GXSetTevOp(channel, 3);
                    numTexGens += 1;
                    numTevStages += 1;
                    break;
                case 4:                             
                    GXSetTevOrder(channel, 0xFFU, 0xFFU, 4);
                    GXSetTevOp(channel, 4);
                    numTevStages += 1;
                    break;
                case 2:
                    break;
            }
            if (CPEnableMultiTexture == 0) {
                break;
            }
        }
        GXSetNumTexGens(numTexGens);
        GXSetNumTevStages(numTevStages);
    } else {
        tevMode = setting & 0xF;
        switch (tevMode) {
            case 0:
            case 1:
                SHDRBindTexture(SHDRModulateShader, 0, DOTexObj);
                SHDRBindRasterized(SHDRModulateShader, 0, 4);
                shader = SHDRModulateShader;
                CurrentTexObj = DOTexObj;
                break;            
            case 3:
                SHDRBindTexture(SHDRReplaceShader, 0, DOTexObj);
                shader = SHDRReplaceShader;
                CurrentTexObj = DOTexObj;
                break;
            case 4:
                SHDRBindRasterized(SHDRPassThruShader, 0, 4);
                shader = SHDRPassThruShader;
                CurrentTexObj = NULL;
                break;
        }
        if (dispObj->shaderFunc) {
            dispObj->shaderFunc(shader, dispObj, tevMode, CurrentTexObj, ColorChanUsed, dispObj->shaderData);
        } else {
            SHDRExecute(shader);
        }
    }
    LastShader = tevMode;
    ShaderSet = 1;
}

void DOSetEffectsShader(struct DODisplayObj *dispObj, char *shaderFunc, char *data) {
    dispObj->shaderFunc = (void*)shaderFunc;
    dispObj->shaderData = data;
}

void DOOverrideTextureState(enum DOTextureState state, enum DOTextureSetting setting, f32 LODValue) {
    TextureStateOverrides[state] = setting;
    if (state == DOTS_MIN_LOD) {
        OverrideMinLOD = LODValue;
    } else if (state == DOTS_MAX_LOD) {
        OverrideMaxLOD = LODValue;
    } else if (state == DOTS_LOD_BIAS) {
        OverrideLODBias = LODValue;
    }
}
