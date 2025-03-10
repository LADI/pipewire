/* Spa */
/* SPDX-FileCopyrightText: Copyright © 2018 Wim Taymans */
/* SPDX-License-Identifier: MIT */

#include "fmt-ops.h"

#include <immintrin.h>
// GCC: workaround for missing AVX intrinsic: "_mm256_setr_m128()"
//      (see https://stackoverflow.com/questions/32630458/setting-m256i-to-the-value-of-two-m128i-values)
#ifndef _mm256_setr_m128i
#  ifndef _mm256_set_m128i
#    define _mm256_set_m128i(v0, v1)  _mm256_insertf128_si256(_mm256_castsi128_si256(v1), (v0), 1)
#  endif
#  define _mm256_setr_m128i(v0, v1) _mm256_set_m128i((v1), (v0))
#endif

#define _MM_CLAMP_PS(r,min,max)				\
	_mm_min_ps(_mm_max_ps(r, min), max)

#define _MM256_CLAMP_PS(r,min,max)			\
	_mm256_min_ps(_mm256_max_ps(r, min), max)

#define _MM_CLAMP_SS(r,min,max)				\
	_mm_min_ss(_mm_max_ss(r, min), max)

#define _MM256_BSWAP_EPI16(x)						\
({									\
	_mm256_or_si256(						\
		_mm256_slli_epi16(x, 8),				\
		_mm256_srli_epi16(x, 8));				\
})

static void
conv_s16_to_f32d_1s_avx2(void *data, void * SPA_RESTRICT dst[], const void * SPA_RESTRICT src,
		uint32_t n_channels, uint32_t n_samples)
{
	const int16_t *s = src;
	float *d0 = dst[0];
	uint32_t n, unrolled;
	__m256i in = _mm256_setzero_si256();
	__m256 out, factor = _mm256_set1_ps(1.0f / S16_SCALE);

	if (SPA_LIKELY(SPA_IS_ALIGNED(d0, 32)))
		unrolled = n_samples & ~7;
	else
		unrolled = 0;

	for(n = 0; n < unrolled; n += 8) {
		in = _mm256_insert_epi16(in, s[0*n_channels],  1);
		in = _mm256_insert_epi16(in, s[1*n_channels],  3);
		in = _mm256_insert_epi16(in, s[2*n_channels],  5);
		in = _mm256_insert_epi16(in, s[3*n_channels],  7);
		in = _mm256_insert_epi16(in, s[4*n_channels],  9);
		in = _mm256_insert_epi16(in, s[5*n_channels], 11);
		in = _mm256_insert_epi16(in, s[6*n_channels], 13);
		in = _mm256_insert_epi16(in, s[7*n_channels], 15);

		in = _mm256_srai_epi32(in, 16);
		out = _mm256_cvtepi32_ps(in);
		out = _mm256_mul_ps(out, factor);
		_mm256_store_ps(&d0[n], out);
		s += 8*n_channels;
	}
	for(; n < n_samples; n++) {
		__m128 out, factor = _mm_set1_ps(1.0f / S16_SCALE);
		out = _mm_cvtsi32_ss(factor, s[0]);
		out = _mm_mul_ss(out, factor);
		_mm_store_ss(&d0[n], out);
		s += n_channels;
	}
}

void
conv_s16_to_f32d_avx2(struct convert *conv, void * SPA_RESTRICT dst[], const void * SPA_RESTRICT src[],
		uint32_t n_samples)
{
	const int16_t *s = src[0];
	uint32_t i = 0, n_channels = conv->n_channels;

	for(; i < n_channels; i++)
		conv_s16_to_f32d_1s_avx2(conv, &dst[i], &s[i], n_channels, n_samples);
}


static void
conv_s16s_to_f32d_1s_avx2(void *data, void * SPA_RESTRICT dst[], const void * SPA_RESTRICT src,
		uint32_t n_channels, uint32_t n_samples)
{
	const uint16_t *s = src;
	float *d0 = dst[0];
	uint32_t n, unrolled;
	__m256i in = _mm256_setzero_si256();
	__m256 out, factor = _mm256_set1_ps(1.0f / S16_SCALE);

	if (SPA_LIKELY(SPA_IS_ALIGNED(d0, 32)))
		unrolled = n_samples & ~7;
	else
		unrolled = 0;

	for(n = 0; n < unrolled; n += 8) {
		in = _mm256_insert_epi16(in, s[0*n_channels],  1);
		in = _mm256_insert_epi16(in, s[1*n_channels],  3);
		in = _mm256_insert_epi16(in, s[2*n_channels],  5);
		in = _mm256_insert_epi16(in, s[3*n_channels],  7);
		in = _mm256_insert_epi16(in, s[4*n_channels],  9);
		in = _mm256_insert_epi16(in, s[5*n_channels], 11);
		in = _mm256_insert_epi16(in, s[6*n_channels], 13);
		in = _mm256_insert_epi16(in, s[7*n_channels], 15);
		in = _MM256_BSWAP_EPI16(in);

		in = _mm256_srai_epi32(in, 16);
		out = _mm256_cvtepi32_ps(in);
		out = _mm256_mul_ps(out, factor);
		_mm256_store_ps(&d0[n], out);
		s += 8*n_channels;
	}
	for(; n < n_samples; n++) {
		__m128 out, factor = _mm_set1_ps(1.0f / S16_SCALE);
		out = _mm_cvtsi32_ss(factor, (int16_t)bswap_16(s[0]));
		out = _mm_mul_ss(out, factor);
		_mm_store_ss(&d0[n], out);
		s += n_channels;
	}
}

void
conv_s16s_to_f32d_avx2(struct convert *conv, void * SPA_RESTRICT dst[], const void * SPA_RESTRICT src[],
		uint32_t n_samples)
{
	const uint16_t *s = src[0];
	uint32_t i = 0, n_channels = conv->n_channels;

	for(; i < n_channels; i++)
		conv_s16s_to_f32d_1s_avx2(conv, &dst[i], &s[i], n_channels, n_samples);
}

void
conv_s16_to_f32d_2_avx2(struct convert *conv, void * SPA_RESTRICT dst[], const void * SPA_RESTRICT src[],
		uint32_t n_samples)
{
	const int16_t *s = src[0];
	float *d0 = dst[0], *d1 = dst[1];
	uint32_t n, unrolled;
	__m256i in[2], t[4];
	__m256 out[4], factor = _mm256_set1_ps(1.0f / S16_SCALE);

	if (SPA_IS_ALIGNED(s, 32) &&
	    SPA_IS_ALIGNED(d0, 32) &&
	    SPA_IS_ALIGNED(d1, 32))
		unrolled = n_samples & ~15;
	else
		unrolled = 0;

	for(n = 0; n < unrolled; n += 16) {
		in[0] = _mm256_load_si256((__m256i*)(s + 0));
		in[1] = _mm256_load_si256((__m256i*)(s + 16));

		t[0] = _mm256_slli_epi32(in[0], 16);
		t[0] = _mm256_srai_epi32(t[0], 16);
		out[0] = _mm256_cvtepi32_ps(t[0]);
		out[0] = _mm256_mul_ps(out[0], factor);

		t[1] = _mm256_srai_epi32(in[0], 16);
		out[1] = _mm256_cvtepi32_ps(t[1]);
		out[1] = _mm256_mul_ps(out[1], factor);

		t[2] = _mm256_slli_epi32(in[1], 16);
		t[2] = _mm256_srai_epi32(t[2], 16);
		out[2] = _mm256_cvtepi32_ps(t[2]);
		out[2] = _mm256_mul_ps(out[2], factor);

		t[3] = _mm256_srai_epi32(in[1], 16);
		out[3] = _mm256_cvtepi32_ps(t[3]);
		out[3] = _mm256_mul_ps(out[3], factor);

		_mm256_store_ps(&d0[n + 0], out[0]);
		_mm256_store_ps(&d1[n + 0], out[1]);
		_mm256_store_ps(&d0[n + 8], out[2]);
		_mm256_store_ps(&d1[n + 8], out[3]);

		s += 32;
	}
	for(; n < n_samples; n++) {
		__m128 out[4], factor = _mm_set1_ps(1.0f / S16_SCALE);
		out[0] = _mm_cvtsi32_ss(factor, s[0]);
		out[0] = _mm_mul_ss(out[0], factor);
		out[1] = _mm_cvtsi32_ss(factor, s[1]);
		out[1] = _mm_mul_ss(out[1], factor);
		_mm_store_ss(&d0[n], out[0]);
		_mm_store_ss(&d1[n], out[1]);
		s += 2;
	}
}

