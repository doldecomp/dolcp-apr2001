#ifndef _DOLPHIN_CP_ACTOR_H_
#define _DOLPHIN_CP_ACTOR_H_

typedef struct sBone sBone;

void ACTSetBoneTime(sBone *bone, f32 time);
void ACTSetBoneSpeed(sBone *bone, f32 speed);
void ACTTickBone(sBone *bone);
void ACTSetBoneTrack(sBone *bone, struct ANIMAnimTrack *track, f32 time);

#endif // _DOLPHIN_CP_ACTOR_H_
