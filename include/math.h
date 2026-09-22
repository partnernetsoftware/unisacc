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

static int _m_hi(double x) { return *(1 + (int *)&x); }
static unsigned int _m_lo(double x) { return *(unsigned int *)&x; }
static double _m_mk(int hi, unsigned int lo) {
    double r;
    *(unsigned int *)&r = lo;
    *(1 + (int *)&r) = hi;
    return r;
}
static double _m_sethi(double x, int hi) { return _m_mk(hi, _m_lo(x)); }

static double sqrt(double x) { return __builtin_sqrt(x); }
static float sqrtf(float x) { return __builtin_sqrtf(x); }

static double fabs(double x) { return _m_mk(_m_hi(x) & 2147483647, _m_lo(x)); }
static float fabsf(float x) { return (float)fabs((double)x); }
static double copysign(double x, double y) {
    return _m_mk((_m_hi(x) & 2147483647) | (_m_hi(y) & (0 - 2147483647 - 1)), _m_lo(x));
}

static int isnan(double x) { return x != x; }
static int isinf(double x) { return (_m_hi(x) & 2147483647) == 2146435072 && _m_lo(x) == 0; }
static int isfinite(double x) { return (_m_hi(x) & 2146435072) != 2146435072; }
static int signbit(double x) { return _m_hi(x) < 0; }

/* integral parts: exact.  Past 2^52 every double is already an integer. */
static double trunc(double x) {
    double a;
    a = fabs(x);
    if (a >= 4503599627370496.0 || x != x) return x;
    return copysign((double)(long)x, x);
}
static double floor(double x) {
    double t;
    t = trunc(x);
    if (t > x) t = t - 1.0;
    if (t == 0.0) return copysign(0.0, x);        /* floor(-0.0) is -0.0 */
    return t;
}
static double ceil(double x) {
    double t;
    t = trunc(x);
    if (t < x) t = t + 1.0;
    if (t == 0.0) return copysign(0.0, x);
    return t;
}
static double round(double x) {                  /* halfway away from zero */
    double t;
    t = trunc(x);
    if (fabs(x - t) >= 0.5) t = t + copysign(1.0, x);
    if (t == 0.0) return copysign(0.0, x);
    return t;
}
static long lround(double x) { return (long)round(x); }
static float floorf(float x) { return (float)floor((double)x); }
static float ceilf(float x) { return (float)ceil((double)x); }

static double modf(double x, double *ip) {
    double t;
    t = trunc(x);
    *ip = t;
    if (isinf(x)) return copysign(0.0, x);
    return copysign(x - t, x);
}

/* x = m * 2^e with 0.5 <= |m| < 1 */
static double frexp(double x, int *e) {
    int hx; int ix;
    hx = _m_hi(x); ix = hx & 2147483647;
    *e = 0;
    if (ix >= 2146435072 || (ix | (int)_m_lo(x)) == 0) return x;   /* 0, inf, nan */
    if (ix < 1048576) {                                 /* subnormal */
        x = x * 18014398509481984.0;                    /* 2^54 */
        hx = _m_hi(x); ix = hx & 2147483647; *e = 0 - 54;
    }
    *e = *e + (ix >> 20) - 1022;
    return _m_sethi(x, (hx & (0 - 2146435073)) | 1071644672);
}
static double ldexp(double x, int n) {
    /* by exact powers of two, in steps that cannot overflow the exponent */
    while (n > 1000) { x = x * 1.0715086071862673e301; n = n - 1000; }       /* 2^1000 */
    while (n < 0 - 1000) { x = x * 9.3326361850321888e-302; n = n + 1000; }  /* 2^-1000 */
    if (n >= 0) return x * _m_mk((n + 1023) << 20, 0);
    if (n > 0 - 1022) return x * _m_mk((n + 1023) << 20, 0);
    return x * _m_mk((n + 1023 + 60) << 20, 0) * 8.673617379884035e-19;    /* 2^-60 */
}
static double scalbn(double x, int n) { return ldexp(x, n); }

/* fmod, exactly: C99 7.12.10.1 -- the result is x - n*y for the integer n
   that makes it have x's sign and be smaller than |y|, and it is exact. */
static double fmod(double x, double y) {
    int ex; int ey;
    double ax; double ay; double r;
    if (y != y || x != x || isinf(x) || y == 0.0) return (x * y) / (x * y);
    ax = fabs(x); ay = fabs(y);
    if (ax < ay) return x;
    frexp(ax, &ex); frexp(ay, &ey);
    r = ax;
    while (ex >= ey) {
        double t;
        t = ldexp(ay, ex - ey);
        if (t > r) { if (ex == ey) break; t = ldexp(ay, ex - ey - 1); }
        if (t <= r) r = r - t;                   /* exact: same binade or below */
        if (r < ay) break;
        frexp(r, &ex);
    }
    return copysign(r, x);
}

