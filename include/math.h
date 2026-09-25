/* <math.h> for the unisa C subset: real implementations, compiled from source,
 * because there is no libm to link against.
 *
 * The algorithms are fdlibm's (Sun Microsystems, 1993: "Permission to use,
 * copy, modify, and distribute this software is freely granted, provided that
 * this notice is preserved."), whose results are within one ulp -- which is
 * what makes a printed result agree with the platform's libm.  sqrt is the
 * hardware instruction, correctly rounded; fabs, copysign, floor, ceil,
 * trunc, round, frexp, ldexp and modf are exact bit manipulation.
 *
 * Words of a double are read and written through its address: the low word
 * first, both our ISAs being little-endian.
 */
#ifndef _UNISA_MATH_H
#define _UNISA_MATH_H

#define HUGE_VAL (1.0 / 0.0)
#define INFINITY (1.0f / 0.0f)
#define NAN (0.0f / 0.0f)
#define M_E        2.71828182845904523536
#define M_LOG2E    1.44269504088896340736
#define M_LOG10E   0.434294481903251827651
#define M_LN2      0.693147180559945309417
#define M_LN10     2.30258509299404568402
#define M_PI       3.14159265358979323846
#define M_PI_2     1.57079632679489661923
#define M_PI_4     0.785398163397448309616
#define M_1_PI     0.318309886183790671538
#define M_2_PI     0.636619772367581343076
#define M_SQRT2    1.41421356237309504880
#define M_SQRT1_2  0.707106781186547524401

static int _m_hi(double __u_x) { return *(1 + (int *)&__u_x); }
static unsigned int _m_lo(double __u_x) { return *(unsigned int *)&__u_x; }
static double _m_mk(int __u_hi, unsigned int __u_lo) {
    double __u_r;
    *(unsigned int *)&__u_r = __u_lo;
    *(1 + (int *)&__u_r) = __u_hi;
    return __u_r;
}
static double _m_sethi(double __u_x, int __u_hi) { return _m_mk(__u_hi, _m_lo(__u_x)); }

static double sqrt(double __u_x) { return __builtin_sqrt(__u_x); }
static float sqrtf(float __u_x) { return __builtin_sqrtf(__u_x); }

static double fabs(double __u_x) { return _m_mk(_m_hi(__u_x) & 2147483647, _m_lo(__u_x)); }
static float fabsf(float __u_x) { return (float)fabs((double)__u_x); }
static double copysign(double __u_x, double __u_y) {
    return _m_mk((_m_hi(__u_x) & 2147483647) | (_m_hi(__u_y) & (0 - 2147483647 - 1)), _m_lo(__u_x));
}

static int isnan(double __u_x) { return __u_x != __u_x; }
static int isinf(double __u_x) { return (_m_hi(__u_x) & 2147483647) == 2146435072 && _m_lo(__u_x) == 0; }
static int isfinite(double __u_x) { return (_m_hi(__u_x) & 2146435072) != 2146435072; }
static int signbit(double __u_x) { return _m_hi(__u_x) < 0; }

/* integral parts: exact.  Past 2^52 every double is already an integer. */
static double trunc(double __u_x) {
    double __u_a;
    __u_a = fabs(__u_x);
    if (__u_a >= 4503599627370496.0 || __u_x != __u_x) return __u_x;
    return copysign((double)(long)__u_x, __u_x);
}
static double floor(double __u_x) {
    double __u_t;
    __u_t = trunc(__u_x);
    if (__u_t > __u_x) __u_t = __u_t - 1.0;
    if (__u_t == 0.0) return copysign(0.0, __u_x);        /* floor(-0.0) is -0.0 */
    return __u_t;
}
static double ceil(double __u_x) {
    double __u_t;
    __u_t = trunc(__u_x);
    if (__u_t < __u_x) __u_t = __u_t + 1.0;
    if (__u_t == 0.0) return copysign(0.0, __u_x);
    return __u_t;
}
static double round(double __u_x) {                  /* halfway away from zero */
    double __u_t;
    __u_t = trunc(__u_x);
    if (fabs(__u_x - __u_t) >= 0.5) __u_t = __u_t + copysign(1.0, __u_x);
    if (__u_t == 0.0) return copysign(0.0, __u_x);
    return __u_t;
}
static long lround(double __u_x) { return (long)round(__u_x); }
static float floorf(float __u_x) { return (float)floor((double)__u_x); }
static float ceilf(float __u_x) { return (float)ceil((double)__u_x); }

static double modf(double __u_x, double *__u_ip) {
    double __u_t;
    __u_t = trunc(__u_x);
    *__u_ip = __u_t;
    if (isinf(__u_x)) return copysign(0.0, __u_x);
    return copysign(__u_x - __u_t, __u_x);
}

/* x = m * 2^e with 0.5 <= |m| < 1 */
static double frexp(double __u_x, int *__u_e) {
    int __u_hx; int __u_ix;
    __u_hx = _m_hi(__u_x); __u_ix = __u_hx & 2147483647;
    *__u_e = 0;
    if (__u_ix >= 2146435072 || (__u_ix | (int)_m_lo(__u_x)) == 0) return __u_x;   /* 0, inf, nan */
    if (__u_ix < 1048576) {                                 /* subnormal */
        __u_x = __u_x * 18014398509481984.0;                    /* 2^54 */
        __u_hx = _m_hi(__u_x); __u_ix = __u_hx & 2147483647; *__u_e = 0 - 54;
    }
    *__u_e = *__u_e + (__u_ix >> 20) - 1022;
    return _m_sethi(__u_x, (__u_hx & (0 - 2146435073)) | 1071644672);
}
static double ldexp(double __u_x, int __u_n) {
    /* by exact powers of two, in steps that cannot overflow the exponent */
    while (__u_n > 1000) { __u_x = __u_x * 1.0715086071862673e301; __u_n = __u_n - 1000; }       /* 2^1000 */
    while (__u_n < 0 - 1000) { __u_x = __u_x * 9.3326361850321888e-302; __u_n = __u_n + 1000; }  /* 2^-1000 */
    if (__u_n >= 0) return __u_x * _m_mk((__u_n + 1023) << 20, 0);
    if (__u_n > 0 - 1022) return __u_x * _m_mk((__u_n + 1023) << 20, 0);
    return __u_x * _m_mk((__u_n + 1023 + 60) << 20, 0) * 8.673617379884035e-19;    /* 2^-60 */
}
static double scalbn(double __u_x, int __u_n) { return ldexp(__u_x, __u_n); }

