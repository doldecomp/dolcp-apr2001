#include <dolphin.h>
#include "charPipeline.h"
#include "fake_tgmath.h"

static f32 QuatShift = 0.000061035159f;
static f32 EaseShift = 0.000061035159f;
static u8 QuatType = 3;
static u8 EaseType = 3;

// functions
static void DoMatrixAnimation(struct ANIMPipe * animPipe, struct KeyFrame *startFrame, struct KeyFrame *endFrame);
static void DoQuatAnimation(struct ANIMPipe * animPipe, struct KeyFrame *startFrame, struct KeyFrame *endFrame);
static void DoEulerAnimation(struct ANIMPipe * animPipe, struct KeyFrame *startFrame, struct KeyFrame *endFrame);
static void DoScaleAnimation(struct ANIMPipe * animPipe, struct KeyFrame *startFrame, struct KeyFrame *endFrame);
static void DoTranslationAnimation(struct ANIMPipe * animPipe, struct KeyFrame *startFrame, struct KeyFrame *endFrame);
static u32 GetDataSize(u8 quantType);
static void GetData(u8 quantType, f32 quantShift, char * from, f32 * to, u8 count);
static void LinearInterpolateVec(f32 * start, f32 * end, f32 time, f32 * final);
static void LinearInterpolateVecEuler(f32 * start, f32 * end, f32 time, f32 * final);
static void Ticktime(struct ANIMPipe * animPipe);
static f32 NormalizeTracktime(struct ANIMPipe * animPipe, struct KeyFrame *startFrame, struct KeyFrame *endFrame);
static void BezierInterpolateVecEuler(f32 * start, f32 * end, f32 u, f32 * outControl, f32 * inControl, f32 * final);
static void BezierInterpolateVec(f32 * start, f32 * end, f32 u, f32 * outControl, f32 * inControl, f32 * final);
static void HermiteInterpolateVec(f32 * start, f32 * end, f32 * basis, f32 * outControl, f32 * inControl, f32 * final);
static void ComputeHermiteBasis(f32 u, f32 * v);
static f32 Ease(f32 u, f32 a, f32 b);
static void Slerp(Quaternion *p, Quaternion *q, f32 t, Quaternion *r);
static void Squad(Quaternion *p, Quaternion *a, Quaternion *b, Quaternion *q, f32 t, Quaternion *r);

void ANIMBind(struct ANIMPipe *animPipe, Control *control, struct ANIMAnimTrack *animTrack, f32 time) {
    if (animTrack) {
        control->type = 0;
        animPipe->control = control;
        animPipe->currentTrack = animTrack;
        animPipe->time = time;
        animPipe->replaceHierarchyCtrl = animTrack->replaceHierarchyCtrl;
    }
}

void ANIMSetTime(struct ANIMPipe * animPipe, f32 time) {
    if (animPipe) {
        animPipe->time = time;
    }
}

void ANIMSetSpeed(struct ANIMPipe * animPipe, f32 speed) {
    if (animPipe) {
        animPipe->speed = speed;
    }
}

void ANIMTick(struct ANIMPipe * animPipe) {
    KeyFrame *startFrame;
    KeyFrame *endFrame;

    if (animPipe) {
        ANIMGetKeyFrameFromTrack(animPipe->currentTrack, animPipe->time, &startFrame, &endFrame);
        if (animPipe->currentTrack->animType & 0x10) {
            DoMatrixAnimation(animPipe, startFrame, endFrame);
            return;
        }
        if (animPipe->currentTrack->animType & 2) {
            DoScaleAnimation(animPipe, startFrame, endFrame);
        }
        if (animPipe->currentTrack->animType & 8) {
            DoQuatAnimation(animPipe, startFrame, endFrame);
        } else if (animPipe->currentTrack->animType & 4) {
            DoEulerAnimation(animPipe, startFrame, endFrame);
        }
        if (animPipe->currentTrack->animType & 1) {
            DoTranslationAnimation(animPipe, startFrame, endFrame);
        }
        Ticktime(animPipe);
    }
}

static void DoMatrixAnimation(struct ANIMPipe * animPipe, struct KeyFrame *startFrame, struct KeyFrame *endFrame) {
    f32 shift;
    u32 i;

    shift = 1.0f / (1 << (animPipe->currentTrack->quantizeInfo & 0xF));
    for(i = 0; i < 12; i++) {
        ((f32*)startFrame->setting)[i] *= shift;
    }
    CTRLSetMatrix(animPipe->control, (void*)startFrame->setting);
}

