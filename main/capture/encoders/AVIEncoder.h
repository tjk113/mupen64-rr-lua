#pragma once
#include "Encoder.h"


#include <Windows.h>
#include <Vfw.h>

class AVIEncoder final : public Encoder
{
public:
	bool start(Params params) override;
	bool stop() override;
	bool append_video(uint8_t* image) override;
	bool append_audio(uint8_t* audio, size_t length) override;

private:
	static constexpr auto max_avi_size = 0x7B9ACA00;

	bool save_options();
	bool load_options();

	AVICOMPRESSOPTIONS* avi_options = new AVICOMPRESSOPTIONS();
	Encoder::Params video_params{};
	size_t splits = 0;
	bool splitting = false;
	bool avi_opened = false;
	int frame;
	BITMAPINFOHEADER infoHeader;
	PAVIFILE avi_file;
	AVISTREAMINFO video_stream_header;
	PAVISTREAM video_stream;
	PAVISTREAM compressed_video_stream;
	int sample;
	unsigned int AVIFileSize;
	WAVEFORMATEX sound_format;
	AVISTREAMINFO sound_stream_header;
	PAVISTREAM sound_stream;


};
