#include <dolphin.h>
#include "geoPalette.h"
#include "skinning.h"
#include "charPipeline/texPalette.h"
#include "charPipeline/fileCache.h"

// functions
static void *LoadGeoPalette(TEXPaletteData **pal, char *name);
static sHdr *SKNLoadFile(char *name, void *posArrayForSkin);
static void SKNFreeFile(sHdr *skHeader);
static void GeoPaletteFree(TEXPaletteData **pal);
static void InitDisplayObjWithLayout(struct DODisplayObj *dispObj, DODisplayLayout *layout);

sHdr *GEOGetPalette(TEXPaletteData **pal, char *name) {
    sHdr *skHeader;
    void * posArrayForSkin;
    char * sknName;
    char * ptr;

    skHeader = NULL;
    sknName = OSAllocFromHeap(__OSCurrHeap, strlen(name) + 4);
    strcpy(sknName, name);
    ptr = strrchr(sknName, 0x2E);
    if (ptr) {
        ptr[1] = 's';
        ptr[2] = 'k';
        ptr[3] = 'n';
    }
    if (DOCacheInitialized) {
        *pal = (void*)DSGetCacheObj(&DODisplayCache, (Ptr)name);
        skHeader = (void*)DSGetCacheObj(&DODisplayCache, (Ptr)sknName);
    }
    if (*pal == 0) {
        posArrayForSkin = LoadGeoPalette(pal, name);
        if (DOCacheInitialized != 0) {
            DSAddCacheNode(&DODisplayCache, (Ptr)name, (Ptr)*pal, (Ptr)GeoPaletteFree);
            DSGetCacheObj(&DODisplayCache, (Ptr)name);
        }
    }
    if ((skHeader == 0) && (ptr != 0)) {
        skHeader = SKNLoadFile(sknName, posArrayForSkin);
        if ((skHeader != 0) && (DOCacheInitialized != 0)) {
            DSAddCacheNode(&DODisplayCache, (Ptr)sknName, (Ptr)skHeader, (Ptr)SKNFreeFile);
            DSGetCacheObj(&DODisplayCache, (Ptr)sknName);
        }
    }
    OSFreeToHeap(__OSCurrHeap, sknName);
    return skHeader;
}

void GEOReleasePalette(TEXPaletteData **pal) {
    if (DOCacheInitialized != 0) {
        DSReleaseCacheObj(&DODisplayCache, (Ptr)*pal);
        return;
    }
    OSFreeToHeap(__OSCurrHeap, *pal);
    *pal = NULL;
}

u32 GEOGetUserDataSize(TEXPaletteData *pal) {
    ASSERTLINE(0x87, pal);
    return pal->userDataSize;
}

char *GEOGetUserData(TEXPaletteData *pal) {
    ASSERTLINE(0x8E, pal);
    return pal->userData;
}

