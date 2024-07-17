#include "FFmpegEncoder.h"

bool FFmpegEncoder::start(Params params)
{
	return true;
}

bool FFmpegEncoder::stop()
{
	return true;
}

bool FFmpegEncoder::append_video(uint8_t* image)
{
	return true;
}

bool FFmpegEncoder::append_audio(uint8_t* audio, size_t length)
{
	return true;
}