/* ---- exp (fdlibm e_exp.c) ---------------------------------------------- */
static double exp(double x) {
    double hi; double lo; double c; double t; double y;
    int k; int xsb; int hx;
    unsigned int ux;
    hi = 0.0; lo = 0.0; k = 0;
    hx = _m_hi(x);
    xsb = (hx >> 31) & 1;
    ux = hx & 2147483647;
    if (ux >= 1082535490) {                               /* |x| >= 709.78 */
        if (ux >= 2146435072) {
            if (((ux & 1048575) | _m_lo(x)) != 0) return x + x;   /* nan */
            return xsb == 0 ? x : 0.0;                           /* exp(+-inf) */
        }
        if (x > 7.09782712893383973096e+02) return 1.0e300 * 1.0e300;
        if (x < 0 - 7.45133219101941108420e+02) return 1.0e-300 * 1.0e-300;
    }
    if (ux > 1071001154) {                                /* |x| > 0.5 ln2 */
        if (ux < 1072734898) {                            /* |x| < 1.5 ln2 */
            if (xsb == 0) { hi = x - 6.93147180369123816490e-01; lo = 1.90821492927058770002e-10; k = 1; }
            else { hi = x + 6.93147180369123816490e-01; lo = 0 - 1.90821492927058770002e-10; k = 0 - 1; }
        } else {
            k = (int)(1.44269504088896338700e+00 * x + (xsb ? 0 - 0.5 : 0.5));
            t = k;
            hi = x - t * 6.93147180369123816490e-01;
            lo = t * 1.90821492927058770002e-10;
        }
        x = hi - lo;
    } else {
        if (ux < 1043333120) {                            /* |x| < 2^-28 */
            if (1.0e300 + x > 1.0) return 1.0 + x;
        } else k = 0;
    }
    t = x * x;
    c = x - t * (1.66666666666666019037e-01 + t * (0 - 2.77777777770155933842e-03
        + t * (6.61375632143793436117e-05 + t * (0 - 1.65339022054652515390e-06
        + t * 4.13813679705723846039e-08))));
    if (k == 0) return 1.0 - ((x * c) / (c - 2.0) - x);
    y = 1.0 - ((lo - (x * c) / (2.0 - c)) - hi);
    if (k >= 0 - 1021) return _m_sethi(y, _m_hi(y) + (k << 20));
    y = _m_sethi(y, _m_hi(y) + ((k + 1000) << 20));
    return y * 9.33263618503218878990e-302;               /* 2^-1000 */
}
static float expf(float x) { return (float)exp((double)x); }
static double exp2(double x) { return exp(x * 6.93147180559945286227e-01); }

/* ---- log (fdlibm e_log.c) ---------------------------------------------- */
static double log(double x) {
    double hfsq; double f; double s; double z; double R; double w; double t1;
    double t2; double dk;
    int k; int hx; int i; int j;
    unsigned int lx;
    hx = _m_hi(x); lx = _m_lo(x);
    k = 0;
    if (hx < 1048576) {                                   /* x < 2^-1022 */
        if (((hx & 2147483647) | lx) == 0) return (0 - 1.80143985094819840000e+16) / 0.0;
        if (hx < 0) return (x - x) / 0.0;
        k = k - 54; x = x * 1.80143985094819840000e+16;
        hx = _m_hi(x);
    }
    if (hx >= 2146435072) return x + x;
    k = k + (hx >> 20) - 1023;
    hx = hx & 1048575;
    i = (hx + 614244) & 1048576;
    x = _m_sethi(x, hx | (i ^ 1072693248));
    k = k + (i >> 20);
    f = x - 1.0;
    if ((1048575 & (2 + hx)) < 3) {
        if (f == 0.0) {
            if (k == 0) return 0.0;
            dk = k; return dk * 6.93147180369123816490e-01 + dk * 1.90821492927058770002e-10;
        }
        R = f * f * (0.5 - 0.33333333333333333 * f);
        if (k == 0) return f - R;
        dk = k; return dk * 6.93147180369123816490e-01 - ((R - dk * 1.90821492927058770002e-10) - f);
    }
    s = f / (2.0 + f);
    dk = k;
    z = s * s;
    i = hx - 398458;
    w = z * z;
    j = 440401 - hx;
    t1 = w * (3.999999999940941908e-01 + w * (2.222219843214978396e-01 + w * 1.531383769920937332e-01));
    t2 = z * (6.666666666666735130e-01 + w * (2.857142874366239149e-01 + w * (1.818357216161805012e-01
         + w * 1.479819860511658591e-01)));
    i = i | j;
    R = t2 + t1;
    if (i > 0) {
        hfsq = 0.5 * f * f;
        if (k == 0) return f - (hfsq - s * (hfsq + R));
        return dk * 6.93147180369123816490e-01 - ((hfsq - (s * (hfsq + R) + dk * 1.90821492927058770002e-10)) - f);
    }
    if (k == 0) return f - s * (f - R);
    return dk * 6.93147180369123816490e-01 - ((s * (f - R) - dk * 1.90821492927058770002e-10) - f);
}
static float logf(float x) { return (float)log((double)x); }
static double log10(double x) {                            /* fdlibm e_log10.c */
    double y; double z;
    int hx; int k; int i;
    unsigned int lx;
    hx = _m_hi(x); lx = _m_lo(x);
    k = 0;
    if (hx < 1048576) {
        if (((hx & 2147483647) | lx) == 0) return (0 - 1.80143985094819840000e+16) / 0.0;
        if (hx < 0) return (x - x) / 0.0;
        k = k - 54; x = x * 1.80143985094819840000e+16;
        hx = _m_hi(x);
    }
    if (hx >= 2146435072) return x + x;
    k = k + (hx >> 20) - 1023;
    i = (k & (0 - 2147483647 - 1)) >> 31;
    i = i & 1;
    hx = (hx & 1048575) | ((1023 - i) << 20);
    y = (double)(k + i);
    x = _m_sethi(x, hx);
    z = y * 3.69423907715893078616e-13 + 4.34294481903251816668e-01 * log(x);
    return z + y * 3.01029995663611771306e-01;
}
static double log2(double x) { return log(x) * 1.44269504088896338700e+00; }
/* log(1+x) accurately for small x: the rounding error of 1+x is corrected
   by the ratio x / ((1+x) - 1) (Goldberg, "What every computer scientist
   should know about floating-point arithmetic", thm. 4) */
