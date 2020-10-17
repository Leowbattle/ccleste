// The following is extracted from snippets of musl libc.
// http://musl.libc.org/

#include <math.h>
#include "stdint.h"

/////////////////
// include/math.h
#define M_PI_2          1.57079632679489661923  /* pi/2 */
/////////////////

//////////////////////
// src/internal/libm.h
static inline void fp_force_evalf(float x)
{
	volatile float y;
	y = x;
}
#define FORCE_EVAL(x) fp_force_evalf(x)

#define asuint(f) ((union{float _f; uint32_t _i;}){f})._i
#define GET_FLOAT_WORD(w,d)                       \
do {                                              \
  (w) = asuint(d);                                \
} while (0)
//////////////////////

// This is just a common trick to check for NaN
bool isnan(float x) {
	return x != x;
}

bool signbit(float x)
{
	return (*(uint32_t*)&x & 1);
}

// src/math/fmaxf.c
float fmaxf(float x, float y)
{
	if (isnan(x))
		return y;
	if (isnan(y))
		return x;
	/* handle signed zeroes, see C99 Annex F.9.9.2 */
	if (signbit(x) != signbit(y))
		return signbit(x) ? y : x;
	return x < y ? y : x;
}

// src/math/fminf.c
float fminf(float x, float y)
{
	if (isnan(x))
		return y;
	if (isnan(y))
		return x;
	/* handle signed zeros, see C99 Annex F.9.9.2 */
	if (signbit(x) != signbit(y))
		return signbit(x) ? x : y;
	return x < y ? x : y;
}

// src/math/fmodf.c
float fmodf(float x, float y)
{
	union {float f; uint32_t i;} ux = {x}, uy = {y};
	int ex = ux.i>>23 & 0xff;
	int ey = uy.i>>23 & 0xff;
	uint32_t sx = ux.i & 0x80000000;
	uint32_t i;
	uint32_t uxi = ux.i;

	if (uy.i<<1 == 0 || isnan(y) || ex == 0xff)
		return (x*y)/(x*y);
	if (uxi<<1 <= uy.i<<1) {
		if (uxi<<1 == uy.i<<1)
			return 0*x;
		return x;
	}

	/* normalize x and y */
	if (!ex) {
		for (i = uxi<<9; i>>31 == 0; ex--, i <<= 1);
		uxi <<= -ex + 1;
	} else {
		uxi &= -1U >> 9;
		uxi |= 1U << 23;
	}
	if (!ey) {
		for (i = uy.i<<9; i>>31 == 0; ey--, i <<= 1);
		uy.i <<= -ey + 1;
	} else {
		uy.i &= -1U >> 9;
		uy.i |= 1U << 23;
	}

	/* x mod y */
	for (; ex > ey; ex--) {
		i = uxi - uy.i;
		if (i >> 31 == 0) {
			if (i == 0)
				return 0*x;
			uxi = i;
		}
		uxi <<= 1;
	}
	i = uxi - uy.i;
	if (i >> 31 == 0) {
		if (i == 0)
			return 0*x;
		uxi = i;
	}
	for (; uxi>>23 == 0; uxi <<= 1, ex--);

	/* scale result up */
	if (ex > 0) {
		uxi -= 1U << 23;
		uxi |= (uint32_t)ex << 23;
	} else {
		uxi >>= -ex + 1;
	}
	uxi |= sx;
	ux.i = uxi;
	return ux.f;
}

// src/math/floorf.c
float floorf(float x)
{
	union {float f; uint32_t i;} u = {x};
	int e = (int)(u.i >> 23 & 0xff) - 0x7f;
	uint32_t m;

	if (e >= 23)
		return x;
	if (e >= 0) {
		m = 0x007fffff >> e;
		if ((u.i & m) == 0)
			return x;
		FORCE_EVAL(x + 0x1p120f);
		if (u.i >> 31)
			u.i += m;
		u.i &= ~m;
	} else {
		FORCE_EVAL(x + 0x1p120f);
		if (u.i >> 31 == 0)
			u.i = 0;
		else if (u.i << 1)
			u.f = -1.0;
	}
	return u.f;
}

// src/math/fabsf.c
float fabsf(float x)
{
	union {float f; uint32_t i;} u = {x};
	u.i &= 0x7fffffff;
	return u.f;
}

// https://www.cemetech.net/forum/viewtopic.php?t=6114&postdays=0&postorder=asc&start=160

#define PIOVERTWO 1.570796327f
#define PI 3.141592654f
#define PISQUARED 9.869604401f
#define EXTRA_PRECISION

float sinf(float x)
{
   while (x > PI) x-=(2.f*PI);
   while (x < -PI) x+=(2.f*PI);
    const float B = 4/PI;
    const float C = -4/(PISQUARED);

    float y = B * x + C * x * fabsf(x);

    #ifdef EXTRA_PRECISION
    //  const float Q = 0.7775;
        const float P = 0.224008178776;

        y = P * (y * fabsf(y) - y) + y;   // Q * y + P * y * abs(y)
    #endif
   
   return y;
}
