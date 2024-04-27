#ifndef _DOLPHIN_CP_ANIM_H_
#define _DOLPHIN_CP_ANIM_H_

#include "unktypes.h"

struct ANIMAnimTrack {
    /* 0x00 */ f32 animTime;
    /* 0x04 */ KeyFrame * keyFrames;
    /* 0x08 */ u16 totalFrames;
    /* 0x0A */ u16 trackID;
    /* 0x0C */ u8 quantizeInfo;
    /* 0x0D */ u8 animType;
    /* 0x0E */ u8 interpolationType;
    /* 0x0F */ u8 replaceHierarchyCtrl;
};

struct ANIMPipe {
    /* 0x00 */ f32 time;
    /* 0x04 */ f32 speed;
    /* 0x08 */ struct ANIMAnimTrack * currentTrack;
    /* 0x0C */ Control * control;
    /* 0x10 */ u8 replaceHierarchyCtrl;
};

#endif // _DOLPHIN_CP_ANIM_H_