void
conv_s16s_to_f32d_2_avx2(struct convert *conv, void * SPA_RESTRICT dst[], const void * SPA_RESTRICT src[],
		uint32_t n_samples)
{
	const uint16_t *s = src[0];
	float *d0 = dst[0], *d1 = dst[1];
	uint32_t n, unrolled;
	__m256i in[2], t[4];
	__m256 out[4], factor = _mm256_set1_ps(1.0f / S16_SCALE);

	if (SPA_IS_ALIGNED(s, 32) &&
	    SPA_IS_ALIGNED(d0, 32) &&
	    SPA_IS_ALIGNED(d1, 32))
		unrolled = n_samples & ~15;
	else
		unrolled = 0;

	for(n = 0; n < unrolled; n += 16) {
		in[0] = _mm256_load_si256((__m256i*)(s + 0));
		in[1] = _mm256_load_si256((__m256i*)(s + 16));
		in[0] = _MM256_BSWAP_EPI16(in[0]);
		in[1] = _MM256_BSWAP_EPI16(in[1]);

		t[0] = _mm256_slli_epi32(in[0], 16);
		t[0] = _mm256_srai_epi32(t[0], 16);
		out[0] = _mm256_cvtepi32_ps(t[0]);
		out[0] = _mm256_mul_ps(out[0], factor);

		t[1] = _mm256_srai_epi32(in[0], 16);
		out[1] = _mm256_cvtepi32_ps(t[1]);
		out[1] = _mm256_mul_ps(out[1], factor);

		t[2] = _mm256_slli_epi32(in[1], 16);
		t[2] = _mm256_srai_epi32(t[2], 16);
		out[2] = _mm256_cvtepi32_ps(t[2]);
		out[2] = _mm256_mul_ps(out[2], factor);

		t[3] = _mm256_srai_epi32(in[1], 16);
		out[3] = _mm256_cvtepi32_ps(t[3]);
		out[3] = _mm256_mul_ps(out[3], factor);

		_mm256_store_ps(&d0[n + 0], out[0]);
		_mm256_store_ps(&d1[n + 0], out[1]);
		_mm256_store_ps(&d0[n + 8], out[2]);
		_mm256_store_ps(&d1[n + 8], out[3]);

		s += 32;
	}
	for(; n < n_samples; n++) {
		__m128 out[4], factor = _mm_set1_ps(1.0f / S16_SCALE);
		out[0] = _mm_cvtsi32_ss(factor, (int16_t)bswap_16(s[0]));
		out[0] = _mm_mul_ss(out[0], factor);
		out[1] = _mm_cvtsi32_ss(factor, (int16_t)bswap_16(s[1]));
		out[1] = _mm_mul_ss(out[1], factor);
		_mm_store_ss(&d0[n], out[0]);
		_mm_store_ss(&d1[n], out[1]);
		s += 2;
	}
}

static void
conv_s24_to_f32d_1s_avx2(void *data, void * SPA_RESTRICT dst[], const void * SPA_RESTRICT src,
		uint32_t n_channels, uint32_t n_samples)
{
	const int8_t *s = src;
	float *d0 = dst[0];
	uint32_t n, unrolled;
	__m128i in;
	__m128 out, factor = _mm_set1_ps(1.0f / S24_SCALE);
	__m128i mask1 = _mm_setr_epi32(0*n_channels, 3*n_channels, 6*n_channels, 9*n_channels);

	if (SPA_IS_ALIGNED(d0, 16) && n_samples > 0) {
		unrolled = n_samples & ~3;
		if ((n_samples & 3) == 0)
			unrolled -= 4;
	}
	else
		unrolled = 0;

	for(n = 0; n < unrolled; n += 4) {
		in = _mm_i32gather_epi32((int*)s, mask1, 1);
		in = _mm_slli_epi32(in, 8);
		in = _mm_srai_epi32(in, 8);
		out = _mm_cvtepi32_ps(in);
		out = _mm_mul_ps(out, factor);
		_mm_store_ps(&d0[n], out);
		s += 12 * n_channels;
	}
	for(; n < n_samples; n++) {
		out = _mm_cvtsi32_ss(factor, s24_to_s32(*(int24_t*)s));
		out = _mm_mul_ss(out, factor);
		_mm_store_ss(&d0[n], out);
		s += 3 * n_channels;
	}
}

static void
conv_s24_to_f32d_2s_avx2(void *data, void * SPA_RESTRICT dst[], const void * SPA_RESTRICT src,
		uint32_t n_channels, uint32_t n_samples)
{
	const int8_t *s = src;
	float *d0 = dst[0], *d1 = dst[1];
	uint32_t n, unrolled;
	__m128i in[2];
	__m128 out[2], factor = _mm_set1_ps(1.0f / S24_SCALE);
	__m128i mask1 = _mm_setr_epi32(0*n_channels, 3*n_channels, 6*n_channels, 9*n_channels);

	if (SPA_IS_ALIGNED(d0, 16) &&
	    SPA_IS_ALIGNED(d1, 16) &&
	    n_samples > 0) {
		unrolled = n_samples & ~3;
		if ((n_samples & 3) == 0)
			unrolled -= 4;
	}
	else
		unrolled = 0;

	for(n = 0; n < unrolled; n += 4) {
		in[0] = _mm_i32gather_epi32((int*)&s[0], mask1, 1);
		in[1] = _mm_i32gather_epi32((int*)&s[3], mask1, 1);

		in[0] = _mm_slli_epi32(in[0], 8);
		in[1] = _mm_slli_epi32(in[1], 8);

		in[0] = _mm_srai_epi32(in[0], 8);
		in[1] = _mm_srai_epi32(in[1], 8);

		out[0] = _mm_cvtepi32_ps(in[0]);
		out[1] = _mm_cvtepi32_ps(in[1]);

		out[0] = _mm_mul_ps(out[0], factor);
		out[1] = _mm_mul_ps(out[1], factor);

		_mm_store_ps(&d0[n], out[0]);
		_mm_store_ps(&d1[n], out[1]);

		s += 12 * n_channels;
	}
	for(; n < n_samples; n++) {
		out[0] = _mm_cvtsi32_ss(factor, s24_to_s32(*((int24_t*)s+0)));
		out[1] = _mm_cvtsi32_ss(factor, s24_to_s32(*((int24_t*)s+1)));
		out[0] = _mm_mul_ss(out[0], factor);
		out[1] = _mm_mul_ss(out[1], factor);
		_mm_store_ss(&d0[n], out[0]);
		_mm_store_ss(&d1[n], out[1]);
		s += 3 * n_channels;
	}
}
static void
conv_s24_to_f32d_4s_avx2(void *data, void * SPA_RESTRICT dst[], const void * SPA_RESTRICT src,
		uint32_t n_channels, uint32_t n_samples)
{
	const int8_t *s = src;
	float *d0 = dst[0], *d1 = dst[1], *d2 = dst[2], *d3 = dst[3];
	uint32_t n, unrolled;
	__m128i in[4];
	__m128 out[4], factor = _mm_set1_ps(1.0f / S24_SCALE);
	__m128i mask1 = _mm_setr_epi32(0*n_channels, 3*n_channels, 6*n_channels, 9*n_channels);

	if (SPA_IS_ALIGNED(d0, 16) &&
	    SPA_IS_ALIGNED(d1, 16) &&
	    SPA_IS_ALIGNED(d2, 16) &&
	    SPA_IS_ALIGNED(d3, 16) &&
	    n_samples > 0) {
		unrolled = n_samples & ~3;
		if ((n_samples & 3) == 0)
			unrolled -= 4;
	}
	else
		unrolled = 0;

	for(n = 0; n < unrolled; n += 4) {
		in[0] = _mm_i32gather_epi32((int*)&s[0], mask1, 1);
		in[1] = _mm_i32gather_epi32((int*)&s[3], mask1, 1);
		in[2] = _mm_i32gather_epi32((int*)&s[6], mask1, 1);
		in[3] = _mm_i32gather_epi32((int*)&s[9], mask1, 1);

		in[0] = _mm_slli_epi32(in[0], 8);
		in[1] = _mm_slli_epi32(in[1], 8);
		in[2] = _mm_slli_epi32(in[2], 8);
		in[3] = _mm_slli_epi32(in[3], 8);

		in[0] = _mm_srai_epi32(in[0], 8);
		in[1] = _mm_srai_epi32(in[1], 8);
		in[2] = _mm_srai_epi32(in[2], 8);
		in[3] = _mm_srai_epi32(in[3], 8);

		out[0] = _mm_cvtepi32_ps(in[0]);
		out[1] = _mm_cvtepi32_ps(in[1]);
		out[2] = _mm_cvtepi32_ps(in[2]);
		out[3] = _mm_cvtepi32_ps(in[3]);

		out[0] = _mm_mul_ps(out[0], factor);
		out[1] = _mm_mul_ps(out[1], factor);
		out[2] = _mm_mul_ps(out[2], factor);
		out[3] = _mm_mul_ps(out[3], factor);

		_mm_store_ps(&d0[n], out[0]);
		_mm_store_ps(&d1[n], out[1]);
		_mm_store_ps(&d2[n], out[2]);
		_mm_store_ps(&d3[n], out[3]);

		s += 12 * n_channels;
	}
	for(; n < n_samples; n++) {
		out[0] = _mm_cvtsi32_ss(factor, s24_to_s32(*((int24_t*)s+0)));
		out[1] = _mm_cvtsi32_ss(factor, s24_to_s32(*((int24_t*)s+1)));
		out[2] = _mm_cvtsi32_ss(factor, s24_to_s32(*((int24_t*)s+2)));
		out[3] = _mm_cvtsi32_ss(factor, s24_to_s32(*((int24_t*)s+3)));
		out[0] = _mm_mul_ss(out[0], factor);
		out[1] = _mm_mul_ss(out[1], factor);
		out[2] = _mm_mul_ss(out[2], factor);
		out[3] = _mm_mul_ss(out[3], factor);
		_mm_store_ss(&d0[n], out[0]);
		_mm_store_ss(&d1[n], out[1]);
		_mm_store_ss(&d2[n], out[2]);
		_mm_store_ss(&d3[n], out[3]);
		s += 3 * n_channels;
	}
}

