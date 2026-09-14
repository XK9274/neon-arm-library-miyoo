#include <stdint.h>
#include "neon.h"

//
//	generic runtime-parametrized area downscale for 32bpp (ARGB8888)
//	args/	src :	src offset		address of top left corner
//		dst :	dst offset		address of top left corner
//		sw  :	src width		pixels
//		sh  :	src height		pixels
//		sp  :	src pitch (stride)	bytes	if 0, (sw * 4) is used
//		dp  :	dst pitch (stride)	bytes	if 0, (dw * 4) is used
//		dw  :	dst width		pixels
//		dh  :	dst height		pixels
//
//	v1: nearest-neighbor sampling, not a true area-weighted box filter.
//	dw/dh are runtime arguments rather than baked into the function name,
//	so one kernel covers any downscale ratio.
//
//	Each source index is computed directly from x/y (sx = x*sw/dw).
//

void downscale_area_c32(void* __restrict src, void* __restrict dst,
                         uint32_t sw, uint32_t sh, uint32_t sp, uint32_t dp,
                         uint32_t dw, uint32_t dh) {
	if (!sw||!sh||!dw||!dh) return;
	uint32_t swl = sw*sizeof(uint32_t);
	if (!sp) { sp = swl; } if (!dp) { dp = dw*sizeof(uint32_t); }

	for (uint32_t y = 0; y < dh; y++) {
		uint32_t sy = (uint32_t)(((uint64_t)y * sh) / dh);
		if (sy >= sh) sy = sh - 1;
		const uint8_t *srow = (const uint8_t*)src + (size_t)sy * sp;
		uint32_t *drow = (uint32_t*)((uint8_t*)dst + (size_t)y * dp);
		for (uint32_t x = 0; x < dw; x++) {
			uint32_t sx = (uint32_t)(((uint64_t)x * sw) / dw);
			if (sx >= sw) sx = sw - 1;
			drow[x] = ((const uint32_t*)srow)[sx];
		}
	}
}

void downscale_area_n32(void* __restrict src, void* __restrict dst,
                         uint32_t sw, uint32_t sh, uint32_t sp, uint32_t dp,
                         uint32_t dw, uint32_t dh) {
	if (!sw||!sh||!dw||!dh) return;
	uint32_t swl = sw*sizeof(uint32_t);
	if (!sp) { sp = swl; } if (!dp) { dp = dw*sizeof(uint32_t); }
	if ( ((uintptr_t)src&3)||((uintptr_t)dst&3)||(sp&3)||(dp&3)||(dw > DOWNSCALE_AREA_MAX_DW) ) {
		downscale_area_c32(src, dst, sw, sh, sp, dp, dw, dh);
		return;
	}

	uint32_t dw4 = dw & ~3u;

	/* Per-column source byte offset, same every row -- computed once. */
	uint32_t xoff[DOWNSCALE_AREA_MAX_DW];
	for (uint32_t x = 0; x < dw; x++) {
		uint32_t sx = (uint32_t)(((uint64_t)x * sw) / dw);
		if (sx >= sw) sx = sw - 1;
		xoff[x] = sx * 4;
	}

	for (uint32_t y = 0; y < dh; y++) {
		uint32_t sy = (uint32_t)(((uint64_t)y * sh) / dh);
		if (sy >= sh) sy = sh - 1;
		const uint8_t *srow = (const uint8_t*)src + (size_t)sy * sp;
		uint8_t *drow = (uint8_t*)dst + (size_t)y * dp;
		uint32_t x = 0;
		/* No hardware gather on ARMv7 NEON: gather 4 nearest-neighbor
		 * source pixels via single-lane loads, store as one quad. */
		for (; x < dw4; x += 4) {
			const uint8_t *p0 = srow + xoff[x + 0];
			const uint8_t *p1 = srow + xoff[x + 1];
			const uint8_t *p2 = srow + xoff[x + 2];
			const uint8_t *p3 = srow + xoff[x + 3];
			uint8_t *d = drow + x * 4;
			asm volatile (
			"	vld1.32	{d0[0]}, [%0]	;"
			"	vld1.32	{d0[1]}, [%1]	;"
			"	vld1.32	{d1[0]}, [%2]	;"
			"	vld1.32	{d1[1]}, [%3]	;"
			"	vst1.32	{d0, d1}, [%4]	"
			:
			: "r"(p0), "r"(p1), "r"(p2), "r"(p3), "r"(d)
			: "d0", "d1", "memory"
			);
		}
		for (; x < dw; x++) {
			*(uint32_t*)(drow + x*4) = *(const uint32_t*)(srow + xoff[x]);
		}
	}
}
