#include <dolphin.h>
#include <dolphin/mtx.h>
#include "charPipeline.h"

#define qr0 0
#define qr6 6
#define qr7 7

// functions
void SKN1Vecs16Norms16(ROMtx m, s16 * srcBase, s16 * dstBase, u32 count);
void SKN2Vecs16Norms16(ROMtx m0, ROMtx m1, u8 * wtBase, s16 * srcBase, s16 * dstBase, u32 count);
void SKN2Vecs16Norms16NoTouch(ROMtx m0, ROMtx m1, u8 * wtBase, s16 * srcBase, s16 * dstBase, u32 count);
void SKNAccVecs16Norms16Iu16(ROMtx m, u16 count, s16 * srcBase, s16 * dstBase, u16 * indices, u8 * weights);

asm void SKN1Vecs16Norms16(register ROMtx m, register s16 * srcBase, register s16 * dstBase, register u32 count) {
    nofralloc
	stwu r1, -64(r1)
	stfd f14, 8(r1)
	stfd f15, 16(r1)
	subi count, count, 1
	stfd f16, 24(r1)
	stfd f17, 32(r1)
	stfd f18, 40(r1)
	mtctr count
	psq_l f0, 0(m), 0, qr0
	subi srcBase, srcBase, 4
	psq_l f1, 8(m), 1, qr0
	subi r5, r5, 2
	psq_l f6, 36(m), 0, qr0
	psq_lu f8, 4(srcBase), 0, qr7
	psq_l f7, 44(m), 1, qr0
	psq_lu f9, 4(srcBase), 0, qr7
	ps_madds0 f11, f0, f8, f6
	psq_l f2, 12(m), 0, qr0
	ps_madds0 f12, f1, f8, f7
	psq_l f3, 20(m), 1, qr0
	ps_muls1 f13, f0, f9
	psq_lu f10, 4(srcBase), 0, qr7
	ps_muls1 f14, f1, f9
	psq_l f5, 32(m), 1, qr0
	ps_madds1 f11, f2, f8, f11
	ps_madds1 f12, f3, f8, f12
	psq_l f4, 24(m), 0, qr0
	ps_madds0 f13, f2, f10, f13
	psq_lu f8, 4(srcBase), 0, qr7
	ps_madds0 f14, f3, f10, f14
	ps_madds0 f15, f4, f9, f11
	ps_madds0 f16, f5, f9, f12
	psq_lu f9, 4(srcBase), 0, qr7
	ps_madds1 f17, f4, f10, f13
	ps_madds1 f18, f5, f10, f14
	psq_lu f10, 4(srcBase), 0, qr7
loop:
	ps_madds0 f11, f0, f8, f6
	psq_stu f15, 2(dstBase), 0, qr7
	ps_madds0 f12, f1, f8, f7
	psq_stu f16, 4(dstBase), 1, qr7
	ps_muls1 f13, f0, f9
	psq_stu f17, 2(dstBase), 0, qr7
	ps_muls1 f14, f1, f9
	psq_stu f18, 4(dstBase), 1, qr7
	ps_madds1 f11, f2, f8, f11
	ps_madds1 f12, f3, f8, f12
	psq_lu f8, 4(srcBase), 0, qr7
	ps_madds0 f13, f2, f10, f13
	ps_madds0 f14, f3, f10, f14
	ps_madds0 f15, f4, f9, f11
	ps_madds0 f16, f5, f9, f12
	psq_lu f9, 4(srcBase), 0, qr7
	ps_madds1 f17, f4, f10, f13
	ps_madds1 f18, f5, f10, f14
	psq_lu f10, 4(srcBase), 0, qr7
	bdnz loop
	psq_stu f15, 2(dstBase), 0, qr7
	psq_stu f16, 4(dstBase), 1, qr7
	psq_stu f17, 2(dstBase), 0, qr7
	psq_stu f18, 4(dstBase), 1, qr7
	lfd f14, 8(r1)
	lfd f15, 16(r1)
	lfd f16, 24(r1)
	lfd f17, 32(r1)
	lfd f18, 40(r1)
	addi r1, r1, 64
	blr
}