void
conv_s24_to_f32d_avx2(struct convert *conv, void * SPA_RESTRICT dst[], const void * SPA_RESTRICT src[],
		uint32_t n_samples)
{
	const int8_t *s = src[0];
	uint32_t i = 0, n_channels = conv->n_channels;

	for(; i + 3 < n_channels; i += 4)
		conv_s24_to_f32d_4s_avx2(conv, &dst[i], &s[3*i], n_channels, n_samples);
	for(; i + 1 < n_channels; i += 2)
		conv_s24_to_f32d_2s_avx2(conv, &dst[i], &s[3*i], n_channels, n_samples);
	for(; i < n_channels; i++)
		conv_s24_to_f32d_1s_avx2(conv, &dst[i], &s[3*i], n_channels, n_samples);
}

static void
conv_s32_to_f32d_4s_avx2(void *data, void * SPA_RESTRICT dst[], const void * SPA_RESTRICT src,
		uint32_t n_channels, uint32_t n_samples)
{
	const int32_t *s = src;
	float *d0 = dst[0], *d1 = dst[1], *d2 = dst[2], *d3 = dst[3];
	uint32_t n, unrolled;
	__m256i in[4];
	__m256 out[4], factor = _mm256_set1_ps(1.0f / S32_SCALE_I2F);
	__m256i mask1 = _mm256_setr_epi32(0*n_channels, 1*n_channels, 2*n_channels, 3*n_channels,
					  4*n_channels, 5*n_channels, 6*n_channels, 7*n_channels);

	if (SPA_IS_ALIGNED(d0, 32) &&
	    SPA_IS_ALIGNED(d1, 32) &&
	    SPA_IS_ALIGNED(d2, 32) &&
	    SPA_IS_ALIGNED(d3, 32))
		unrolled = n_samples & ~7;
	else
		unrolled = 0;

	for(n = 0; n < unrolled; n += 8) {
		in[0] = _mm256_i32gather_epi32((int*)&s[0], mask1, 4);
		in[1] = _mm256_i32gather_epi32((int*)&s[1], mask1, 4);
		in[2] = _mm256_i32gather_epi32((int*)&s[2], mask1, 4);
		in[3] = _mm256_i32gather_epi32((int*)&s[3], mask1, 4);

		out[0] = _mm256_cvtepi32_ps(in[0]);
		out[1] = _mm256_cvtepi32_ps(in[1]);
		out[2] = _mm256_cvtepi32_ps(in[2]);
		out[3] = _mm256_cvtepi32_ps(in[3]);

		out[0] = _mm256_mul_ps(out[0], factor);
		out[1] = _mm256_mul_ps(out[1], factor);
		out[2] = _mm256_mul_ps(out[2], factor);
		out[3] = _mm256_mul_ps(out[3], factor);

		_mm256_store_ps(&d0[n], out[0]);
		_mm256_store_ps(&d1[n], out[1]);
		_mm256_store_ps(&d2[n], out[2]);
		_mm256_store_ps(&d3[n], out[3]);

		s += 8*n_channels;
	}
	for(; n < n_samples; n++) {
		__m128 out[4], factor = _mm_set1_ps(1.0f / S32_SCALE_I2F);
		out[0] = _mm_cvtsi32_ss(factor, s[0]);
		out[1] = _mm_cvtsi32_ss(factor, s[1]);
		out[2] = _mm_cvtsi32_ss(factor, s[2]);
		out[3] = _mm_cvtsi32_ss(factor, s[3]);
		out[0] = _mm_mul_ss(out[0], factor);
		out[1] = _mm_mul_ss(out[1], factor);
		out[2] = _mm_mul_ss(out[2], factor);
		out[3] = _mm_mul_ss(out[3], factor);
		_mm_store_ss(&d0[n], out[0]);
		_mm_store_ss(&d1[n], out[1]);
		_mm_store_ss(&d2[n], out[2]);
		_mm_store_ss(&d3[n], out[3]);
		s += n_channels;
	}
}

static void
conv_s32_to_f32d_2s_avx2(void *data, void * SPA_RESTRICT dst[], const void * SPA_RESTRICT src,
		uint32_t n_channels, uint32_t n_samples)
{
	const int32_t *s = src;
	float *d0 = dst[0], *d1 = dst[1];
	uint32_t n, unrolled;
	__m256i in[4];
	__m256 out[4], factor = _mm256_set1_ps(1.0f / S32_SCALE_I2F);
	__m256i mask1 = _mm256_setr_epi32(0*n_channels, 1*n_channels, 2*n_channels, 3*n_channels,
					  4*n_channels, 5*n_channels, 6*n_channels, 7*n_channels);

	if (SPA_IS_ALIGNED(d0, 32) &&
	    SPA_IS_ALIGNED(d1, 32))
		unrolled = n_samples & ~7;
	else
		unrolled = 0;

	for(n = 0; n < unrolled; n += 8) {
		in[0] = _mm256_i32gather_epi32((int*)&s[0], mask1, 4);
		in[1] = _mm256_i32gather_epi32((int*)&s[1], mask1, 4);

		out[0] = _mm256_cvtepi32_ps(in[0]);
		out[1] = _mm256_cvtepi32_ps(in[1]);

		out[0] = _mm256_mul_ps(out[0], factor);
		out[1] = _mm256_mul_ps(out[1], factor);

		_mm256_store_ps(&d0[n], out[0]);
		_mm256_store_ps(&d1[n], out[1]);

		s += 8*n_channels;
	}
	for(; n < n_samples; n++) {
		__m128 out[2], factor = _mm_set1_ps(1.0f / S32_SCALE_I2F);
		out[0] = _mm_cvtsi32_ss(factor, s[0]);
		out[1] = _mm_cvtsi32_ss(factor, s[1]);
		out[0] = _mm_mul_ss(out[0], factor);
		out[1] = _mm_mul_ss(out[1], factor);
		_mm_store_ss(&d0[n], out[0]);
		_mm_store_ss(&d1[n], out[1]);
		s += n_channels;
	}
}

