#include <stdint.h>
#include <arm_neon.h>
#include "neon.h"

//
//	solid-color alpha blend fill, 32bpp (ARGB8888) only. SDL_BLENDMODE_BLEND
//	"over" compositing: dstRGB = srcRGB*srcA + dstRGB*(1-srcA),
//	dstA = srcA + dstA*(1-srcA), each term rounded via (x+127)/255.
//	args/	dst   :	dst offset		address of top left corner
//		color :	blend color		packed 32bpp ARGB8888 pixel
//		w     :	width			pixels
//		h     :	height			pixels
//		dp    :	dst pitch (stride)	bytes	if 0, (w * 4) is used
//
//	blend_solid_n32 falls back to blend_solid_c32 when dst or dp aren't
//	4-byte aligned.
//

static inline uint32_t mult255(uint32_t a, uint32_t b) {
	uint32_t t = a * b + 128;
	return (t + (t >> 8)) >> 8;
}

void blend_solid_c32(void* __restrict dst, uint32_t color, uint32_t w, uint32_t h, uint32_t dp) {
	if (!w||!h) return;
	uint32_t swl = w*sizeof(uint32_t);
	if (!dp) { dp = swl; }
	uint32_t srcA = (color>>24)&0xFF;
	uint32_t srcR = (color>>16)&0xFF;
	uint32_t srcG = (color>>8)&0xFF;
	uint32_t srcB = color&0xFF;
	uint32_t invA = 255 - srcA;
	uint32_t blendR = mult255(srcR, srcA);
	uint32_t blendG = mult255(srcG, srcA);
	uint32_t blendB = mult255(srcB, srcA);
	for (; h>0; h--, dst=(uint8_t*)dst+dp) {
		uint32_t *row = (uint32_t*)dst;
		for (uint32_t x = 0; x < w; x++) {
			uint32_t d = row[x];
			uint32_t dA = (d>>24)&0xFF, dR = (d>>16)&0xFF, dG = (d>>8)&0xFF, dB = d&0xFF;
			uint32_t outA = srcA + mult255(dA, invA);
			uint32_t outR = blendR + mult255(dR, invA);
			uint32_t outG = blendG + mult255(dG, invA);
			uint32_t outB = blendB + mult255(dB, invA);
			row[x] = (outA<<24)|(outR<<16)|(outG<<8)|outB;
		}
	}
}

void blend_solid_n32(void* __restrict dst, uint32_t color, uint32_t w, uint32_t h, uint32_t dp) {
	if (!w||!h) return;
	uint32_t swl = w*sizeof(uint32_t);
	if (!dp) { dp = swl; }
	if ( ((uintptr_t)dst&3)||(dp&3) ) { blend_solid_c32(dst,color,w,h,dp); return; }

	uint32_t srcA = (color>>24)&0xFF;
	uint32_t srcR = (color>>16)&0xFF;
	uint32_t srcG = (color>>8)&0xFF;
	uint32_t srcB = color&0xFF;
	uint32_t invA = 255 - srcA;
	uint8_t blendR = (uint8_t)mult255(srcR, srcA);
	uint8_t blendG = (uint8_t)mult255(srcG, srcA);
	uint8_t blendB = (uint8_t)mult255(srcB, srcA);
	uint8_t blendA = (uint8_t)srcA;

	uint8x8_t vInvA = vdup_n_u8((uint8_t)invA);
	uint8x8_t vBlendR = vdup_n_u8(blendR);
	uint8x8_t vBlendG = vdup_n_u8(blendG);
	uint8x8_t vBlendB = vdup_n_u8(blendB);
	uint8x8_t vBlendA = vdup_n_u8(blendA);
	uint16x8_t v128 = vdupq_n_u16(128);

	uint32_t wn = w & ~(uint32_t)7;

	for (; h>0; h--, dst=(uint8_t*)dst+dp) {
		uint8_t *row = (uint8_t*)dst;
		uint32_t x = 0;
		for (; x < wn; x += 8) {
			uint8x8x4_t d = vld4_u8(row + x*4);

			uint16x8_t pB = vmull_u8(d.val[0], vInvA);
			uint16x8_t pG = vmull_u8(d.val[1], vInvA);
			uint16x8_t pR = vmull_u8(d.val[2], vInvA);
			uint16x8_t pA = vmull_u8(d.val[3], vInvA);

			uint16x8_t tB = vaddq_u16(pB, v128);
			uint16x8_t tG = vaddq_u16(pG, v128);
			uint16x8_t tR = vaddq_u16(pR, v128);
			uint16x8_t tA = vaddq_u16(pA, v128);

			uint8x8_t mB = vshrn_n_u16(vaddq_u16(tB, vshrq_n_u16(tB,8)), 8);
			uint8x8_t mG = vshrn_n_u16(vaddq_u16(tG, vshrq_n_u16(tG,8)), 8);
			uint8x8_t mR = vshrn_n_u16(vaddq_u16(tR, vshrq_n_u16(tR,8)), 8);
			uint8x8_t mA = vshrn_n_u16(vaddq_u16(tA, vshrq_n_u16(tA,8)), 8);

			d.val[0] = vadd_u8(mB, vBlendB);
			d.val[1] = vadd_u8(mG, vBlendG);
			d.val[2] = vadd_u8(mR, vBlendR);
			d.val[3] = vadd_u8(mA, vBlendA);

			vst4_u8(row + x*4, d);
		}
		for (; x < w; x++) {
			uint32_t *px = (uint32_t*)(row + x*4);
			uint32_t d = *px;
			uint32_t dA = (d>>24)&0xFF, dR = (d>>16)&0xFF, dG = (d>>8)&0xFF, dB = d&0xFF;
			uint32_t outA = srcA + mult255(dA, invA);
			uint32_t outR = blendR + mult255(dR, invA);
			uint32_t outG = blendG + mult255(dG, invA);
			uint32_t outB = blendB + mult255(dB, invA);
			*px = (outA<<24)|(outR<<16)|(outG<<8)|outB;
		}
	}
}