static void *LoadGeoPalette(TEXPaletteData **pal, char *name) {
    struct DVDFileInfo dfi;
    TEXPaletteData * geoPal;
    DODisplayLayout *layout;
    u16 i;
    u16 j;
    void *positionArrayForSkin;

    positionArrayForSkin = NULL;
    if (DVDOpen(name, &dfi) == 0) {
        OSReport("LoadGeoPalette: Could not find file %s", name);
        OSPanic(__FILE__, 0x9F, "");
    }
    geoPal = OSAllocFromHeap(__OSCurrHeap, (dfi.length + 0x1F) & 0xFFFFFFE0);
    DVDReadPrio(&dfi, geoPal, (dfi.length + 0x1F) & 0xFFFFFFE0, 0, 2);
    DVDClose(&dfi);
    if ((geoPal->versionNumber) != 12012000) {
        OSReport("LoadGeoPalette: Incompatible version number %d for %s, since\n", geoPal->versionNumber, name);
        OSReport("                the library version number is %d.\n", 12012000);
        OSPanic(__FILE__, 0xAA, "");
    }
    if ((geoPal->userDataSize != 0U) && (geoPal->userData)) {
        geoPal->userData = (void*)((u32)geoPal->userData + (u32)geoPal);
    }
    geoPal->descriptorArray = (void*)((u32)geoPal->descriptorArray + (u32)geoPal);
    for(i = 0; i < geoPal->numDescriptors; i++) {
        geoPal->descriptorArray[i].textureHeader = (void*)((u32)geoPal + (u32)geoPal->descriptorArray[i].textureHeader);
        geoPal->descriptorArray[i].CLUTHeader = (void*)((u32)geoPal + (u32)geoPal->descriptorArray[i].CLUTHeader);
        layout = (void*)(geoPal->descriptorArray[i].textureHeader); // what? these arent the same struct.
        if (layout->positionData != 0) {
            layout->positionData = (void*)((u32)layout->positionData + (u32)layout);
            if (layout->positionData->positionArray) {
                layout->positionData->positionArray = (void*)((u32)layout->positionData->positionArray + (u32)layout);
            }
            if (layout->positionData->compCount == 6) {
                positionArrayForSkin = layout->positionData->positionArray;
            }
        }
        if (layout->colorData != 0) {
            layout->colorData = (void*)((u32)layout->colorData + (u32)layout);
            if (layout->colorData->colorArray != 0) {
                layout->colorData->colorArray = (void*)((u32)layout->colorData->colorArray + (u32)layout);
            }
        }
        if (layout->textureData != 0) {
            layout->textureData = (void*)((u32)layout->textureData + (u32)layout);
            for(j = 0; j < layout->numTextureChannels; j++) {
                if (layout->textureData[j].textureCoordArray) {
                    layout->textureData[j].textureCoordArray = (void*)((u32)layout->textureData[j].textureCoordArray + (u32)layout);
                }
                if (layout->textureData[j].texturePaletteName) {
                    layout->textureData[j].texturePaletteName = (void*)((u32)layout->textureData[j].texturePaletteName + (u32)layout);
                    TEXGetPalette(&layout->textureData[j].texturePalette, layout->textureData[j].texturePaletteName);
                }
            }
        }
        if (layout->lightingData) {
            layout->lightingData = (void*)((u32)layout->lightingData + (u32)layout);
            if (layout->lightingData->normalArray) {
                layout->lightingData->normalArray = (void*)((u32)layout->lightingData->normalArray + (u32)layout);
            }
        }
        if (layout->displayData) {
            layout->displayData = (void*)((u32)layout->displayData + (u32)layout);
            if (layout->displayData->primitiveBank) {
                layout->displayData->primitiveBank = (void*)((u32)layout->displayData->primitiveBank + (u32)layout);
            }
            if (layout->displayData->displayStateList) {
                layout->displayData->displayStateList = (void*)((u32)layout->displayData->displayStateList + (u32)layout);
            }
            for(j = 0; j < layout->displayData->numStateEntries; j++) {
                if (layout->displayData->displayStateList[j].primitiveList) {
                    layout->displayData->displayStateList[j].primitiveList = (void*)((u32)layout->displayData->displayStateList[j].primitiveList + (u32)layout);
                }
            }
        }
    }
    *pal = geoPal;
    return positionArrayForSkin;
}

static sHdr *SKNLoadFile(char *name, void *posArrayForSkin) {
    struct DVDFileInfo dfi;
    long fileEntryNum;
    sHdr *skHeader;
    u32 i;
    SK1List *sk1;
    SK2List *sk2;
    SKAccList *skAcc;

    fileEntryNum = DVDConvertPathToEntrynum(name);
    if (fileEntryNum == -1) {
        return NULL;
    }
    DVDFastOpen(fileEntryNum, &dfi);
    skHeader = OSAllocFromHeap(__OSCurrHeap, (dfi.length + 0x1F) & 0xFFFFFFE0);
    DVDReadPrio(&dfi, skHeader, (dfi.length + 0x1F) & 0xFFFFFFE0, 0, 2);
    DVDClose(&dfi);
    if (skHeader->sk1ListArray != 0) {
        skHeader->sk1ListArray = (void*)((u32)skHeader->sk1ListArray + (u32)skHeader);
    }
    if (skHeader->sk2ListArray != 0) {
        skHeader->sk2ListArray = (void*)((u32)skHeader->sk2ListArray+ (u32)skHeader);
    }
    if (skHeader->skAccListArray != 0) {
        skHeader->skAccListArray = (void*)((u32)skHeader->skAccListArray + (u32)skHeader);
    }
    if (skHeader->skBzeroSize != 0) {
        skHeader->skBzeroBase = (void*)((u32)skHeader->skBzeroBase + (u32)posArrayForSkin);
    }
    if (skHeader->flushIndices != 0) {
        skHeader->flushIndices = (void*)((u32)skHeader->flushIndices + (u32)skHeader);
    }
    for(i = 0; i < skHeader->numSk1List; i++) {
        sk1 = &skHeader->sk1ListArray[i];
        sk1->vertSrc = (void* ) ((u32)sk1->vertSrc + (u32)skHeader);
        sk1->vertDst = (void* ) ((u32)sk1->vertDst + (u32)posArrayForSkin);
    }
    for(i = 0; i < skHeader->numSk2List; i++) {
        sk2 = &skHeader->sk2ListArray[i];
        sk2->vertSrc = (void* ) ((u32)sk2->vertSrc + (u32)skHeader);
        sk2->weights = (void* ) ((u32)sk2->weights + (u32)skHeader);
        sk2->vertDst = (void* ) ((u32)sk2->vertDst + (u32)posArrayForSkin);
    }
    for(i = 0; i < skHeader->numSkAccList; i++) {
        skAcc = &skHeader->skAccListArray[i];
        skAcc->vertSrc = (void* ) ((u32)skAcc->vertSrc + (u32)skHeader);
        skAcc->weights = (void* ) ((u32)skAcc->weights + (u32)skHeader);
        skAcc->vertIndices = (void* ) ((u32)skAcc->vertIndices + (u32)skHeader);
        skAcc->vertDst = (void* ) ((u32)skAcc->vertDst + (u32)posArrayForSkin);
    }
    SKNInit();
    return skHeader;
}