static void
conv_s32_to_f32d_1s_avx2(void *data, void * SPA_RESTRICT dst[], const void * SPA_RESTRICT src,
		uint32_t n_channels, uint32_t n_samples)
{
	const int32_t *s = src;
	float *d0 = dst[0];
	uint32_t n, unrolled;
	__m256i in[2];
	__m256 out[2], factor = _mm256_set1_ps(1.0f / S32_SCALE_I2F);
	__m256i mask1 = _mm256_setr_epi32(0*n_channels, 1*n_channels, 2*n_channels, 3*n_channels,
					  4*n_channels, 5*n_channels, 6*n_channels, 7*n_channels);

	if (SPA_IS_ALIGNED(d0, 32))
		unrolled = n_samples & ~15;
	else
		unrolled = 0;

	for(n = 0; n < unrolled; n += 16) {
		in[0] = _mm256_i32gather_epi32(&s[0*n_channels], mask1, 4);
		in[1] = _mm256_i32gather_epi32(&s[8*n_channels], mask1, 4);

		out[0] = _mm256_cvtepi32_ps(in[0]);
		out[1] = _mm256_cvtepi32_ps(in[1]);

		out[0] = _mm256_mul_ps(out[0], factor);
		out[1] = _mm256_mul_ps(out[1], factor);

		_mm256_store_ps(&d0[n+0], out[0]);
		_mm256_store_ps(&d0[n+8], out[1]);

		s += 16*n_channels;
	}
	for(; n < n_samples; n++) {
		__m128 out, factor = _mm_set1_ps(1.0f / S32_SCALE_I2F);
		out = _mm_cvtsi32_ss(factor, s[0]);
		out = _mm_mul_ss(out, factor);
		_mm_store_ss(&d0[n], out);
		s += n_channels;
	}
}

void
conv_s32_to_f32d_avx2(struct convert *conv, void * SPA_RESTRICT dst[], const void * SPA_RESTRICT src[],
		uint32_t n_samples)
{
	const int32_t *s = src[0];
	uint32_t i = 0, n_channels = conv->n_channels;

	for(; i + 3 < n_channels; i += 4)
		conv_s32_to_f32d_4s_avx2(conv, &dst[i], &s[i], n_channels, n_samples);
	for(; i + 1 < n_channels; i += 2)
		conv_s32_to_f32d_2s_avx2(conv, &dst[i], &s[i], n_channels, n_samples);
	for(; i < n_channels; i++)
		conv_s32_to_f32d_1s_avx2(conv, &dst[i], &s[i], n_channels, n_samples);
}

static void
conv_f32d_to_s32_1s_avx2(void *data, void * SPA_RESTRICT dst, const void * SPA_RESTRICT src[],
		uint32_t n_channels, uint32_t n_samples)
{
	const float *s0 = src[0];
	int32_t *d = dst;
	uint32_t n, unrolled;
	__m128 in[1];
	__m128i out[4];
	__m128 scale = _mm_set1_ps(S32_SCALE_F2I);
	__m128 int_min = _mm_set1_ps(S32_MIN_F2I);
	__m128 int_max = _mm_set1_ps(S32_MAX_F2I);

	if (SPA_IS_ALIGNED(s0, 16))
		unrolled = n_samples & ~3;
	else
		unrolled = 0;

	for(n = 0; n < unrolled; n += 4) {
		in[0] = _mm_mul_ps(_mm_load_ps(&s0[n]), scale);
		in[0] = _MM_CLAMP_PS(in[0], int_min, int_max);
		out[0] = _mm_cvtps_epi32(in[0]);
		out[1] = _mm_shuffle_epi32(out[0], _MM_SHUFFLE(0, 3, 2, 1));
		out[2] = _mm_shuffle_epi32(out[0], _MM_SHUFFLE(1, 0, 3, 2));
		out[3] = _mm_shuffle_epi32(out[0], _MM_SHUFFLE(2, 1, 0, 3));

		d[0*n_channels] = _mm_cvtsi128_si32(out[0]);
		d[1*n_channels] = _mm_cvtsi128_si32(out[1]);
		d[2*n_channels] = _mm_cvtsi128_si32(out[2]);
		d[3*n_channels] = _mm_cvtsi128_si32(out[3]);
		d += 4*n_channels;
	}
	for(; n < n_samples; n++) {
		in[0] = _mm_load_ss(&s0[n]);
		in[0] = _mm_mul_ss(in[0], scale);
		in[0] = _MM_CLAMP_SS(in[0], int_min, int_max);
		*d = _mm_cvtss_si32(in[0]);
		d += n_channels;
	}
}

#define spa_write_unaligned(ptr, type, val) \
__extension__ ({ \
	__typeof__(type) _val = (val); \
	memcpy((ptr), &_val, sizeof(_val)); \
})

static void
conv_f32d_to_s32_2s_avx2(void *data, void * SPA_RESTRICT dst, const void * SPA_RESTRICT src[],
		uint32_t n_channels, uint32_t n_samples)
{
	const float *s0 = src[0], *s1 = src[1];
	int32_t *d = dst;
	uint32_t n, unrolled;
	__m256 in[2];
	__m256i out[2], t[2];
	__m256 scale = _mm256_set1_ps(S32_SCALE_F2I);
	__m256 int_min = _mm256_set1_ps(S32_MIN_F2I);
	__m256 int_max = _mm256_set1_ps(S32_MAX_F2I);

	if (SPA_IS_ALIGNED(s0, 32) &&
		SPA_IS_ALIGNED(s1, 32))
		unrolled = n_samples & ~7;
	else
		unrolled = 0;

	for(n = 0; n < unrolled; n += 8) {
		in[0] = _mm256_mul_ps(_mm256_load_ps(&s0[n]), scale);
		in[1] = _mm256_mul_ps(_mm256_load_ps(&s1[n]), scale);

		in[0] = _MM256_CLAMP_PS(in[0], int_min, int_max);
		in[1] = _MM256_CLAMP_PS(in[1], int_min, int_max);

		out[0] = _mm256_cvtps_epi32(in[0]);	/* a0 a1 a2 a3 a4 a5 a6 a7 */
		out[1] = _mm256_cvtps_epi32(in[1]);	/* b0 b1 b2 b3 b4 b5 b6 b7 */

		t[0] = _mm256_unpacklo_epi32(out[0], out[1]); /* a0 b0 a1 b1 a4 b4 a5 b5 */
		t[1] = _mm256_unpackhi_epi32(out[0], out[1]); /* a2 b2 a3 b3 a6 b6 a7 b7 */

#ifdef __x86_64__
		spa_write_unaligned(d + 0*n_channels, uint64_t, _mm256_extract_epi64(t[0], 0));
		spa_write_unaligned(d + 1*n_channels, uint64_t, _mm256_extract_epi64(t[0], 1));
		spa_write_unaligned(d + 2*n_channels, uint64_t, _mm256_extract_epi64(t[1], 0));
		spa_write_unaligned(d + 3*n_channels, uint64_t, _mm256_extract_epi64(t[1], 1));
		spa_write_unaligned(d + 4*n_channels, uint64_t, _mm256_extract_epi64(t[0], 2));
		spa_write_unaligned(d + 5*n_channels, uint64_t, _mm256_extract_epi64(t[0], 3));
		spa_write_unaligned(d + 6*n_channels, uint64_t, _mm256_extract_epi64(t[1], 2));
		spa_write_unaligned(d + 7*n_channels, uint64_t, _mm256_extract_epi64(t[1], 3));
#else
		_mm_storel_pi((__m64*)(d + 0*n_channels), (__m128)_mm256_extracti128_si256(t[0], 0));
		_mm_storeh_pi((__m64*)(d + 1*n_channels), (__m128)_mm256_extracti128_si256(t[0], 0));
		_mm_storel_pi((__m64*)(d + 2*n_channels), (__m128)_mm256_extracti128_si256(t[1], 0));
		_mm_storeh_pi((__m64*)(d + 3*n_channels), (__m128)_mm256_extracti128_si256(t[1], 0));
		_mm_storel_pi((__m64*)(d + 4*n_channels), (__m128)_mm256_extracti128_si256(t[0], 1));
		_mm_storeh_pi((__m64*)(d + 5*n_channels), (__m128)_mm256_extracti128_si256(t[0], 1));
		_mm_storel_pi((__m64*)(d + 6*n_channels), (__m128)_mm256_extracti128_si256(t[1], 1));
		_mm_storeh_pi((__m64*)(d + 7*n_channels), (__m128)_mm256_extracti128_si256(t[1], 1));
#endif
		d += 8*n_channels;
	}
	for(; n < n_samples; n++) {
		__m128 in[2];
		__m128i out[2];
		__m128 scale = _mm_set1_ps(S32_SCALE_F2I);
		__m128 int_min = _mm_set1_ps(S32_MIN_F2I);
		__m128 int_max = _mm_set1_ps(S32_MAX_F2I);

		in[0] = _mm_load_ss(&s0[n]);
		in[1] = _mm_load_ss(&s1[n]);

		in[0] = _mm_unpacklo_ps(in[0], in[1]);

		in[0] = _mm_mul_ps(in[0], scale);
		in[0] = _MM_CLAMP_PS(in[0], int_min, int_max);
		out[0] = _mm_cvtps_epi32(in[0]);
		_mm_storel_epi64((__m128i*)d, out[0]);
		d += n_channels;
	}
}