/* fmod, exactly: C99 7.12.10.1 -- the result is x - n*y for the integer n
   that makes it have x's sign and be smaller than |y|, and it is exact. */
static double fmod(double __u_x, double __u_y) {
    int __u_ex; int __u_ey;
    double __u_ax; double __u_ay; double __u_r;
    if (__u_y != __u_y || __u_x != __u_x || isinf(__u_x) || __u_y == 0.0) return (__u_x * __u_y) / (__u_x * __u_y);
    __u_ax = fabs(__u_x); __u_ay = fabs(__u_y);
    if (__u_ax < __u_ay) return __u_x;
    frexp(__u_ax, &__u_ex); frexp(__u_ay, &__u_ey);
    __u_r = __u_ax;
    while (__u_ex >= __u_ey) {
        double __u_t;
        __u_t = ldexp(__u_ay, __u_ex - __u_ey);
        if (__u_t > __u_r) { if (__u_ex == __u_ey) break; __u_t = ldexp(__u_ay, __u_ex - __u_ey - 1); }
        if (__u_t <= __u_r) __u_r = __u_r - __u_t;                   /* exact: same binade or below */
        if (__u_r < __u_ay) break;
        frexp(__u_r, &__u_ex);
    }
    return copysign(__u_r, __u_x);
}

/* ---- exp (fdlibm e_exp.c) ---------------------------------------------- */
static double exp(double __u_x) {
    double __u_hi; double __u_lo; double __u_c; double __u_t; double __u_y;
    int __u_k; int __u_xsb; int __u_hx;
    unsigned int __u_ux;
    __u_hi = 0.0; __u_lo = 0.0; __u_k = 0;
    __u_hx = _m_hi(__u_x);
    __u_xsb = (__u_hx >> 31) & 1;
    __u_ux = __u_hx & 2147483647;
    if (__u_ux >= 1082535490) {                               /* |x| >= 709.78 */
        if (__u_ux >= 2146435072) {
            if (((__u_ux & 1048575) | _m_lo(__u_x)) != 0) return __u_x + __u_x;   /* nan */
            return __u_xsb == 0 ? __u_x : 0.0;                           /* exp(+-inf) */
        }
        if (__u_x > 7.09782712893383973096e+02) return 1.0e300 * 1.0e300;
        if (__u_x < 0 - 7.45133219101941108420e+02) return 1.0e-300 * 1.0e-300;
    }
    if (__u_ux > 1071001154) {                                /* |x| > 0.5 ln2 */
        if (__u_ux < 1072734898) {                            /* |x| < 1.5 ln2 */
            if (__u_xsb == 0) { __u_hi = __u_x - 6.93147180369123816490e-01; __u_lo = 1.90821492927058770002e-10; __u_k = 1; }
            else { __u_hi = __u_x + 6.93147180369123816490e-01; __u_lo = 0 - 1.90821492927058770002e-10; __u_k = 0 - 1; }
        } else {
            __u_k = (int)(1.44269504088896338700e+00 * __u_x + (__u_xsb ? 0 - 0.5 : 0.5));
            __u_t = __u_k;
            __u_hi = __u_x - __u_t * 6.93147180369123816490e-01;
            __u_lo = __u_t * 1.90821492927058770002e-10;
        }
        __u_x = __u_hi - __u_lo;
    } else {
        if (__u_ux < 1043333120) {                            /* |x| < 2^-28 */
            if (1.0e300 + __u_x > 1.0) return 1.0 + __u_x;
        } else __u_k = 0;
    }
    __u_t = __u_x * __u_x;
    __u_c = __u_x - __u_t * (1.66666666666666019037e-01 + __u_t * (0 - 2.77777777770155933842e-03
        + __u_t * (6.61375632143793436117e-05 + __u_t * (0 - 1.65339022054652515390e-06
        + __u_t * 4.13813679705723846039e-08))));
    if (__u_k == 0) return 1.0 - ((__u_x * __u_c) / (__u_c - 2.0) - __u_x);
    __u_y = 1.0 - ((__u_lo - (__u_x * __u_c) / (2.0 - __u_c)) - __u_hi);
    if (__u_k >= 0 - 1021) return _m_sethi(__u_y, _m_hi(__u_y) + (__u_k << 20));
    __u_y = _m_sethi(__u_y, _m_hi(__u_y) + ((__u_k + 1000) << 20));
    return __u_y * 9.33263618503218878990e-302;               /* 2^-1000 */
}
static float expf(float __u_x) { return (float)exp((double)__u_x); }
static double exp2(double __u_x) { return exp(__u_x * 6.93147180559945286227e-01); }

