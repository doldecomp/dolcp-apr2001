#include <dolphin.h>
#include "libc/math.h"
#include "charPipeline.h"

void CTRLSetScale(Control *control, f32 x, f32 y, f32 z) {
    control->type |= CTRL_SCALE;
    control->type &= ~(CTRL_MTX);
    control->controlParams.srt.s.x = x;
    control->controlParams.srt.s.y = y;
    control->controlParams.srt.s.z = z;
}

void CTRLSetRotation(Control *control, f32 x, f32 y, f32 z) {
    control->type |= CTRL_ROT_EULER;
    control->type &= ~(CTRL_ROT_QUAT);
    control->type &= ~(CTRL_MTX);
    control->controlParams.srt.r.x = x;
    control->controlParams.srt.r.y = y;
    control->controlParams.srt.r.z = z;
}

void CTRLSetQuat(Control *control, f32 x, f32 y, f32 z, f32 w) {
    control->type |= CTRL_ROT_QUAT;
    control->type &= ~(CTRL_ROT_EULER);
    control->type &= ~(CTRL_MTX);
    control->controlParams.srt.r.x = x;
    control->controlParams.srt.r.y = y;
    control->controlParams.srt.r.z = z;
    control->controlParams.srt.r.w = w;
}

void CTRLSetTranslation(Control *control, f32 x, f32 y, f32 z) {
    control->type |= CTRL_TRANS;
    control->type &= ~(CTRL_MTX);
    control->controlParams.srt.t.x = x;
    control->controlParams.srt.t.y = y;
    control->controlParams.srt.t.z = z;
}

void CTRLSetMatrix(Control *control, MtxPtr m) {
    control->type = CTRL_MTX;
    MTXCopy(m, control->controlParams.mtx.m);
}

void CTRLGetScale(Control *control, f32 *x, f32 *y, f32 *z) {
    ASSERTLINE(0xC6, control && x && y && z);
    ASSERTLINE(0xC7, control->type & CTRL_SCALE);
    *x = control->controlParams.srt.s.x;
    *y = control->controlParams.srt.s.y;
    *z = control->controlParams.srt.s.z;
}

void CTRLGetRotation(Control *control, f32 *x, f32 *y, f32 *z) {
    ASSERTLINE(0xDC, control && x && y && z);
    ASSERTLINE(0xDD, control->type & CTRL_ROT_EULER);
    *x = control->controlParams.srt.r.x;
    *y = control->controlParams.srt.r.y;
    *z = control->controlParams.srt.r.z;
}

void CTRLGetQuat(Control *control, f32 *x, f32 *y, f32 *z, f32 *w) {
    ASSERTLINE(0xF2, control && x && y && z && w);
    ASSERTLINE(0xF3, control->type & CTRL_ROT_QUAT);
    *x = control->controlParams.srt.r.x;
    *y = control->controlParams.srt.r.y;
    *z = control->controlParams.srt.r.z;
    *w = control->controlParams.srt.r.w;
}

void CTRLGetTranslation(Control *control, f32 *x, f32 *y, f32 *z) {
    ASSERTLINE(0x109, control && x && y && z);
    ASSERTLINE(0x10A, control->type & CTRL_TRANS);
    *x = control->controlParams.srt.t.x;
    *y = control->controlParams.srt.t.y;
    *z = control->controlParams.srt.t.z;
}

void CTRLBuildMatrix(Control *control, Mtx m) {
    f32 nRad;
    f32 nSin;
    f32 nCos;
    f32 temp;

    if (control->type & CTRL_MTX) {
        MTXCopy(control->controlParams.mtx.m, m);
        return;
    }
    if (control->type & CTRL_ROT_QUAT) {
        MTXQuat(m, &control->controlParams.srt.r);
    } else if (control->type & CTRL_ROT_EULER) {
        if (0.0f != control->controlParams.srt.r.x) {
            nRad = 0.0174532925f * control->controlParams.srt.r.x;
            nSin = sinf(nRad);
            nCos = cosf(nRad);
            m[0][0] = 1.0f;
            m[0][1] = 0.0f;
            m[0][2] = 0.0f;
            m[0][3] = 0.0f;
            m[1][0] = 0.0f;
            m[1][1] = nCos;
            m[1][2] = -nSin;
            m[1][3] = 0.0f;
            m[2][0] = 0.0f;
            m[2][1] = nSin;
            m[2][2] = nCos;
            m[2][3] = 0.0f;
        } else {
            MTXIdentity(m);
        }
        if (0.0f != control->controlParams.srt.r.y) {
            nRad = 0.0174532925f * control->controlParams.srt.r.y;
            nSin = sinf(nRad);
            nCos = cosf(nRad);
            m[0][0] = nCos;
            m[0][1] = m[2][1] * nSin;
            m[0][2] = m[2][2] * nSin;
            m[2][0] = -nSin;
            m[2][1] = m[2][1] * nCos;
            m[2][2] = m[2][2] * nCos;
        }
        if (0.0f != control->controlParams.srt.r.z) {
            nRad = 0.0174532925f * control->controlParams.srt.r.z;
            nSin = sinf(nRad);
            nCos = cosf(nRad);
            m[1][0] = m[0][0] * nSin;
            m[0][0] = m[0][0] * nCos;
            temp = m[1][1];
            m[1][1] = (m[0][1] * nSin) + (m[1][1] * nCos);
            m[0][1] = (m[0][1] * nCos) - (temp * nSin);
            temp = m[1][2];
            m[1][2] = (m[0][2] * nSin) + (m[1][2] * nCos);
            m[0][2] = (m[0][2] * nCos) - (temp * nSin);
        }
    } else {
        MTXIdentity(m);
    }
    if (control->type & CTRL_SCALE) {
        m[0][0] = m[0][0] * control->controlParams.srt.s.x;
        m[1][0] = m[1][0] * control->controlParams.srt.s.x;
        m[2][0] = m[2][0] * control->controlParams.srt.s.x;
        m[0][1] = m[0][1] * control->controlParams.srt.s.y;
        m[1][1] = m[1][1] * control->controlParams.srt.s.y;
        m[2][1] = m[2][1] * control->controlParams.srt.s.y;
        m[0][2] = m[0][2] * control->controlParams.srt.s.z;
        m[1][2] = m[1][2] * control->controlParams.srt.s.z;
        m[2][2] = m[2][2] * control->controlParams.srt.s.z;
    }
    if (control->type & CTRL_TRANS) {
        m[0][3] = control->controlParams.srt.t.x;
        m[1][3] = control->controlParams.srt.t.y;
        m[2][3] = control->controlParams.srt.t.z;
    }
}

