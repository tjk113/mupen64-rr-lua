#include "FFmpegEncoder.h"

#include <shared/Config.hpp>

bool FFmpegEncoder::start(Params params)
{
    m_params = params;
    m_params.path.replace_extension(".mp4");

    constexpr auto bufsize_video = 2048;
    const auto bufsize_audio = m_params.arate * 8;

#define VIDEO_PIPE_NAME "\\\\.\\pipe\\mupenvideo"
#define AUDIO_PIPE_NAME "\\\\.\\pipe\\mupenaudio"

    m_video_pipe = CreateNamedPipe(
        VIDEO_PIPE_NAME,
        PIPE_ACCESS_OUTBOUND, 
        PIPE_TYPE_BYTE |
        PIPE_READMODE_BYTE |
        PIPE_WAIT,
        PIPE_UNLIMITED_INSTANCES,
        bufsize_video,
        0,
        0,
        NULL); // default security attribute 

    if (!m_video_pipe)
    {
        return false;
    }

    m_audio_pipe = CreateNamedPipe(
        AUDIO_PIPE_NAME,
        PIPE_ACCESS_OUTBOUND,
        PIPE_TYPE_BYTE |
        PIPE_READMODE_BYTE |
        PIPE_WAIT,
        PIPE_UNLIMITED_INSTANCES,
        bufsize_audio,
        0,
        0,
        NULL);

    if (!m_audio_pipe)
    {
        CloseHandle(m_video_pipe);
        return false;
    }


    char options[360] = {0};
    sprintf(options,
            g_config.ffmpeg_final_options.c_str(),
            m_params.width,
            m_params.height,
            m_params.fps,
            VIDEO_PIPE_NAME,
            m_params.arate,
            AUDIO_PIPE_NAME,
            m_params.path.string().c_str());

    if (!CreateProcess("C:/ffmpeg/bin/ffmpeg.exe",
                       options,
                       NULL,
                       NULL,
                       FALSE,
                       DETACHED_PROCESS | CREATE_NEW_PROCESS_GROUP,
                       NULL,
                       NULL,
                       &m_si,
                       &m_pi)
    )
    {
        MessageBox(NULL, "Could not start ffmpeg process! Does ffmpeg exist on disk?", "Error", MB_ICONERROR);
        printf("CreateProcess failed (%d).\n", GetLastError());
        CloseHandle(m_video_pipe);
        CloseHandle(m_audio_pipe);
        return false;
    }

    m_silence_buffer = static_cast<uint8_t*>(calloc(params.arate, 1));

    m_video_thread = std::thread(&FFmpegEncoder::write_video_thread, this);
    m_audio_thread = std::thread(&FFmpegEncoder::write_audio_thread, this);

    return true;
}

bool FFmpegEncoder::stop()
{
    m_stop_thread = true;
    DisconnectNamedPipe(m_video_pipe);
    DisconnectNamedPipe(m_audio_pipe);
    WaitForSingleObject(m_pi.hProcess, INFINITE);
    CloseHandle(m_pi.hProcess);
    CloseHandle(m_pi.hThread);
    m_audio_thread.join();
    m_video_thread.join();
    free(m_silence_buffer);
    return true;
}

bool FFmpegEncoder::append_audio_impl(uint8_t* audio, size_t length, bool needs_free)
{
    m_audio_queue_mutex.lock();
    this->m_audio_queue.push(std::make_tuple(audio, length, needs_free));
    m_audio_queue_mutex.unlock();
    return true;
}

bool FFmpegEncoder::append_video(uint8_t* image)
{
    const auto buf_size = m_params.width * m_params.height * 3;

    m_video_queue_mutex.lock();
    this->m_video_queue.push(std::make_tuple(image, true));
    m_video_queue_mutex.unlock();
    return true;
}

bool FFmpegEncoder::append_audio(uint8_t* audio, size_t length)
{
    return append_audio_impl(audio, length, true);
}

bool write_pipe_checked(const HANDLE pipe, const char* buffer, const unsigned buffer_size)
{
    DWORD written{};
    auto res = WriteFile(pipe, buffer, buffer_size, &written, NULL);
    if (written != buffer_size or res != TRUE)
    {
        return false;
    }
    return true;
}

void FFmpegEncoder::write_audio_thread()
{
    while (!this->m_stop_thread)
    {
        if (!this->m_audio_queue.empty())
        {
            this->m_audio_queue_mutex.lock();
            auto tuple = this->m_audio_queue.front();
            this->m_audio_queue_mutex.unlock();

            auto buf = std::get<0>(tuple);
            auto len = std::get<1>(tuple);
            auto needs_free = std::get<2>(tuple);

            auto result = write_pipe_checked(m_audio_pipe, (char*)buf, len);

            if (result)
            {
                this->m_audio_queue_mutex.lock();

                // We don't want to free our own silence buffer :P
                if (needs_free)
                {
                    m_params.audio_free(buf);
                }
                this->m_audio_queue.pop();

                this->m_audio_queue_mutex.unlock();
            }
            else
            {
                printf(">>>>>>>>>>>>>>>writing audio fail, queue size: %d\n", this->m_audio_queue.size());
            }
        }
        else
        {
            Sleep(10);
        }
    }

    printf(">>>Remaining audio queue stuff: %d\n", this->m_audio_queue.size());
}

void FFmpegEncoder::write_video_thread()
{
    while (!this->m_stop_thread)
    {
        if (!this->m_video_queue.empty())
        {
            this->m_video_queue_mutex.lock();

            auto tuple = this->m_video_queue.front();

            this->m_video_queue_mutex.unlock();

            auto buf = std::get<0>(tuple);
            auto needs_free = std::get<1>(tuple);

            auto result = write_pipe_checked(m_video_pipe, (char*)buf, m_params.width * m_params.height * 3);

            if (result)
            {
                this->m_video_queue_mutex.lock();

                if (needs_free)
                {
                    m_params.video_free(buf);
                }
                this->m_video_queue.pop();

                this->m_video_queue_mutex.unlock();
            }
            else
            {
                printf(">>>>>>>>>>>>>>>writing video fail, queue size: %d\n", this->m_video_queue.size());
            }
        }
        else
        {
            Sleep(10);
        }
    }

    printf(">>>Remaining video queue stuff: %d\n", this->m_video_queue.size());
}