/* ---- log (fdlibm e_log.c) ---------------------------------------------- */
static double log(double __u_x) {
    double __u_hfsq; double __u_f; double __u_s; double __u_z; double __u_R; double __u_w; double __u_t1;
    double __u_t2; double __u_dk;
    int __u_k; int __u_hx; int __u_i; int __u_j;
    unsigned int __u_lx;
    __u_hx = _m_hi(__u_x); __u_lx = _m_lo(__u_x);
    __u_k = 0;
    if (__u_hx < 1048576) {                                   /* x < 2^-1022 */
        if (((__u_hx & 2147483647) | __u_lx) == 0) return (0 - 1.80143985094819840000e+16) / 0.0;
        if (__u_hx < 0) return (__u_x - __u_x) / 0.0;
        __u_k = __u_k - 54; __u_x = __u_x * 1.80143985094819840000e+16;
        __u_hx = _m_hi(__u_x);
    }
    if (__u_hx >= 2146435072) return __u_x + __u_x;
    __u_k = __u_k + (__u_hx >> 20) - 1023;
    __u_hx = __u_hx & 1048575;
    __u_i = (__u_hx + 614244) & 1048576;
    __u_x = _m_sethi(__u_x, __u_hx | (__u_i ^ 1072693248));
    __u_k = __u_k + (__u_i >> 20);
    __u_f = __u_x - 1.0;
    if ((1048575 & (2 + __u_hx)) < 3) {
        if (__u_f == 0.0) {
            if (__u_k == 0) return 0.0;
            __u_dk = __u_k; return __u_dk * 6.93147180369123816490e-01 + __u_dk * 1.90821492927058770002e-10;
        }
        __u_R = __u_f * __u_f * (0.5 - 0.33333333333333333 * __u_f);
        if (__u_k == 0) return __u_f - __u_R;
        __u_dk = __u_k; return __u_dk * 6.93147180369123816490e-01 - ((__u_R - __u_dk * 1.90821492927058770002e-10) - __u_f);
    }
    __u_s = __u_f / (2.0 + __u_f);
    __u_dk = __u_k;
    __u_z = __u_s * __u_s;
    __u_i = __u_hx - 398458;
    __u_w = __u_z * __u_z;
    __u_j = 440401 - __u_hx;
    __u_t1 = __u_w * (3.999999999940941908e-01 + __u_w * (2.222219843214978396e-01 + __u_w * 1.531383769920937332e-01));
    __u_t2 = __u_z * (6.666666666666735130e-01 + __u_w * (2.857142874366239149e-01 + __u_w * (1.818357216161805012e-01
         + __u_w * 1.479819860511658591e-01)));
    __u_i = __u_i | __u_j;
    __u_R = __u_t2 + __u_t1;
    if (__u_i > 0) {
        __u_hfsq = 0.5 * __u_f * __u_f;
        if (__u_k == 0) return __u_f - (__u_hfsq - __u_s * (__u_hfsq + __u_R));
        return __u_dk * 6.93147180369123816490e-01 - ((__u_hfsq - (__u_s * (__u_hfsq + __u_R) + __u_dk * 1.90821492927058770002e-10)) - __u_f);
    }
    if (__u_k == 0) return __u_f - __u_s * (__u_f - __u_R);
    return __u_dk * 6.93147180369123816490e-01 - ((__u_s * (__u_f - __u_R) - __u_dk * 1.90821492927058770002e-10) - __u_f);
}
static float logf(float __u_x) { return (float)log((double)__u_x); }
static double log10(double __u_x) {                            /* fdlibm e_log10.c */
    double __u_y; double __u_z;
    int __u_hx; int __u_k; int __u_i;
    unsigned int __u_lx;
    __u_hx = _m_hi(__u_x); __u_lx = _m_lo(__u_x);
    __u_k = 0;
    if (__u_hx < 1048576) {
        if (((__u_hx & 2147483647) | __u_lx) == 0) return (0 - 1.80143985094819840000e+16) / 0.0;
        if (__u_hx < 0) return (__u_x - __u_x) / 0.0;
        __u_k = __u_k - 54; __u_x = __u_x * 1.80143985094819840000e+16;
        __u_hx = _m_hi(__u_x);
    }
    if (__u_hx >= 2146435072) return __u_x + __u_x;
    __u_k = __u_k + (__u_hx >> 20) - 1023;
    __u_i = (__u_k & (0 - 2147483647 - 1)) >> 31;
    __u_i = __u_i & 1;
    __u_hx = (__u_hx & 1048575) | ((1023 - __u_i) << 20);
    __u_y = (double)(__u_k + __u_i);
    __u_x = _m_sethi(__u_x, __u_hx);
    __u_z = __u_y * 3.69423907715893078616e-13 + 4.34294481903251816668e-01 * log(__u_x);
    return __u_z + __u_y * 3.01029995663611771306e-01;
}
static double log2(double __u_x) { return log(__u_x) * 1.44269504088896338700e+00; }
/* log(1+x) accurately for small x: the rounding error of 1+x is corrected
   by the ratio x / ((1+x) - 1) (Goldberg, "What every computer scientist
   should know about floating-point arithmetic", thm. 4) */
static double log1p(double __u_x) {
    double __u_u;
    __u_u = 1.0 + __u_x;
    if (__u_u == 1.0) return __u_x;
    return log(__u_u) * __u_x / (__u_u - 1.0);
}

/* ---- sin, cos, tan (fdlibm k_sin.c, k_cos.c, e_rem_pio2.c) -------------- */
static double _m_ksin(double __u_x, double __u_y, int __u_iy) {
    double __u_z; double __u_r; double __u_v;
    int __u_ix;
    __u_ix = _m_hi(__u_x) & 2147483647;
    if (__u_ix < 1044381696) { if ((int)__u_x == 0) return __u_x; }        /* |x| < 2^-27 */
    __u_z = __u_x * __u_x;
    __u_v = __u_z * __u_x;
    __u_r = 8.33333333332248946124e-03 + __u_z * (0 - 1.98412698298579493134e-04 + __u_z * (2.75573137070700676789e-06
        + __u_z * (0 - 2.50507602534068634195e-08 + __u_z * 1.58969099521155010221e-10)));
    if (__u_iy == 0) return __u_x + __u_v * (0 - 1.66666666666666324348e-01 + __u_z * __u_r);
    return __u_x - ((__u_z * (0.5 * __u_y - __u_v * __u_r) - __u_y) - __u_v * (0 - 1.66666666666666324348e-01));
}
static double _m_kcos(double __u_x, double __u_y) {
    double __u_a; double __u_hz; double __u_z; double __u_r; double __u_qx;
    int __u_ix;
    __u_ix = _m_hi(__u_x) & 2147483647;
    if (__u_ix < 1044381696) { if ((int)__u_x == 0) return 1.0; }
    __u_z = __u_x * __u_x;
    __u_r = __u_z * (4.16666666666666019037e-02 + __u_z * (0 - 1.38888888888741095749e-03 + __u_z * (2.48015872894767294178e-05
        + __u_z * (0 - 2.75573143513906633035e-07 + __u_z * (2.08757232129817482790e-09
        + __u_z * (0 - 1.13596475577881948265e-11))))));
    if (__u_ix < 1070805811) return 1.0 - (0.5 * __u_z - (__u_z * __u_r - __u_x * __u_y));   /* |x| < 0.3 */
    if (__u_ix > 1072234496) __u_qx = 0.28125;                               /* |x| > 0.78125 */
    else __u_qx = _m_mk(__u_ix - 2097152, 0);
    __u_hz = 0.5 * __u_z - __u_qx;
    __u_a = 1.0 - __u_qx;
    return __u_a - (__u_hz - (__u_z * __u_r - __u_x * __u_y));
}
/* x = n*(pi/2) + (y[0] + y[1]); returns n.  The medium range is fdlibm's;
   beyond |x| ~ 2^20*pi/2 the reduction is only as good as three terms of
   pi/2 make it (fdlibm's Payne-Hanek path is not carried). */
