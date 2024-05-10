#include <dolphin.h>
#include "charPipeline.h"
#include "charPipeline/fileCache.h"

// functions
static void InitActorWithLayout(Actor *actor, ActorLayout *layout);
static void InitBone(sBone *parent, ActorBone *layout, Actor *actor, u16 *numBones);
static void LoadActorLayout(ActorLayout **layout, char *name);
static void BuildBoneSkinOrientationMatrix(sBone *bone, MtxPtr orientation);
static void FreeActorLayout(ActorLayout **layout);

static void InitActorWithLayout(Actor *actor, ActorLayout *layout) {
    sBone *temp;
    u16 numBones;
    Mtx identity;

    temp = NULL;
    numBones = 0;
    actor->skHeader = NULL;
    MTXIdentity(identity);
    actor->actorID = layout->actorID;
    actor->layout = layout;
    actor->totalBones = layout->totalBones;
    DSInitTree(&actor->hierarchy, (void*)temp, (void*)&temp->branch);
    DSInitList(&actor->drawPriorityList, (void*)temp, (void*)&temp->drawPriorityLink);
    actor->pal = 0;
    if (layout->geoPaletteName != 0U) {
        actor->skHeader = GEOGetPalette(&actor->pal, layout->geoPaletteName);
    }
    actor->boneArray = OSAllocFromHeap(__OSCurrHeap, layout->totalBones * 4);
    actor->forwardMtxArray = OSAllocFromHeap(__OSCurrHeap, layout->totalBones * 0x30);
    if (layout->skinFileID != 0xFFFF) {
        DOGet(&actor->skinObject, actor->pal, layout->skinFileID, 0);
        DOSetWorldMatrix(actor->skinObject, identity);
        actor->skinMtxArray = OSAllocFromHeap(__OSCurrHeap, layout->totalBones * 0x30);
        actor->orientationInvMtxArray = OSAllocFromHeap(__OSCurrHeap, layout->totalBones * 0x30);
        if (actor->skinObject->lightingData != 0U) {
            actor->skinInvTransposeMtxArray = OSAllocFromHeap(__OSCurrHeap, layout->totalBones * 0x30);
        } else {
            actor->skinInvTransposeMtxArray = NULL;
        }
    } else {
        actor->skinObject = NULL;
        actor->skinMtxArray = NULL;
        actor->orientationInvMtxArray = NULL;
        actor->skinInvTransposeMtxArray = NULL;
    }
    actor->worldControl.type = 0;
    if (layout->hierarchy.Root != 0) {
        InitBone(NULL, (void*)layout->hierarchy.Root, actor, &numBones);
    }
    if (numBones != actor->totalBones) {
        OSPanic(__FILE__, 0x84, "Incorrect number of bones in actor");
    }
    if (actor->skinObject != 0) {
        BuildBoneSkinOrientationMatrix((void*)actor->hierarchy.Root, identity);
    }
    ACTSort(actor);
}

static void InitBone(sBone *parent, ActorBone *layout, Actor *actor, u16 *numBones) {
    sBone *newBone;

    for(; layout; layout = (void*)layout->branch.Next) {
        newBone = OSAllocFromHeap(__OSCurrHeap, 0x9C);
        newBone->branch.Children = NULL;
        DSInsertBranchBelow(&actor->hierarchy, (Ptr)parent, (Ptr)newBone);
        actor->boneArray[*numBones] = newBone;
        newBone->boneID = layout->boneID;
        newBone->inheritanceFlag = layout->inheritanceFlag;
        newBone->drawingPriority = layout->drawingPriority;
        newBone->orientationCtrl.type = 0;
        if (layout->orientationCtrl != 0) {
            *((Control*)((char*)&newBone->branch.Children + 0x8)) = *(Control*)layout->orientationCtrl; // i dont think this is correct, but what else is it doing?
        }
        newBone->dispObj = 0;
        if (layout->geoFileID != 0xFFFF) {
            DOGet(&newBone->dispObj, actor->pal, layout->geoFileID, 0);
        }
        newBone->animPipe = 0;
        newBone->animationCtrl.type = 0;
        newBone->forwardMtx = &actor->forwardMtxArray[*numBones * 3];
        MTXIdentity(newBone->forwardMtx);
        if (actor->skinObject != 0) {
            newBone->skinMtx = &actor->skinMtxArray[*numBones * 3];
            MTXIdentity(newBone->skinMtx);
            newBone->orientationInvMtx = &actor->orientationInvMtxArray[*numBones * 3];
            MTXIdentity(newBone->orientationInvMtx);
            if (actor->skinObject->lightingData != 0U) {
                newBone->skinInvTransposeMtx = &actor->skinInvTransposeMtxArray[*numBones * 3];
                MTXIdentity(newBone->skinInvTransposeMtx);
            } else {
                newBone->skinInvTransposeMtx = NULL;
            }
        } else {
            newBone->skinMtx = NULL;
            newBone->orientationInvMtx = NULL;
            newBone->skinInvTransposeMtx = NULL;
        }
        *numBones+=1;
        if (layout->branch.Children != 0) {
            InitBone(newBone, (void*)layout->branch.Children, actor, numBones);
        }
    }
}