static void
conv_f32d_to_s32_4s_avx2(void *data, void * SPA_RESTRICT dst, const void * SPA_RESTRICT src[],
		uint32_t n_channels, uint32_t n_samples)
{
	const float *s0 = src[0], *s1 = src[1], *s2 = src[2], *s3 = src[3];
	int32_t *d = dst;
	uint32_t n, unrolled;
	__m256 in[4];
	__m256i out[4], t[4];
	__m256 scale = _mm256_set1_ps(S32_SCALE_F2I);
	__m256 int_min = _mm256_set1_ps(S32_MIN_F2I);
	__m256 int_max = _mm256_set1_ps(S32_MAX_F2I);

	if (SPA_IS_ALIGNED(s0, 32) &&
		SPA_IS_ALIGNED(s1, 32) &&
		SPA_IS_ALIGNED(s2, 32) &&
		SPA_IS_ALIGNED(s3, 32))
		unrolled = n_samples & ~7;
	else
		unrolled = 0;

	for(n = 0; n < unrolled; n += 8) {
		in[0] = _mm256_mul_ps(_mm256_load_ps(&s0[n]), scale);
		in[1] = _mm256_mul_ps(_mm256_load_ps(&s1[n]), scale);
		in[2] = _mm256_mul_ps(_mm256_load_ps(&s2[n]), scale);
		in[3] = _mm256_mul_ps(_mm256_load_ps(&s3[n]), scale);

		in[0] = _MM256_CLAMP_PS(in[0], int_min, int_max);
		in[1] = _MM256_CLAMP_PS(in[1], int_min, int_max);
		in[2] = _MM256_CLAMP_PS(in[2], int_min, int_max);
		in[3] = _MM256_CLAMP_PS(in[3], int_min, int_max);

		out[0] = _mm256_cvtps_epi32(in[0]); /* a0 a1 a2 a3 a4 a5 a6 a7 */
		out[1] = _mm256_cvtps_epi32(in[1]); /* b0 b1 b2 b3 b4 b5 b6 b7 */
		out[2] = _mm256_cvtps_epi32(in[2]); /* c0 c1 c2 c3 c4 c5 c6 c7 */
		out[3] = _mm256_cvtps_epi32(in[3]); /* d0 d1 d2 d3 d4 d5 d6 d7 */

		t[0] = _mm256_unpacklo_epi32(out[0], out[1]); /* a0 b0 a1 b1 a4 b4 a5 b5 */
		t[1] = _mm256_unpackhi_epi32(out[0], out[1]); /* a2 b2 a3 b3 a6 b6 a7 b7 */
		t[2] = _mm256_unpacklo_epi32(out[2], out[3]); /* c0 d0 c1 d1 c4 d4 c5 d5 */
		t[3] = _mm256_unpackhi_epi32(out[2], out[3]); /* c2 d2 c3 d3 c6 d6 c7 d7 */

		out[0] = _mm256_unpacklo_epi64(t[0], t[2]);   /* a0 b0 c0 d0 a4 b4 c4 d4 */
		out[1] = _mm256_unpackhi_epi64(t[0], t[2]);   /* a1 b1 c1 d1 a5 b5 c5 d5 */
		out[2] = _mm256_unpacklo_epi64(t[1], t[3]);   /* a2 b2 c2 d2 a6 b6 c6 d6 */
		out[3] = _mm256_unpackhi_epi64(t[1], t[3]);   /* a3 b3 c3 d3 a7 b7 c7 d7 */

		_mm_storeu_si128((__m128i*)(d + 0*n_channels), _mm256_extracti128_si256(out[0], 0));
		_mm_storeu_si128((__m128i*)(d + 1*n_channels), _mm256_extracti128_si256(out[1], 0));
		_mm_storeu_si128((__m128i*)(d + 2*n_channels), _mm256_extracti128_si256(out[2], 0));
		_mm_storeu_si128((__m128i*)(d + 3*n_channels), _mm256_extracti128_si256(out[3], 0));
		_mm_storeu_si128((__m128i*)(d + 4*n_channels), _mm256_extracti128_si256(out[0], 1));
		_mm_storeu_si128((__m128i*)(d + 5*n_channels), _mm256_extracti128_si256(out[1], 1));
		_mm_storeu_si128((__m128i*)(d + 6*n_channels), _mm256_extracti128_si256(out[2], 1));
		_mm_storeu_si128((__m128i*)(d + 7*n_channels), _mm256_extracti128_si256(out[3], 1));
		d += 8*n_channels;
	}
	for(; n < n_samples; n++) {
		__m128 in[4];
		__m128i out[4];
		__m128 scale = _mm_set1_ps(S32_SCALE_F2I);
		__m128 int_min = _mm_set1_ps(S32_MIN_F2I);
		__m128 int_max = _mm_set1_ps(S32_MAX_F2I);

		in[0] = _mm_load_ss(&s0[n]);
		in[1] = _mm_load_ss(&s1[n]);
		in[2] = _mm_load_ss(&s2[n]);
		in[3] = _mm_load_ss(&s3[n]);

		in[0] = _mm_unpacklo_ps(in[0], in[2]);
		in[1] = _mm_unpacklo_ps(in[1], in[3]);
		in[0] = _mm_unpacklo_ps(in[0], in[1]);

		in[0] = _mm_mul_ps(in[0], scale);
		in[0] = _MM_CLAMP_PS(in[0], int_min, int_max);
		out[0] = _mm_cvtps_epi32(in[0]);
		_mm_storeu_si128((__m128i*)d, out[0]);
		d += n_channels;
	}
}

void
conv_f32d_to_s32_avx2(struct convert *conv, void * SPA_RESTRICT dst[], const void * SPA_RESTRICT src[],
		uint32_t n_samples)
{
	int32_t *d = dst[0];
	uint32_t i = 0, n_channels = conv->n_channels;

	for(; i + 3 < n_channels; i += 4)
		conv_f32d_to_s32_4s_avx2(conv, &d[i], &src[i], n_channels, n_samples);
	for(; i + 1 < n_channels; i += 2)
		conv_f32d_to_s32_2s_avx2(conv, &d[i], &src[i], n_channels, n_samples);
	for(; i < n_channels; i++)
		conv_f32d_to_s32_1s_avx2(conv, &d[i], &src[i], n_channels, n_samples);
}

