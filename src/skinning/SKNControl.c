#include <dolphin.h>
#include "skinning.h"

sAccBuffers SKAccBuffers[2];
void * SKBuffers[4];

static u32 Frames;

// functions
static void SKNFlushByIndex(s16 * indices, u32 count, s16 * base);

asm static void SKNFlushByIndex(register s16 * indices, register u32 count, register s16 * base) {
    nofralloc
	mtctr count
	subi indices, indices, 2
loop:
	lhzu r6, 2(indices)
	add r7, r6, r6
	add r6, r7, r6
	add r6, r6, r6
	add r6, r6, r6
	dcbf r6, base
	bdnz loop
	blr
}

void SKNIt(sBone **bones, sHdr *hdr) {
    u32 i;
    u32 vnsize;
    u16 blocks[2];
    SK1List *curr1;
    SK2List *curr2;
    SKAccList *currA;

    PPCMtmmcr1(0);
    PPCMtmmcr0(0);
    SKNStatistics[0].cycles[0] = PPCMfpmc4();
    SKNStatistics[0].missCycles[0] = PPCMfpmc3();
    SKNStatistics[0].loadStores[0] = PPCMfpmc2();
    SKNStatistics[0].instructions[0] = PPCMfpmc1();
    PPCMtmmcr0(0x8B);
    PPCMtmmcr1(0x78400000);
    ASSERTLINE(0x9D, hdr->posNrmShift > 0);
    ASSERTLINE(0x9E, hdr->posNrmShift < GQR_SCALE_MAX);
    GQRSetup7(hdr->posNrmShift, 7, hdr->posNrmShift, 7);
    PPCMtmmcr1(0);
    PPCMtmmcr0(0);
    SKNStatistics[1].cycles[0] = PPCMfpmc4();
    SKNStatistics[1].missCycles[0] = PPCMfpmc3();
    SKNStatistics[1].loadStores[0] = PPCMfpmc2();
    SKNStatistics[1].instructions[0] = PPCMfpmc1();
    PPCMtmmcr0(0x8B);
    PPCMtmmcr1(0x78400000);

    if (hdr->numSk1List != 0) {
        ASSERTLINE(0xAF, hdr->sk1ListArray);
        ASSERTLINE(0xB0, hdr->sk1ListArray[0].count);
        ASSERTLINE(0xB1, hdr->sk1ListArray[0].vertSrc);
        vnsize = hdr->sk1ListArray[0].vertSrcOffset + hdr->sk1ListArray[0].count * 0xC;
        blocks[0] = SKNLCTouchData(0, hdr->sk1ListArray->vertSrc, vnsize);
        for(i = 0; i < (hdr->numSk1List - 1); i++) {
            vnsize = hdr->sk1ListArray[i+1].vertSrcOffset + hdr->sk1ListArray[i+1].count * 0xC;
            blocks[((i + 1) & 1)] = SKNLCTouchData(((i + 1) & 1) * 2, hdr->sk1ListArray[i + 1].vertSrc, vnsize);
            curr1 = &hdr->sk1ListArray[i];
            PSMTXReorder(bones[curr1->boneIndex]->skinMtx, curr1->m);
            LCQueueWait(1);
            ASSERTLINE(0xD0, curr1->count > 2);
            SKNLC1Vecs16Norms16(curr1->m, curr1->vertDst, curr1->count, (i & 1) * 2, curr1->vertSrcOffset, blocks[i & 1]);
        }
        curr1 = &hdr->sk1ListArray[i];
        PSMTXReorder(bones[curr1->boneIndex]->skinMtx, curr1->m);
        ASSERTLINE(0xDF, curr1->count > 2);
        SKNLC1Vecs16Norms16(curr1->m, curr1->vertDst, curr1->count, (i & 1) * 2, curr1->vertSrcOffset, blocks[i & 1]);
    }
    PPCMtmmcr1(0);
    PPCMtmmcr0(0);
    SKNStatistics[1].hits++;
    SKNStatistics[1].cycles[1] = PPCMfpmc4();
    SKNStatistics[1].missCycles[1] = PPCMfpmc3();
    SKNStatistics[1].loadStores[1] = PPCMfpmc2();
    SKNStatistics[1].instructions[1] = PPCMfpmc1();
    SKNStatistics[1].cycleTotal += (SKNStatistics[1].cycles[1] - SKNStatistics[1].cycles[0]);
    SKNStatistics[1].missCycleTotal += (SKNStatistics[1].missCycles[1] - SKNStatistics[1].missCycles[0]);
    SKNStatistics[1].loadStoreTotal += (SKNStatistics[1].loadStores[1] - SKNStatistics[1].loadStores[0]);
    SKNStatistics[1].instructionTotal += (SKNStatistics[1].instructions[1] - SKNStatistics[1].instructions[0]);
    PPCMtmmcr0(0x8B);
    PPCMtmmcr1(0x78400000);
    PPCMtmmcr1(0);
    PPCMtmmcr0(0);
    SKNStatistics[2].cycles[0] = PPCMfpmc4();
    SKNStatistics[2].missCycles[0] = PPCMfpmc3();
    SKNStatistics[2].loadStores[0] = PPCMfpmc2();
    SKNStatistics[2].instructions[0] = PPCMfpmc1();
    PPCMtmmcr0(0x8B);
    PPCMtmmcr1(0x78400000);

    if (hdr->numSk2List > 0) {
        ASSERTLINE(0xF1, hdr->sk2ListArray);
        ASSERTLINE(0xF2, hdr->sk2ListArray[0].count);
        ASSERTLINE(0xF3, hdr->sk2ListArray[0].vertSrc);
        vnsize = hdr->sk2ListArray[0].vertSrcOffset + hdr->sk2ListArray[0].count * 0xC;
        blocks[0] = SKNLC2TouchData(0, hdr->sk2ListArray[0].vertSrc, vnsize);
        SKNLC2TouchData(1, hdr->sk2ListArray[0].weights, hdr->sk2ListArray[0].count * 2);
        for(i = 0; i < (hdr->numSk2List - 1); i++) {
            curr2 = &hdr->sk2ListArray[i];
            vnsize = hdr->sk2ListArray[i+1].vertSrcOffset + hdr->sk2ListArray[i+1].count * 12;
            blocks[(i + 1) % 2] = SKNLC2TouchData((u8)(((i + 1) % 2) * 2), hdr->sk2ListArray[i+1].vertSrc, vnsize);
            SKNLC2TouchData((u8)(((i + 1) % 2) * 2) + 1, hdr->sk2ListArray[i+1].weights, hdr->sk2ListArray[i+1].count * 2);
            PSMTXReorder(bones[curr2->bone0Index]->skinMtx, curr2->m0);
            PSMTXReorder(bones[curr2->bone1Index]->skinMtx, curr2->m1);
            ASSERTLINE(0x112, curr2->count > 2);
            LCQueueWait(2);
            SKNLC2Vecs16Norms16(curr2->m0, curr2->m1, (i & 1) * 2, curr2->vertDst, curr2->count, curr2->vertSrcOffset, blocks[i & 1]);
        }
        curr2 = &hdr->sk2ListArray[i];
        PSMTXReorder(bones[curr2->bone0Index]->skinMtx, curr2->m0);
        PSMTXReorder(bones[curr2->bone1Index]->skinMtx, curr2->m1);
        ASSERTLINE(0x121, curr2->count > 2);
        LCQueueWait(0);
        SKNLC2Vecs16Norms16(curr2->m0, curr2->m1, (i & 1) * 2, curr2->vertDst, curr2->count, curr2->vertSrcOffset, blocks[i & 1]);
    }

    PPCMtmmcr1(0);
    PPCMtmmcr0(0);
    SKNStatistics[2].hits++;
    SKNStatistics[2].cycles[1] = PPCMfpmc4();
    SKNStatistics[2].missCycles[1] = PPCMfpmc3();
    SKNStatistics[2].loadStores[1] = PPCMfpmc2();
    SKNStatistics[2].instructions[1] = PPCMfpmc1();
    SKNStatistics[2].cycleTotal += (SKNStatistics[2].cycles[1] - SKNStatistics[2].cycles[0]);
    SKNStatistics[2].missCycleTotal += (SKNStatistics[2].missCycles[1] - SKNStatistics[2].missCycles[0]);
    SKNStatistics[2].loadStoreTotal += (SKNStatistics[2].loadStores[1] - SKNStatistics[2].loadStores[0]);
    SKNStatistics[2].instructionTotal += (SKNStatistics[2].instructions[1] - SKNStatistics[2].instructions[0]);
    PPCMtmmcr0(0x8B);
    PPCMtmmcr1(0x78400000);
    PPCMtmmcr1(0);
    PPCMtmmcr0(0);
    SKNStatistics[3].cycles[0] = PPCMfpmc4();
    SKNStatistics[3].missCycles[0] = PPCMfpmc3();
    SKNStatistics[3].loadStores[0] = PPCMfpmc2();
    SKNStatistics[3].instructions[0] = PPCMfpmc1();
    PPCMtmmcr0(0x8B);
    PPCMtmmcr1(0x78400000);
    
    if (hdr->numSkAccList > 0) {
        ASSERTLINE(0x134, hdr->skAccListArray);
        ASSERTLINE(0x135, hdr->skAccListArray[0].count);
        ASSERTLINE(0x136, hdr->skAccListArray[0].vertSrc);
        PPCMtmmcr1(0);
        PPCMtmmcr0(0);
        SKNStatistics[4].cycles[0] = PPCMfpmc4();
        SKNStatistics[4].missCycles[0] = PPCMfpmc3();
        SKNStatistics[4].loadStores[0] = PPCMfpmc2();
        SKNStatistics[4].instructions[0] = PPCMfpmc1();
        PPCMtmmcr0(0x8B);
        PPCMtmmcr1(0x78400000);
        if (hdr->skBzeroSize) {
            SKNBzero32B(hdr->skBzeroBase, ((hdr->skBzeroSize + 0x1F) << 5) >> 5U);
        }
        PPCMtmmcr1(0);
        PPCMtmmcr0(0);
        SKNStatistics[4].hits++;
        SKNStatistics[4].cycles[1] = PPCMfpmc4();
        SKNStatistics[4].missCycles[1] = PPCMfpmc3();
        SKNStatistics[4].loadStores[1] = PPCMfpmc2();
        SKNStatistics[4].instructions[1] = PPCMfpmc1();
        SKNStatistics[4].cycleTotal += (SKNStatistics[4].cycles[1] - SKNStatistics[4].cycles[0]);
        SKNStatistics[4].missCycleTotal += (SKNStatistics[4].missCycles[1] - SKNStatistics[4].missCycles[0]);
        SKNStatistics[4].loadStoreTotal += (SKNStatistics[4].loadStores[1] - SKNStatistics[4].loadStores[0]);
        SKNStatistics[4].instructionTotal += (SKNStatistics[4].instructions[1] - SKNStatistics[4].instructions[0]);
        PPCMtmmcr0(0x8B);
        PPCMtmmcr1(0x78400000);
        SKNLCAccTouchData(0, hdr->skAccListArray[0].vertSrc, hdr->skAccListArray[0].vertIndices, hdr->skAccListArray[0].weights, hdr->skAccListArray[0].count);
        LCQueueWait(0);
        for(i = 0; i < (hdr->numSkAccList - 1); i++) {
            SKNLCAccTouchData((u8)(((i + 1) % 2)), hdr->skAccListArray[i+1].vertSrc, hdr->skAccListArray[i+1].vertIndices, hdr->skAccListArray[i+1].weights, hdr->skAccListArray[i+1].count);
            currA = &hdr->skAccListArray[i];
            PSMTXReorder(bones[currA->boneIndex]->skinMtx, currA->m);
            SKNAccVecs16Norms16Iu16(currA->m, currA->count, SKAccBuffers[(u8)(i % 2)].src, currA->vertDst, SKAccBuffers[(u8)(i % 2)].indices, SKAccBuffers[(u8)(i % 2)].weights);
        }
        currA = &hdr->skAccListArray[i];
        PSMTXReorder(bones[currA->boneIndex]->skinMtx, currA->m);
        SKNAccVecs16Norms16Iu16(currA->m, currA->count, SKAccBuffers[(u8)(i % 2)].src, currA->vertDst, SKAccBuffers[(u8)(i % 2)].indices, SKAccBuffers[(u8)(i % 2)].weights);
        PPCMtmmcr1(0);
        PPCMtmmcr0(0);
        SKNStatistics[3].hits++;
        SKNStatistics[3].cycles[1] = PPCMfpmc4();
        SKNStatistics[3].missCycles[1] = PPCMfpmc3();
        SKNStatistics[3].loadStores[1] = PPCMfpmc2();
        SKNStatistics[3].instructions[1] = PPCMfpmc1();
        SKNStatistics[3].cycleTotal += (SKNStatistics[3].cycles[1] - SKNStatistics[3].cycles[0]);
        SKNStatistics[3].missCycleTotal += (SKNStatistics[3].missCycles[1] - SKNStatistics[3].missCycles[0]);
        SKNStatistics[3].loadStoreTotal += (SKNStatistics[3].loadStores[1] - SKNStatistics[3].loadStores[0]);
        SKNStatistics[3].instructionTotal += (SKNStatistics[3].instructions[1] - SKNStatistics[3].instructions[0]);
        PPCMtmmcr0(0x8B);
        PPCMtmmcr1(0x78400000);
        PPCMtmmcr1(0);
        PPCMtmmcr0(0);
        SKNStatistics[5].cycles[0] = PPCMfpmc4();
        SKNStatistics[5].missCycles[0] = PPCMfpmc3();
        SKNStatistics[5].loadStores[0] = PPCMfpmc2();
        SKNStatistics[5].instructions[0] = PPCMfpmc1();
        PPCMtmmcr0(0x8B);
        PPCMtmmcr1(0x78400000);
        DCStoreRange(hdr->skBzeroBase, (hdr->skBzeroSize + 0x1F) << 5 >> 5);
        if (hdr->flushCount) {
            ASSERTLINE(0x17A, hdr->numSkAccList);
            SKNFlushByIndex(hdr->flushIndices, hdr->flushCount, hdr->skAccListArray[0].vertDst);
        }
        PPCMtmmcr1(0);
        PPCMtmmcr0(0);
        SKNStatistics[5].hits++;
        SKNStatistics[5].cycles[1] = PPCMfpmc4();
        SKNStatistics[5].missCycles[1] = PPCMfpmc3();
        SKNStatistics[5].loadStores[1] = PPCMfpmc2();
        SKNStatistics[5].instructions[1] = PPCMfpmc1();
        SKNStatistics[5].cycleTotal += (SKNStatistics[5].cycles[1] - SKNStatistics[5].cycles[0]);
        SKNStatistics[5].missCycleTotal += (SKNStatistics[5].missCycles[1] - SKNStatistics[5].missCycles[0]);
        SKNStatistics[5].loadStoreTotal += (SKNStatistics[5].loadStores[1] - SKNStatistics[5].loadStores[0]);
        SKNStatistics[5].instructionTotal += (SKNStatistics[5].instructions[1] - SKNStatistics[5].instructions[0]);
        PPCMtmmcr0(0x8B);
        PPCMtmmcr1(0x78400000);
    }
    PPCMtmmcr1(0);
    PPCMtmmcr0(0);
    SKNStatistics[0].hits++;
    SKNStatistics[0].cycles[1] = PPCMfpmc4();
    SKNStatistics[0].missCycles[1] = PPCMfpmc3();
    SKNStatistics[0].loadStores[1] = PPCMfpmc2();
    SKNStatistics[0].instructions[1] = PPCMfpmc1();
    SKNStatistics[0].cycleTotal += (SKNStatistics[0].cycles[1] - SKNStatistics[0].cycles[0]);
    SKNStatistics[0].missCycleTotal += (SKNStatistics[0].missCycles[1] - SKNStatistics[0].missCycles[0]);
    SKNStatistics[0].loadStoreTotal += (SKNStatistics[0].loadStores[1] - SKNStatistics[0].loadStores[0]);
    SKNStatistics[0].instructionTotal += (SKNStatistics[0].instructions[1] - SKNStatistics[0].instructions[0]);
    PPCMtmmcr0(0x8B);
    PPCMtmmcr1(0x78400000);
    if (Frames++ == 666) {
        SKNPrintStats();
    }
}

