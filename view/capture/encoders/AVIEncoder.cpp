#include "AVIEncoder.h"

#include <view/helpers/IOHelpers.h>
#include <view/gui/Main.h>

bool AVIEncoder::start(Params params)
{
	if (!m_splitting)
	{
		m_params = params;
	}
	m_avi_file_size = 0;
	m_frame = 0;
	m_info_hdr.biSize = sizeof(BITMAPINFOHEADER);
	m_info_hdr.biWidth = params.width;
	m_info_hdr.biHeight = params.height;
	m_info_hdr.biPlanes = 1;
	m_info_hdr.biBitCount = 24;
	m_info_hdr.biCompression = BI_RGB;
	m_info_hdr.biSizeImage = params.width * params.height * 3;
	m_info_hdr.biXPelsPerMeter = 0;
	m_info_hdr.biYPelsPerMeter = 0;
	m_info_hdr.biClrUsed = 0;
	m_info_hdr.biClrImportant = 0;

	AVIFileInit();
	AVIFileOpen(&m_avi_file, params.path.string().c_str(), OF_WRITE | OF_CREATE,
	            NULL);

	ZeroMemory(&m_video_stream_hdr, sizeof(AVISTREAMINFO));
	m_video_stream_hdr.fccType = streamtypeVIDEO;
	m_video_stream_hdr.dwScale = 1;
	m_video_stream_hdr.dwRate = params.fps;
	m_video_stream_hdr.dwSuggestedBufferSize = 0;
	AVIFileCreateStream(m_avi_file, &m_video_stream, &m_video_stream_hdr);

	if (params.ask_for_encoding_settings)
	{
		if (!AVISaveOptions(g_main_hwnd, 0, 1, &m_video_stream, &m_avi_options))
		{
			printf("[AVIEncoder] Failed to save options\n");
			return false;
		}
	} else
	{
		if (!load_options())
		{
			printf("[AVIEncoder] Failed to load options\n");
			return false;
		}
	}

	save_options();
	AVIMakeCompressedStream(&m_compressed_video_stream, m_video_stream,
	                        m_avi_options, NULL);
	AVIStreamSetFormat(m_compressed_video_stream, 0, &m_info_hdr,
	                   m_info_hdr.biSize + m_info_hdr.biClrUsed * sizeof(
		                   RGBQUAD));

	m_sample = 0;
	m_sound_format.wFormatTag = WAVE_FORMAT_PCM;
	m_sound_format.nChannels = 2;
	m_sound_format.nSamplesPerSec = 44100;
	m_sound_format.nAvgBytesPerSec = 44100 * (2 * 16 / 8);
	m_sound_format.nBlockAlign = 2 * 16 / 8;
	m_sound_format.wBitsPerSample = 16;
	m_sound_format.cbSize = 0;

	ZeroMemory(&m_sound_stream_hdr, sizeof(AVISTREAMINFO));
	m_sound_stream_hdr.fccType = streamtypeAUDIO;
	m_sound_stream_hdr.dwQuality = (DWORD)-1;
	m_sound_stream_hdr.dwScale = m_sound_format.nBlockAlign;
	m_sound_stream_hdr.dwInitialFrames = 1;
	m_sound_stream_hdr.dwRate = m_sound_format.nAvgBytesPerSec;
	m_sound_stream_hdr.dwSampleSize = m_sound_format.nBlockAlign;
	AVIFileCreateStream(m_avi_file, &m_sound_stream, &m_sound_stream_hdr);
	AVIStreamSetFormat(m_sound_stream, 0, &m_sound_format, sizeof(WAVEFORMATEX));
	return true;
}

bool AVIEncoder::stop()
{
	AVIStreamClose(m_compressed_video_stream);
	AVIStreamRelease(m_video_stream);
	AVIStreamClose(m_sound_stream);
	AVIFileClose(m_avi_file);
	AVIFileExit();

	return true;
}

bool AVIEncoder::append_video(uint8_t* image)
{
	if (m_avi_file_size > MAX_AVI_SIZE)
	{
		// If AVI file gets too big, it will corrupt or crash. Since this is a limitation the caller shouldn't care about, we split the recording silently.
		// When splitting, filenames follow this pattern: <fname> <n>.avi
		Params new_params = m_params;
		new_params.path = with_name(new_params.path,
		                            get_name(m_params.path) + " " +
		                            std::to_string(++m_splits));

		m_splitting = true;
		stop();
		start(new_params);
		m_splitting = false;
	}
	LONG written_len;
	BOOL ret = AVIStreamWrite(m_compressed_video_stream, m_frame++, 1, image,
	                          m_info_hdr.biSizeImage, AVIIF_KEYFRAME, NULL,
	                          &written_len);
	m_avi_file_size += written_len;
	m_params.video_free(image);
	return !ret;
}

bool AVIEncoder::append_audio(uint8_t* audio, size_t length)
{
	BOOL ok = (0 == AVIStreamWrite(m_sound_stream, m_sample,
	                               length / m_sound_format.nBlockAlign, audio, length, 0,
	                               NULL, NULL));
	m_sample += length / m_sound_format.nBlockAlign;
	m_avi_file_size += length;
	m_params.audio_free(audio);
	return ok;
}

bool AVIEncoder::save_options()
{
	FILE* f = fopen("avi.cfg", "wb");
	fwrite(m_avi_options, sizeof(AVICOMPRESSOPTIONS), 1, f);
	fwrite(m_avi_options->lpParms, m_avi_options->cbParms, 1, f);
	fclose(f);
	return true;
}

bool AVIEncoder::load_options()
{
	FILE* f = fopen("avi.cfg", "rb");

	if (!f)
		return false;

	fseek(f, 0, SEEK_END);

	// Too small...
	if (ftell(f) < sizeof(AVICOMPRESSOPTIONS))
		goto error;

	fseek(f, 0, SEEK_SET);

	fread(m_avi_options, sizeof(AVICOMPRESSOPTIONS), 1, f);

	{
		void* moreOptions = malloc(m_avi_options->cbParms);
		fread(moreOptions, m_avi_options->cbParms, 1, f);
		m_avi_options->lpParms = moreOptions;
	}

	fclose(f);
	return true;

error:
	fclose(f);
	return false;
}