static void LoadActorLayout(ActorLayout **layout, char *name) {
    ActorLayout *actLayout;
    ActorBone *bone;
    struct DVDFileInfo dfi;
    u32 i;

    if (DVDOpen(name, &dfi) == 0) {
        OSReport("LoadActorLayout: Could not find file %s", name);
        OSPanic(__FILE__, 0xF0, "");
    }
    actLayout = OSAllocFromHeap(__OSCurrHeap, (dfi.length + 0x1F) & 0xFFFFFFE0);
    DVDReadPrio(&dfi, actLayout, (dfi.length + 0x1F) & 0xFFFFFFE0, 0, 2);
    DVDClose(&dfi);
    if (actLayout->versionNumber != 8092000) {
        OSReport("LoadActorLayout: Incompatible version number %d for %s, since\n", actLayout->versionNumber, name);
        OSReport("                 the library version number is %d.\n", 8092000);
        OSPanic(__FILE__, 0xFB, "");
    }
    if (actLayout->userDataSize != 0 && actLayout->userData != 0) {
        actLayout->userData = (void*)((u32)actLayout->userData + (u32)actLayout);
    }
    if (actLayout->hierarchy.Root != 0) {
        actLayout->hierarchy.Root = (void*)((u32)actLayout->hierarchy.Root + (u32)actLayout);
    }
    if (actLayout->geoPaletteName != 0) {
        actLayout->geoPaletteName = (void*)((u32)actLayout->geoPaletteName + (u32)actLayout);
    }
    bone = (void*)((u32)actLayout + sizeof(ActorLayout));
    for(i = 0; i < actLayout->totalBones; i++) {
        if (bone[i].orientationCtrl) {
            bone[i].orientationCtrl = (void*)((u32)bone[i].orientationCtrl + (u32)actLayout);
        }
        if (bone[i].branch.Prev) {
            bone[i].branch.Prev = (void*)((u32)bone[i].branch.Prev + (u32)actLayout);
        }
        if (bone[i].branch.Next) {
            bone[i].branch.Next = (void*)((u32)bone[i].branch.Next + (u32)actLayout);
        }
        if (bone[i].branch.Parent) {
            bone[i].branch.Parent = (void*)((u32)bone[i].branch.Parent + (u32)actLayout);
        }
        if (bone[i].branch.Children) {
            bone[i].branch.Children = (void*)((u32)bone[i].branch.Children + (u32)actLayout);
        }
    }
    *layout = actLayout;
}

void ACTGet(Actor **actor, char *name) {
    ActorLayout *layout;

    layout = NULL;
    if (*actor != 0) {
        ACTRelease(actor);
    }
    *actor = OSAllocFromHeap(__OSCurrHeap, 0x70);
    if (DOCacheInitialized != 0) {
        layout = (void*)DSGetCacheObj(&DODisplayCache, name);
    }
    if (layout == 0) {
        LoadActorLayout(&layout, name);
        if (DOCacheInitialized != 0) {
            DSAddCacheNode(&DODisplayCache, name, (Ptr)layout, (Ptr)FreeActorLayout);
            DSGetCacheObj(&DODisplayCache, name);
        }
    }
    InitActorWithLayout(*actor, layout);
}

