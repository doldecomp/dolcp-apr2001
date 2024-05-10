#ifndef _DOLPHIN_CP_ACTOR_H_
#define _DOLPHIN_CP_ACTOR_H_

#include "control.h"

typedef struct DODisplayData *DODisplayDataPtr;
typedef struct sBone sBone;
typedef struct sHdr sHdr;
typedef struct ANIMBank ANIMBank;

typedef struct {
    /* 0x00 */ u32 Offset;
    /* 0x04 */ char * Root;
} Heirarchy;

typedef struct {
    /* 0x00 */ u32 versionNumber;
    /* 0x04 */ u16 actorID;
    /* 0x06 */ u16 totalBones;
    /* 0x08 */ Heirarchy hierarchy;
    /* 0x10 */ char * geoPaletteName;
    /* 0x14 */ u16 skinFileID;
    /* 0x16 */ u16 pad16;
    /* 0x18 */ u32 userDataSize;
    /* 0x1C */ char * userData;
} ActorLayout;

typedef struct {
    /* 0x00 */ u32 Offset;
    /* 0x04 */ char * Head;
    /* 0x08 */ char * Tail;
} ActorList;

typedef struct {
    /* 0x00 */ ActorLayout *layout;
    /* 0x04 */ u16 actorID;
    /* 0x06 */ u16 totalBones;
    /* 0x08 */ Heirarchy hierarchy;
    /* 0x10 */ DODisplayDataPtr *pal;
    /* 0x14 */ struct DODisplayObj *skinObject;
    /* 0x18 */ sBone **boneArray;
    /* 0x1C */ Control worldControl;
    /* 0x50 */ MtxPtr forwardMtxArray;
    /* 0x54 */ MtxPtr skinMtxArray;
    /* 0x58 */ MtxPtr skinInvTransposeMtxArray;
    /* 0x5C */ MtxPtr orientationInvMtxArray;
    /* 0x60 */ ActorList drawPriorityList;
    /* 0x6C */ sHdr * skHeader;
} Actor;

// actorAnim.c
void ACTSetAnimation(Actor *actor, ANIMBank *animBank, char *sequenceName, u16 seqNum, f32 time);
void ACTSetTime(Actor *actor, f32 time);
void ACTSetSpeed(Actor *actor, f32 speed);
void ACTTick(Actor *actor);

// boneAnim.c
void ACTSetBoneTime(sBone *bone, f32 time);
void ACTSetBoneSpeed(sBone *bone, f32 speed);
void ACTTickBone(sBone *bone);
void ACTSetBoneTrack(sBone *bone, struct ANIMAnimTrack *track, f32 time);

#endif // _DOLPHIN_CP_ACTOR_H_