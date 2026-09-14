#ifndef __SCALER_NEON_H__
#define __SCALER_NEON_H__
#include <stdint.h>
#include <stddef.h>

//
//	arm NEON / C integer scalers for ARMv7 devices
//	args/	src :	src offset		address of top left corner
//		dst :	dst offset		address	of top left corner
//		sw  :	src width		pixels
//		sh  :	src height		pixels
//		sp  :	src pitch (stride)	bytes	if 0, (src width * [2|4]) is used
//		dp  :	dst pitch (stride)	bytes	if 0, (src width * [2|4] * multiplier) is used
//
//	** NOTE **
//	since 32bit aligned addresses need to be processed for NEON scalers,
//	x-offset and stride pixels must be even# in the case of 16bpp,
//	if odd#, then handled by the C scaler
//

//	NEON scalers
void scale1x1_n16(void* __restrict src, void* __restrict dst, uint32_t sw, uint32_t sh, uint32_t sp, uint32_t dp);
void scale1x1_n32(void* __restrict src, void* __restrict dst, uint32_t sw, uint32_t sh, uint32_t sp, uint32_t dp);
void scale1x2_n16(void* __restrict src, void* __restrict dst, uint32_t sw, uint32_t sh, uint32_t sp, uint32_t dp);
void scale1x2_n32(void* __restrict src, void* __restrict dst, uint32_t sw, uint32_t sh, uint32_t sp, uint32_t dp);
void scale1x3_n16(void* __restrict src, void* __restrict dst, uint32_t sw, uint32_t sh, uint32_t sp, uint32_t dp);
void scale1x3_n32(void* __restrict src, void* __restrict dst, uint32_t sw, uint32_t sh, uint32_t sp, uint32_t dp);
void scale1x4_n16(void* __restrict src, void* __restrict dst, uint32_t sw, uint32_t sh, uint32_t sp, uint32_t dp);
void scale1x4_n32(void* __restrict src, void* __restrict dst, uint32_t sw, uint32_t sh, uint32_t sp, uint32_t dp);
void scale2x1_n16(void* __restrict src, void* __restrict dst, uint32_t sw, uint32_t sh, uint32_t sp, uint32_t dp);
void scale2x1_n32(void* __restrict src, void* __restrict dst, uint32_t sw, uint32_t sh, uint32_t sp, uint32_t dp);
void scale2x2_n16(void* __restrict src, void* __restrict dst, uint32_t sw, uint32_t sh, uint32_t sp, uint32_t dp);
void scale2x2_n32(void* __restrict src, void* __restrict dst, uint32_t sw, uint32_t sh, uint32_t sp, uint32_t dp);
void scale2x3_n16(void* __restrict src, void* __restrict dst, uint32_t sw, uint32_t sh, uint32_t sp, uint32_t dp);
void scale2x3_n32(void* __restrict src, void* __restrict dst, uint32_t sw, uint32_t sh, uint32_t sp, uint32_t dp);
void scale2x4_n16(void* __restrict src, void* __restrict dst, uint32_t sw, uint32_t sh, uint32_t sp, uint32_t dp);
void scale2x4_n32(void* __restrict src, void* __restrict dst, uint32_t sw, uint32_t sh, uint32_t sp, uint32_t dp);
void scale4x1_n16(void* __restrict src, void* __restrict dst, uint32_t sw, uint32_t sh, uint32_t sp, uint32_t dp);
void scale4x1_n32(void* __restrict src, void* __restrict dst, uint32_t sw, uint32_t sh, uint32_t sp, uint32_t dp);
void scale4x2_n16(void* __restrict src, void* __restrict dst, uint32_t sw, uint32_t sh, uint32_t sp, uint32_t dp);
void scale4x2_n32(void* __restrict src, void* __restrict dst, uint32_t sw, uint32_t sh, uint32_t sp, uint32_t dp);
void scale4x3_n16(void* __restrict src, void* __restrict dst, uint32_t sw, uint32_t sh, uint32_t sp, uint32_t dp);
void scale4x3_n32(void* __restrict src, void* __restrict dst, uint32_t sw, uint32_t sh, uint32_t sp, uint32_t dp);
void scale4x4_n16(void* __restrict src, void* __restrict dst, uint32_t sw, uint32_t sh, uint32_t sp, uint32_t dp);
void scale4x4_n32(void* __restrict src, void* __restrict dst, uint32_t sw, uint32_t sh, uint32_t sp, uint32_t dp);

// NEON optimized memcpy
void* neon_memcpy(void* dest, const void* src, size_t n);

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
void blend_solid_n32(void* __restrict dst, uint32_t color, uint32_t w, uint32_t h, uint32_t dp);
void blend_solid_c32(void* __restrict dst, uint32_t color, uint32_t w, uint32_t h, uint32_t dp);