static void
conv_f32d_to_s16_1s_avx2(void *data, void * SPA_RESTRICT dst, const void * SPA_RESTRICT src[],
		uint32_t n_channels, uint32_t n_samples)
{
	const float *s0 = src[0];
	int16_t *d = dst;
	uint32_t n, unrolled;
	__m128 in[2];
	__m128i out[2];
	__m128 int_scale = _mm_set1_ps(S16_SCALE);
	__m128 int_max = _mm_set1_ps(S16_MAX);
	__m128 int_min = _mm_set1_ps(S16_MIN);

	if (SPA_IS_ALIGNED(s0, 16))
		unrolled = n_samples & ~7;
	else
		unrolled = 0;

	for(n = 0; n < unrolled; n += 8) {
		in[0] = _mm_mul_ps(_mm_load_ps(&s0[n]), int_scale);
		in[1] = _mm_mul_ps(_mm_load_ps(&s0[n+4]), int_scale);
		out[0] = _mm_cvtps_epi32(in[0]);
		out[1] = _mm_cvtps_epi32(in[1]);
		out[0] = _mm_packs_epi32(out[0], out[1]);

		d[0*n_channels] = _mm_extract_epi16(out[0], 0);
		d[1*n_channels] = _mm_extract_epi16(out[0], 1);
		d[2*n_channels] = _mm_extract_epi16(out[0], 2);
		d[3*n_channels] = _mm_extract_epi16(out[0], 3);
		d[4*n_channels] = _mm_extract_epi16(out[0], 4);
		d[5*n_channels] = _mm_extract_epi16(out[0], 5);
		d[6*n_channels] = _mm_extract_epi16(out[0], 6);
		d[7*n_channels] = _mm_extract_epi16(out[0], 7);
		d += 8*n_channels;
	}
	for(; n < n_samples; n++) {
		in[0] = _mm_mul_ss(_mm_load_ss(&s0[n]), int_scale);
		in[0] = _MM_CLAMP_SS(in[0], int_min, int_max);
		*d = _mm_cvtss_si32(in[0]);
		d += n_channels;
	}
}

static void
conv_f32d_to_s16_2s_avx2(void *data, void * SPA_RESTRICT dst, const void * SPA_RESTRICT src[],
		uint32_t n_channels, uint32_t n_samples)
{
	const float *s0 = src[0], *s1 = src[1];
	int16_t *d = dst;
	uint32_t n, unrolled;
	__m256 in[2];
	__m256i out[4], t[2];
	__m256 int_scale = _mm256_set1_ps(S16_SCALE);

	if (SPA_IS_ALIGNED(s0, 32) &&
	    SPA_IS_ALIGNED(s1, 32))
		unrolled = n_samples & ~15;
	else
		unrolled = 0;

	for(n = 0; n < unrolled; n += 8) {
		in[0] = _mm256_mul_ps(_mm256_load_ps(&s0[n+0]), int_scale);
		in[1] = _mm256_mul_ps(_mm256_load_ps(&s1[n+0]), int_scale);

		out[0] = _mm256_cvtps_epi32(in[0]); /* a0 a1 a2 a3 a4 a5 a6 a7 */
		out[1] = _mm256_cvtps_epi32(in[1]); /* b0 b1 b2 b3 b4 b5 b6 b7 */

		t[0] = _mm256_unpacklo_epi32(out[0], out[1]); /* a0 b0 a1 b1 a4 b4 a5 b5 */
		t[1] = _mm256_unpackhi_epi32(out[0], out[1]); /* a2 b2 a3 b3 a6 b6 a7 b7 */

		out[0] = _mm256_packs_epi32(t[0], t[1]); /* a0 b0 a1 b1 a2 b2 a3 b3 a4 b4 a5 b5 a6 b6 a7 b7 */

		spa_write_unaligned(d + 0*n_channels, uint32_t, _mm256_extract_epi32(out[0],0));
		spa_write_unaligned(d + 1*n_channels, uint32_t, _mm256_extract_epi32(out[0],1));
		spa_write_unaligned(d + 2*n_channels, uint32_t, _mm256_extract_epi32(out[0],2));
		spa_write_unaligned(d + 3*n_channels, uint32_t, _mm256_extract_epi32(out[0],3));
		spa_write_unaligned(d + 4*n_channels, uint32_t, _mm256_extract_epi32(out[0],4));
		spa_write_unaligned(d + 5*n_channels, uint32_t, _mm256_extract_epi32(out[0],5));
		spa_write_unaligned(d + 6*n_channels, uint32_t, _mm256_extract_epi32(out[0],6));
		spa_write_unaligned(d + 7*n_channels, uint32_t, _mm256_extract_epi32(out[0],7));

		d += 8*n_channels;
	}
	for(; n < n_samples; n++) {
		__m128 in[2];
		__m128 int_scale = _mm_set1_ps(S16_SCALE);
		__m128 int_max = _mm_set1_ps(S16_MAX);
		__m128 int_min = _mm_set1_ps(S16_MIN);

		in[0] = _mm_mul_ss(_mm_load_ss(&s0[n]), int_scale);
		in[1] = _mm_mul_ss(_mm_load_ss(&s1[n]), int_scale);
		in[0] = _MM_CLAMP_SS(in[0], int_min, int_max);
		in[1] = _MM_CLAMP_SS(in[1], int_min, int_max);
		d[0] = _mm_cvtss_si32(in[0]);
		d[1] = _mm_cvtss_si32(in[1]);
		d += n_channels;
	}
}