static double log1p(double x) {
    double u;
    u = 1.0 + x;
    if (u == 1.0) return x;
    return log(u) * x / (u - 1.0);
}

/* ---- sin, cos, tan (fdlibm k_sin.c, k_cos.c, e_rem_pio2.c) -------------- */
static double _m_ksin(double x, double y, int iy) {
    double z; double r; double v;
    int ix;
    ix = _m_hi(x) & 2147483647;
    if (ix < 1044381696) { if ((int)x == 0) return x; }        /* |x| < 2^-27 */
    z = x * x;
    v = z * x;
    r = 8.33333333332248946124e-03 + z * (0 - 1.98412698298579493134e-04 + z * (2.75573137070700676789e-06
        + z * (0 - 2.50507602534068634195e-08 + z * 1.58969099521155010221e-10)));
    if (iy == 0) return x + v * (0 - 1.66666666666666324348e-01 + z * r);
    return x - ((z * (0.5 * y - v * r) - y) - v * (0 - 1.66666666666666324348e-01));
}
static double _m_kcos(double x, double y) {
    double a; double hz; double z; double r; double qx;
    int ix;
    ix = _m_hi(x) & 2147483647;
    if (ix < 1044381696) { if ((int)x == 0) return 1.0; }
    z = x * x;
    r = z * (4.16666666666666019037e-02 + z * (0 - 1.38888888888741095749e-03 + z * (2.48015872894767294178e-05
        + z * (0 - 2.75573143513906633035e-07 + z * (2.08757232129817482790e-09
        + z * (0 - 1.13596475577881948265e-11))))));
    if (ix < 1070805811) return 1.0 - (0.5 * z - (z * r - x * y));   /* |x| < 0.3 */
    if (ix > 1072234496) qx = 0.28125;                               /* |x| > 0.78125 */
    else qx = _m_mk(ix - 2097152, 0);
    hz = 0.5 * z - qx;
    a = 1.0 - qx;
    return a - (hz - (z * r - x * y));
}
/* x = n*(pi/2) + (y[0] + y[1]); returns n.  The medium range is fdlibm's;
   beyond |x| ~ 2^20*pi/2 the reduction is only as good as three terms of
   pi/2 make it (fdlibm's Payne-Hanek path is not carried). */
