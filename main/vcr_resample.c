//#include "../config.h"
#ifdef VCR_SUPPORT

#include "vcr_resample.h"
#include "speex/speex_resampler.h"

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static SpeexResamplerState* speex_ctx = NULL;
static short out_samps[44100 * 2 * 2]; // big enough i guess?
static int err = 0;

static int rates_changed(int cur_in, int cur_out)
{
	static spx_uint32_t in;
	static spx_uint32_t out;
	speex_resampler_get_rate(speex_ctx, &in, &out);
	return in != cur_in || out != cur_out;
}

int
VCR_getResampleLen(int dst_freq, int src_freq, int src_bitrate, int src_len)
{
	int dst_len;
	float ratio;

	// convert bitrate to 16 bits
	if (src_bitrate != 16)
	{
		if (src_bitrate != 4 && src_bitrate != 8)
			return -1;	// unknown result

		src_len = src_len * (16 / src_bitrate);
	}

	ratio = src_freq / (float)dst_freq;
	dst_len = src_len / ratio;

	return dst_len;
}

int
VCR_resample( short **dst, int dst_freq,
              const short *src, int src_freq, int src_bitrate, int src_len )
{
	if (src_bitrate != 16) abort();

	if (!speex_ctx)
	{
		speex_ctx = speex_resampler_init(2, src_freq, dst_freq, 6, &err);
	}

	if (rates_changed(src_freq, dst_freq))
	{
		speex_resampler_set_rate(speex_ctx, src_freq, dst_freq);
	}

	spx_uint32_t in_pos = (spx_uint32_t)src_len * 2;
	spx_uint32_t out_pos = sizeof (out_samps);
	speex_resampler_process_interleaved_int(speex_ctx, src, &in_pos, out_samps, &out_pos);

	*dst = out_samps;
	return (int)out_pos / 2;
}

#endif // VCR_SUPPORT