static void
conv_f32d_to_s16_4s_avx2(void *data, void * SPA_RESTRICT dst, const void * SPA_RESTRICT src[],
		uint32_t n_channels, uint32_t n_samples)
{
	const float *s0 = src[0], *s1 = src[1], *s2 = src[2], *s3 = src[3];
	int16_t *d = dst;
	uint32_t n, unrolled;
	__m256 in[4];
	__m256i out[4], t[4];
	__m256 int_scale = _mm256_set1_ps(S16_SCALE);

	if (SPA_IS_ALIGNED(s0, 32) &&
	    SPA_IS_ALIGNED(s1, 32) &&
	    SPA_IS_ALIGNED(s2, 32) &&
	    SPA_IS_ALIGNED(s3, 32))
		unrolled = n_samples & ~7;
	else
		unrolled = 0;

	for(n = 0; n < unrolled; n += 8) {
		in[0] = _mm256_mul_ps(_mm256_load_ps(&s0[n]), int_scale);
		in[1] = _mm256_mul_ps(_mm256_load_ps(&s1[n]), int_scale);
		in[2] = _mm256_mul_ps(_mm256_load_ps(&s2[n]), int_scale);
		in[3] = _mm256_mul_ps(_mm256_load_ps(&s3[n]), int_scale);

		t[0] = _mm256_cvtps_epi32(in[0]);  /* a0 a1 a2 a3 a4 a5 a6 a7 */
		t[1] = _mm256_cvtps_epi32(in[1]);  /* b0 b1 b2 b3 b4 b5 b6 b7 */
		t[2] = _mm256_cvtps_epi32(in[2]);  /* c0 c1 c2 c3 c4 c5 c6 c7 */
		t[3] = _mm256_cvtps_epi32(in[3]);  /* d0 d1 d2 d3 d4 d5 d6 d7 */

		t[0] = _mm256_packs_epi32(t[0], t[2]); /* a0 a1 a2 a3 c0 c1 c2 c3 a4 a5 a6 a7 c4 c5 c6 c7 */
		t[1] = _mm256_packs_epi32(t[1], t[3]); /* b0 b1 b2 b3 d0 d1 d2 d3 b4 b5 b6 b7 d4 d5 d6 d7 */

		out[0] = _mm256_unpacklo_epi16(t[0], t[1]);     /* a0 b0 a1 b1 a2 b2 a3 b3 a4 b4 a5 b5 a6 b6 a7 b7 */
		out[1] = _mm256_unpackhi_epi16(t[0], t[1]);     /* c0 d0 c1 d1 c2 d2 c3 d3 c4 d4 c5 d5 c6 d6 c7 d7 */

		out[2] = _mm256_unpacklo_epi32(out[0], out[1]); /* a0 b0 c0 d0 a1 b1 c1 d1 a4 b4 c4 d4 a5 b5 c5 d5 */
		out[3] = _mm256_unpackhi_epi32(out[0], out[1]); /* a2 b2 c2 d2 a3 b3 c3 d3 a6 b6 c6 d6 a7 b7 c7 d7 */

#ifdef __x86_64__
		spa_write_unaligned(d + 0*n_channels, uint64_t, _mm256_extract_epi64(out[2], 0)); /* a0 b0 c0 d0 */
		spa_write_unaligned(d + 1*n_channels, uint64_t, _mm256_extract_epi64(out[2], 1)); /* a1 b1 c1 d1 */
		spa_write_unaligned(d + 2*n_channels, uint64_t, _mm256_extract_epi64(out[3], 0)); /* a2 b2 c2 d2 */
		spa_write_unaligned(d + 3*n_channels, uint64_t, _mm256_extract_epi64(out[3], 1)); /* a3 b3 c3 d3 */
		spa_write_unaligned(d + 4*n_channels, uint64_t, _mm256_extract_epi64(out[2], 2)); /* a4 b4 c4 d4 */
		spa_write_unaligned(d + 5*n_channels, uint64_t, _mm256_extract_epi64(out[2], 3)); /* a5 b5 c5 d5 */
		spa_write_unaligned(d + 6*n_channels, uint64_t, _mm256_extract_epi64(out[3], 2)); /* a6 b6 c6 d6 */
		spa_write_unaligned(d + 7*n_channels, uint64_t, _mm256_extract_epi64(out[3], 3)); /* a7 b7 c7 d7 */
#else
		_mm_storel_pi((__m64*)(d + 0*n_channels), (__m128)_mm256_extracti128_si256(out[2], 0));
		_mm_storeh_pi((__m64*)(d + 1*n_channels), (__m128)_mm256_extracti128_si256(out[2], 0));
		_mm_storel_pi((__m64*)(d + 2*n_channels), (__m128)_mm256_extracti128_si256(out[3], 0));
		_mm_storeh_pi((__m64*)(d + 3*n_channels), (__m128)_mm256_extracti128_si256(out[3], 0));
		_mm_storel_pi((__m64*)(d + 4*n_channels), (__m128)_mm256_extracti128_si256(out[2], 1));
		_mm_storeh_pi((__m64*)(d + 5*n_channels), (__m128)_mm256_extracti128_si256(out[2], 1));
		_mm_storel_pi((__m64*)(d + 6*n_channels), (__m128)_mm256_extracti128_si256(out[3], 1));
		_mm_storeh_pi((__m64*)(d + 7*n_channels), (__m128)_mm256_extracti128_si256(out[3], 1));
#endif

		d += 8*n_channels;
	}
	for(; n < n_samples; n++) {
		__m128 in[4];
		__m128 int_scale = _mm_set1_ps(S16_SCALE);
		__m128 int_max = _mm_set1_ps(S16_MAX);
		__m128 int_min = _mm_set1_ps(S16_MIN);

		in[0] = _mm_mul_ss(_mm_load_ss(&s0[n]), int_scale);
		in[1] = _mm_mul_ss(_mm_load_ss(&s1[n]), int_scale);
		in[2] = _mm_mul_ss(_mm_load_ss(&s2[n]), int_scale);
		in[3] = _mm_mul_ss(_mm_load_ss(&s3[n]), int_scale);
		in[0] = _MM_CLAMP_SS(in[0], int_min, int_max);
		in[1] = _MM_CLAMP_SS(in[1], int_min, int_max);
		in[2] = _MM_CLAMP_SS(in[2], int_min, int_max);
		in[3] = _MM_CLAMP_SS(in[3], int_min, int_max);
		d[0] = _mm_cvtss_si32(in[0]);
		d[1] = _mm_cvtss_si32(in[1]);
		d[2] = _mm_cvtss_si32(in[2]);
		d[3] = _mm_cvtss_si32(in[3]);
		d += n_channels;
	}
}

void
conv_f32d_to_s16_avx2(struct convert *conv, void * SPA_RESTRICT dst[], const void * SPA_RESTRICT src[],
		uint32_t n_samples)
{
	int16_t *d = dst[0];
	uint32_t i = 0, n_channels = conv->n_channels;

	for(; i + 3 < n_channels; i += 4)
		conv_f32d_to_s16_4s_avx2(conv, &d[i], &src[i], n_channels, n_samples);
	for(; i + 1 < n_channels; i += 2)
		conv_f32d_to_s16_2s_avx2(conv, &d[i], &src[i], n_channels, n_samples);
	for(; i < n_channels; i++)
		conv_f32d_to_s16_1s_avx2(conv, &d[i], &src[i], n_channels, n_samples);
}