static void DoQuatAnimation(struct ANIMPipe * animPipe, struct KeyFrame *startFrame, struct KeyFrame *endFrame) {
    Quaternion final;
    f32 start[4];
    f32 end[4];
    f32 a[4];
    f32 b[4];
    f32 easeIn;
    f32 easeOut;
    u32 offset;
    f32 time;
    u32 data;
    u32 paramSize;
    u8 quantType;

    quantType = animPipe->currentTrack->quantizeInfo >> 4;
    paramSize = GetDataSize(quantType) * 3;
    offset = 0;
    if (animPipe->currentTrack->animType & 2) {
        offset = paramSize;
    }
    GetData(QuatType, QuatShift, &startFrame->setting[offset], start, 4);
    GetData(QuatType, QuatShift, &endFrame->setting[offset], end, 4);
    time = NormalizeTracktime(animPipe, startFrame, endFrame);
    offset = 0;
    if (animPipe->currentTrack->animType & 2) {
        data = (animPipe->currentTrack->interpolationType >> 2) & 3;
        if (data == 2) {
            offset = (paramSize * 2);
        }
        if (data == 3) {
            offset = (paramSize * 2) + 4;
        }
    }
    
    switch ((u8)((animPipe->currentTrack->interpolationType >> 4) & 7)) {
        case 5:
            GetData(EaseType, EaseShift, &endFrame->interpolation[offset] + 0x10, &easeIn, 1);
            GetData(EaseType, EaseShift, &startFrame->interpolation[offset] + 0x12, &easeOut, 1);
            time = Ease(time, easeOut, easeIn);
        case 4:
            GetData(QuatType, QuatShift, &startFrame->interpolation[offset] + 8, a, 4);
            GetData(QuatType, QuatShift, &endFrame->interpolation[offset], b, 4);
            Squad((QtrnPtr)start, (QtrnPtr)a, (QtrnPtr)b, (QtrnPtr)end, time, &final);
            break;
        case 6:
            Slerp((QtrnPtr)start, (QtrnPtr)end, time, &final);
            break;
        case 0:
            final.x = start[0];
            final.y = start[1];
            final.z = start[2];
            final.w = start[3];
            break;
        default:
            OSPanic(__FILE__, 0xFC, "Unknown interpolation setting!");
            break;
    }
    CTRLSetQuat(animPipe->control, final.x, final.y, final.z, final.w);
}

static void DoEulerAnimation(struct ANIMPipe * animPipe, struct KeyFrame *startFrame, struct KeyFrame *endFrame) {
    f32 start[3];
    f32 end[3];
    f32 final[3];
    f32 outControl[3];
    f32 inControl[3];
    u32 offset;
    f32 time;
    u32 data;
    u32 paramSize;
    f32 quantShift;
    u8 quantType;

    quantShift = 1.0f / (1 << (animPipe->currentTrack->quantizeInfo & 0xF));
    quantType = animPipe->currentTrack->quantizeInfo >> 4;
    paramSize = GetDataSize(quantType) * 3;
    offset = 0;
    if (animPipe->currentTrack->animType & 2) {
        offset = paramSize;
    }
    GetData(quantType, quantShift, &startFrame->setting[offset], start, 3);
    GetData(quantType, quantShift, &endFrame->setting[offset], end, 3);
    time = NormalizeTracktime(animPipe, startFrame, endFrame);
    offset = 0;
    if (animPipe->currentTrack->animType & 2) {
        data = (animPipe->currentTrack->interpolationType >> 2U) & 3;
        if (data == 2) {
            offset = paramSize * 2;
        }
        if (data == 3) {
            offset = (paramSize * 2) + 4;
        }
    }
    switch ((u8)((animPipe->currentTrack->interpolationType >> 4U) & 7)) {
        case 0:
            final[0] = start[0];
            final[1] = start[1];
            final[2] = start[2];
            break;
        case 1:
            LinearInterpolateVecEuler(start, end, time, final);
            break;
        case 2:
            GetData(quantType, quantShift, &startFrame->interpolation[offset] + paramSize, outControl, 3);
            GetData(quantType, quantShift, &endFrame->interpolation[offset], inControl, 3);
            BezierInterpolateVec(start, end, time, outControl, inControl, final);
            break;
        case 3:
            OSPanic(__FILE__, 0x147, "Hermite interpolation not yet supported for Euler angles");
            break;
        default:
            OSPanic(__FILE__, 0x14A, "Unknown interpolation setting!");
            break;
    }
    CTRLSetRotation(animPipe->control, final[0], final[1], final[2]);
}

