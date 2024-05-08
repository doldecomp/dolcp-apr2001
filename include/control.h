#ifndef _DOLPHIN_CP_CONTROL_H_
#define _DOLPHIN_CP_CONTROL_H_

#include <dolphin/mtx.h>

typedef struct Srt {
    /* 0x00 */ Vec s;
    /* 0x0C */ Quaternion r;
    /* 0x1C */ Vec t;
} Srt;

typedef union ControlParams {
    /* 0x00 */ Srt srt;
    /* 0x00 */ struct {
        /* 0x00 */ Mtx m;
    } mtx;
} ControlParams;

typedef struct Control {
    /* 0x00 */ u8 type;
    /* 0x01 */ u8 pad8;
    /* 0x02 */ u16 pad16;
    /* 0x04 */ ControlParams controlParams;
} Control;

// unsorted externs
extern void CTRLBuildMatrix(struct Control *control, Mtx44Ptr m);

#endif // _DOLPHIN_CP_CONTROL_H_
