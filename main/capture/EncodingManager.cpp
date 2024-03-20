#include "EncodingManager.h"

#include <cassert>

#include <guifuncs.h>
#include "messenger.h"
#include "Resampler.h"
#include "../../r4300/Plugin.hpp"
#include "win/main_win.h"
#include "../../r4300/rom.h"
#include "win/Config.hpp"
#include "../../memory/memory.h"
#include "encoders/AVIEncoder.h"
#include "encoders/Encoder.h"
#include "encoders/FFmpegEncoder.h"
#include "win/features/MGECompositor.h"

namespace EncodingManager
{
	// 44100=1s sample, soundbuffer capable of holding 4s future data in circular buffer
#define SOUND_BUF_SIZE (44100*2*2)

	const auto ffmpeg_path = "C:\\ffmpeg\\bin\\ffmpeg.exe";

	// 0x30018
	int m_audio_freq = 33000;
	int m_audio_bitrate = 16;
	long double m_video_frame = 0;
	long double m_audio_frame = 0;

	char sound_buf[SOUND_BUF_SIZE];
	char sound_buf_empty[SOUND_BUF_SIZE];
	int sound_buf_pos = 0;
	long last_sound = 0;

	size_t split_count = 0;

	bool capturing = false;
	EncoderType current_container;
	std::filesystem::path capture_path;

	std::unique_ptr<Encoder> m_encoder;


	/**
	 * \brief Writes the window contents overlaid with lua into the front buffer
	 * \param dest The buffer holding video data of size <c>width * height</c>
	 * \param width The buffer's width
	 * \param height The buffer's height
	 */
	void readscreen_window(void** dest, long* width, long* height)
	{
		const auto info = get_window_info();
		*width = info.width & ~3;
		*height = info.height & ~3;
		*dest = (uint8_t*)malloc(*width * *height * 3 + 1);

		HDC dc = GetDC(mainHWND);
		HDC compat_dc = CreateCompatibleDC(dc);
		HBITMAP bitmap = CreateCompatibleBitmap(dc, *width, *height);

		SelectObject(compat_dc, bitmap);

		BitBlt(compat_dc, 0, 0, *width, *height, dc, 0, 0, SRCCOPY);

		BITMAPINFO bmp_info{};
		bmp_info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bmp_info.bmiHeader.biWidth = info.width;
		bmp_info.bmiHeader.biHeight = info.height;
		bmp_info.bmiHeader.biPlanes = 1;
		bmp_info.bmiHeader.biBitCount = 24;
		bmp_info.bmiHeader.biCompression = BI_RGB;

		GetDIBits(compat_dc, bitmap, 0, *height, *dest, &bmp_info, DIB_RGB_COLORS);

		SelectObject(compat_dc, nullptr);
		DeleteObject(bitmap);
		DeleteDC(compat_dc);
		ReleaseDC(mainHWND, dc);
	}

	/**
	 * \brief Writes the desktop contents cropped to the window into the front buffer
	 * \param dest The buffer holding video data of size <c>width * height</c>
	 * \param width The buffer's width
	 * \param height The buffer's height
	 */
	void readscreen_desktop(void** dest, long* width, long* height)
	{
		const auto info = get_window_info();
		*width = info.width & ~3;
		*height = info.height & ~3;
		*dest = (uint8_t*)malloc(*width * *height * 3 + 1);

		POINT pt{};
		ClientToScreen(mainHWND, &pt);

		HDC dc = GetDC(nullptr);
		HDC compat_dc = CreateCompatibleDC(dc);
		HBITMAP bitmap = CreateCompatibleBitmap(dc, *width, *height);

		SelectObject(compat_dc, bitmap);

		BitBlt(compat_dc, 0, 0, *width, *height, dc, pt.x,
		       pt.y + (info.height - *height), SRCCOPY);

		BITMAPINFO bmp_info{};
		bmp_info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bmp_info.bmiHeader.biWidth = info.width;
		bmp_info.bmiHeader.biHeight = info.height;
		bmp_info.bmiHeader.biPlanes = 1;
		bmp_info.bmiHeader.biBitCount = 24;
		bmp_info.bmiHeader.biCompression = BI_RGB;

		GetDIBits(compat_dc, bitmap, 0, *height, *dest, &bmp_info, DIB_RGB_COLORS);

		SelectObject(compat_dc, nullptr);
		DeleteObject(bitmap);
		DeleteDC(compat_dc);
		ReleaseDC(nullptr, dc);
	}