static void DoScaleAnimation(struct ANIMPipe * animPipe, struct KeyFrame *startFrame, struct KeyFrame *endFrame) {
    f32 start[3];
    f32 end[3];
    f32 final[3];
    f32 outControl[3];
    f32 inControl[3];
    f32 easeOut;
    f32 easeIn;
    f32 time;
    f32 basis[4];
    u32 data;
    u32 paramSize;
    f32 quantShift;
    u8 quantType;

    quantShift = 1.0f / (1 << (animPipe->currentTrack->quantizeInfo & 0xF));
    quantType = animPipe->currentTrack->quantizeInfo >> 4;
    paramSize = GetDataSize(quantType) * 3;
    GetData(quantType, quantShift, startFrame->setting, start, 3);
    GetData(quantType, quantShift, endFrame->setting, end, 3);
    time = NormalizeTracktime(animPipe, startFrame, endFrame);
    switch ((u8)((animPipe->currentTrack->interpolationType >> 2U) & 3)) {
        case 0:
            final[0] = start[0];
            final[1] = start[1];
            final[2] = start[2];
            break;
        case 1:
            LinearInterpolateVec(start, end, time, final);
            break;
        case 2:
            GetData(quantType, quantShift, &startFrame->interpolation[paramSize], outControl, 3);
            GetData(quantType, quantShift, &endFrame->interpolation[0], inControl, 3);
            BezierInterpolateVec(start, end, time, outControl, inControl, final);
            break;
        case 3:
            data = (u32)&startFrame->interpolation[paramSize];
            GetData(quantType, quantShift, (void*)data, outControl, 3);
            GetData(EaseType, EaseShift, (void*)(data + paramSize + 2), &easeOut, 1U);
            GetData(quantType, quantShift, &endFrame->interpolation[0], inControl, 3U);
            GetData(EaseType, EaseShift, &endFrame->interpolation[paramSize * 2], &easeIn, 1U);
            time = Ease(time, easeOut, easeIn);
            ComputeHermiteBasis(time, basis);
            HermiteInterpolateVec(start, end, basis, outControl, inControl, final);
            break;
        default:
            OSPanic(__FILE__, 0x18B, "Unknown interpolation setting!");
            break;
    }
    CTRLSetScale(animPipe->control, final[0], final[1], final[2]);
}

static void DoTranslationAnimation(struct ANIMPipe * animPipe, struct KeyFrame *startFrame, struct KeyFrame *endFrame) {
    f32 start[3];
    f32 end[3];
    f32 final[3];
    f32 outControl[3];
    f32 inControl[3];
    f32 easeOut;
    f32 easeIn;
    u32 offset;
    f32 time;
    f32 basis[4];
    u32 data;
    u32 paramSize;
    f32 quantShift;
    u8 quantType;

    quantShift = 1.0f / (1 << (animPipe->currentTrack->quantizeInfo & 0xF));
    quantType = animPipe->currentTrack->quantizeInfo >> 4;
    paramSize = GetDataSize(quantType) * 3;
    offset = 0;
    if (animPipe->currentTrack->animType & 2) {
        offset = paramSize;
    }
    if (animPipe->currentTrack->animType & 4) {
        offset += paramSize;
    }
    if (animPipe->currentTrack->animType & 8) {
        offset += 8;
    }
    GetData(quantType, quantShift, &startFrame->setting[offset], start, 3);
    GetData(quantType, quantShift, &endFrame->setting[offset], end, 3);
    time = NormalizeTracktime(animPipe, startFrame, endFrame);
    offset = 0;
    if (animPipe->currentTrack->animType & 2) {
        data = ((u32)(animPipe->currentTrack->interpolationType >> 2)) & 3;
        if (data == 2) {
            offset = paramSize * 2;
        }
        if (data == 3) {
            offset = (paramSize * 2) + 4;
        }
    }
    if (animPipe->currentTrack->animType & 8) {
        data = ((u32)(animPipe->currentTrack->interpolationType >> 4)) & 7;
        if (data == 4) {
            offset += 0x10;
        }
        if (data == 5) {
            offset += 0x14;
        }
    } else if (animPipe->currentTrack->animType & 4) {
        data = ((u32)(animPipe->currentTrack->interpolationType >> 4)) & 7;
        if (data == 2) {
            offset += paramSize * 2;
        }
        if (data == 3) {
            offset += 4 + paramSize * 2;
        }
    }
    switch (animPipe->currentTrack->interpolationType & 3) {
        case 0:
            final[0] = start[0];
            final[1] = start[1];
            final[2] = start[2];
            break;
        case 1:
            LinearInterpolateVec(start, end, time, final);
            break;
        case 2:
            GetData(quantType, quantShift, &startFrame->interpolation[offset] + paramSize, outControl, 3);
            GetData(quantType, quantShift, &endFrame->interpolation[offset], inControl, 3);
            BezierInterpolateVec(start, end, time, outControl, inControl, final);
            break;
        case 3:
            data = (u32)&startFrame->interpolation[offset] + paramSize;
            GetData(quantType, quantShift, (void*)data, outControl, 3);
            GetData(EaseType, EaseShift, (void*)(data + paramSize + 2), &easeOut, 1);
            data = (u32)&endFrame->interpolation[offset];
            GetData(quantType, quantShift, (void*)data, inControl, 3);
            GetData(EaseType, EaseShift, (void*)(data + paramSize * 2), &easeIn, 1);
            time = Ease(time, easeOut, easeIn);
            ComputeHermiteBasis(time, basis);
            HermiteInterpolateVec(start, end, basis, outControl, inControl, final);
            break;
        default:
            OSPanic(__FILE__, 0x1F2, "Unknown interpolation setting!");
            break;
    }
    CTRLSetTranslation(animPipe->control, final[0], final[1], final[2]);
}