static void SKNFreeFile(sHdr *skHeader) {
    OSFreeToHeap(__OSCurrHeap, skHeader);
}

void DOGet(struct DODisplayObj **dispObj, DODisplayData *pal, u16 id, char *name) {
    DODisplayLayout *layout;
    u32 i;

    if (name) {
        for(i = 0; i < pal->numDescriptors; i++) {
            if (Strcmp(pal->descriptorArray[i].name, name) == 0) {
                layout = pal->descriptorArray[i].layout;
                *dispObj = OSAllocFromHeap(__OSCurrHeap, 0x54);
                InitDisplayObjWithLayout(*dispObj, layout);
                return;
            }
        }
        ASSERTMSGLINE(0x15E, 0, "display object not found in Geometry Palette");
        goto exit;
    }
    ASSERTMSGLINE(0x163, id < pal->numDescriptors, "bad display object id");
    layout = pal->descriptorArray[id].layout;
exit:;
    *dispObj = OSAllocFromHeap(__OSCurrHeap, 0x54);
    //! @bug? It is possible to reach this call while layout is uninitialized if name is
    //  defined while pal->numDescriptors is either 0 or Strcmp never returns as being
    //  0, resulting in layout not being initialized. In non-debug versions of this
    //  code, it will go past where an assert normally is and pass an uninitialized
    //  pointer into this function.
    InitDisplayObjWithLayout(*dispObj, layout);
}

static void GeoPaletteFree(TEXPaletteData **pal) {
    OSFreeToHeap(__OSCurrHeap, *pal);
    *pal = NULL;
}

static void InitDisplayObjWithLayout(struct DODisplayObj *dispObj, DODisplayLayout *layout) {
    u8 i;

    if (layout->positionData != 0) {
        dispObj->positionData = OSAllocFromHeap(__OSCurrHeap, 8);
        *dispObj->positionData = *layout->positionData;
    } else {
        dispObj->positionData = NULL;
    }
    if (layout->colorData != 0) {
        dispObj->colorData = OSAllocFromHeap(__OSCurrHeap, 8);
        *dispObj->colorData = *layout->colorData;
    } else {
        dispObj->colorData = NULL;
    }
    if ((layout->textureData != 0) && (layout->numTextureChannels != 0)) {
        dispObj->textureData = OSAllocFromHeap(__OSCurrHeap, layout->numTextureChannels * 0x10);
        dispObj->numTextureChannels = layout->numTextureChannels;
        for(i = 0; i < layout->numTextureChannels; i++) {
            dispObj->textureData[i] = layout->textureData[i];
        }
    } else {
        dispObj->textureData = NULL;
        dispObj->numTextureChannels = 0;
    }
    if (layout->lightingData != 0) {
        dispObj->lightingData = OSAllocFromHeap(__OSCurrHeap, 0xC);
        *dispObj->lightingData = *layout->lightingData;
    } else {
        dispObj->lightingData = NULL;
    }
    if (layout->displayData != 0) {
        dispObj->displayData = OSAllocFromHeap(__OSCurrHeap, 0xC);
        *dispObj->displayData = *layout->displayData;
    } else {
        dispObj->displayData = NULL;
    }
    dispObj->visibility = 1;
    MTXIdentity(dispObj->worldMatrix);
    dispObj->shaderFunc = NULL;
    dispObj->shaderData = NULL;
}