void ACTRelease(Actor **actor) {
    u16 numBones;

    for(numBones = 0; numBones < (*actor)->totalBones; numBones++) {
        if ((*actor)->boneArray[numBones]->dispObj) {
            DORelease(&(*actor)->boneArray[numBones]->dispObj);
        }
        if ((*actor)->boneArray[numBones]->animPipe) {
            OSFreeToHeap(__OSCurrHeap, (*actor)->boneArray[numBones]->animPipe);
            (*actor)->boneArray[numBones]->animPipe = 0;
        }
        OSFreeToHeap(__OSCurrHeap, (*actor)->boneArray[numBones]);
    }
    if ((*actor)->skinObject != 0) {
        OSFreeToHeap(__OSCurrHeap, (*actor)->skinMtxArray);
        OSFreeToHeap(__OSCurrHeap, (*actor)->orientationInvMtxArray);
        if ((*actor)->skinObject->lightingData != 0U) {
            OSFreeToHeap(__OSCurrHeap, (*actor)->skinInvTransposeMtxArray);
        }
        DORelease(&(*actor)->skinObject);
    }
    if ((*actor)->pal != 0U) {
        GEOReleasePalette(&(*actor)->pal);
    }
    OSFreeToHeap(__OSCurrHeap, (*actor)->forwardMtxArray);
    OSFreeToHeap(__OSCurrHeap, (*actor)->boneArray);
    if (DOCacheInitialized != 0) {
        if ((*actor)->layout != 0) {
            DSReleaseCacheObj(&DODisplayCache, (Ptr)(*actor)->layout);
        }
    } else if ((*actor)->layout != 0) {
        OSFreeToHeap(__OSCurrHeap, (*actor)->layout);
        (*actor)->layout = NULL;
    }
    OSFreeToHeap(__OSCurrHeap, *actor);
    *actor = NULL;
}

void ACTRender(Actor *actor, MtxPtr camera, u8 numLights, ...) {
    sBone *temp;
    va_list ptr;

    if (actor->skinObject != 0U) {
        va_start(ptr, numLights);
        if (actor->skHeader != 0U) {
            DOVARender(actor->skinObject, camera, numLights, &ptr);
        } else {
            DOVARenderSkin(actor->skinObject, camera, actor->skinMtxArray, actor->skinInvTransposeMtxArray, numLights, &ptr);
        }
        va_end(ptr);
    }
    for(temp = (void*)actor->drawPriorityList.Head; temp; temp = (void*)temp->drawPriorityLink.Next) {
        va_start(ptr, numLights);
        DOVARender(temp->dispObj, camera, numLights, &ptr);
        va_end(ptr);
    }
}

void ACTHide(Actor *actor) {
    u16 numBones;

    if (actor->skinObject != 0U) {
        DOHide(actor->skinObject);
    }
    for(numBones = 0; numBones < actor->totalBones; numBones++) {
        if (actor->boneArray[numBones]->dispObj) {
            DOHide(actor->boneArray[numBones]->dispObj);
        }
    }
}

void ACTShow(Actor *actor) {
    u16 numBones;

    if (actor->skinObject != 0U) {
        DOShow(actor->skinObject);
    }
    for(numBones = 0; numBones < actor->totalBones; numBones++) {
        if (actor->boneArray[numBones]->dispObj) {
            DOShow(actor->boneArray[numBones]->dispObj);
        }
    }
}

static void BuildBoneSkinOrientationMatrix(sBone *bone, MtxPtr orientation) {
    Mtx boneMtx;
    Mtx newOrientation;

    for (; bone; bone = (void*)bone->branch.Next) {
        if (bone->inheritanceFlag == 1) {
            CTRLBuildMatrix(&bone->orientationCtrl, boneMtx);
            MTXConcat(orientation, boneMtx, newOrientation);
            MTXInverse(newOrientation, bone->orientationInvMtx);
        } else {
            CTRLBuildInverseMatrix(&bone->orientationCtrl, bone->orientationInvMtx);
        }
        if (bone->branch.Children) {
            BuildBoneSkinOrientationMatrix((void*)bone->branch.Children, newOrientation);
        }
    }
}