static u32 GetDataSize(u8 quantType) {
    switch(quantType) {
        case 0:
        case 1:
            return 1;
        case 2:
        case 3:
            return 2;
        case 4:
            return 4;
    }
    return 0;
}

static void GetData(u8 quantType, f32 quantShift, char * from, f32 * to, u8 count) {
    u8 i;

    switch(quantType) {
        case 0: // as unsigned 8-bit
            for(i = 0; i < count; i++) {
                to[i] = quantShift * (f32)((u8*)from)[i];
            }
            return;
        case 1: // as signed 8-bit
            for(i = 0; i < count; i++) {
                to[i] = quantShift * (f32)((s8*)from)[i];
            }
            return;
        case 2: // as unsigned 16-bit
            for(i = 0; i < count; i++) {
                to[i] = quantShift * (f32)((u16*)from)[i];
            }
            return;
        case 3: // as signed 16-bit
            for(i = 0; i < count; i++) {
                to[i] = quantShift * (f32)((s16*)from)[i];
            }
            return;
        case 4: // as float
            for(i = 0; i < count; i++) {
                to[i] = (f32)((f32*)from)[i];
            }
            return;
    }
}

static void LinearInterpolateVec(f32 * start, f32 * end, f32 time, f32 * final) {
    u32 i;

    for(i = 0; i < 3; i++) {
        final[i] = (time * end[i]) + ((1.0f - time) * start[i]);
    }
}

static void LinearInterpolateVecEuler(f32 * start, f32 * end, f32 time, f32 * final) {
    f32 diff;
    u32 i;

    for(i = 0; i < 3; i++) {
        ASSERTLINE(0x248, fabs(start[i]) >= 0.0F && fabs(start[i]) <= 360.0F);
        ASSERTLINE(0x249, fabs(end[i]) >= 0.0F && fabs(end[i]) <= 360.0F);
        diff = start[i] - end[i];
        if (diff > 180.0f) {
            start[i] -= 360.0f;
        } else if (diff < -180.0f) {
            end[i] -= 360.0f;
        }
        final[i] = (time * end[i]) + ((1.0f - time) * start[i]);
    }
}

static void Ticktime(struct ANIMPipe * animPipe) {
    animPipe->time += animPipe->speed;
    if (animPipe->time > animPipe->currentTrack->animTime) {
        animPipe->time -= animPipe->currentTrack->animTime;
    }
}

static f32 NormalizeTracktime(struct ANIMPipe * animPipe, struct KeyFrame *startFrame, struct KeyFrame *endFrame) {
    f32 time;

    time = animPipe->time - startFrame->time;
    if (!time) {
        return 0.0f;
    }
    if (endFrame->time > startFrame->time) {
        return time / (endFrame->time - startFrame->time);
    }
    return time / (animPipe->currentTrack->animTime - startFrame->time);
}