asm void SKN2Vecs16Norms16(register ROMtx m0, register ROMtx m1, register u8 * wtBase, register s16 * srcBase, register s16 * dstBase, register u32 count) {
    nofralloc
	stwu r1, -160(r1)
	stfd f14, 8(r1)
	subi r9, count, 1
	stfd f15, 16(r1)
	stfd f16, 24(r1)
	stfd f17, 32(r1)
	stfd f18, 40(r1)
	stfd f19, 48(r1)
	stfd f20, 56(r1)
	stfd f21, 64(r1)
	li r10, 32
	stfd f22, 72(r1)
	stfd f23, 80(r1)
	stfd f24, 88(r1)
	stfd f25, 96(r1)
	stfd f26, 104(r1)
	stfd f27, 112(r1)
	mtctr r9
	psq_l f0, 0(m0), 0, qr0
	subi srcBase, srcBase, 4
	psq_l f1, 8(m0), 1, qr0
	subi r7, r7, 2
	psq_l f6, 36(m0), 0, qr0
	subi wtBase, wtBase, 2
	psq_lu f8, 4(srcBase), 0, qr7
	psq_l f7, 44(m0), 1, qr0
	psq_lu f9, 4(srcBase), 0, qr7
	psq_lu f27, 2(wtBase), 0, qr6
	ps_madds0 f15, f0, f8, f6
	psq_l f2, 12(m0), 0, qr0
	ps_madds0 f16, f1, f8, f7
	psq_l f3, 20(m0), 1, qr0
	ps_muls1 f17, f0, f9
	psq_lu f10, 4(srcBase), 0, qr7
	ps_muls1 f18, f1, f9
	psq_l f5, 32(m0), 1, qr0
	ps_madds1 f15, f2, f8, f15
	psq_l f19, 0(m1), 0, qr0
	ps_madds1 f16, f3, f8, f16
	psq_l f4, 24(m0), 0, qr0
	ps_madds0 f17, f2, f10, f17
	psq_l f20, 8(m1), 0, qr0
	ps_madds0 f18, f3, f10, f18
	psq_l f21, 12(m1), 0, qr0
	ps_madds0 f15, f4, f9, f15
	psq_l f22, 20(m1), 0, qr0
	ps_madds0 f16, f5, f9, f16
	psq_l f23, 24(m1), 0, qr0
	ps_madds1 f17, f4, f10, f17
	psq_l f24, 32(m1), 0, qr0
	ps_madds1 f18, f5, f10, f18
	psq_l f25, 36(m1), 0, qr0
	ps_muls0 f15, f15, f27
	psq_l f26, 44(m1), 0, qr0
	ps_muls0 f16, f16, f27
	ps_muls0 f17, f17, f27
	ps_muls0 f18, f18, f27
	ps_madds0 f11, f19, f8, f25
	ps_madds0 f12, f20, f8, f26
	ps_muls1 f13, f19, f9
	ps_muls1 f14, f20, f9
	ps_madds1 f11, f21, f8, f11
	ps_madds1 f12, f22, f8, f12
	ps_madds0 f13, f21, f10, f13
	psq_lu f8, 4(srcBase), 0, qr7
	ps_madds0 f14, f22, f10, f14
	ps_madds0 f11, f23, f9, f11
	ps_madds0 f12, f24, f9, f12
	ps_madds1 f13, f23, f10, f13
	ps_madds1 f14, f24, f10, f14
	psq_lu f9, 4(srcBase), 0, qr7
	ps_madds1 f11, f11, f27, f15
	ps_madds1 f12, f12, f27, f16
	ps_madds1 f13, f13, f27, f17
	psq_lu f10, 4(srcBase), 0, qr7
	ps_madds1 f14, f14, f27, f18
loop:
	ps_madds0 f15, f0, f8, f6
	psq_stu f11, 2(dstBase), 0, qr7
	ps_madds0 f16, f1, f8, f7
	psq_stu f12, 4(dstBase), 1, qr7
	ps_muls1 f17, f0, f9
	psq_stu f13, 2(dstBase), 0, qr7
	ps_muls1 f18, f1, f9
	psq_stu f14, 4(dstBase), 1, qr7
	ps_madds1 f15, f2, f8, f15
	ps_madds1 f16, f3, f8, f16
	dcbt r10, srcBase
	ps_madds0 f17, f2, f10, f17
	ps_madds0 f18, f3, f10, f18
	ps_madds0 f15, f4, f9, f15
	ps_madds0 f16, f5, f9, f16
	psq_lu f27, 2(wtBase), 0, qr6
	ps_madds1 f17, f4, f10, f17
	ps_madds1 f18, f5, f10, f18
	ps_muls0 f15, f15, f27
	ps_muls0 f16, f16, f27
	ps_muls0 f17, f17, f27
	ps_muls0 f18, f18, f27
	ps_madds0 f11, f19, f8, f25
	ps_madds0 f12, f20, f8, f26
	dcbt r10, dstBase
	ps_muls1 f13, f19, f9
	ps_muls1 f14, f20, f9
	ps_madds1 f11, f21, f8, f11
	ps_madds1 f12, f22, f8, f12
	ps_madds0 f13, f21, f10, f13
	psq_lu f8, 4(srcBase), 0, qr7
	ps_madds0 f14, f22, f10, f14
	ps_madds0 f11, f23, f9, f11
	ps_madds0 f12, f24, f9, f12
	ps_madds1 f13, f23, f10, f13
	ps_madds1 f14, f24, f10, f14
	psq_lu f9, 4(srcBase), 0, qr7
	ps_madds1 f11, f11, f27, f15
	ps_madds1 f12, f12, f27, f16
	ps_madds1 f13, f13, f27, f17
	psq_lu f10, 4(srcBase), 0, qr7
	ps_madds1 f14, f14, f27, f18
	bdnz loop
	psq_stu f11, 2(dstBase), 0, qr7
	psq_stu f12, 4(dstBase), 1, qr7
	psq_stu f13, 2(dstBase), 0, qr7
	psq_stu f14, 4(dstBase), 1, qr7
	lfd f14, 8(r1)
	lfd f15, 16(r1)
	lfd f16, 24(r1)
	lfd f17, 32(r1)
	lfd f18, 40(r1)
	lfd f19, 48(r1)
	lfd f20, 56(r1)
	lfd f21, 64(r1)
	lfd f22, 72(r1)
	lfd f23, 80(r1)
	lfd f24, 88(r1)
	lfd f25, 96(r1)
	lfd f26, 104(r1)
	lfd f27, 112(r1)
	addi r1, r1, 160
	blr
}