	// assumes: len <= writeSize
	void write_sound(char* buf, int len, const int min_write_size,
	                 const int max_write_size,
	                 const BOOL force)
	{
		if ((len <= 0 && !force) || len > max_write_size)
			return;

		if (sound_buf_pos + len > min_write_size || force)
		{
			if (int len2 = vcr_get_resample_len(44100, m_audio_freq,
			                                    m_audio_bitrate,
			                                    sound_buf_pos); (len2 % 8) == 0
				|| len > max_write_size)
			{
				static short* buf2 = nullptr;
				len2 = vcr_resample(&buf2, 44100,
				                    reinterpret_cast<short*>(sound_buf),
				                    m_audio_freq,
				                    m_audio_bitrate, sound_buf_pos);
				if (len2 > 0)
				{
					if ((len2 % 4) != 0)
					{
						printf(
							"[EncodingManager]: Warning: Possible stereo sound error detected.\n");
						fprintf(
							stderr,
							"[EncodingManager]: Warning: Possible stereo sound error detected.\n");
					}
					if (!m_encoder->append_audio((unsigned char*)buf2, len2))
					{
						show_modal_info(
							"Audio output failure!\nA call to addAudioData() (AVIStreamWrite) failed.\nPerhaps you ran out of memory?",
							nullptr);
						stop_capture();
					}
				}
				sound_buf_pos = 0;
			}
		}

		if (len > 0)
		{
			if ((unsigned int)(sound_buf_pos + len) > SOUND_BUF_SIZE * sizeof(
				char))
			{
				MessageBox(nullptr, "Fatal error", "Sound buffer overflow",
				           MB_ICONERROR);
				printf("SOUND BUFFER OVERFLOW\n");
				return;
			}
#ifdef _DEBUG
			else
			{
				long double pro = (long double)(sound_buf_pos + len) * 100 / (
					SOUND_BUF_SIZE * sizeof(char));
				if (pro > 75) printf("---!!!---");
				printf("sound buffer: %.2f%%\n", pro);
			}
#endif
			memcpy(sound_buf + sound_buf_pos, (char*)buf, len);
			sound_buf_pos += len;
			m_audio_frame += ((len / 4) / (long double)m_audio_freq) *
				get_vis_per_second(ROM_HEADER.Country_code);
		}
	}

	bool is_capturing()
	{
		return capturing;
	}

	auto effective_readscreen()
	{
		if (Config.capture_mode == 0)
		{
			return MGECompositor::available() ? MGECompositor::read_screen : readScreen;
		}
		if (Config.capture_mode == 1)
		{
			return readscreen_window;
		}
		return readscreen_desktop;
	}

	void dummy_free(void*)
	{
	};

	auto effective_readscreen_free()
	{
		if (Config.capture_mode == 0)
		{
			return MGECompositor::available() ? dummy_free : DllCrtFree;
		}
		return free;
	}

