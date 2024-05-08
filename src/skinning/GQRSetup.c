#include <dolphin.h>
#include "charPipeline.h"

// functions
static void __MTGQR0(u32 val);
static void __MTGQR1(u32 val);
static void __MTGQR2(u32 val);
static void __MTGQR3(u32 val);
static void __MTGQR4(u32 val);
static void __MTGQR5(u32 val);
static void __MTGQR6(u32 val);
static void __MTGQR7(u32 val);

asm static void __MTGQR0(register u32 val) {
    nofralloc
    mtspr GQR0, val
    blr
}

asm static void __MTGQR1(register u32 val) {
    nofralloc
    mtspr GQR1, val
    blr
}

asm static void __MTGQR2(register u32 val) {
    nofralloc
    mtspr GQR2, val
    blr
}

asm static void __MTGQR3(register u32 val) {
    nofralloc
    mtspr GQR3, val
    blr
}

asm static void __MTGQR4(register u32 val) {
    nofralloc
    mtspr GQR4, val
    blr
}

asm static void __MTGQR5(register u32 val) {
    nofralloc
    mtspr GQR5, val
    blr
}

asm static void __MTGQR6(register u32 val) {
    nofralloc
    mtspr GQR6, val
    blr
}

asm static void __MTGQR7(register u32 val) {
    nofralloc
    mtspr GQR7, val
    blr
}

void GQRSetup0(u32 loadScale, u32 loadType, u32 storeScale, u32 storeType) {
    u32 reg;

    reg  = ((loadScale  << 8) + loadType) << 0x10;
    reg |=  (storeScale << 8) + storeType;

    __MTGQR0(reg);
}

void GQRSetup1(u32 loadScale, u32 loadType, u32 storeScale, u32 storeType) {
    u32 reg;

    reg  = ((loadScale  << 8) + loadType) << 0x10;
    reg |=  (storeScale << 8) + storeType;

    __MTGQR1(reg);
}

void GQRSetup2(u32 loadScale, u32 loadType, u32 storeScale, u32 storeType) {
    u32 reg;

    reg  = ((loadScale  << 8) + loadType) << 0x10;
    reg |=  (storeScale << 8) + storeType;

    __MTGQR2(reg);
}

void GQRSetup3(u32 loadScale, u32 loadType, u32 storeScale, u32 storeType) {
    u32 reg;

    reg  = ((loadScale  << 8) + loadType) << 0x10;
    reg |=  (storeScale << 8) + storeType;

    __MTGQR3(reg);
}

void GQRSetup4(u32 loadScale, u32 loadType, u32 storeScale, u32 storeType) {
    u32 reg;

    reg  = ((loadScale  << 8) + loadType) << 0x10;
    reg |=  (storeScale << 8) + storeType;

    __MTGQR4(reg);
}

void GQRSetup5(u32 loadScale, u32 loadType, u32 storeScale, u32 storeType) {
    u32 reg;

    reg  = ((loadScale  << 8) + loadType) << 0x10;
    reg |=  (storeScale << 8) + storeType;

    __MTGQR5(reg);
}

void GQRSetup6(u32 loadScale, u32 loadType, u32 storeScale, u32 storeType) {
    u32 reg;

    reg  = ((loadScale  << 8) + loadType) << 0x10;
    reg |=  (storeScale << 8) + storeType;

    __MTGQR6(reg);
}

void GQRSetup7(u32 loadScale, u32 loadType, u32 storeScale, u32 storeType) {
    u32 reg;

    reg  = ((loadScale  << 8) + loadType) << 0x10;
    reg |=  (storeScale << 8) + storeType;

    __MTGQR7(reg);
}