void CTRLBuildInverseMatrix(Control *control, Mtx m) {
    f32 nRad;
    f32 nSin;
    f32 nCos;
    f32 temp;
    Quaternion q;

    if (control->type & CTRL_MTX) {
        MTXInverse(control->controlParams.mtx.m, m);
        return;
    }
    if (control->type & CTRL_ROT_QUAT) {
        q.x = -control->controlParams.srt.r.x;
        q.y = -control->controlParams.srt.r.y;
        q.z = -control->controlParams.srt.r.z;
        q.w = -control->controlParams.srt.r.w;
        MTXQuat(m, &q);
    } else if (control->type & CTRL_ROT_EULER) {
        if (0.0f != control->controlParams.srt.r.z) {
            nRad = 0.0174532925f * -control->controlParams.srt.r.x;
            nSin = sinf(nRad);
            nCos = cosf(nRad);
            m[0][0] = nCos;
            m[0][1] = -nSin;
            m[0][2] = 0.0f;
            m[0][3] = 0.0f;
            m[1][0] = nSin;
            m[1][1] = nCos;
            m[1][2] = 0.0f;
            m[1][3] = 0.0f;
            m[2][0] = 0.0f;
            m[2][1] = 0.0f;
            m[2][2] = 1.0f;
            m[2][3] = 0.0f;
        } else {
            MTXIdentity(m);
        }
        if (0.0f != control->controlParams.srt.r.y) {
            nRad = 0.0174532925f * -control->controlParams.srt.r.y;
            nSin = sinf(nRad);
            nCos = cosf(nRad);
            m[2][0] = m[0][0] * -nSin;
            m[2][1] = m[0][1] * -nSin;
            m[2][2] = nCos;
            m[0][0] = m[0][0] * nCos;
            m[0][1] = m[0][1] * nCos;
            m[0][2] = nSin;
        }
        if (0.0f != control->controlParams.srt.r.x) {
            nRad = 0.0174532925f * -control->controlParams.srt.r.x;
            nSin = sinf(nRad);
            nCos = cosf(nRad);
            m[1][2] = m[2][2] * -nSin;
            m[2][2] = m[2][2] * nCos;
            temp = m[1][0];
            m[1][0] = (m[1][0] * nCos) - (m[2][0] * nSin);
            m[2][0] = (m[2][0] * nCos) + (temp * nSin);
            temp = m[1][1];
            m[1][1] = (m[1][1] * nCos) - (m[2][1] * nSin);
            m[2][1] = (m[2][1] * nCos) + (temp * nSin);
        }
    } else {
        MTXIdentity(m);
    }
    if (control->type & CTRL_TRANS) {
        if ((control->type & CTRL_ROT_QUAT) || (control->type & CTRL_ROT_EULER)) {
            m[0][3] = -((m[0][2] * control->controlParams.srt.t.z) + ((m[0][0] * control->controlParams.srt.t.x) + (m[0][1] * control->controlParams.srt.t.y)));
            m[1][3] = -((m[1][2] * control->controlParams.srt.t.z) + ((m[1][0] * control->controlParams.srt.t.x) + (m[1][1] * control->controlParams.srt.t.y)));
            m[2][3] = -((m[2][2] * control->controlParams.srt.t.z) + ((m[2][0] * control->controlParams.srt.t.x) + (m[2][1] * control->controlParams.srt.t.y)));
        } else {
            m[0][3] = -control->controlParams.srt.t.x;
            m[1][3] = -control->controlParams.srt.t.y;
            m[2][3] = -control->controlParams.srt.t.z;
        }
    }
    if (control->type & CTRL_SCALE) {
        q.x = 1.0f / control->controlParams.srt.s.x;
        q.y = 1.0f / control->controlParams.srt.s.y;
        q.z = 1.0f / control->controlParams.srt.s.z;
        m[0][0] *= q.x;
        m[0][1] *= q.x;
        m[0][2] *= q.x;
        m[0][3] *= q.x;
        m[1][0] *= q.y;
        m[1][1] *= q.y;
        m[1][2] *= q.y;
        m[1][3] *= q.y;
        m[2][0] *= q.z;
        m[2][1] *= q.z;
        m[2][2] *= q.z;
        m[2][3] *= q.z;
    }
}