	bool start_capture(std::filesystem::path path, EncoderType encoder_type,
	                   const bool ask_for_encoding_settings)
	{
		if (!effective_readscreen())
		{
			MessageBox(mainHWND, "Couldn't find a readScreen candidate.\r\nTry selecting another capture mode.", "Capture", MB_ICONERROR | MB_OK);
			return false;
		}

		switch (encoder_type)
		{
		case EncoderType::VFW:
			m_encoder = std::make_unique<AVIEncoder>();
			break;
		case EncoderType::FFmpeg:
			m_encoder = std::make_unique<FFmpegEncoder>();
			break;
		default:
			assert(false);
		}

		current_container = encoder_type;

		memset(sound_buf_empty, 0, sizeof(sound_buf_empty));
		memset(sound_buf, 0, sizeof(sound_buf));
		last_sound = 0;
		m_video_frame = 0.0;
		m_audio_frame = 0.0;

		// If we are capturing internally, we get our dimensions from the window, otherwise from the GFX plugin
		long width = 0, height = 0;
		void* dummy;
		effective_readscreen()(&dummy, &width, &height);

		auto result = m_encoder->start(Encoder::Params{
			.path = path.string().c_str(),
			.width = (uint32_t)width,
			.height = (uint32_t)height,
			.fps = get_vis_per_second(ROM_HEADER.Country_code),
			.ask_for_encoding_settings = ask_for_encoding_settings
		});

		if (!result)
		{
			return false;
		}

		capturing = true;
		capture_path = path;

		Messenger::broadcast(Messenger::Message::CapturingChanged, true);

		printf("[EncodingManager]: Starting capture...\n");

		return true;
	}

	bool stop_capture()
	{
		write_sound(nullptr, 0, m_audio_freq, m_audio_freq * 2, TRUE);

		if (!m_encoder->stop())
		{
			return false;
		}

		m_encoder.release();

		split_count = 0;

		capturing = false;
		Messenger::broadcast(Messenger::Message::CapturingChanged, false);

		printf("[EncodingManager]: Capture finished.\n");
		return true;
	}


	void at_vi()
	{
		if (!capturing)
		{
			return;
		}

		if (Config.capture_delay)
		{
			Sleep(Config.capture_delay);
		}

		void* image = nullptr;
		long width = 0, height = 0;

		const auto start = std::chrono::high_resolution_clock::now();
		effective_readscreen()(&image, &width, &height);
		const auto end = std::chrono::high_resolution_clock::now();
		const std::chrono::duration<double, std::milli> time = (end - start);
		printf("ReadScreen: %lf ms\n", time.count());

		if (image == nullptr)
		{
			printf(
				"[EncodingManager]: Couldn't read screen (out of memory?)\n");
			return;
		}

		if (Config.synchronization_mode != (int)Sync::Audio && Config.
			synchronization_mode != (int)Sync::None)
		{
			return;
		}

		// AUDIO SYNC
		// This type of syncing assumes the audio is authoratative, and drops or duplicates frames to keep the video as close to
		// it as possible. Some games stop updating the screen entirely at certain points, such as loading zones, which will cause
		// audio to drift away by default. This method of syncing prevents this, at the cost of the video feed possibly freezing or jumping
		// (though in practice this rarely happens - usually a loading scene just appears shorter or something).

		int audio_frames = (int)(m_audio_frame - m_video_frame + 0.1);
		// i've seen a few games only do ~0.98 frames of audio for a frame, let's account for that here

		if (Config.synchronization_mode == (int)Sync::Audio)
		{
			if (audio_frames < 0)
			{
				show_modal_info("Audio frames became negative!", nullptr);
				stop_capture();
				goto cleanup;
			}

			if (audio_frames == 0)
			{
				printf("\nDropped Frame! a/v: %Lg/%Lg", m_video_frame,
				       m_audio_frame);
			} else if (audio_frames > 0)
			{
				if (!m_encoder->append_video((unsigned char*)image))
				{
					show_modal_info(
						"Video codec failure!\nA call to addVideoFrame() (AVIStreamWrite) failed.\nPerhaps you ran out of memory?",
						nullptr);
					stop_capture();
					goto cleanup;
				}
				m_video_frame += 1.0;
				audio_frames--;
			}

			// can this actually happen?
			while (audio_frames > 0)
			{
				if (!m_encoder->append_video((unsigned char*)image))
				{
					show_modal_info(
						"Video codec failure!\nA call to addVideoFrame() (AVIStreamWrite) failed.\nPerhaps you ran out of memory?",
						nullptr);
					stop_capture();
					goto cleanup;
				}
				printf("\nDuped Frame! a/v: %Lg/%Lg", m_video_frame,
				       m_audio_frame);
				m_video_frame += 1.0;
				audio_frames--;
			}
		} else
		{
			if (!m_encoder->append_video((unsigned char*)image))
			{
				show_modal_info(
					"Video codec failure!\nA call to addVideoFrame() (AVIStreamWrite) failed.\nPerhaps you ran out of memory?",
					nullptr);
				stop_capture();
				goto cleanup;
			}
			m_video_frame += 1.0;
		}

	cleanup:
		effective_readscreen_free()(image);
	}