static int _m_rempio2(double __u_x, double *__u_y) {
    double __u_z; double __u_w; double __u_t; double __u_r; double __u_fn;
    int __u_i; int __u_j; int __u_n; int __u_ix; int __u_hx;
    __u_hx = _m_hi(__u_x);
    __u_ix = __u_hx & 2147483647;
    if (__u_ix <= 1072243195) { __u_y[0] = __u_x; __u_y[1] = 0.0; return 0; }        /* |x| <= pi/4 */
    if (__u_ix < 1073928572) {                                           /* |x| < 3pi/4 */
        if (__u_hx > 0) {
            __u_z = __u_x - 1.57079632673412561417e+00;
            if (__u_ix != 1073291771) {
                __u_y[0] = __u_z - 6.07710050650619224932e-11;
                __u_y[1] = (__u_z - __u_y[0]) - 6.07710050650619224932e-11;
            } else {
                __u_z = __u_z - 6.07710050630396597660e-11;
                __u_y[0] = __u_z - 2.02226624879595063154e-21;
                __u_y[1] = (__u_z - __u_y[0]) - 2.02226624879595063154e-21;
            }
            return 1;
        }
        __u_z = __u_x + 1.57079632673412561417e+00;
        if (__u_ix != 1073291771) {
            __u_y[0] = __u_z + 6.07710050650619224932e-11;
            __u_y[1] = (__u_z - __u_y[0]) + 6.07710050650619224932e-11;
        } else {
            __u_z = __u_z + 6.07710050630396597660e-11;
            __u_y[0] = __u_z + 2.02226624879595063154e-21;
            __u_y[1] = (__u_z - __u_y[0]) + 2.02226624879595063154e-21;
        }
        return 0 - 1;
    }
    __u_t = fabs(__u_x);
    __u_n = (int)(__u_t * 6.36619772367581382433e-01 + 0.5);
    __u_fn = (double)__u_n;
    __u_r = __u_t - __u_fn * 1.57079632673412561417e+00;
    __u_w = __u_fn * 6.07710050650619224932e-11;
    __u_j = __u_ix >> 20;
    __u_y[0] = __u_r - __u_w;
    __u_i = __u_j - ((_m_hi(__u_y[0]) >> 20) & 2047);
    if (__u_i > 16) {
        __u_t = __u_r;
        __u_w = __u_fn * 6.07710050630396597660e-11;
        __u_r = __u_t - __u_w;
        __u_w = __u_fn * 2.02226624879595063154e-21 - ((__u_t - __u_r) - __u_w);
        __u_y[0] = __u_r - __u_w;
        __u_i = __u_j - ((_m_hi(__u_y[0]) >> 20) & 2047);
        if (__u_i > 49) {
            __u_t = __u_r;
            __u_w = __u_fn * 2.02226624871116645580e-21;
            __u_r = __u_t - __u_w;
            __u_w = __u_fn * 8.47842766036889956997e-32 - ((__u_t - __u_r) - __u_w);
            __u_y[0] = __u_r - __u_w;
        }
    }
    __u_y[1] = (__u_r - __u_y[0]) - __u_w;
    if (__u_hx < 0) { __u_y[0] = 0 - __u_y[0]; __u_y[1] = 0 - __u_y[1]; return 0 - __u_n; }
    return __u_n;
}
static double sin(double __u_x) {
    double __u_y[2];
    int __u_n; int __u_ix;
    __u_ix = _m_hi(__u_x) & 2147483647;
    if (__u_ix <= 1072243195) return _m_ksin(__u_x, 0.0, 0);
    if (__u_ix >= 2146435072) return __u_x - __u_x;
    __u_n = _m_rempio2(__u_x, __u_y);
    __u_n = __u_n & 3;
    if (__u_n == 0) return _m_ksin(__u_y[0], __u_y[1], 1);
    if (__u_n == 1) return _m_kcos(__u_y[0], __u_y[1]);
    if (__u_n == 2) return 0 - _m_ksin(__u_y[0], __u_y[1], 1);
    return 0 - _m_kcos(__u_y[0], __u_y[1]);
}
static double cos(double __u_x) {
    double __u_y[2];
    int __u_n; int __u_ix;
    __u_ix = _m_hi(__u_x) & 2147483647;
    if (__u_ix <= 1072243195) return _m_kcos(__u_x, 0.0);
    if (__u_ix >= 2146435072) return __u_x - __u_x;
    __u_n = _m_rempio2(__u_x, __u_y);
    __u_n = __u_n & 3;
    if (__u_n == 0) return _m_kcos(__u_y[0], __u_y[1]);
    if (__u_n == 1) return 0 - _m_ksin(__u_y[0], __u_y[1], 1);
    if (__u_n == 2) return 0 - _m_kcos(__u_y[0], __u_y[1]);
    return _m_ksin(__u_y[0], __u_y[1], 1);
}
static double tan(double __u_x) { return sin(__u_x) / cos(__u_x); }
static float sinf(float __u_x) { return (float)sin((double)__u_x); }
static float cosf(float __u_x) { return (float)cos((double)__u_x); }
static float tanf(float __u_x) { return (float)tan((double)__u_x); }

