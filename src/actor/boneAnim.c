#include <dolphin.h>
#include "charPipeline.h"

void ACTSetBoneTime(sBone *bone, f32 time) {
    ANIMSetTime(bone->animPipe, time);
}

void ACTSetBoneSpeed(sBone *bone, f32 speed) {
    ANIMSetSpeed(bone->animPipe, speed);
}

void ACTTickBone(sBone *bone) {
    ANIMTick(bone->animPipe);
}

void ACTSetBoneTrack(sBone *bone, struct ANIMAnimTrack *track, f32 time) {
    u8 hierControlType;
    Quaternion param;

    if (track) {
        if (bone->animPipe == 0) {
            bone->animPipe = OSAllocFromHeap(__OSCurrHeap, 0x14);
            bone->animPipe->time = 0.0f;
            bone->animPipe->speed = 1.0f;
            bone->animPipe->currentTrack = NULL;
            bone->animPipe->control = NULL;
            bone->animPipe->replaceHierarchyCtrl = 0;
        }
        ANIMBind(bone->animPipe, &bone->animationCtrl, track, time);
        hierControlType = bone->orientationCtrl.type;
        if ((hierControlType & 8) && !(track->animType & 1)) {
            CTRLGetTranslation(&bone->orientationCtrl, &param.x, &param.y, &param.z);
            CTRLSetTranslation(&bone->animationCtrl, param.x, param.y, param.z);
        }
        if ((hierControlType & 1) && !(track->animType & 2)) {
            CTRLGetScale(&bone->orientationCtrl, &param.x, &param.y, &param.z);
            CTRLSetScale(&bone->animationCtrl, param.x, param.y, param.z);
        }
        if ((hierControlType & 2) && !(track->animType & 4)) {
            CTRLGetRotation(&bone->orientationCtrl, &param.x, &param.y, &param.z);
            CTRLSetRotation(&bone->animationCtrl, param.x, param.y, param.z);
            return;
        }
        if ((hierControlType & 4) && !(track->animType & 8)) {
            CTRLGetQuat(&bone->orientationCtrl, &param.x, &param.y, &param.z, &param.w);
            CTRLSetQuat(&bone->animationCtrl, param.x, param.y, param.z, param.w);
        }
    }
}
