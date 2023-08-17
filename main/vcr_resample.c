//#include "../config.h"
#ifdef VCR_SUPPORT

#include "vcr_resample.h"
#include "speex/speex_resampler.h"

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static SpeexResamplerState* speex_ctx = NULL;
static short in_samps[44100 * 2 * 2]; // big enough i guess?
static short out_samps[44100 * 2 * 2]; // big enough i guess?
static int err = 0;

static int rates_changed(int cur_in, int cur_out) {
	static spx_uint32_t in;
	static spx_uint32_t out;
	speex_resampler_get_rate(speex_ctx, &in, &out);
	return in != (unsigned int) cur_in || out != (unsigned int) cur_out;
}

int
VCR_getResampleLen(int dst_freq, int src_freq, int src_bitrate, int src_len) {
	int dst_len;
	long double ratio;

	// convert bitrate to 16 bits
	if (src_bitrate != 16) {
		if (src_bitrate != 4 && src_bitrate != 8)
			return -1;	// unknown result

		src_len = src_len * (16 / src_bitrate);
	}

	ratio = src_freq / (long double)dst_freq;
	dst_len = src_len / ratio;

	return dst_len;
}

int
VCR_resample(short** dst, int dst_freq,
	const short* src, int src_freq, int src_bitrate, int src_len) {
	if (src_bitrate != 16) {
		puts("[VCR]: resample: Bitrate is not 16 bits");
		return -1;
	}

	// for some reason channels are swapped in src (endianess weirdness?)
	// fix it here
	for (unsigned i = 0, samps = src_len >> 1; i < samps; i += 2) {
		in_samps[i + 1] = src[i + 0];
		in_samps[i + 0] = src[i + 1];
	}

	if (!speex_ctx) {
		speex_ctx = speex_resampler_init(2, src_freq, dst_freq, 6, &err);
	}

	if (rates_changed(src_freq, dst_freq)) {
		speex_resampler_set_rate(speex_ctx, src_freq, dst_freq);
	}

	spx_uint32_t in_pos = (spx_uint32_t)src_len / 4;
	spx_uint32_t out_pos = sizeof(out_samps) / 4;
	speex_resampler_process_interleaved_int(speex_ctx, in_samps, &in_pos, out_samps, &out_pos);

	*dst = out_samps;
	return (int)out_pos * 4;
}

#endif // VCR_SUPPORT
