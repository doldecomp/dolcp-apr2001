#ifndef _DOLPHIN_CP_SKINNING_H_
#define _DOLPHIN_CP_SKINNING_H_

typedef struct _sStats {
    /* 0x00 */ u32 cycles[2];
    /* 0x08 */ u32 loadStores[2];
    /* 0x10 */ u32 missCycles[2];
    /* 0x18 */ u32 instructions[2];
    /* 0x20 */ u32 cycleTotal;
    /* 0x24 */ u32 loadStoreTotal;
    /* 0x28 */ u32 missCycleTotal;
    /* 0x2C */ u32 instructionTotal;
    /* 0x30 */ u32 hits;
} sStats;

#endif // _DOLPHIN_CP_SKINNING_H_