static void BezierInterpolateVecEuler(f32 * start, f32 * end, f32 u, f32 * outControl, f32 * inControl, f32 * final) {
    f32 diff[3];
    u32 i;

    for(i = 0; i < 3; i++) {
        ASSERTLINE(0x283, fabs(start[i]) >= 0.0F && fabs(start[i]) <= 360.0F);
        ASSERTLINE(0x284, fabs(end[i]) >= 0.0F && fabs(end[i]) <= 360.0F);
        diff[i] = start[i] - end[i];
        if (diff[i] > 180.0f) {
            start[i] -= 360.0f;
        } else if (diff[i] < -180.0f) {
            end[i] -= 360.0f;
        }
    }
    BezierInterpolateVec(start, end, u, outControl, inControl, final);
}

static void BezierInterpolateVec(f32 * start, f32 * end, f32 u, f32 * outControl, f32 * inControl, f32 * final) {
    f32 s;
    f32 u2;
    f32 out;
    f32 in;
    u32 i;

    s = 1.0f - u;
    u2 = u*u;
    for(i = 0; i < 3; i++) {
        out = outControl[i] + start[i];
        in = inControl[i] + end[i];
        final[i] = (s * ((3.0f * u2 * in) + (s * ((s * start[i]) + (3.0f * u * out))))) + (u * u2 * end[i]);
    }
}

static void HermiteInterpolateVec(f32 * start, f32 * end, f32 * basis, f32 * outControl, f32 * inControl, f32 * final) {
    u32 i;

    for(i = 0; i < 3; i++) {
        final[i] = (basis[3] * inControl[i]) + ((basis[2] * outControl[i]) + ((basis[0] * start[i]) + (basis[1] * end[i])));
    }
}

static void ComputeHermiteBasis(f32 u, f32 * v) {
    f32 u2 = u*u;
    f32 u3 = u2*u;
    f32 a = (2.0f * u3) - (3.0f * u2);

    v[0] = 1.0f + a;
    v[1] = -a;
    v[2] = u3 + (u - (2.0f * u2));
    v[3] = (-u2 + u3);
}

static f32 Ease(f32 u, f32 a, f32 b) {
    f32 k;
    f32 s;

    s = a + b;
    if ((0.0f == u) || (1.0f == u) || 0.0f == s) {
        return u;
    }
    
    if (s > 1.0f) {
        s = 1.0f / s;
        a *= s;
        b *= s;
        k = 0.5f - s;
    } else {
        k = 0.5f - (1.0f / s);
    }
    if (u < a) {
        return u * (u * (k / a));
    }
    if (u < (1.0f - b)) {
        return k * ((2.0f * u) - a);
    }
    u = 1.0f - u;
    u = 1.0f - (u * (u * (k / b)));
    return u;
}

static void Slerp(Quaternion *p, Quaternion *q, f32 t, Quaternion *r) {
    f32 cosom;
    f32 omega;
    f32 sinom;
    f32 scale0;
    f32 scale1;

    cosom = (p->w * q->w) + ((p->z * q->z) + ((p->x * q->x) + (p->y * q->y)));
    if ((1.0f + cosom) > 0.000001f) {
        if ((1.0f - cosom) > 0.000001f) {
            omega = acosf(cosom);
            cosom = t * omega;
            sinom = 1.0f / sinf(omega);
            scale0 = sinom * sinf(omega - cosom);
            scale1 = sinom * sinf(cosom);
        } else {
            scale0 = 1.0f - t;
            scale1 = t;
        }
        r->x = (scale0 * p->x) + (scale1 * q->x);
        r->y = (scale0 * p->y) + (scale1 * q->y);
        r->z = (scale0 * p->z) + (scale1 * q->z);
        r->w = (scale0 * p->w) + (scale1 * q->w);
        return;
    }
    r->x = -q->y;
    r->y =  q->x;
    r->z = -q->w;
    r->w =  q->z;
    cosom = 1.5707964f * t;
    scale0 = sinf(1.5707964f - cosom);
    scale1 = sinf(cosom);
    r->x = (scale0 * p->x) + (scale1 * r->x);
    r->y = (scale0 * p->y) + (scale1 * r->y);
    r->z = (scale0 * p->z) + (scale1 * r->z);
    r->w = (scale0 * p->w) + (scale1 * r->w);
}

static void Squad(Quaternion *p, Quaternion *a, Quaternion *b, Quaternion *q, f32 t, Quaternion *r) {
    f32 k;
    Quaternion r1;
    Quaternion r2;

    k = 2.0f * (1.0f - t) * t;
    Slerp(p, q, t, &r1);
    Slerp(a, b, t, &r2);
    Slerp(&r1, &r2, k, r);
}