	void ai_len_changed()
	{
		const auto p = reinterpret_cast<short*>((char*)rdram + (ai_register.
			ai_dram_addr & 0xFFFFFF));
		const auto buf = (char*)p;
		const int ai_len = (int)ai_register.ai_len;

		m_audio_bitrate = (int)ai_register.ai_bitrate + 1;

		if (!capturing)
		{
			return;
		}


		if (ai_len <= 0)
			return;

		const int len = ai_len;
		const int write_size = 2 * m_audio_freq;
		// we want (writeSize * 44100 / m_audioFreq) to be an integer

		if (Config.synchronization_mode == (int)Sync::Video || Config.
			synchronization_mode == (int)Sync::None)
		{
			// VIDEO SYNC
			// This is the original syncing code, which adds silence to the audio track to get it to line up with video.
			// The N64 appears to have the ability to arbitrarily disable its sound processing facilities and no audio samples
			// are generated. When this happens, the video track will drift away from the audio. This can happen at load boundaries
			// in some games, for example.
			//
			// The only new difference here is that the desync flag is checked for being greater than 1.0 instead of 0.
			// This is because the audio and video in mupen tend to always be diverged just a little bit, but stay in sync
			// over time. Checking if desync is not 0 causes the audio stream to to get thrashed which results in clicks
			// and pops.

			long double desync = m_video_frame - m_audio_frame;

			if (Config.synchronization_mode == (int)Sync::None) // HACK
				desync = 0;

			if (desync > 1.0)
			{
				printf(
					"[EncodingManager]: Correcting for A/V desynchronization of %+Lf frames\n",
					desync);
				int len3 = (int)(m_audio_freq / (long double)
						get_vis_per_second(ROM_HEADER.Country_code)) * (int)
					desync;
				len3 <<= 2;
				const int empty_size =
					len3 > write_size ? write_size : len3;

				for (int i = 0; i < empty_size; i += 4)
					*reinterpret_cast<long*>(sound_buf_empty + i) =
						last_sound;

				while (len3 > write_size)
				{
					write_sound(sound_buf_empty, write_size, m_audio_freq,
					            write_size,
					            FALSE);
					len3 -= write_size;
				}
				write_sound(sound_buf_empty, len3, m_audio_freq, write_size,
				            FALSE);
			} else if (desync <= -10.0)
			{
				printf(
					"[EncodingManager]: Waiting from A/V desynchronization of %+Lf frames\n",
					desync);
			}
		}

		write_sound(static_cast<char*>(buf), len, m_audio_freq, write_size,
		            FALSE);
		last_sound = *(reinterpret_cast<long*>(buf + len) - 1);
	}

	void ai_dacrate_changed(system_type type)
	{
		if (capturing)
		{
			printf("Fatal error, audio frequency changed during capture\n");
			stop_capture();
			return;
		}

		m_audio_bitrate = (int)ai_register.ai_bitrate + 1;
		switch (type)
		{
		case ntsc:
			m_audio_freq = (int)(48681812 / (ai_register.ai_dacrate + 1));
			break;
		case pal:
			m_audio_freq = (int)(49656530 / (ai_register.ai_dacrate + 1));
			break;
		case mpal:
			m_audio_freq = (int)(48628316 / (ai_register.ai_dacrate + 1));
			break;
		default:
			assert(false);
			break;
		}
	}
}