void SKNInit(void) {
    u8 * base;
    u32 i;

    if ((PPCMfhid2() & 0x10000000) == 0) {
        DCInvalidateRange((void*)0xE0000000, 0x4000);
        LCEnable();
    }
    base = (void*)0xE0000000;
    for(i = 0; i < 4; i++) {
        SKBuffers[i] = base;
        base += 0x1000;
    }
    base = (void*)0xE0000000;
    for(i = 0; i < 2; i++) {
        SKAccBuffers[i].src = (void*)base;
        base += 0x1000;
        SKAccBuffers[i].indices = (void*)base;
        base += 0x800;
        SKAccBuffers[i].weights = (void*)base;
        base += 0x800;
    }
    GQRSetup6(8, 4, 8, 4);
}

asm void SKNBzero32B(register void *base, register u32 size) {
    nofralloc
	li r0, 0
	srwi size, size, 5
	mtctr size
loop:
	dcbz r0, base
	addi base, base, 32
	bdnz loop
	blr
}

void SKNLCBzero(void *base, u32 size) {
    u32 numBlocks;

    numBlocks = (u32) (size + 0x1F) >> 5U;
    DCZeroRange(SKBuffers[0], 0x1000);
    while (numBlocks) {
        if (numBlocks < 0x80U) {
            LCStoreBlocks(base, SKBuffers[0], numBlocks);
            return;
        }
        LCStoreBlocks(base, SKBuffers[0], 0);
        numBlocks -= 0x80;
        ((char*)base) = ((char*)base) + 0x1000;
    }
}