static int _m_rempio2(double x, double *y) {
    double z; double w; double t; double r; double fn;
    int i; int j; int n; int ix; int hx;
    hx = _m_hi(x);
    ix = hx & 2147483647;
    if (ix <= 1072243195) { y[0] = x; y[1] = 0.0; return 0; }        /* |x| <= pi/4 */
    if (ix < 1073928572) {                                           /* |x| < 3pi/4 */
        if (hx > 0) {
            z = x - 1.57079632673412561417e+00;
            if (ix != 1073291771) {
                y[0] = z - 6.07710050650619224932e-11;
                y[1] = (z - y[0]) - 6.07710050650619224932e-11;
            } else {
                z = z - 6.07710050630396597660e-11;
                y[0] = z - 2.02226624879595063154e-21;
                y[1] = (z - y[0]) - 2.02226624879595063154e-21;
            }
            return 1;
        }
        z = x + 1.57079632673412561417e+00;
        if (ix != 1073291771) {
            y[0] = z + 6.07710050650619224932e-11;
            y[1] = (z - y[0]) + 6.07710050650619224932e-11;
        } else {
            z = z + 6.07710050630396597660e-11;
            y[0] = z + 2.02226624879595063154e-21;
            y[1] = (z - y[0]) + 2.02226624879595063154e-21;
        }
        return 0 - 1;
    }
    t = fabs(x);
    n = (int)(t * 6.36619772367581382433e-01 + 0.5);
    fn = (double)n;
    r = t - fn * 1.57079632673412561417e+00;
    w = fn * 6.07710050650619224932e-11;
    j = ix >> 20;
    y[0] = r - w;
    i = j - ((_m_hi(y[0]) >> 20) & 2047);
    if (i > 16) {
        t = r;
        w = fn * 6.07710050630396597660e-11;
        r = t - w;
        w = fn * 2.02226624879595063154e-21 - ((t - r) - w);
        y[0] = r - w;
        i = j - ((_m_hi(y[0]) >> 20) & 2047);
        if (i > 49) {
            t = r;
            w = fn * 2.02226624871116645580e-21;
            r = t - w;
            w = fn * 8.47842766036889956997e-32 - ((t - r) - w);
            y[0] = r - w;
        }
    }
    y[1] = (r - y[0]) - w;
    if (hx < 0) { y[0] = 0 - y[0]; y[1] = 0 - y[1]; return 0 - n; }
    return n;
}
static double sin(double x) {
    double y[2];
    int n; int ix;
    ix = _m_hi(x) & 2147483647;
    if (ix <= 1072243195) return _m_ksin(x, 0.0, 0);
    if (ix >= 2146435072) return x - x;
    n = _m_rempio2(x, y);
    n = n & 3;
    if (n == 0) return _m_ksin(y[0], y[1], 1);
    if (n == 1) return _m_kcos(y[0], y[1]);
    if (n == 2) return 0 - _m_ksin(y[0], y[1], 1);
    return 0 - _m_kcos(y[0], y[1]);
}
static double cos(double x) {
    double y[2];
    int n; int ix;
    ix = _m_hi(x) & 2147483647;
    if (ix <= 1072243195) return _m_kcos(x, 0.0);
    if (ix >= 2146435072) return x - x;
    n = _m_rempio2(x, y);
    n = n & 3;
    if (n == 0) return _m_kcos(y[0], y[1]);
    if (n == 1) return 0 - _m_ksin(y[0], y[1], 1);
    if (n == 2) return 0 - _m_kcos(y[0], y[1]);
    return _m_ksin(y[0], y[1], 1);
}
static double tan(double x) { return sin(x) / cos(x); }
static float sinf(float x) { return (float)sin((double)x); }
static float cosf(float x) { return (float)cos((double)x); }
static float tanf(float x) { return (float)tan((double)x); }

