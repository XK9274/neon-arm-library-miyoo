#include <stdint.h>
#include <arm_neon.h>
#include "neon.h"

//
//	solid-color additive fill, 32bpp (ARGB8888) only. SDL_BLENDMODE_ADD
//	compositing: dstRGB = clamp(srcRGB*srcA + dstRGB, 0, 255), dstA
//	unchanged, each src term rounded via (x+127)/255.
//	args/	dst   :	dst offset		address of top left corner
//		color :	add color		packed 32bpp ARGB8888 pixel
//		w     :	width			pixels
//		h     :	height			pixels
//		dp    :	dst pitch (stride)	bytes	if 0, (w * 4) is used
//
//	add_solid_n32 falls back to add_solid_c32 when dst or dp aren't
//	4-byte aligned.
//

static inline uint32_t mult255(uint32_t a, uint32_t b) {
	uint32_t t = a * b + 128;
	return (t + (t >> 8)) >> 8;
}

static inline uint32_t sat_add8(uint32_t a, uint32_t b) {
	uint32_t s = a + b;
	return s > 255 ? 255 : s;
}

void add_solid_c32(void* __restrict dst, uint32_t color, uint32_t w, uint32_t h, uint32_t dp) {
	if (!w||!h) return;
	uint32_t swl = w*sizeof(uint32_t);
	if (!dp) { dp = swl; }
	uint32_t srcA = (color>>24)&0xFF;
	uint32_t srcR = (color>>16)&0xFF;
	uint32_t srcG = (color>>8)&0xFF;
	uint32_t srcB = color&0xFF;
	uint32_t addR = mult255(srcR, srcA);
	uint32_t addG = mult255(srcG, srcA);
	uint32_t addB = mult255(srcB, srcA);
	for (; h>0; h--, dst=(uint8_t*)dst+dp) {
		uint32_t *row = (uint32_t*)dst;
		for (uint32_t x = 0; x < w; x++) {
			uint32_t d = row[x];
			uint32_t dA = (d>>24)&0xFF, dR = (d>>16)&0xFF, dG = (d>>8)&0xFF, dB = d&0xFF;
			uint32_t outR = sat_add8(dR, addR);
			uint32_t outG = sat_add8(dG, addG);
			uint32_t outB = sat_add8(dB, addB);
			row[x] = (dA<<24)|(outR<<16)|(outG<<8)|outB;
		}
	}
}

void add_solid_n32(void* __restrict dst, uint32_t color, uint32_t w, uint32_t h, uint32_t dp) {
	if (!w||!h) return;
	uint32_t swl = w*sizeof(uint32_t);
	if (!dp) { dp = swl; }
	if ( ((uintptr_t)dst&3)||(dp&3) ) { add_solid_c32(dst,color,w,h,dp); return; }

	uint32_t srcA = (color>>24)&0xFF;
	uint32_t srcR = (color>>16)&0xFF;
	uint32_t srcG = (color>>8)&0xFF;
	uint32_t srcB = color&0xFF;
	uint8_t addR = (uint8_t)mult255(srcR, srcA);
	uint8_t addG = (uint8_t)mult255(srcG, srcA);
	uint8_t addB = (uint8_t)mult255(srcB, srcA);

	uint8x8_t vAddR = vdup_n_u8(addR);
	uint8x8_t vAddG = vdup_n_u8(addG);
	uint8x8_t vAddB = vdup_n_u8(addB);
	uint8x8_t vAddA = vdup_n_u8(0);

	uint32_t wn = w & ~(uint32_t)7;

	for (; h>0; h--, dst=(uint8_t*)dst+dp) {
		uint8_t *row = (uint8_t*)dst;
		uint32_t x = 0;
		for (; x < wn; x += 8) {
			uint8x8x4_t d = vld4_u8(row + x*4);

			d.val[0] = vqadd_u8(d.val[0], vAddB);
			d.val[1] = vqadd_u8(d.val[1], vAddG);
			d.val[2] = vqadd_u8(d.val[2], vAddR);
			d.val[3] = vqadd_u8(d.val[3], vAddA);

			vst4_u8(row + x*4, d);
		}
		for (; x < w; x++) {
			uint32_t *px = (uint32_t*)(row + x*4);
			uint32_t d = *px;
			uint32_t dA = (d>>24)&0xFF, dR = (d>>16)&0xFF, dG = (d>>8)&0xFF, dB = d&0xFF;
			uint32_t outR = sat_add8(dR, addR);
			uint32_t outG = sat_add8(dG, addG);
			uint32_t outB = sat_add8(dB, addB);
			*px = (dA<<24)|(outR<<16)|(outG<<8)|outB;
		}
	}
}
