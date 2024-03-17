#pragma once
#include "Encoder.h"

class FFmpegEncoder : public Encoder
{
public:
	bool start(Params params) override;
	bool stop() override;
	bool append_video(uint8_t* image) override;
	bool append_audio(uint8_t* audio, size_t length) override;
};
