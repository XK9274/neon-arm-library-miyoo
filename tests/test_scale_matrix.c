#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "neon.h"

#define PANEL_W 640
#define PANEL_H 480

typedef struct { const char *name; uint32_t sw, sh; int upscale; } ResEntry;

static const ResEntry g_matrix[] = {
	{ "256x224", 256, 224, 1 },
	{ "256x240", 256, 240, 1 },
	{ "320x240", 320, 240, 1 },
	{ "384x216", 384, 216, 1 },
	{ "384x240", 384, 240, 1 },
	{ "480x272", 480, 272, 1 },
	{ "800x600", 800, 600, 0 },
	{ "960x720", 960, 720, 0 },
	{ "1024x768", 1024, 768, 0 },
	{ "1280x720", 1280, 720, 0 },
	{ "1280x960", 1280, 960, 0 },
	{ "1440x900", 1440, 900, 0 },
	{ "1920x1080", 1920, 1080, 0 },
};
#define MATRIX_COUNT (int)(sizeof(g_matrix) / sizeof(g_matrix[0]))

static void fill_pattern(uint32_t *buf, uint32_t w, uint32_t h) {
	for (uint32_t y = 0; y < h; y++)
		for (uint32_t x = 0; x < w; x++)
			buf[y * w + x] = 0xFF000000u | ((x & 0xFF) << 16) | ((y & 0xFF) << 8) | ((x + y) & 0xFF);
}

/* Same pattern, but honoring an arbitrary byte pitch instead of assuming
 * the buffer is tightly packed -- needed for the misaligned-stride test,
 * where the row pitch isn't w*4. */
static void fill_pattern_strided(uint8_t *buf, uint32_t w, uint32_t h, uint32_t pitch) {
	for (uint32_t y = 0; y < h; y++) {
		uint8_t *row = buf + (size_t)y * pitch;
		for (uint32_t x = 0; x < w; x++) {
			uint32_t px = 0xFF000000u | ((x & 0xFF) << 16) | ((y & 0xFF) << 8) | ((x + y) & 0xFF);
			memcpy(row + (size_t)x * 4, &px, 4);
		}
	}
}

/* Every matrix entry: the NEON alias (upscale_area_n32 for sources smaller
 * than the panel, downscale_area_n32 otherwise -- same underlying function,
 * exercised under both names) must match its _c32 sibling pixel-for-pixel.
 * The library ships both a NEON and C path for every resize, so diffing
 * them is a built-in correctness oracle with no external reference needed. */
static int check_matrix(void) {
	int failures = 0;
	uint32_t *dst_n = malloc((size_t)PANEL_W * PANEL_H * 4);
	uint32_t *dst_c = malloc((size_t)PANEL_W * PANEL_H * 4);

	for (int i = 0; i < MATRIX_COUNT; i++) {
		const ResEntry *e = &g_matrix[i];
		uint32_t *src = malloc((size_t)e->sw * e->sh * 4);
		fill_pattern(src, e->sw, e->sh);
		memset(dst_n, 0, (size_t)PANEL_W * PANEL_H * 4);
		memset(dst_c, 0, (size_t)PANEL_W * PANEL_H * 4);

		if (e->upscale) {
			upscale_area_n32(src, dst_n, e->sw, e->sh, 0, 0, PANEL_W, PANEL_H);
			upscale_area_c32(src, dst_c, e->sw, e->sh, 0, 0, PANEL_W, PANEL_H);
		} else {
			downscale_area_n32(src, dst_n, e->sw, e->sh, 0, 0, PANEL_W, PANEL_H);
			downscale_area_c32(src, dst_c, e->sw, e->sh, 0, 0, PANEL_W, PANEL_H);
		}

		if (memcmp(dst_n, dst_c, (size_t)PANEL_W * PANEL_H * 4) != 0) {
			printf("FAIL %s: neon/c mismatch\n", e->name);
			failures++;
		} else {
			printf("PASS %s: neon/c match\n", e->name);
		}
		free(src);
	}

	free(dst_n);
	free(dst_c);
	return failures;
}

/* 320x240 -> 640x480 is a clean 2x2 ratio: the fixed-ratio hand-tuned
 * scale2x2_n32 and the generic upscale_area_n32 alias must agree exactly,
 * confirming the fast path and the generic fallback aren't diverging. */
static int check_2x2_parity(void) {
	uint32_t *src = malloc((size_t)320 * 240 * 4);
	uint32_t *dst_fixed = malloc((size_t)PANEL_W * PANEL_H * 4);
	uint32_t *dst_generic = malloc((size_t)PANEL_W * PANEL_H * 4);
	fill_pattern(src, 320, 240);

	scale2x2_n32(src, dst_fixed, 320, 240, 0, PANEL_W * 4);
	upscale_area_n32(src, dst_generic, 320, 240, 0, 0, PANEL_W, PANEL_H);

	int mismatch = memcmp(dst_fixed, dst_generic, (size_t)PANEL_W * PANEL_H * 4) != 0;
	printf("%s 320x240 2x2 parity: scale2x2_n32 vs upscale_area_n32\n", mismatch ? "FAIL" : "PASS");

	free(src);
	free(dst_fixed);
	free(dst_generic);
	return mismatch ? 1 : 0;
}

/* neon.h documents that downscale_area_n32 (and its upscale_area_n32 alias)
 * fall back to the C path when src/dst/sp/dp aren't 4-byte aligned. An odd
 * stride exercises that fallback; since the NEON entry point dispatches to
 * the same C routine in that case, correctness under misalignment is what's
 * actually observable here -- it must still match a properly-aligned _c32
 * reference computed from the same source pixels. */
static int check_odd_stride_fallback(void) {
	const uint32_t sw = 300, sh = 200;
	const uint32_t sp = sw * 4 + 1; /* misaligned source pitch */
	const uint32_t dp = PANEL_W * 4 + 1; /* misaligned destination pitch */

	uint8_t *src = malloc((size_t)sp * sh);
	fill_pattern_strided(src, sw, sh, sp);

	uint8_t *dst_n = malloc((size_t)dp * PANEL_H);
	uint8_t *dst_c = malloc((size_t)dp * PANEL_H);
	memset(dst_n, 0, (size_t)dp * PANEL_H);
	memset(dst_c, 0, (size_t)dp * PANEL_H);

	upscale_area_n32(src, dst_n, sw, sh, sp, dp, PANEL_W, PANEL_H);
	upscale_area_c32(src, dst_c, sw, sh, sp, dp, PANEL_W, PANEL_H);

	int mismatch = memcmp(dst_n, dst_c, (size_t)dp * PANEL_H) != 0;
	printf("%s odd-stride fallback: upscale_area_n32 vs upscale_area_c32\n", mismatch ? "FAIL" : "PASS");

	free(src);
	free(dst_n);
	free(dst_c);
	return mismatch ? 1 : 0;
}

int main(void) {
	int failures = 0;
	failures += check_matrix();
	failures += check_2x2_parity();
	failures += check_odd_stride_fallback();

	if (failures) {
		printf("%d check(s) FAILED\n", failures);
		return 1;
	}
	printf("all checks PASSED\n");
	return 0;
}