asm void SKN2Vecs16Norms16NoTouch(register ROMtx m0, register ROMtx m1, register u8 * wtBase, register s16 * srcBase, register s16 * dstBase, register u32 count) {
    nofralloc
	stwu r1, -160(r1)
	stfd f14, 8(r1)
	subi r9, count, 1
	stfd f15, 16(r1)
	stfd f16, 24(r1)
	stfd f17, 32(r1)
	stfd f18, 40(r1)
	stfd f19, 48(r1)
	stfd f20, 56(r1)
	stfd f21, 64(r1)
	stfd f22, 72(r1)
	stfd f23, 80(r1)
	stfd f24, 88(r1)
	stfd f25, 96(r1)
	stfd f26, 104(r1)
	stfd f27, 112(r1)
	mtctr r9
	psq_l f0, 0(m0), 0, qr0
	subi srcBase, srcBase, 4
	psq_l f1, 8(m0), 1, qr0
	subi dstBase, dstBase, 2
	psq_l f6, 36(m0), 0, qr0
	subi wtBase, wtBase, 2
	psq_lu f8, 4(srcBase), 0, qr7
	psq_l f7, 44(m0), 1, qr0
	psq_lu f9, 4(srcBase), 0, qr7
	psq_lu f27, 2(wtBase), 0, qr6
	ps_madds0 f15, f0, f8, f6
	psq_l f2, 12(m0), 0, qr0
	ps_madds0 f16, f1, f8, f7
	psq_l f3, 20(m0), 1, qr0
	ps_muls1 f17, f0, f9
	psq_lu f10, 4(srcBase), 0, qr7
	ps_muls1 f18, f1, f9
	psq_l f5, 32(m0), 1, qr0
	ps_madds1 f15, f2, f8, f15
	psq_l f19, 0(m1), 0, qr0
	ps_madds1 f16, f3, f8, f16
	psq_l f4, 24(m0), 0, qr0
	ps_madds0 f17, f2, f10, f17
	psq_l f20, 8(m1), 1, qr0
	ps_madds0 f18, f3, f10, f18
	psq_l f21, 12(m1), 0, qr0
	ps_madds0 f15, f4, f9, f15
	psq_l f22, 20(m1), 1, qr0
	ps_madds0 f16, f5, f9, f16
	psq_l f23, 24(m1), 0, qr0
	ps_madds1 f17, f4, f10, f17
	psq_l f24, 32(m1), 1, qr0
	ps_madds1 f18, f5, f10, f18
	psq_l f25, 36(m1), 0, qr0
	ps_muls0 f15, f15, f27
	psq_l f26, 44(m1), 1, qr0
	ps_muls0 f16, f16, f27
	ps_muls0 f17, f17, f27
	ps_muls0 f18, f18, f27
	ps_madds0 f11, f19, f8, f25
	ps_madds0 f12, f20, f8, f26
	ps_muls1 f13, f19, f9
	ps_muls1 f14, f20, f9
	ps_madds1 f11, f21, f8, f11
	ps_madds1 f12, f22, f8, f12
	ps_madds0 f13, f21, f10, f13
	psq_lu f8, 4(srcBase), 0, qr7
	ps_madds0 f14, f22, f10, f14
	ps_madds0 f11, f23, f9, f11
	ps_madds0 f12, f24, f9, f12
	ps_madds1 f13, f23, f10, f13
	ps_madds1 f14, f24, f10, f14
	psq_lu f9, 4(srcBase), 0, qr7
	ps_madds1 f11, f11, f27, f15
	ps_madds1 f12, f12, f27, f16
	ps_madds1 f13, f13, f27, f17
	psq_lu f10, 4(srcBase), 0, qr7
	ps_madds1 f14, f14, f27, f18
loop:
	ps_madds0 f15, f0, f8, f6
	psq_stu f11, 2(dstBase), 0, qr7
	ps_madds0 f16, f1, f8, f7
	psq_stu f12, 4(dstBase), 1, qr7
	ps_muls1 f17, f0, f9
	psq_stu f13, 2(dstBase), 0, qr7
	ps_muls1 f18, f1, f9
	psq_stu f14, 4(dstBase), 1, qr7
	ps_madds1 f15, f2, f8, f15
	ps_madds1 f16, f3, f8, f16
	ps_madds0 f17, f2, f10, f17
	ps_madds0 f18, f3, f10, f18
	ps_madds0 f15, f4, f9, f15
	ps_madds0 f16, f5, f9, f16
	psq_lu f27, 2(wtBase), 0, qr6
	ps_madds1 f17, f4, f10, f17
	ps_madds1 f18, f5, f10, f18
	ps_muls0 f15, f15, f27
	ps_muls0 f16, f16, f27
	ps_muls0 f17, f17, f27
	ps_muls0 f18, f18, f27
	ps_madds0 f11, f19, f8, f25
	ps_madds0 f12, f20, f8, f26
	ps_muls1 f13, f19, f9
	ps_muls1 f14, f20, f9
	ps_madds1 f11, f21, f8, f11
	ps_madds1 f12, f22, f8, f12
	ps_madds0 f13, f21, f10, f13
	psq_lu f8, 4(srcBase), 0, qr7
	ps_madds0 f14, f22, f10, f14
	ps_madds0 f11, f23, f9, f11
	ps_madds0 f12, f24, f9, f12
	ps_madds1 f13, f23, f10, f13
	ps_madds1 f14, f24, f10, f14
	psq_lu f9, 4(srcBase), 0, qr7
	ps_madds1 f11, f11, f27, f15
	ps_madds1 f12, f12, f27, f16
	ps_madds1 f13, f13, f27, f17
	psq_lu f10, 4(srcBase), 0, qr7
	ps_madds1 f14, f14, f27, f18
	bdnz loop
	psq_stu f11, 2(dstBase), 0, qr7
	psq_stu f12, 4(dstBase), 1, qr7
	psq_stu f13, 2(dstBase), 0, qr7
	psq_stu f14, 4(dstBase), 1, qr7
	lfd f14, 8(r1)
	lfd f15, 16(r1)
	lfd f16, 24(r1)
	lfd f17, 32(r1)
	lfd f18, 40(r1)
	lfd f19, 48(r1)
	lfd f20, 56(r1)
	lfd f21, 64(r1)
	lfd f22, 72(r1)
	lfd f23, 80(r1)
	lfd f24, 88(r1)
	lfd f25, 96(r1)
	lfd f26, 104(r1)
	lfd f27, 112(r1)
	addi r1, r1, 160
	blr
}

