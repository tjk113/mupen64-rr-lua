#include "EncodingManager.h"

#include <cassert>

#include <shared/services/FrontendService.h>
#include <shared/Messenger.h>
#include "Resampler.h"
#include <core/r4300/Plugin.hpp>
#include <view/gui/Main.h>
#include <core/r4300/rom.h>
#include <shared/Config.hpp>
#include <core/memory/memory.h>
#include "encoders/AVIEncoder.h"
#include "encoders/Encoder.h"
#include "encoders/FFmpegEncoder.h"
#include <view/gui/features/MGECompositor.h>
#include <view/lua/LuaConsole.h>
#include <view/gui/features/Dispatcher.h>
#include <view/helpers/IOHelpers.h>

namespace EncodingManager
{
    // 44100=1s sample, soundbuffer capable of holding 4s future data in circular buffer
#define SOUND_BUF_SIZE (44100*2*2)

    // 0x30018
    int m_audio_freq = 33000;
    int m_audio_bitrate = 16;
    long double m_video_frame = 0;
    long double m_audio_frame = 0;

    // Video buffer, allocated once when recording starts and freed when it ends.
    uint8_t* m_video_buf = nullptr;
    long m_video_width;
    long m_video_height;

    char sound_buf[SOUND_BUF_SIZE];
    char sound_buf_empty[SOUND_BUF_SIZE];
    int sound_buf_pos = 0;
    long last_sound = 0;

    std::atomic m_capturing = false;
    EncoderType m_encoder_type;
    std::unique_ptr<Encoder> m_encoder;

    void readscreen_plugin()
    {
        if (MGECompositor::available())
        {
            MGECompositor::copy_video(m_video_buf);
        }
        else
        {
            void* buf = nullptr;
            long width;
            long height;
            readScreen(&buf, &width, &height);
            DllCrtFree(buf);
        }
    }

    void readscreen_window()
    {
        HDC dc = GetDC(g_main_hwnd);
        HDC compat_dc = CreateCompatibleDC(dc);
        HBITMAP bitmap = CreateCompatibleBitmap(dc, m_video_width, m_video_height);

        SelectObject(compat_dc, bitmap);

        BitBlt(compat_dc, 0, 0, m_video_width, m_video_height, dc, 0, 0, SRCCOPY);

        BITMAPINFO bmp_info{};
        bmp_info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmp_info.bmiHeader.biWidth = m_video_width;
        bmp_info.bmiHeader.biHeight = m_video_height;
        bmp_info.bmiHeader.biPlanes = 1;
        bmp_info.bmiHeader.biBitCount = 24;
        bmp_info.bmiHeader.biCompression = BI_RGB;

        GetDIBits(compat_dc, bitmap, 0, m_video_height, m_video_buf, &bmp_info, DIB_RGB_COLORS);

        SelectObject(compat_dc, nullptr);
        DeleteObject(bitmap);
        DeleteDC(compat_dc);
        ReleaseDC(g_main_hwnd, dc);
    }

    void readscreen_desktop()
    {
        POINT pt{};
        ClientToScreen(g_main_hwnd, &pt);

        HDC dc = GetDC(nullptr);
        HDC compat_dc = CreateCompatibleDC(dc);
        HBITMAP bitmap = CreateCompatibleBitmap(dc, m_video_width, m_video_height);

        SelectObject(compat_dc, bitmap);

        BitBlt(compat_dc, 0, 0, m_video_width, m_video_height, dc, pt.x,
               pt.y, SRCCOPY);

        BITMAPINFO bmp_info{};
        bmp_info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmp_info.bmiHeader.biWidth = m_video_width;
        bmp_info.bmiHeader.biHeight = m_video_height;
        bmp_info.bmiHeader.biPlanes = 1;
        bmp_info.bmiHeader.biBitCount = 24;
        bmp_info.bmiHeader.biCompression = BI_RGB;

        GetDIBits(compat_dc, bitmap, 0, m_video_height, m_video_buf, &bmp_info, DIB_RGB_COLORS);

        SelectObject(compat_dc, nullptr);
        DeleteObject(bitmap);
        DeleteDC(compat_dc);
        ReleaseDC(nullptr, dc);
    }