/* ---- atan, atan2, asin, acos (fdlibm s_atan.c, e_atan2.c) --------------- */
static double atan(double __u_x) {
    double __u_w; double __u_s1; double __u_s2; double __u_z; double __u_hi; double __u_lo;
    int __u_ix; int __u_hx; int __u_id;
    __u_hx = _m_hi(__u_x);
    __u_ix = __u_hx & 2147483647;
    if (__u_ix >= 1141899264) {                                /* |x| >= 2^66 */
        if (__u_ix > 2146435072 || (__u_ix == 2146435072 && _m_lo(__u_x) != 0)) return __u_x + __u_x;
        if (__u_hx > 0) return 1.57079632679489655800e+00 + 6.12323399573676603587e-17;
        return 0 - 1.57079632679489655800e+00 - 6.12323399573676603587e-17;
    }
    if (__u_ix < 1071382528) {                                 /* |x| < 0.4375 */
        if (__u_ix < 1042284544) { if (1.0e300 + __u_x > 1.0) return __u_x; }
        __u_id = 0 - 1;
    } else {
        __u_x = fabs(__u_x);
        if (__u_ix < 1072889856) {                             /* |x| < 1.1875 */
            if (__u_ix < 1072037888) { __u_id = 0; __u_x = (2.0 * __u_x - 1.0) / (2.0 + __u_x); }
            else { __u_id = 1; __u_x = (__u_x - 1.0) / (__u_x + 1.0); }
        } else {
            if (__u_ix < 1073971200) { __u_id = 2; __u_x = (__u_x - 1.5) / (1.0 + 1.5 * __u_x); }
            else { __u_id = 3; __u_x = (0 - 1.0) / __u_x; }
        }
    }
    __u_z = __u_x * __u_x;
    __u_w = __u_z * __u_z;
    __u_s1 = __u_z * (3.33333333333329318027e-01 + __u_w * (1.42857142725034663711e-01 + __u_w * (9.09088713343650656196e-02
         + __u_w * (6.66107313738753120669e-02 + __u_w * (4.97687799461593236017e-02 + __u_w * 1.62858201153657823623e-02)))));
    __u_s2 = __u_w * (0 - 1.99999999998764832476e-01 + __u_w * (0 - 1.11111104054623557880e-01 + __u_w * (0 - 7.69187620504482999495e-02
         + __u_w * (0 - 5.83357013379057348645e-02 + __u_w * (0 - 3.65315727442169155270e-02)))));
    if (__u_id < 0) return __u_x - __u_x * (__u_s1 + __u_s2);
    if (__u_id == 0) { __u_hi = 4.63647609000806093515e-01; __u_lo = 2.26987774529616870924e-17; }
    if (__u_id == 1) { __u_hi = 7.85398163397448278999e-01; __u_lo = 3.06161699786838301793e-17; }
    if (__u_id == 2) { __u_hi = 9.82793723247329054082e-01; __u_lo = 1.39033110312309984516e-17; }
    if (__u_id == 3) { __u_hi = 1.57079632679489655800e+00; __u_lo = 6.12323399573676603587e-17; }
    __u_z = __u_hi - ((__u_x * (__u_s1 + __u_s2) - __u_lo) - __u_x);
    return __u_hx < 0 ? 0 - __u_z : __u_z;
}
static double atan2(double __u_y, double __u_x) {
    double __u_z;
    if (__u_x != __u_x || __u_y != __u_y) return __u_x + __u_y;
    if (__u_x == 1.0) return atan(__u_y);
    if (__u_y == 0.0) {
        if (signbit(__u_x)) return signbit(__u_y) ? 0 - 3.14159265358979311600e+00 : 3.14159265358979311600e+00;
        return __u_y;
    }
    if (__u_x == 0.0) return __u_y > 0 ? 1.57079632679489655800e+00 : 0 - 1.57079632679489655800e+00;
    if (isinf(__u_x)) {
        if (isinf(__u_y)) {
            if (__u_x > 0) return __u_y > 0 ? 7.85398163397448278999e-01 : 0 - 7.85398163397448278999e-01;
            return __u_y > 0 ? 2.35619449019234492885e+00 : 0 - 2.35619449019234492885e+00;
        }
        if (__u_x > 0) return copysign(0.0, __u_y);
        return __u_y > 0 ? 3.14159265358979311600e+00 : 0 - 3.14159265358979311600e+00;
    }
    if (isinf(__u_y)) return __u_y > 0 ? 1.57079632679489655800e+00 : 0 - 1.57079632679489655800e+00;
    __u_z = atan(fabs(__u_y / __u_x));
    if (__u_x > 0) return __u_y > 0 ? __u_z : 0 - __u_z;
    __u_z = 3.14159265358979311600e+00 - (__u_z - 1.22464679914735317720e-16);
    return __u_y > 0 ? __u_z : 0 - __u_z;
}
static double asin(double __u_x) {
    if (__u_x > 1.0 || __u_x < 0 - 1.0) return (__u_x - __u_x) / (__u_x - __u_x);
    return atan2(__u_x, sqrt((1.0 - __u_x) * (1.0 + __u_x)));
}
static double acos(double __u_x) {
    if (__u_x > 1.0 || __u_x < 0 - 1.0) return (__u_x - __u_x) / (__u_x - __u_x);
    return atan2(sqrt((1.0 - __u_x) * (1.0 + __u_x)), __u_x);
}
static float atanf(float __u_x) { return (float)atan((double)__u_x); }
static float atan2f(float __u_y, float __u_x) { return (float)atan2((double)__u_y, (double)__u_x); }

/* ---- expm1, sinh, cosh, tanh (fdlibm s_expm1.c, e_sinh.c, e_cosh.c,
 *      s_tanh.c) -------------------------------------------------------- */