asm void SKNAccVecs16Norms16Iu16(register ROMtx m, register u16 count, register s16 * srcBase, register s16 * dstBase, register u16 * indices, register u8 * weights) {
    nofralloc
	stwu r1, -160(r1)
	subi indices, indices, 2
	stfd f14, 8(r1)
	lhzu r9, 2(indices)
	stfd f15, 16(r1)
	stfd f16, 24(r1)
	add r11, r9, r9
	stfd f17, 32(r1)
	add r9, r9, r11
	stfd f18, 40(r1)
	add r9, r9, r9
	stfd f19, 48(r1)
	add r9, r9, r9
	stfd f20, 56(r1)
	add r10, r9, dstBase
	stfd f21, 64(r1)
	subi weights, weights, 1
	stfd f22, 72(r1)
	stfd f23, 80(r1)
	mtctr count
	psq_l f0, 0(m), 0, qr0
	subi r5, r5, 4
	psq_l f1, 8(m), 1, qr0
	psq_l f6, 36(m), 0, qr0
	psq_l f7, 44(m), 1, qr0
	psq_lu f8, 4(r5), 0, qr7
	psq_lu f9, 4(r5), 0, qr7
	psq_l f2, 12(m), 0, qr0
	psq_l f3, 20(m), 1, qr0
	psq_lu f10, 4(r5), 0, qr7
	psq_l f5, 32(m), 1, qr0
	psq_l f4, 24(m), 0, qr0
loop:
	ps_madds0 f11, f0, f8, f6
	psq_lu f23, 1(weights), 1, qr6
	ps_madds0 f12, f1, f8, f7
	ps_muls1 f13, f0, f9
	psq_l f19, 0(r10), 0, qr7
	ps_muls1 f14, f1, f9
	ps_madds1 f11, f2, f8, f11
	ps_madds1 f12, f3, f8, f12
	psq_l f20, 4(r10), 1, qr7
	ps_madds0 f13, f2, f10, f13
	ps_madds0 f14, f3, f10, f14
	psq_l f21, 6(r10), 0, qr7
	ps_madds0 f15, f4, f9, f11
	ps_madds0 f16, f5, f9, f12
	lhzu r9, 2(indices)
	ps_madds1 f17, f4, f10, f13
	ps_madds1 f18, f5, f10, f14
	psq_l f22, 10(r10), 1, qr7
	ps_madds0 f19, f15, f23, f19
	psq_lu f8, 4(srcBase), 0, qr7
	ps_madds0 f20, f16, f23, f20
	psq_lu f9, 4(srcBase), 0, qr7
	ps_madds0 f21, f17, f23, f21
	psq_lu f10, 4(srcBase), 0, qr7
	ps_madds0 f22, f18, f23, f22
	add r11, r9, r9
	psq_st f19, 0(r10), 0, qr7
	add r9, r9, r11
	psq_st f20, 4(r10), 1, qr7
	add r9, r9, r9
	psq_st f21, 6(r10), 0, qr7
	add r9, r9, r9
	psq_st f22, 10(r10), 1, qr7
	dcbst r0, r10
	add r10, r9, dstBase
	bdnz loop
	lfd f14, 8(r1)
	lfd f15, 16(r1)
	lfd f16, 24(r1)
	lfd f17, 32(r1)
	lfd f18, 40(r1)
	lfd f19, 48(r1)
	lfd f20, 56(r1)
	lfd f21, 64(r1)
	lfd f22, 72(r1)
	lfd f23, 80(r1)
	addi r1, r1, 160
	blr
}
