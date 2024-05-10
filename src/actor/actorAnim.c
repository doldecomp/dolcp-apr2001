#include <dolphin.h>
#include "charPipeline.h"

void ACTSetAnimation(Actor *actor, ANIMBank *animBank, char *sequenceName, u16 seqNum, f32 time) {
    u16 i;
    ANIMSequences *animSeq;
    struct ANIMAnimTrack * track;

    animSeq = ANIMGetSequence(animBank, sequenceName, seqNum);
    for(i = 0; i < actor->totalBones; i++) {
        track = ANIMGetTrackFromSeq(animSeq, actor->boneArray[i]->boneID);
        ACTSetBoneTrack(actor->boneArray[i], track, time);
    }
}

void ACTSetTime(Actor *actor, f32 time) {
    u16 i;

    for(i = 0; i < actor->totalBones; i++) {
        ACTSetBoneTime(actor->boneArray[i], time);
    }
}

void ACTSetSpeed(Actor *actor, f32 speed) {
    u16 i;

    for(i = 0; i < actor->totalBones; i++) {
        ACTSetBoneSpeed(actor->boneArray[i], speed);
    }
}

void ACTTick(Actor *actor) {
    u16 i;

    for(i = 0; i < actor->totalBones; i++) {
        ACTTickBone(actor->boneArray[i]);
    }
}