void ACTBuildMatrices(Actor *actor) {
    Mtx actorMtx;
    Mtx animatedBoneMtx;
    Mtx orientationMtx;
    sBone *bone;
    sBone *parent;
    u16 boneIndex;

    CTRLBuildMatrix(&actor->worldControl, actorMtx);
    for(boneIndex = 0; boneIndex < actor->totalBones; boneIndex++) {
        bone = actor->boneArray[boneIndex];
        if (bone->animationCtrl.type != 0) {
            CTRLBuildMatrix(&bone->animationCtrl, animatedBoneMtx);
            if (bone->animPipe->replaceHierarchyCtrl == 0) {
                CTRLBuildMatrix(&bone->orientationCtrl, orientationMtx);
                MTXConcat(orientationMtx, animatedBoneMtx, animatedBoneMtx);
            }
        } else {
            CTRLBuildMatrix(&bone->orientationCtrl, animatedBoneMtx);
        }
        parent = (void*)bone->branch.Parent;
        if (bone->inheritanceFlag == 1 && parent != 0) {
            MTXConcat(parent->forwardMtx, animatedBoneMtx, bone->forwardMtx);
        } else {
            MTXConcat(actorMtx, animatedBoneMtx, bone->forwardMtx);
        }
        if (bone->dispObj != 0) {
            DOSetWorldMatrix(bone->dispObj, bone->forwardMtx);
        }
        if (actor->skinObject != 0) {
            MTXConcat(bone->forwardMtx, bone->orientationInvMtx, bone->skinMtx);
            if (actor->skinObject->lightingData != 0U) {
                MTXInverse(bone->skinMtx, animatedBoneMtx);
                MTXTranspose(animatedBoneMtx, bone->skinInvTransposeMtx);
            }
        }
    }
    if (actor->skHeader != 0U) {
        SKNIt(actor->boneArray, actor->skHeader);
    }
}

Control *ACTGetControl(Actor *actor) {
    return &actor->worldControl;
}

void ACTSetInheritance(Actor *actor, u8 inheritanceFlag) {
    u16 i;

    for(i = 0; i < actor->totalBones; i++) {
        actor->boneArray[i]->inheritanceFlag = inheritanceFlag;
    }
}

static void FreeActorLayout(ActorLayout **layout) {
    OSFreeToHeap(__OSCurrHeap, *layout);
    *layout = NULL;
}

void ACTSetAmbientPercentage(Actor *actor, f32 percent) {
    u16 numBones;

    numBones = 0;
    if (actor->skinObject != 0U) {
        DOSetAmbientPercentage(actor->skinObject, percent);
    }
    for(; numBones < actor->totalBones; numBones++) {
        if (actor->boneArray[numBones]->dispObj) {
            DOSetAmbientPercentage(actor->boneArray[numBones]->dispObj, percent);
        }
    }
}

void ACTSort(Actor *actor) {
    u16 numBones;
    sBone *temp = NULL;

    numBones = 0;
    actor->drawPriorityList.Head = NULL;
    actor->drawPriorityList.Tail = NULL;
    for(; numBones < actor->totalBones; numBones++) {
        if (actor->boneArray[numBones]->dispObj) {
            for(temp = (void*)actor->drawPriorityList.Head; temp; temp = (void*)temp->drawPriorityLink.Next) {
                if (temp->drawingPriority > actor->boneArray[numBones]->drawingPriority) {
                    break;
                }
            }
            DSInsertListObject(&actor->drawPriorityList, (Ptr)temp, (Ptr)actor->boneArray[numBones]);
        }
    }
}

void ACTSetEffectsShader(Actor *actor, char *shaderFunc, char *data) {
    u16 numBones;

    numBones = 0;
    if (actor->skinObject != 0U) {
        DOSetEffectsShader(actor->skinObject, shaderFunc, data);
    }
    for(; numBones < actor->totalBones; numBones++) {
        if (actor->boneArray[numBones]->dispObj) {
            DOSetEffectsShader(actor->boneArray[numBones]->dispObj, shaderFunc, data);
        }
    }
}

u32 ACTGetUserDataSize(Actor *actor) {
    ASSERTLINE(0x25D, actor && actor->layout);
    return actor->layout->userDataSize;
}

char *ACTGetUserData(Actor *actor) {
    ASSERTLINE(0x264, actor && actor->layout);
    return actor->layout->userData;
}