//
//	generic runtime-parametrized area resize, 32bpp (ARGB8888) only.
//	dw/dh (destination width/height) are runtime arguments -- one kernel
//	covers any resize ratio, direction-agnostic (sx = x*sw/dw, sy = y*sh/dh
//	work whether dw/dh are smaller or larger than sw/sh). v1 is
//	nearest-neighbor sampling, not a true area-weighted box filter.
//
//	downscale_area_n32 silently falls back to downscale_area_c32 (~20x
//	slower) when src/dst/sp/dp aren't 4-byte aligned or dw exceeds
//	DOWNSCALE_AREA_MAX_DW.
//
//	upscale_area_n32/_c32 are aliases of downscale_area_n32/_c32 for
//	call sites that know they're upscaling -- same function, same limits,
//	kept as separate names for readability only.
//
#define DOWNSCALE_AREA_MAX_DW 1024
void downscale_area_n32(void* __restrict src, void* __restrict dst, uint32_t sw, uint32_t sh, uint32_t sp, uint32_t dp, uint32_t dw, uint32_t dh);
void downscale_area_c32(void* __restrict src, void* __restrict dst, uint32_t sw, uint32_t sh, uint32_t sp, uint32_t dp, uint32_t dw, uint32_t dh);
void upscale_area_n32(void* __restrict src, void* __restrict dst, uint32_t sw, uint32_t sh, uint32_t sp, uint32_t dp, uint32_t dw, uint32_t dh);
void upscale_area_c32(void* __restrict src, void* __restrict dst, uint32_t sw, uint32_t sh, uint32_t sp, uint32_t dp, uint32_t dw, uint32_t dh);

//	C scalers
void scale1x1_c16(void* __restrict src, void* __restrict dst, uint32_t sw, uint32_t sh, uint32_t sp, uint32_t dp);
void scale1x1_c32(void* __restrict src, void* __restrict dst, uint32_t sw, uint32_t sh, uint32_t sp, uint32_t dp);
void scale1x2_c16(void* __restrict src, void* __restrict dst, uint32_t sw, uint32_t sh, uint32_t sp, uint32_t dp);
void scale1x2_c32(void* __restrict src, void* __restrict dst, uint32_t sw, uint32_t sh, uint32_t sp, uint32_t dp);
void scale1x3_c16(void* __restrict src, void* __restrict dst, uint32_t sw, uint32_t sh, uint32_t sp, uint32_t dp);
void scale1x3_c32(void* __restrict src, void* __restrict dst, uint32_t sw, uint32_t sh, uint32_t sp, uint32_t dp);
void scale1x4_c16(void* __restrict src, void* __restrict dst, uint32_t sw, uint32_t sh, uint32_t sp, uint32_t dp);
void scale1x4_c32(void* __restrict src, void* __restrict dst, uint32_t sw, uint32_t sh, uint32_t sp, uint32_t dp);
void scale2x1_c16(void* __restrict src, void* __restrict dst, uint32_t sw, uint32_t sh, uint32_t sp, uint32_t dp);
void scale2x1_c32(void* __restrict src, void* __restrict dst, uint32_t sw, uint32_t sh, uint32_t sp, uint32_t dp);
void scale2x2_c16(void* __restrict src, void* __restrict dst, uint32_t sw, uint32_t sh, uint32_t sp, uint32_t dp);
void scale2x2_c32(void* __restrict src, void* __restrict dst, uint32_t sw, uint32_t sh, uint32_t sp, uint32_t dp);
void scale2x3_c16(void* __restrict src, void* __restrict dst, uint32_t sw, uint32_t sh, uint32_t sp, uint32_t dp);
void scale2x3_c32(void* __restrict src, void* __restrict dst, uint32_t sw, uint32_t sh, uint32_t sp, uint32_t dp);
void scale2x4_c16(void* __restrict src, void* __restrict dst, uint32_t sw, uint32_t sh, uint32_t sp, uint32_t dp);
void scale2x4_c32(void* __restrict src, void* __restrict dst, uint32_t sw, uint32_t sh, uint32_t sp, uint32_t dp);
void scale4x1_c16(void* __restrict src, void* __restrict dst, uint32_t sw, uint32_t sh, uint32_t sp, uint32_t dp);
void scale4x1_c32(void* __restrict src, void* __restrict dst, uint32_t sw, uint32_t sh, uint32_t sp, uint32_t dp);
void scale4x2_c16(void* __restrict src, void* __restrict dst, uint32_t sw, uint32_t sh, uint32_t sp, uint32_t dp);
void scale4x2_c32(void* __restrict src, void* __restrict dst, uint32_t sw, uint32_t sh, uint32_t sp, uint32_t dp);
void scale4x3_c16(void* __restrict src, void* __restrict dst, uint32_t sw, uint32_t sh, uint32_t sp, uint32_t dp);
void scale4x3_c32(void* __restrict src, void* __restrict dst, uint32_t sw, uint32_t sh, uint32_t sp, uint32_t dp);
void scale4x4_c16(void* __restrict src, void* __restrict dst, uint32_t sw, uint32_t sh, uint32_t sp, uint32_t dp);
void scale4x4_c32(void* __restrict src, void* __restrict dst, uint32_t sw, uint32_t sh, uint32_t sp, uint32_t dp);

#endif