static double expm1(double __u_x) {
    double __u_y; double __u_hi; double __u_lo; double __u_c; double __u_t; double __u_e; double __u_hxs;
    double __u_hfx; double __u_r1;
    int __u_k; int __u_xsb;
    unsigned int __u_hx;
    __u_c = 0.0;
    __u_hx = _m_hi(__u_x);
    __u_xsb = __u_hx & 2147483648;
    if (__u_xsb == 0) __u_y = __u_x; else __u_y = 0 - __u_x;
    __u_hx = __u_hx & 2147483647;
    if (__u_hx >= 1078159482) {                                 /* |x| >= 56 ln2 */
        if (__u_hx >= 1082535490) {                             /* |x| >= 709.78 */
            if (__u_hx >= 2146435072) {
                if (((__u_hx & 1048575) | _m_lo(__u_x)) != 0) return __u_x + __u_x;
                return __u_xsb == 0 ? __u_x : 0 - 1.0;
            }
            if (__u_x > 7.09782712893383973096e+02) return 1.0e300 * 1.0e300;
        }
        if (__u_xsb != 0) { if (__u_x + 1.0e-300 < 0.0) return 1.0e-300 - 1.0; }
    }
    if (__u_hx > 1071001154) {                                  /* |x| > 0.5 ln2 */
        if (__u_hx < 1072734898) {                              /* |x| < 1.5 ln2 */
            if (__u_xsb == 0) { __u_hi = __u_x - 6.93147180369123816490e-01; __u_lo = 1.90821492927058770002e-10; __u_k = 1; }
            else { __u_hi = __u_x + 6.93147180369123816490e-01; __u_lo = 0 - 1.90821492927058770002e-10; __u_k = 0 - 1; }
        } else {
            __u_k = (int)(1.44269504088896338700e+00 * __u_x + (__u_xsb == 0 ? 0.5 : 0 - 0.5));
            __u_t = __u_k;
            __u_hi = __u_x - __u_t * 6.93147180369123816490e-01;
            __u_lo = __u_t * 1.90821492927058770002e-10;
        }
        __u_x = __u_hi - __u_lo;
        __u_c = (__u_hi - __u_x) - __u_lo;
    } else {
        if (__u_hx < 1016070144) {                              /* |x| < 2^-54 */
            __u_t = 1.0e300 + __u_x;
            return __u_x - (__u_t - (1.0e300 + __u_x));
        }
        __u_k = 0;
    }
    __u_hfx = 0.5 * __u_x;
    __u_hxs = __u_x * __u_hfx;
    __u_r1 = 1.0 + __u_hxs * (0 - 3.33333333333331316428e-02 + __u_hxs * (1.58730158725481460165e-03
         + __u_hxs * (0 - 7.93650757867487942473e-05 + __u_hxs * (4.00821782732936239552e-06
         + __u_hxs * (0 - 2.01099218183624371326e-07)))));
    __u_t = 3.0 - __u_r1 * __u_hfx;
    __u_e = __u_hxs * ((__u_r1 - __u_t) / (6.0 - __u_x * __u_t));
    if (__u_k == 0) return __u_x - (__u_x * __u_e - __u_hxs);
    __u_e = (__u_x * (__u_e - __u_c) - __u_c);
    __u_e = __u_e - __u_hxs;
    if (__u_k == 0 - 1) return 0.5 * (__u_x - __u_e) - 0.5;
    if (__u_k == 1) {
        if (__u_x < 0 - 0.25) return 0 - 2.0 * (__u_e - (__u_x + 0.5));
        return 1.0 + 2.0 * (__u_x - __u_e);
    }
    if (__u_k <= 0 - 2 || __u_k > 56) {
        __u_y = 1.0 - (__u_e - __u_x);
        __u_y = _m_sethi(__u_y, _m_hi(__u_y) + (__u_k << 20));
        return __u_y - 1.0;
    }
    __u_t = 1.0;
    if (__u_k < 20) {
        __u_t = _m_sethi(__u_t, 1072693248 - (2097152 >> __u_k));       /* 1 - 2^-k */
        __u_y = __u_t - (__u_e - __u_x);
        __u_y = _m_sethi(__u_y, _m_hi(__u_y) + (__u_k << 20));
    } else {
        __u_t = _m_sethi(__u_t, (1023 - __u_k) << 20);                  /* 2^-k */
        __u_y = __u_x - (__u_e + __u_t);
        __u_y = __u_y + 1.0;
        __u_y = _m_sethi(__u_y, _m_hi(__u_y) + (__u_k << 20));
    }
    return __u_y;
}
static double sinh(double __u_x) {
    double __u_t; double __u_w; double __u_h;
    int __u_ix; int __u_jx;
    unsigned int __u_lx;
    __u_jx = _m_hi(__u_x);
    __u_ix = __u_jx & 2147483647;
    if (__u_ix >= 2146435072) return __u_x + __u_x;
    __u_h = 0.5;
    if (__u_jx < 0) __u_h = 0 - __u_h;
    if (__u_ix < 1077280768) {                                  /* |x| < 22 */
        if (__u_ix < 1043333120) { if (1.0e307 + __u_x > 1.0) return __u_x; }   /* |x| < 2^-28 */
        __u_t = expm1(fabs(__u_x));
        if (__u_ix < 1072693248) return __u_h * (2.0 * __u_t - __u_t * __u_t / (__u_t + 1.0));
        return __u_h * (__u_t + __u_t / (__u_t + 1.0));
    }
    if (__u_ix < 1082535490) return __u_h * exp(fabs(__u_x));           /* |x| < log(DBL_MAX) */
    __u_lx = _m_lo(__u_x);
    if (__u_ix < 1082536910 || (__u_ix == 1082536910 && __u_lx <= 2411329661)) {
        __u_w = exp(0.5 * fabs(__u_x));
        __u_t = __u_h * __u_w;
        return __u_t * __u_w;
    }
    return __u_x * 1.0e307;
}
static double cosh(double __u_x) {
    double __u_t; double __u_w;
    int __u_ix;
    unsigned int __u_lx;
    __u_ix = _m_hi(__u_x) & 2147483647;
    if (__u_ix >= 2146435072) return __u_x * __u_x;
    if (__u_ix < 1071001155) {                                  /* |x| < 0.5 ln2 */
        __u_t = expm1(fabs(__u_x));
        __u_w = 1.0 + __u_t;
        if (__u_ix < 1015021568) return __u_w;                      /* |x| < 2^-55 */
        return 1.0 + (__u_t * __u_t) / (__u_w + __u_w);
    }
    if (__u_ix < 1077280768) { __u_t = exp(fabs(__u_x)); return 0.5 * __u_t + 0.5 / __u_t; }
    if (__u_ix < 1082535490) return 0.5 * exp(fabs(__u_x));
    __u_lx = _m_lo(__u_x);
    if (__u_ix < 1082536910 || (__u_ix == 1082536910 && __u_lx <= 2411329661)) {
        __u_w = exp(0.5 * fabs(__u_x));
        __u_t = 0.5 * __u_w;
        return __u_t * __u_w;
    }
    return 1.0e300 * 1.0e300;
}
static double tanh(double __u_x) {
    double __u_t; double __u_z;
    int __u_jx; int __u_ix;
    __u_jx = _m_hi(__u_x);
    __u_ix = __u_jx & 2147483647;
    if (__u_ix >= 2146435072) { if (__u_jx >= 0) return 1.0 / __u_x + 1.0; return 1.0 / __u_x - 1.0; }
    if (__u_ix < 1077280768) {                                  /* |x| < 22 */
        if (__u_ix < 1015021568) return __u_x * (1.0 + __u_x);          /* |x| < 2^-55 */
        if (__u_ix >= 1072693248) {                             /* |x| >= 1 */
            __u_t = expm1(2.0 * fabs(__u_x));
            __u_z = 1.0 - 2.0 / (__u_t + 2.0);
        } else {
            __u_t = expm1(0 - 2.0 * fabs(__u_x));
            __u_z = (0 - __u_t) / (__u_t + 2.0);
        }
    } else __u_z = 1.0 - 1.0e-300;
    return __u_jx >= 0 ? __u_z : 0 - __u_z;
}

