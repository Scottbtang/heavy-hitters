#include <stdio.h>
#include <inttypes.h>
#include <assert.h>
#include <math.h>

#include "xutil.h"
#include "hash.h"

extern short M;

static uint32_t h31_internal(uint32_t x, uint32_t a, uint32_t b) {
	uint64_t result;

	result   = (a * x) + b;
	result   = ( (result >> 31) + result ) & MOD_P;

	return (uint32_t) result;
}

uint32_t h31(uint32_t x, uint32_t w, uint32_t a, uint32_t b) {
	uint32_t res = h31_internal(x, a, b);
	return res % w;
}

uint32_t h31p2(uint32_t x, uint32_t w, uint32_t a, uint32_t b) {
	uint32_t res = h31_internal(x, a, b);

	// Amount of bins must be power of 2
	assert( w && !(w & (w - 1)) );

	return res & (w - 1);
}

uint32_t ms(uint32_t x, uint32_t w, uint32_t a, uint32_t b) {
	(void) w;

	// a < 2^w
	assert( a <= UINT32_MAX ) ;

	// b < 2^w-M
	assert( b <= pow(2, sizeof(uint32_t)*BYTE-M) ) ;

	// Amount of bins must be power of 2
	assert( w && !(w & (w - 1)) );

	return (uint32_t) (a*x+b) >> (sizeof(uint32_t)*BYTE-M);
}

uint32_t h31_agen () {
	return 1 + (xuni_rand() * (MOD_P - 2));
}

uint32_t h31_bgen () {
	return xuni_rand() * (MOD_P - 1);
}

uint32_t ms_agen () {
	return xuni_rand() * UINT32_MAX;
}

uint32_t ms_bgen () {
	return xuni_rand() * (1 << (sizeof(uint32_t)*BYTE-M));
}

hash_t hash31 = {
	.hash = (hash) h31,
	.agen = (agen) h31_agen,
	.bgen = (bgen) h31_bgen,
	.c    = 1,
};

hash_t hash31p2 = {
	.hash = (hash) h31p2,
	.agen = (agen) h31_agen,
	.bgen = (bgen) h31_bgen,
	.c    = 1,
};

hash_t multiplyShift = {
	.hash = (hash) ms,
	.agen = (agen) ms_agen,
	.bgen = (bgen) ms_bgen,
	.c    = 1,
};