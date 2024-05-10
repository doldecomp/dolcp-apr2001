#include <dolphin.h>
#include "charPipeline.h"
#include "charPipeline/structures/dolphinString.h"

void ANIMGet(ANIMBank **animBank, char *name) {
    ANIMBank *anmBank;
    ANIMSequences *seq;
    struct ANIMAnimTrack * track;
    KeyFrame *frame;
    struct DVDFileInfo dfi;
    u32 i;
    
    if (DVDOpen(name, &dfi) == 0) {
        OSReport("ANIMGet: Could not find file %s", name);
        OSPanic(__FILE__, 0x2E, "");
    }
    anmBank = OSAllocFromHeap(__OSCurrHeap, (dfi.length + 0x1F) & 0xFFFFFFE0);
    DVDReadPrio(&dfi, anmBank, (dfi.length + 0x1F) & 0xFFFFFFE0, 0, 2);
    DVDClose(&dfi);
    if (anmBank->versionNumber != 8092000) {
        OSReport("ANIMGet: Incompatible version number %d for %s, since\n", anmBank->versionNumber, name);
        OSReport("         the library version number is %d.\n", 8092000);
        OSPanic(__FILE__, 0x38, "");
    }
    if (anmBank->userDataSize != 0 && (anmBank->userData != 0)) {
        anmBank->userData = (void*)((u32)anmBank->userData + (u32)anmBank);
    }
    seq = (ANIMSequences*)((u32)anmBank + 0x18);
    track = (void*)&seq[anmBank->numSequences];
    frame = (void*)&track[anmBank->numTracks];
    anmBank->animSequences = (ANIMSequences*)((u32)anmBank->animSequences + (u32)anmBank);
    for(i = 0; i < anmBank->numSequences; i++) {
        seq[i].sequenceName = (void*)((u32)seq[i].sequenceName + (u32)anmBank);
        seq[i].trackArray = (void*)((u32)seq[i].trackArray + (u32)anmBank);
    }
    for(i = 0; i < anmBank->numTracks; i++) {
        track[i].keyFrames = (void*)((u32)track[i].keyFrames + (u32)anmBank);
    }
    for(i = 0; i < anmBank->numKeyFrames; i++) {
        frame[i].setting = (void*)((u32)frame[i].setting + (u32)anmBank);
        frame[i].interpolation = (void*)((u32)frame[i].interpolation + (u32)anmBank);
    }
    *animBank = anmBank;
}

void ANIMRelease(ANIMBank **animBank) {
    OSFreeToHeap(__OSCurrHeap, *animBank);
    *animBank = 0;
}

ANIMSequences *ANIMGetSequence(ANIMBank *animBank, char *sequenceName, u16 seqNum) {
    u32 i;

    if (sequenceName) {
        for(i = 0; i < animBank->numSequences; i++) {
            if (Strcmp(animBank->animSequences[i].sequenceName, sequenceName) == 0) {
                return &animBank->animSequences[i];
            }
        }
    } else {
        ASSERTMSGLINE(0x72, seqNum < animBank->numSequences, "Specified sequence ID too large for the animBank");
        return &animBank->animSequences[seqNum];
    }
    return 0;
}

struct ANIMAnimTrack *ANIMGetTrackFromSeq(ANIMSequences *animSeq, u16 animTrackID) {
    u32 i;

    for(i = 0; i < animSeq->totalTracks; i++) {
        if (animTrackID == animSeq->trackArray[i].trackID) {
            return &animSeq->trackArray[i];
        }
    }
    return 0;
}

void ANIMGetKeyFrameFromTrack(struct ANIMAnimTrack * animTrack, f32 time, KeyFrame **currentFrame, KeyFrame **nextFrame) {
    u16 i;

    for(i = 0; i < animTrack->totalFrames; i++) {
        if (animTrack->keyFrames[i].time <= time) {
            if (i < (animTrack->totalFrames - 1)) {
                if (animTrack->keyFrames[i + 1].time > time) {
                    if (currentFrame != 0) {
                        *currentFrame = &animTrack->keyFrames[i];
                    }
                    if (nextFrame != 0) {
                        *nextFrame = &animTrack->keyFrames[i + 1];
                    }
                    return;
                }
            } else {
                if (currentFrame != 0) {
                    *currentFrame = &animTrack->keyFrames[i];
                }
                if (nextFrame != 0) {
                    *nextFrame = &animTrack->keyFrames[0];
                }
                return;
            }
        }
    }
    ASSERTMSGLINE(0xA2, 0, "Keyframes not found");
}

u32 ANIMGetUserDataSize(ANIMBank *animBank) {
    ASSERTLINE(0xA8, animBank);
    return animBank->userDataSize;
}

char *ANIMGetUserData(ANIMBank *animBank) {
    ASSERTLINE(0xAF, animBank);
    return animBank->userData;
}