/* ---- atan, atan2, asin, acos (fdlibm s_atan.c, e_atan2.c) --------------- */
static double atan(double x) {
    double w; double s1; double s2; double z; double hi; double lo;
    int ix; int hx; int id;
    hx = _m_hi(x);
    ix = hx & 2147483647;
    if (ix >= 1141899264) {                                /* |x| >= 2^66 */
        if (ix > 2146435072 || (ix == 2146435072 && _m_lo(x) != 0)) return x + x;
        if (hx > 0) return 1.57079632679489655800e+00 + 6.12323399573676603587e-17;
        return 0 - 1.57079632679489655800e+00 - 6.12323399573676603587e-17;
    }
    if (ix < 1071382528) {                                 /* |x| < 0.4375 */
        if (ix < 1042284544) { if (1.0e300 + x > 1.0) return x; }
        id = 0 - 1;
    } else {
        x = fabs(x);
        if (ix < 1072889856) {                             /* |x| < 1.1875 */
            if (ix < 1072037888) { id = 0; x = (2.0 * x - 1.0) / (2.0 + x); }
            else { id = 1; x = (x - 1.0) / (x + 1.0); }
        } else {
            if (ix < 1073971200) { id = 2; x = (x - 1.5) / (1.0 + 1.5 * x); }
            else { id = 3; x = (0 - 1.0) / x; }
        }
    }
    z = x * x;
    w = z * z;
    s1 = z * (3.33333333333329318027e-01 + w * (1.42857142725034663711e-01 + w * (9.09088713343650656196e-02
         + w * (6.66107313738753120669e-02 + w * (4.97687799461593236017e-02 + w * 1.62858201153657823623e-02)))));
    s2 = w * (0 - 1.99999999998764832476e-01 + w * (0 - 1.11111104054623557880e-01 + w * (0 - 7.69187620504482999495e-02
         + w * (0 - 5.83357013379057348645e-02 + w * (0 - 3.65315727442169155270e-02)))));
    if (id < 0) return x - x * (s1 + s2);
    if (id == 0) { hi = 4.63647609000806093515e-01; lo = 2.26987774529616870924e-17; }
    if (id == 1) { hi = 7.85398163397448278999e-01; lo = 3.06161699786838301793e-17; }
    if (id == 2) { hi = 9.82793723247329054082e-01; lo = 1.39033110312309984516e-17; }
    if (id == 3) { hi = 1.57079632679489655800e+00; lo = 6.12323399573676603587e-17; }
    z = hi - ((x * (s1 + s2) - lo) - x);
    return hx < 0 ? 0 - z : z;
}
static double atan2(double y, double x) {
    double z;
    if (x != x || y != y) return x + y;
    if (x == 1.0) return atan(y);
    if (y == 0.0) {
        if (signbit(x)) return signbit(y) ? 0 - 3.14159265358979311600e+00 : 3.14159265358979311600e+00;
        return y;
    }
    if (x == 0.0) return y > 0 ? 1.57079632679489655800e+00 : 0 - 1.57079632679489655800e+00;
    if (isinf(x)) {
        if (isinf(y)) {
            if (x > 0) return y > 0 ? 7.85398163397448278999e-01 : 0 - 7.85398163397448278999e-01;
            return y > 0 ? 2.35619449019234492885e+00 : 0 - 2.35619449019234492885e+00;
        }
        if (x > 0) return copysign(0.0, y);
        return y > 0 ? 3.14159265358979311600e+00 : 0 - 3.14159265358979311600e+00;
    }
    if (isinf(y)) return y > 0 ? 1.57079632679489655800e+00 : 0 - 1.57079632679489655800e+00;
    z = atan(fabs(y / x));
    if (x > 0) return y > 0 ? z : 0 - z;
    z = 3.14159265358979311600e+00 - (z - 1.22464679914735317720e-16);
    return y > 0 ? z : 0 - z;
}
static double asin(double x) {
    if (x > 1.0 || x < 0 - 1.0) return (x - x) / (x - x);
    return atan2(x, sqrt((1.0 - x) * (1.0 + x)));
}
static double acos(double x) {
    if (x > 1.0 || x < 0 - 1.0) return (x - x) / (x - x);
    return atan2(sqrt((1.0 - x) * (1.0 + x)), x);
}
static float atanf(float x) { return (float)atan((double)x); }
static float atan2f(float y, float x) { return (float)atan2((double)y, (double)x); }

/* ---- expm1, sinh, cosh, tanh (fdlibm s_expm1.c, e_sinh.c, e_cosh.c,
 *      s_tanh.c) -------------------------------------------------------- */