/* ---- pow (fdlibm e_pow.c) ----------------------------------------------- */
static double pow(double __u_x, double __u_y) {
    double __u_z; double __u_ax; double __u_z_h; double __u_z_l; double __u_p_h; double __u_p_l;
    double __u_y1; double __u_t1; double __u_t2; double __u_r; double __u_s; double __u_t; double __u_u;
    double __u_v; double __u_w;
    double __u_ss; double __u_s2; double __u_s_h; double __u_s_l; double __u_t_h; double __u_t_l;
    double __u_bpk; double __u_dphk; double __u_dplk;
    int __u_i; int __u_j; int __u_k; int __u_yisint; int __u_n;
    int __u_hx; int __u_hy; int __u_ix; int __u_iy;
    unsigned int __u_lx; unsigned int __u_ly;
    __u_hx = _m_hi(__u_x); __u_lx = _m_lo(__u_x);
    __u_hy = _m_hi(__u_y); __u_ly = _m_lo(__u_y);
    __u_ix = __u_hx & 2147483647; __u_iy = __u_hy & 2147483647;
    if ((__u_iy | __u_ly) == 0) return 1.0;                         /* x**0 = 1 */
    if (__u_ix > 2146435072 || (__u_ix == 2146435072 && __u_lx != 0)
        || __u_iy > 2146435072 || (__u_iy == 2146435072 && __u_ly != 0)) return __u_x + __u_y;
    /* is y an odd integer (1), an even one (2), or not one (0)? for x < 0 */
    __u_yisint = 0;
    if (__u_hx < 0) {
        if (__u_iy >= 1128267776) __u_yisint = 2;                   /* |y| >= 2^53 */
        else { if (__u_iy >= 1072693248) {
            __u_k = (__u_iy >> 20) - 1023;
            if (__u_k > 20) {
                __u_j = __u_ly >> (52 - __u_k);
                if ((unsigned int)(__u_j << (52 - __u_k)) == __u_ly) __u_yisint = 2 - (__u_j & 1);
            } else { if (__u_ly == 0) {
                __u_j = __u_iy >> (20 - __u_k);
                if ((__u_j << (20 - __u_k)) == __u_iy) __u_yisint = 2 - (__u_j & 1);
            } }
        } }
    }
    if (__u_ly == 0) {
        if (__u_iy == 2146435072) {                             /* y is +-inf */
            if (((__u_ix - 1072693248) | __u_lx) == 0) return __u_y - __u_y;
            if (__u_ix >= 1072693248) return __u_hy >= 0 ? __u_y : 0.0;
            return __u_hy < 0 ? 0 - __u_y : 0.0;
        }
        if (__u_iy == 1072693248) { if (__u_hy < 0) return 1.0 / __u_x; return __u_x; }   /* +-1 */
        if (__u_hy == 1073741824) return __u_x * __u_x;                              /* 2 */
        if (__u_hy == 1071644672) { if (__u_hx >= 0) return sqrt(__u_x); }           /* 0.5 */
    }
    __u_ax = fabs(__u_x);
    if (__u_lx == 0) {
        if (__u_ix == 2146435072 || __u_ix == 0 || __u_ix == 1072693248) {  /* +-0, +-inf, +-1 */
            __u_z = __u_ax;
            if (__u_hy < 0) __u_z = 1.0 / __u_z;
            if (__u_hx < 0) {
                if (((__u_ix - 1072693248) | __u_yisint) == 0) __u_z = (__u_z - __u_z) / (__u_z - __u_z);
                else { if (__u_yisint == 1) __u_z = 0 - __u_z; }
            }
            return __u_z;
        }
    }
    __u_n = __u_hx < 0 ? 0 : 1;                                     /* (hx >> 31) + 1 */
    if ((__u_n | __u_yisint) == 0) return (__u_x - __u_x) / (__u_x - __u_x);         /* (x<0)**non-int */
    __u_s = 1.0;
    if ((__u_n | (__u_yisint - 1)) == 0) __u_s = 0 - 1.0;                /* (-x)**odd */
    if (__u_iy > 1105199104) {                                  /* |y| > 2^31 */
        if (__u_iy > 1139802112) {                              /* |y| > 2^64 */
            if (__u_ix <= 1072693247) return __u_hy < 0 ? 1.0e300 * 1.0e300 : 1.0e-300 * 1.0e-300;
            if (__u_ix >= 1072693248) return __u_hy > 0 ? 1.0e300 * 1.0e300 : 1.0e-300 * 1.0e-300;
        }
        if (__u_ix < 1072693247) return __u_hy < 0 ? __u_s * 1.0e300 * 1.0e300 : __u_s * 1.0e-300 * 1.0e-300;
        if (__u_ix > 1072693248) return __u_hy > 0 ? __u_s * 1.0e300 * 1.0e300 : __u_s * 1.0e-300 * 1.0e-300;
        __u_t = __u_ax - 1.0;
        __u_w = (__u_t * __u_t) * (0.5 - __u_t * (0.3333333333333333333333 - __u_t * 0.25));
        __u_u = 1.44269502162933349609e+00 * __u_t;
        __u_v = __u_t * 1.92596299112661746887e-08 - __u_w * 1.44269504088896338700e+00;
        __u_t1 = __u_u + __u_v;
        __u_t1 = _m_mk(_m_hi(__u_t1), 0);
        __u_t2 = __u_v - (__u_t1 - __u_u);
    } else {
        __u_n = 0;
        if (__u_ix < 1048576) { __u_ax = __u_ax * 9007199254740992.0; __u_n = __u_n - 53; __u_ix = _m_hi(__u_ax); }
        __u_n = __u_n + (__u_ix >> 20) - 1023;
        __u_j = __u_ix & 1048575;
        __u_ix = __u_j | 1072693248;
        if (__u_j <= 235662) __u_k = 0;                             /* |x| < sqrt(3/2) */
        else { if (__u_j < 767610) __u_k = 1;                       /* |x| < sqrt(3) */
               else { __u_k = 0; __u_n = __u_n + 1; __u_ix = __u_ix - 1048576; } }
        __u_ax = _m_sethi(__u_ax, __u_ix);
        __u_bpk = __u_k ? 1.5 : 1.0;
        __u_dphk = __u_k ? 5.84962487220764160156e-01 : 0.0;
        __u_dplk = __u_k ? 1.35003920212974897128e-08 : 0.0;
        __u_u = __u_ax - __u_bpk;
        __u_v = 1.0 / (__u_ax + __u_bpk);
        __u_ss = __u_u * __u_v;
        __u_s_h = _m_mk(_m_hi(__u_ss), 0);
        __u_t_h = _m_mk(((__u_ix >> 1) | 536870912) + 524288 + (__u_k << 18), 0);
        __u_t_l = __u_ax - (__u_t_h - __u_bpk);
        __u_s_l = __u_v * ((__u_u - __u_s_h * __u_t_h) - __u_s_h * __u_t_l);
        __u_s2 = __u_ss * __u_ss;
        __u_r = __u_s2 * __u_s2 * (5.99999999999994648725e-01 + __u_s2 * (4.28571428578550184252e-01
            + __u_s2 * (3.33333329818377432918e-01 + __u_s2 * (2.72728123808534006489e-01
            + __u_s2 * (2.30660745775561754067e-01 + __u_s2 * 2.06975017800338417784e-01)))));
        __u_r = __u_r + __u_s_l * (__u_s_h + __u_ss);
        __u_s2 = __u_s_h * __u_s_h;
        __u_t_h = 3.0 + __u_s2 + __u_r;
        __u_t_h = _m_mk(_m_hi(__u_t_h), 0);
        __u_t_l = __u_r - ((__u_t_h - 3.0) - __u_s2);
        __u_u = __u_s_h * __u_t_h;
        __u_v = __u_s_l * __u_t_h + __u_t_l * __u_ss;
        __u_p_h = __u_u + __u_v;
        __u_p_h = _m_mk(_m_hi(__u_p_h), 0);
        __u_p_l = __u_v - (__u_p_h - __u_u);
        __u_z_h = 9.61796700954437255859e-01 * __u_p_h;
        __u_z_l = (0 - 7.02846165095275826516e-09) * __u_p_h + __u_p_l * 9.61796693925975554329e-01 + __u_dplk;
        __u_t = (double)__u_n;
        __u_t1 = (((__u_z_h + __u_z_l) + __u_dphk) + __u_t);
        __u_t1 = _m_mk(_m_hi(__u_t1), 0);
        __u_t2 = __u_z_l - (((__u_t1 - __u_t) - __u_dphk) - __u_z_h);
    }
    __u_y1 = _m_mk(_m_hi(__u_y), 0);
    __u_p_l = (__u_y - __u_y1) * __u_t1 + __u_y * __u_t2;
    __u_p_h = __u_y1 * __u_t1;
    __u_z = __u_p_l + __u_p_h;
    __u_j = _m_hi(__u_z);
    __u_i = _m_lo(__u_z);
    if (__u_j >= 1083179008) {                                  /* z >= 1024 */
        if (((__u_j - 1083179008) | __u_i) != 0) return __u_s * 1.0e300 * 1.0e300;
        if (__u_p_l + 8.0085662595372944372e-17 > __u_z - __u_p_h) return __u_s * 1.0e300 * 1.0e300;
    } else { if ((__u_j & 2147483647) >= 1083231232) {          /* z <= -1075 */
        if (((__u_j - (0 - 1064252416)) | __u_i) != 0) return __u_s * 1.0e-300 * 1.0e-300;
        if (__u_p_l <= __u_z - __u_p_h) return __u_s * 1.0e-300 * 1.0e-300;
    } }
    __u_i = __u_j & 2147483647;
    __u_k = (__u_i >> 20) - 1023;
    __u_n = 0;
    if (__u_i > 1071644672) {                                   /* |z| > 0.5 */
        __u_n = __u_j + (1048576 >> (__u_k + 1));
        __u_k = ((__u_n & 2147483647) >> 20) - 1023;
        __u_t = _m_mk(__u_n & ~(1048575 >> __u_k), 0);
        __u_n = ((__u_n & 1048575) | 1048576) >> (20 - __u_k);
        if (__u_j < 0) __u_n = 0 - __u_n;
        __u_p_h = __u_p_h - __u_t;
    }
    __u_t = __u_p_l + __u_p_h;
    __u_t = _m_mk(_m_hi(__u_t), 0);
    __u_u = __u_t * 6.93147182464599609375e-01;
    __u_v = (__u_p_l - (__u_t - __u_p_h)) * 6.93147180559945286227e-01 + __u_t * (0 - 1.90465429995776804525e-09);
    __u_z = __u_u + __u_v;
    __u_w = __u_v - (__u_z - __u_u);
    __u_t = __u_z * __u_z;
    __u_t1 = __u_z - __u_t * (1.66666666666666019037e-01 + __u_t * (0 - 2.77777777770155933842e-03
         + __u_t * (6.61375632143793436117e-05 + __u_t * (0 - 1.65339022054652515390e-06
         + __u_t * 4.13813679705723846039e-08))));
    __u_r = (__u_z * __u_t1) / (__u_t1 - 2.0) - (__u_w + __u_z * __u_w);
    __u_z = 1.0 - (__u_r - __u_z);
    __u_j = _m_hi(__u_z);
    __u_j = __u_j + (__u_n << 20);
    if ((__u_j >> 20) <= 0) __u_z = scalbn(__u_z, __u_n);                   /* subnormal */
    else __u_z = _m_sethi(__u_z, _m_hi(__u_z) + (__u_n << 20));
    return __u_s * __u_z;
}
static float powf(float __u_x, float __u_y) { return (float)pow((double)__u_x, (double)__u_y); }
static double hypot(double __u_x, double __u_y) {
    double __u_a; double __u_b; double __u_t;
    __u_a = fabs(__u_x); __u_b = fabs(__u_y);
    if (isinf(__u_a) || isinf(__u_b)) return 1.0 / 0.0;
    if (__u_a < __u_b) { __u_t = __u_a; __u_a = __u_b; __u_b = __u_t; }
    if (__u_a == 0.0) return 0.0;
    __u_t = __u_b / __u_a;
    return __u_a * sqrt(1.0 + __u_t * __u_t);
}
static double cbrt(double __u_x) {
    double __u_y;
    int __u_k;
    if (__u_x == 0.0 || __u_x != __u_x || isinf(__u_x)) return __u_x;
    __u_y = copysign(exp(log(fabs(__u_x)) / 3.0), __u_x);
    __u_k = 0;
    while (__u_k < 2) { __u_y = __u_y - (__u_y * __u_y * __u_y - __u_x) / (3.0 * __u_y * __u_y); __u_k = __u_k + 1; }   /* Newton */
    return __u_y;
}
static double fmax(double __u_a, double __u_b) { if (__u_a != __u_a) return __u_b; if (__u_b != __u_b) return __u_a; return __u_a > __u_b ? __u_a : __u_b; }
static double fmin(double __u_a, double __u_b) { if (__u_a != __u_a) return __u_b; if (__u_b != __u_b) return __u_a; return __u_a < __u_b ? __u_a : __u_b; }
static double fdim(double __u_a, double __u_b) { return __u_a > __u_b ? __u_a - __u_b : 0.0; }
#endif