void
conv_f32d_to_s16_4_avx2(struct convert *conv, void * SPA_RESTRICT dst[], const void * SPA_RESTRICT src[],
		uint32_t n_samples)
{
	const float *s0 = src[0], *s1 = src[1], *s2 = src[2], *s3 = src[3];
	int16_t *d = dst[0];
	uint32_t n, unrolled;
	__m256 in[4];
	__m256i out[4], t[4];
	__m256 int_scale = _mm256_set1_ps(S16_SCALE);

	if (SPA_IS_ALIGNED(s0, 32) &&
	    SPA_IS_ALIGNED(s1, 32) &&
	    SPA_IS_ALIGNED(s2, 32) &&
	    SPA_IS_ALIGNED(s3, 32))
		unrolled = n_samples & ~7;
	else
		unrolled = 0;

	for(n = 0; n < unrolled; n += 8) {
		in[0] = _mm256_mul_ps(_mm256_load_ps(&s0[n]), int_scale);
		in[1] = _mm256_mul_ps(_mm256_load_ps(&s1[n]), int_scale);
		in[2] = _mm256_mul_ps(_mm256_load_ps(&s2[n]), int_scale);
		in[3] = _mm256_mul_ps(_mm256_load_ps(&s3[n]), int_scale);

		t[0] = _mm256_cvtps_epi32(in[0]);  /* a0 a1 a2 a3 a4 a5 a6 a7 */
		t[1] = _mm256_cvtps_epi32(in[1]);  /* b0 b1 b2 b3 b4 b5 b6 b7 */
		t[2] = _mm256_cvtps_epi32(in[2]);  /* c0 c1 c2 c3 c4 c5 c6 c7 */
		t[3] = _mm256_cvtps_epi32(in[3]);  /* d0 d1 d2 d3 d4 d5 d6 d7 */

		t[0] = _mm256_packs_epi32(t[0], t[2]); /* a0 a1 a2 a3 c0 c1 c2 c3 a4 a5 a6 a7 c4 c5 c6 c7 */
		t[1] = _mm256_packs_epi32(t[1], t[3]); /* b0 b1 b2 b3 d0 d1 d2 d3 b4 b5 b6 b7 d4 d5 d6 d7 */

		out[0] = _mm256_unpacklo_epi16(t[0], t[1]);     /* a0 b0 a1 b1 a2 b2 a3 b3 a4 b4 a5 b5 a6 b6 a7 b7 */
		out[1] = _mm256_unpackhi_epi16(t[0], t[1]);     /* c0 d0 c1 d1 c2 d2 c3 d3 c4 d4 c5 d5 c6 d6 c7 d7 */

		t[0] = _mm256_unpacklo_epi32(out[0], out[1]);   /* a0 b0 c0 d0 a1 b1 c1 d1 a4 b4 c4 d4 a5 b5 c5 d5 */
		t[2] = _mm256_unpackhi_epi32(out[0], out[1]);   /* a2 b2 c2 d2 a3 b3 c3 d3 a6 b6 c6 d6 a7 b7 c7 d7 */

		out[0] = _mm256_inserti128_si256(t[0], _mm256_extracti128_si256(t[2], 0), 1);
		out[2] = _mm256_inserti128_si256(t[2], _mm256_extracti128_si256(t[0], 1), 0);

		_mm256_store_si256((__m256i*)(d+0), out[0]);
		_mm256_store_si256((__m256i*)(d+16), out[2]);
		d += 32;
	}
	for(; n < n_samples; n++) {
		__m128 in[4];
		__m128 int_scale = _mm_set1_ps(S16_SCALE);
		__m128 int_max = _mm_set1_ps(S16_MAX);
		__m128 int_min = _mm_set1_ps(S16_MIN);

		in[0] = _mm_mul_ss(_mm_load_ss(&s0[n]), int_scale);
		in[1] = _mm_mul_ss(_mm_load_ss(&s1[n]), int_scale);
		in[2] = _mm_mul_ss(_mm_load_ss(&s2[n]), int_scale);
		in[3] = _mm_mul_ss(_mm_load_ss(&s3[n]), int_scale);
		in[0] = _MM_CLAMP_SS(in[0], int_min, int_max);
		in[1] = _MM_CLAMP_SS(in[1], int_min, int_max);
		in[2] = _MM_CLAMP_SS(in[2], int_min, int_max);
		in[3] = _MM_CLAMP_SS(in[3], int_min, int_max);
		d[0] = _mm_cvtss_si32(in[0]);
		d[1] = _mm_cvtss_si32(in[1]);
		d[2] = _mm_cvtss_si32(in[2]);
		d[3] = _mm_cvtss_si32(in[3]);
		d += 4;
	}
}
void
conv_f32d_to_s16_2_avx2(struct convert *conv, void * SPA_RESTRICT dst[], const void * SPA_RESTRICT src[],
		uint32_t n_samples)
{
	const float *s0 = src[0], *s1 = src[1];
	int16_t *d = dst[0];
	uint32_t n, unrolled;
	__m256 in[4];
	__m256i out[4], t[4];
	__m256 int_scale = _mm256_set1_ps(S16_SCALE);

	if (SPA_IS_ALIGNED(s0, 32) &&
	    SPA_IS_ALIGNED(s1, 32))
		unrolled = n_samples & ~15;
	else
		unrolled = 0;

	for(n = 0; n < unrolled; n += 16) {
		in[0] = _mm256_mul_ps(_mm256_load_ps(&s0[n+0]), int_scale);
		in[1] = _mm256_mul_ps(_mm256_load_ps(&s1[n+0]), int_scale);
		in[2] = _mm256_mul_ps(_mm256_load_ps(&s0[n+8]), int_scale);
		in[3] = _mm256_mul_ps(_mm256_load_ps(&s1[n+8]), int_scale);

		out[0] = _mm256_cvtps_epi32(in[0]); /* a0 a1 a2 a3 a4 a5 a6 a7 */
		out[1] = _mm256_cvtps_epi32(in[1]); /* b0 b1 b2 b3 b4 b5 b6 b7 */
		out[2] = _mm256_cvtps_epi32(in[2]); /* a0 a1 a2 a3 a4 a5 a6 a7 */
		out[3] = _mm256_cvtps_epi32(in[3]); /* b0 b1 b2 b3 b4 b5 b6 b7 */

		t[0] = _mm256_unpacklo_epi32(out[0], out[1]); /* a0 b0 a1 b1 a4 b4 a5 b5 */
		t[1] = _mm256_unpackhi_epi32(out[0], out[1]); /* a2 b2 a3 b3 a6 b6 a7 b7 */
		t[2] = _mm256_unpacklo_epi32(out[2], out[3]); /* a0 b0 a1 b1 a4 b4 a5 b5 */
		t[3] = _mm256_unpackhi_epi32(out[2], out[3]); /* a2 b2 a3 b3 a6 b6 a7 b7 */

		out[0] = _mm256_packs_epi32(t[0], t[1]); /* a0 b0 a1 b1 a2 b2 a3 b3 a4 b4 a5 b5 a6 b6 a7 b7 */
		out[1] = _mm256_packs_epi32(t[2], t[3]); /* a0 b0 a1 b1 a2 b2 a3 b3 a4 b4 a5 b5 a6 b6 a7 b7 */

		_mm256_store_si256((__m256i*)(d+0), out[0]);
		_mm256_store_si256((__m256i*)(d+16), out[1]);

		d += 32;
	}
	for(; n < n_samples; n++) {
		__m128 in[4];
		__m128 int_scale = _mm_set1_ps(S16_SCALE);
		__m128 int_max = _mm_set1_ps(S16_MAX);
		__m128 int_min = _mm_set1_ps(S16_MIN);

		in[0] = _mm_mul_ss(_mm_load_ss(&s0[n]), int_scale);
		in[1] = _mm_mul_ss(_mm_load_ss(&s1[n]), int_scale);
		in[0] = _MM_CLAMP_SS(in[0], int_min, int_max);
		in[1] = _MM_CLAMP_SS(in[1], int_min, int_max);
		d[0] = _mm_cvtss_si32(in[0]);
		d[1] = _mm_cvtss_si32(in[1]);
		d += 2;
	}
}

void
conv_f32d_to_s16s_2_avx2(struct convert *conv, void * SPA_RESTRICT dst[], const void * SPA_RESTRICT src[],
		uint32_t n_samples)
{
	const float *s0 = src[0], *s1 = src[1];
	uint16_t *d = dst[0];
	uint32_t n, unrolled;
	__m256 in[4];
	__m256i out[4], t[4];
	__m256 int_scale = _mm256_set1_ps(S16_SCALE);

	if (SPA_IS_ALIGNED(s0, 32) &&
	    SPA_IS_ALIGNED(s1, 32))
		unrolled = n_samples & ~15;
	else
		unrolled = 0;

	for(n = 0; n < unrolled; n += 16) {
		in[0] = _mm256_mul_ps(_mm256_load_ps(&s0[n+0]), int_scale);
		in[1] = _mm256_mul_ps(_mm256_load_ps(&s1[n+0]), int_scale);
		in[2] = _mm256_mul_ps(_mm256_load_ps(&s0[n+8]), int_scale);
		in[3] = _mm256_mul_ps(_mm256_load_ps(&s1[n+8]), int_scale);

		out[0] = _mm256_cvtps_epi32(in[0]); /* a0 a1 a2 a3 a4 a5 a6 a7 */
		out[1] = _mm256_cvtps_epi32(in[1]); /* b0 b1 b2 b3 b4 b5 b6 b7 */
		out[2] = _mm256_cvtps_epi32(in[2]); /* a0 a1 a2 a3 a4 a5 a6 a7 */
		out[3] = _mm256_cvtps_epi32(in[3]); /* b0 b1 b2 b3 b4 b5 b6 b7 */

		t[0] = _mm256_unpacklo_epi32(out[0], out[1]); /* a0 b0 a1 b1 a4 b4 a5 b5 */
		t[1] = _mm256_unpackhi_epi32(out[0], out[1]); /* a2 b2 a3 b3 a6 b6 a7 b7 */
		t[2] = _mm256_unpacklo_epi32(out[2], out[3]); /* a0 b0 a1 b1 a4 b4 a5 b5 */
		t[3] = _mm256_unpackhi_epi32(out[2], out[3]); /* a2 b2 a3 b3 a6 b6 a7 b7 */

		out[0] = _mm256_packs_epi32(t[0], t[1]); /* a0 b0 a1 b1 a2 b2 a3 b3 a4 b4 a5 b5 a6 b6 a7 b7 */
		out[1] = _mm256_packs_epi32(t[2], t[3]); /* a0 b0 a1 b1 a2 b2 a3 b3 a4 b4 a5 b5 a6 b6 a7 b7 */
		out[0] = _MM256_BSWAP_EPI16(out[0]);
		out[1] = _MM256_BSWAP_EPI16(out[1]);

		_mm256_store_si256((__m256i*)(d+0), out[0]);
		_mm256_store_si256((__m256i*)(d+16), out[1]);

		d += 32;
	}
	for(; n < n_samples; n++) {
		__m128 in[4];
		__m128 int_scale = _mm_set1_ps(S16_SCALE);
		__m128 int_max = _mm_set1_ps(S16_MAX);
		__m128 int_min = _mm_set1_ps(S16_MIN);

		in[0] = _mm_mul_ss(_mm_load_ss(&s0[n]), int_scale);
		in[1] = _mm_mul_ss(_mm_load_ss(&s1[n]), int_scale);
		in[0] = _MM_CLAMP_SS(in[0], int_min, int_max);
		in[1] = _MM_CLAMP_SS(in[1], int_min, int_max);
		d[0] = bswap_16((uint16_t)_mm_cvtss_si32(in[0]));
		d[1] = bswap_16((uint16_t)_mm_cvtss_si32(in[1]));
		d += 2;
	}
}