static double expm1(double x) {
    double y; double hi; double lo; double c; double t; double e; double hxs;
    double hfx; double r1;
    int k; int xsb;
    unsigned int hx;
    c = 0.0;
    hx = _m_hi(x);
    xsb = hx & 2147483648;
    if (xsb == 0) y = x; else y = 0 - x;
    hx = hx & 2147483647;
    if (hx >= 1078159482) {                                 /* |x| >= 56 ln2 */
        if (hx >= 1082535490) {                             /* |x| >= 709.78 */
            if (hx >= 2146435072) {
                if (((hx & 1048575) | _m_lo(x)) != 0) return x + x;
                return xsb == 0 ? x : 0 - 1.0;
            }
            if (x > 7.09782712893383973096e+02) return 1.0e300 * 1.0e300;
        }
        if (xsb != 0) { if (x + 1.0e-300 < 0.0) return 1.0e-300 - 1.0; }
    }
    if (hx > 1071001154) {                                  /* |x| > 0.5 ln2 */
        if (hx < 1072734898) {                              /* |x| < 1.5 ln2 */
            if (xsb == 0) { hi = x - 6.93147180369123816490e-01; lo = 1.90821492927058770002e-10; k = 1; }
            else { hi = x + 6.93147180369123816490e-01; lo = 0 - 1.90821492927058770002e-10; k = 0 - 1; }
        } else {
            k = (int)(1.44269504088896338700e+00 * x + (xsb == 0 ? 0.5 : 0 - 0.5));
            t = k;
            hi = x - t * 6.93147180369123816490e-01;
            lo = t * 1.90821492927058770002e-10;
        }
        x = hi - lo;
        c = (hi - x) - lo;
    } else {
        if (hx < 1016070144) {                              /* |x| < 2^-54 */
            t = 1.0e300 + x;
            return x - (t - (1.0e300 + x));
        }
        k = 0;
    }
    hfx = 0.5 * x;
    hxs = x * hfx;
    r1 = 1.0 + hxs * (0 - 3.33333333333331316428e-02 + hxs * (1.58730158725481460165e-03
         + hxs * (0 - 7.93650757867487942473e-05 + hxs * (4.00821782732936239552e-06
         + hxs * (0 - 2.01099218183624371326e-07)))));
    t = 3.0 - r1 * hfx;
    e = hxs * ((r1 - t) / (6.0 - x * t));
    if (k == 0) return x - (x * e - hxs);
    e = (x * (e - c) - c);
    e = e - hxs;
    if (k == 0 - 1) return 0.5 * (x - e) - 0.5;
    if (k == 1) {
        if (x < 0 - 0.25) return 0 - 2.0 * (e - (x + 0.5));
        return 1.0 + 2.0 * (x - e);
    }
    if (k <= 0 - 2 || k > 56) {
        y = 1.0 - (e - x);
        y = _m_sethi(y, _m_hi(y) + (k << 20));
        return y - 1.0;
    }
    t = 1.0;
    if (k < 20) {
        t = _m_sethi(t, 1072693248 - (2097152 >> k));       /* 1 - 2^-k */
        y = t - (e - x);
        y = _m_sethi(y, _m_hi(y) + (k << 20));
    } else {
        t = _m_sethi(t, (1023 - k) << 20);                  /* 2^-k */
        y = x - (e + t);
        y = y + 1.0;
        y = _m_sethi(y, _m_hi(y) + (k << 20));
    }
    return y;
}
static double sinh(double x) {
    double t; double w; double h;
    int ix; int jx;
    unsigned int lx;
    jx = _m_hi(x);
    ix = jx & 2147483647;
    if (ix >= 2146435072) return x + x;
    h = 0.5;
    if (jx < 0) h = 0 - h;
    if (ix < 1077280768) {                                  /* |x| < 22 */
        if (ix < 1043333120) { if (1.0e307 + x > 1.0) return x; }   /* |x| < 2^-28 */
        t = expm1(fabs(x));
        if (ix < 1072693248) return h * (2.0 * t - t * t / (t + 1.0));
        return h * (t + t / (t + 1.0));
    }
    if (ix < 1082535490) return h * exp(fabs(x));           /* |x| < log(DBL_MAX) */
    lx = _m_lo(x);
    if (ix < 1082536910 || (ix == 1082536910 && lx <= 2411329661)) {
        w = exp(0.5 * fabs(x));
        t = h * w;
        return t * w;
    }
    return x * 1.0e307;
}
static double cosh(double x) {
    double t; double w;
    int ix;
    unsigned int lx;
    ix = _m_hi(x) & 2147483647;
    if (ix >= 2146435072) return x * x;
    if (ix < 1071001155) {                                  /* |x| < 0.5 ln2 */
        t = expm1(fabs(x));
        w = 1.0 + t;
        if (ix < 1015021568) return w;                      /* |x| < 2^-55 */
        return 1.0 + (t * t) / (w + w);
    }
    if (ix < 1077280768) { t = exp(fabs(x)); return 0.5 * t + 0.5 / t; }
    if (ix < 1082535490) return 0.5 * exp(fabs(x));
    lx = _m_lo(x);
    if (ix < 1082536910 || (ix == 1082536910 && lx <= 2411329661)) {
        w = exp(0.5 * fabs(x));
        t = 0.5 * w;
        return t * w;
    }
    return 1.0e300 * 1.0e300;
}
static double tanh(double x) {
    double t; double z;
    int jx; int ix;
    jx = _m_hi(x);
    ix = jx & 2147483647;
    if (ix >= 2146435072) { if (jx >= 0) return 1.0 / x + 1.0; return 1.0 / x - 1.0; }
    if (ix < 1077280768) {                                  /* |x| < 22 */
        if (ix < 1015021568) return x * (1.0 + x);          /* |x| < 2^-55 */
        if (ix >= 1072693248) {                             /* |x| >= 1 */
            t = expm1(2.0 * fabs(x));
            z = 1.0 - 2.0 / (t + 2.0);
        } else {
            t = expm1(0 - 2.0 * fabs(x));
            z = (0 - t) / (t + 2.0);
        }
    } else z = 1.0 - 1.0e-300;
    return jx >= 0 ? z : 0 - z;
}