    void readscreen_hybrid()
    {
        if (MGECompositor::available())
        {
            MGECompositor::copy_video(m_video_buf);
        }
        else
        {
            void* buf = nullptr;
            long width;
            long height;
            readScreen(&buf, &width, &height);
            memcpy(m_video_buf, buf, width * height * 3);
            DllCrtFree(buf);
        }

        long raw_video_width, raw_video_height;
        MGECompositor::get_video_size(&raw_video_width, &raw_video_height);

        BITMAPINFO rs_bmp_info{};
        rs_bmp_info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        rs_bmp_info.bmiHeader.biPlanes = 1;
        rs_bmp_info.bmiHeader.biBitCount = 24;
        rs_bmp_info.bmiHeader.biCompression = BI_RGB;
        rs_bmp_info.bmiHeader.biWidth = m_video_width;
        rs_bmp_info.bmiHeader.biHeight = m_video_height;

        // UI resources, must be accessed from UI thread
        // To avoid GDI weirdness with cross-thread resources, we do all GDI work on UI thread.
        g_main_window_dispatcher->invoke([&]
        {
            // Since atupdatescreen might not have occured for a long time, we force it now.
            // This avoids "outdated" visuals, which are otherwise acceptable during normal gameplay, being blitted to the video stream.
            for (auto& pair : g_hwnd_lua_map)
            {
                pair.second->repaint_visuals();
            }

            GdiFlush();

            HDC dc = GetDC(g_main_hwnd);
            HDC compat_dc = CreateCompatibleDC(dc);
            HBITMAP bitmap = CreateCompatibleBitmap(dc, m_video_width, m_video_height);
            SelectObject(compat_dc, bitmap);

            {
                BITMAPINFO bmp_info{};
                bmp_info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                bmp_info.bmiHeader.biPlanes = 1;
                bmp_info.bmiHeader.biBitCount = 24;
                bmp_info.bmiHeader.biWidth = raw_video_width;
                bmp_info.bmiHeader.biHeight = raw_video_height;
                bmp_info.bmiHeader.biCompression = BI_RGB;

                // Copy the raw readscreen output
                StretchDIBits(compat_dc,
                                      0,
                                      0,
                                      raw_video_width,
                                      raw_video_height,
                                      0, 0,
                                      raw_video_width,
                                      raw_video_height,
                                      m_video_buf,
                                      &bmp_info,
                                      DIB_RGB_COLORS,
                                      SRCCOPY);
            }

            // First, composite the lua's dxgi surfaces
            for (auto& pair : g_hwnd_lua_map)
            {
                pair.second->presenter->blit(compat_dc, {0, 0, (LONG)pair.second->presenter->size().width, (LONG)pair.second->presenter->size().height});
            }

            // Then, blit the GDI back DCs
            for (auto& pair : g_hwnd_lua_map)
            {
                TransparentBlt(compat_dc, 0, 0, pair.second->dc_size.width, pair.second->dc_size.height, pair.second->gdi_back_dc, 0, 0, pair.second->dc_size.width, pair.second->dc_size.height, lua_gdi_color_mask);
            }

            BITMAPINFO bmp_info{};
            bmp_info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            bmp_info.bmiHeader.biWidth = m_video_width;
            bmp_info.bmiHeader.biHeight = m_video_height;
            bmp_info.bmiHeader.biPlanes = 1;
            bmp_info.bmiHeader.biBitCount = 24;
            bmp_info.bmiHeader.biCompression = BI_RGB;

            GetDIBits(compat_dc, bitmap, 0, m_video_height, m_video_buf, &bmp_info, DIB_RGB_COLORS);

            SelectObject(compat_dc, nullptr);
            DeleteObject(bitmap);
            DeleteDC(compat_dc);
            ReleaseDC(g_main_hwnd, dc);
        });
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
			int len2 = rsmp_get_resample_len(44100, m_audio_freq, m_audio_bitrate, sound_buf_pos);
            if ((len2 % 8) == 0 || len > max_write_size)
            {
                static short* buf2 = nullptr;
                len2 = rsmp_resample(&buf2, 44100, reinterpret_cast<short*>(sound_buf), m_audio_freq, m_audio_bitrate, sound_buf_pos);
                if (len2 > 0)
                {
                    if ((len2 % 4) != 0)
                    {
                        g_view_logger->info(
                            "[EncodingManager]: Warning: Possible stereo sound error detected.\n");
                        fprintf(
                            stderr,
                            "[EncodingManager]: Warning: Possible stereo sound error detected.\n");
                    }
                    if (!m_encoder->append_audio(reinterpret_cast<uint8_t*>(buf2), len2))
                    {
                        FrontendService::show_information(
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
                FrontendService::show_error("Sound buffer overflow");
                g_view_logger->info("SOUND BUFFER OVERFLOW");
                return;
            }
#ifdef _DEBUG
            else
            {
                long double pro = (long double)(sound_buf_pos + len) * 100 / (
                    SOUND_BUF_SIZE * sizeof(char));
                if (pro > 75) g_view_logger->warn("Audio buffer almost full ({:.0f}%)!", pro);
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
        return m_capturing;
    }

    void read_screen()
    {
        if (g_config.capture_mode == 0)
        {
            readscreen_plugin();
        }
        else if (g_config.capture_mode == 1)
        {
            readscreen_window();
        }
        else if (g_config.capture_mode == 2)
        {
            readscreen_desktop();
        }
        else if (g_config.capture_mode == 3)
        {
            readscreen_hybrid();
        }
        else
        {
            assert(false);
        }
    }

    void get_video_dimensions(long* width, long* height)
    {
        if (g_config.capture_mode == 0)
        {
            if (MGECompositor::available())
            {
                MGECompositor::get_video_size(width, height);
            }
            else
            {
                void* buf = nullptr;
                readScreen(&buf, width, height);
                DllCrtFree(buf);
            }
        }
        else if (g_config.capture_mode == 1 || g_config.capture_mode == 2 || g_config.capture_mode == 3)
        {
            const auto info = get_window_info();
            *width = info.width & ~3;
            *height = info.height & ~3;
        }
        else
        {
            assert(false);
        }
    }

    bool start_capture(std::filesystem::path path, EncoderType encoder_type,
                       const bool ask_for_encoding_settings)
    {
        assert(is_on_gui_thread());

        if (is_capturing())
        {
            if (!stop_capture())
            {
                g_view_logger->info("[EncodingManager]: Couldn't start capture because the previous capture couldn't be stopped.");
                return false;
            }
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

        m_encoder_type = encoder_type;

        memset(sound_buf_empty, 0, sizeof(sound_buf_empty));
        memset(sound_buf, 0, sizeof(sound_buf));
        last_sound = 0;
        m_video_frame = 0.0;
        m_audio_frame = 0.0;

        free(m_video_buf);
        get_video_dimensions(&m_video_width, &m_video_height);
        m_video_buf = (uint8_t*)malloc(m_video_width * m_video_height * 3);

        auto result = m_encoder->start(Encoder::Params{
            .path = path.string().c_str(),
            .width = (uint32_t)m_video_width,
            .height = (uint32_t)m_video_height,
            .fps = get_vis_per_second(ROM_HEADER.Country_code),
            .arate = (uint32_t)m_audio_freq,
            .ask_for_encoding_settings = ask_for_encoding_settings,
        });

        if (!result)
        {
            FrontendService::show_error("Failed to start encoding.\r\nVerify that the encoding parameters are valid and try again.", "Capture");
            return false;
        }

        m_capturing = true;

        Messenger::broadcast(Messenger::Message::CapturingChanged, true);

        g_view_logger->info("[EncodingManager]: Starting capture at {} x {}...", m_video_width, m_video_height);

        return true;
    }

    bool stop_capture()
    {
        assert(is_on_gui_thread());

        if (!is_capturing())
        {
            return true;
        }

        write_sound(nullptr, 0, m_audio_freq, m_audio_freq * 2, TRUE);

        if (!m_encoder->stop())
        {
            FrontendService::show_error("Failed to stop encoding.", "Capture");
            return false;
        }

        m_encoder.release();

        m_capturing = false;
        Messenger::broadcast(Messenger::Message::CapturingChanged, false);

        g_view_logger->info("[EncodingManager]: Capture finished.");
        return true;
    }


    void at_vi()
    {
        assert(is_on_gui_thread());

        if (!m_capturing)
        {
            return;
        }

        if (g_config.capture_delay)
        {
            Sleep(g_config.capture_delay);
        }

        read_screen();

        if (g_config.encoder_type == static_cast<int32_t>(EncoderType::FFmpeg))
        {
			m_encoder->append_video(m_video_buf);
			return;
        }

        if (g_config.synchronization_mode != (int)Sync::Audio && g_config.
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

        if (g_config.synchronization_mode == (int)Sync::Audio)
        {
            if (audio_frames < 0)
            {
                FrontendService::show_error("Audio frames became negative!", "Capture");
                stop_capture();
                return;
            }

            if (audio_frames == 0)
            {
                g_view_logger->info("Dropped Frame! a/v: %Lg/%Lg", m_video_frame, m_audio_frame);
            }
            else if (audio_frames > 0)
            {
                if (!m_encoder->append_video(m_video_buf))
                {
                    FrontendService::show_error(
                        "Failed to append frame to video.\nPerhaps you ran out of memory?",
                        "Capture");
                    stop_capture();
                    return;
                }
                m_video_frame += 1.0;
                audio_frames--;
            }

            // can this actually happen?
            while (audio_frames > 0)
            {
                if (!m_encoder->append_video(m_video_buf))
                {
                    FrontendService::show_error(
                        "Failed to append frame to video.\nPerhaps you ran out of memory?",
                        "Capture");
                    stop_capture();
                    return;
                }
                g_view_logger->info("Duped Frame! a/v: %Lg/%Lg", m_video_frame,
                       m_audio_frame);
                m_video_frame += 1.0;
                audio_frames--;
            }
        }
        else
        {
            if (!m_encoder->append_video(m_video_buf))
            {
                FrontendService::show_error(
                    "Failed to append frame to video.\nPerhaps you ran out of memory?",
                    "Capture");
                stop_capture();
                return;
            }
            m_video_frame += 1.0;
        }
    }

    void ai_len_changed()
    {
        assert(is_on_gui_thread());

        const auto p = reinterpret_cast<short*>((char*)rdram + (ai_register.
            ai_dram_addr & 0xFFFFFF));
        const auto buf = (char*)p;
        const int ai_len = (int)ai_register.ai_len;

        m_audio_bitrate = (int)ai_register.ai_bitrate + 1;

        if (!m_capturing)
        {
            return;
        }

        if (ai_len <= 0)
            return;

		if (g_config.encoder_type == static_cast<int32_t>(EncoderType::FFmpeg))
		{
			m_encoder->append_audio(reinterpret_cast<uint8_t*>(buf), ai_len);
			return;
		}

        const int len = ai_len;
        const int write_size = 2 * m_audio_freq;
        // we want (writeSize * 44100 / m_audioFreq) to be an integer

        if (g_config.synchronization_mode == (int)Sync::Video || g_config.
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

            if (g_config.synchronization_mode == (int)Sync::None) // HACK
                desync = 0;

            if (desync > 1.0)
            {
                g_view_logger->info(
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
            }
            else if (desync <= -10.0)
            {
                g_view_logger->info(
                    "[EncodingManager]: Waiting from A/V desynchronization of %+Lf frames\n",
                    desync);
            }
        }

        write_sound(static_cast<char*>(buf), len, m_audio_freq, write_size,
                    FALSE);
        last_sound = *(reinterpret_cast<long*>(buf + len) - 1);
    }

    void ai_dacrate_changed(std::any data)
    {
        auto type = std::any_cast<system_type>(data);

        if (m_capturing)
        {
            FrontendService::show_error("Audio frequency changed during capture.\r\nThe capture will be stopped.", "Capture");
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
        g_view_logger->info("[EncodingManager] m_audio_freq: {}", m_audio_freq);
    }

    void init()
    {
        assert(is_on_gui_thread());

        Messenger::subscribe(Messenger::Message::DacrateChanged, ai_dacrate_changed);
    }
}