/* ---- pow (fdlibm e_pow.c) ----------------------------------------------- */
static double pow(double x, double y) {
    double z; double ax; double z_h; double z_l; double p_h; double p_l;
    double y1; double t1; double t2; double r; double s; double t; double u;
    double v; double w;
    double ss; double s2; double s_h; double s_l; double t_h; double t_l;
    double bpk; double dphk; double dplk;
    int i; int j; int k; int yisint; int n;
    int hx; int hy; int ix; int iy;
    unsigned int lx; unsigned int ly;
    hx = _m_hi(x); lx = _m_lo(x);
    hy = _m_hi(y); ly = _m_lo(y);
    ix = hx & 2147483647; iy = hy & 2147483647;
    if ((iy | ly) == 0) return 1.0;                         /* x**0 = 1 */
    if (ix > 2146435072 || (ix == 2146435072 && lx != 0)
        || iy > 2146435072 || (iy == 2146435072 && ly != 0)) return x + y;
    /* is y an odd integer (1), an even one (2), or not one (0)? for x < 0 */
    yisint = 0;
    if (hx < 0) {
        if (iy >= 1128267776) yisint = 2;                   /* |y| >= 2^53 */
        else { if (iy >= 1072693248) {
            k = (iy >> 20) - 1023;
            if (k > 20) {
                j = ly >> (52 - k);
                if ((unsigned int)(j << (52 - k)) == ly) yisint = 2 - (j & 1);
            } else { if (ly == 0) {
                j = iy >> (20 - k);
                if ((j << (20 - k)) == iy) yisint = 2 - (j & 1);
            } }
        } }
    }
    if (ly == 0) {
        if (iy == 2146435072) {                             /* y is +-inf */
            if (((ix - 1072693248) | lx) == 0) return y - y;
            if (ix >= 1072693248) return hy >= 0 ? y : 0.0;
            return hy < 0 ? 0 - y : 0.0;
        }
        if (iy == 1072693248) { if (hy < 0) return 1.0 / x; return x; }   /* +-1 */
        if (hy == 1073741824) return x * x;                              /* 2 */
        if (hy == 1071644672) { if (hx >= 0) return sqrt(x); }           /* 0.5 */
    }
    ax = fabs(x);
    if (lx == 0) {
        if (ix == 2146435072 || ix == 0 || ix == 1072693248) {  /* +-0, +-inf, +-1 */
            z = ax;
            if (hy < 0) z = 1.0 / z;
            if (hx < 0) {
                if (((ix - 1072693248) | yisint) == 0) z = (z - z) / (z - z);
                else { if (yisint == 1) z = 0 - z; }
            }
            return z;
        }
    }
    n = hx < 0 ? 0 : 1;                                     /* (hx >> 31) + 1 */
    if ((n | yisint) == 0) return (x - x) / (x - x);         /* (x<0)**non-int */
    s = 1.0;
    if ((n | (yisint - 1)) == 0) s = 0 - 1.0;                /* (-x)**odd */
    if (iy > 1105199104) {                                  /* |y| > 2^31 */
        if (iy > 1139802112) {                              /* |y| > 2^64 */
            if (ix <= 1072693247) return hy < 0 ? 1.0e300 * 1.0e300 : 1.0e-300 * 1.0e-300;
            if (ix >= 1072693248) return hy > 0 ? 1.0e300 * 1.0e300 : 1.0e-300 * 1.0e-300;
        }
        if (ix < 1072693247) return hy < 0 ? s * 1.0e300 * 1.0e300 : s * 1.0e-300 * 1.0e-300;
        if (ix > 1072693248) return hy > 0 ? s * 1.0e300 * 1.0e300 : s * 1.0e-300 * 1.0e-300;
        t = ax - 1.0;
        w = (t * t) * (0.5 - t * (0.3333333333333333333333 - t * 0.25));
        u = 1.44269502162933349609e+00 * t;
        v = t * 1.92596299112661746887e-08 - w * 1.44269504088896338700e+00;
        t1 = u + v;
        t1 = _m_mk(_m_hi(t1), 0);
        t2 = v - (t1 - u);
    } else {
        n = 0;
        if (ix < 1048576) { ax = ax * 9007199254740992.0; n = n - 53; ix = _m_hi(ax); }
        n = n + (ix >> 20) - 1023;
        j = ix & 1048575;
        ix = j | 1072693248;
        if (j <= 235662) k = 0;                             /* |x| < sqrt(3/2) */
        else { if (j < 767610) k = 1;                       /* |x| < sqrt(3) */
               else { k = 0; n = n + 1; ix = ix - 1048576; } }
        ax = _m_sethi(ax, ix);
        bpk = k ? 1.5 : 1.0;
        dphk = k ? 5.84962487220764160156e-01 : 0.0;
        dplk = k ? 1.35003920212974897128e-08 : 0.0;
        u = ax - bpk;
        v = 1.0 / (ax + bpk);
        ss = u * v;
        s_h = _m_mk(_m_hi(ss), 0);
        t_h = _m_mk(((ix >> 1) | 536870912) + 524288 + (k << 18), 0);
        t_l = ax - (t_h - bpk);
        s_l = v * ((u - s_h * t_h) - s_h * t_l);
        s2 = ss * ss;
        r = s2 * s2 * (5.99999999999994648725e-01 + s2 * (4.28571428578550184252e-01
            + s2 * (3.33333329818377432918e-01 + s2 * (2.72728123808534006489e-01
            + s2 * (2.30660745775561754067e-01 + s2 * 2.06975017800338417784e-01)))));
        r = r + s_l * (s_h + ss);
        s2 = s_h * s_h;
        t_h = 3.0 + s2 + r;
        t_h = _m_mk(_m_hi(t_h), 0);
        t_l = r - ((t_h - 3.0) - s2);
        u = s_h * t_h;
        v = s_l * t_h + t_l * ss;
        p_h = u + v;
        p_h = _m_mk(_m_hi(p_h), 0);
        p_l = v - (p_h - u);
        z_h = 9.61796700954437255859e-01 * p_h;
        z_l = (0 - 7.02846165095275826516e-09) * p_h + p_l * 9.61796693925975554329e-01 + dplk;
        t = (double)n;
        t1 = (((z_h + z_l) + dphk) + t);
        t1 = _m_mk(_m_hi(t1), 0);
        t2 = z_l - (((t1 - t) - dphk) - z_h);
    }
    y1 = _m_mk(_m_hi(y), 0);
    p_l = (y - y1) * t1 + y * t2;
    p_h = y1 * t1;
    z = p_l + p_h;
    j = _m_hi(z);
    i = _m_lo(z);
    if (j >= 1083179008) {                                  /* z >= 1024 */
        if (((j - 1083179008) | i) != 0) return s * 1.0e300 * 1.0e300;
        if (p_l + 8.0085662595372944372e-17 > z - p_h) return s * 1.0e300 * 1.0e300;
    } else { if ((j & 2147483647) >= 1083231232) {          /* z <= -1075 */
        if (((j - (0 - 1064252416)) | i) != 0) return s * 1.0e-300 * 1.0e-300;
        if (p_l <= z - p_h) return s * 1.0e-300 * 1.0e-300;
    } }
    i = j & 2147483647;
    k = (i >> 20) - 1023;
    n = 0;
    if (i > 1071644672) {                                   /* |z| > 0.5 */
        n = j + (1048576 >> (k + 1));
        k = ((n & 2147483647) >> 20) - 1023;
        t = _m_mk(n & ~(1048575 >> k), 0);
        n = ((n & 1048575) | 1048576) >> (20 - k);
        if (j < 0) n = 0 - n;
        p_h = p_h - t;
    }
    t = p_l + p_h;
    t = _m_mk(_m_hi(t), 0);
    u = t * 6.93147182464599609375e-01;
    v = (p_l - (t - p_h)) * 6.93147180559945286227e-01 + t * (0 - 1.90465429995776804525e-09);
    z = u + v;
    w = v - (z - u);
    t = z * z;
    t1 = z - t * (1.66666666666666019037e-01 + t * (0 - 2.77777777770155933842e-03
         + t * (6.61375632143793436117e-05 + t * (0 - 1.65339022054652515390e-06
         + t * 4.13813679705723846039e-08))));
    r = (z * t1) / (t1 - 2.0) - (w + z * w);
    z = 1.0 - (r - z);
    j = _m_hi(z);
    j = j + (n << 20);
    if ((j >> 20) <= 0) z = scalbn(z, n);                   /* subnormal */
    else z = _m_sethi(z, _m_hi(z) + (n << 20));
    return s * z;
}
static float powf(float x, float y) { return (float)pow((double)x, (double)y); }
static double hypot(double x, double y) {
    double a; double b; double t;
    a = fabs(x); b = fabs(y);
    if (isinf(a) || isinf(b)) return 1.0 / 0.0;
    if (a < b) { t = a; a = b; b = t; }
    if (a == 0.0) return 0.0;
    t = b / a;
    return a * sqrt(1.0 + t * t);
}
static double cbrt(double x) {
    double y;
    int k;
    if (x == 0.0 || x != x || isinf(x)) return x;
    y = copysign(exp(log(fabs(x)) / 3.0), x);
    k = 0;
    while (k < 2) { y = y - (y * y * y - x) / (3.0 * y * y); k = k + 1; }   /* Newton */
    return y;
}
static double fmax(double a, double b) { if (a != a) return b; if (b != b) return a; return a > b ? a : b; }
static double fmin(double a, double b) { if (a != a) return b; if (b != b) return a; return a < b ? a : b; }
static double fdim(double a, double b) { return a > b ? a - b : 0.0; }
#endif
