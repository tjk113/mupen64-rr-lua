#include "FFmpegEncoder.h"

#include <assert.h>
#include <shared/Config.hpp>
#include <shared/services/LoggingService.h>
#include <view/gui/Main.h>
#include <view/gui/Loggers.h>

#include "shared/services/FrontendService.h"


bool FFmpegEncoder::start(Params params)
{
    m_params = params;
    m_params.path.replace_extension(".mp4");

    const auto bufsize_video = 2048;
    const auto bufsize_audio = m_params.arate * 8;

    g_view_logger->info("[FFmpegEncoder] arate: {}, bufsize_video: {}, bufsize_audio: {}\n", m_params.arate, bufsize_video, bufsize_audio);

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


    static char options[4096]{};
    memset(options, 0, sizeof(options));

    sprintf(options,
            g_config.ffmpeg_final_options.data(),
            m_params.width,
            m_params.height,
            m_params.fps,
            VIDEO_PIPE_NAME,
            m_params.arate,
            AUDIO_PIPE_NAME,
            m_params.path.string().data());

    g_view_logger->info("[FFmpegEncoder] Starting encode with commandline:");
    g_view_logger->info("[FFmpegEncoder] {}", options);

    if (!CreateProcess(g_config.ffmpeg_path.c_str(),
                       options,
                       NULL,
                       NULL,
                       FALSE,
                       NULL,
                       NULL,
                       NULL,
                       &m_si,
                       &m_pi)
    )
    {
        FrontendService::show_error(std::format("Could not start ffmpeg process! Does ffmpeg exist on disk at '{}'?", g_config.ffmpeg_path).c_str());
        g_view_logger->info("CreateProcess failed ({}).", GetLastError());
        CloseHandle(m_video_pipe);
        CloseHandle(m_audio_pipe);
        return false;
    }

    m_silence_buffer = static_cast<uint8_t*>(calloc(params.arate, 1));

    m_video_thread = std::thread(&FFmpegEncoder::write_video_thread, this);
    m_audio_thread = std::thread(&FFmpegEncoder::write_audio_thread, this);

    Sleep(500);

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
    m_last_write_was_video = false;
    m_audio_queue_mutex.lock();
    this->m_audio_queue.emplace(audio, length, needs_free);
    m_audio_queue_mutex.unlock();
    return true;
}

bool FFmpegEncoder::append_video(uint8_t* image)
{
    if (m_last_write_was_video)
    {
        g_view_logger->info("[FFmpegEncoder] writing silence buffer, length {}\n", m_params.arate / 16);
        append_audio_impl(m_silence_buffer, m_params.arate / 16, false);
    }
    m_last_write_was_video = true;
    m_video_queue_mutex.lock();
    this->m_video_queue.push(image);
    m_video_queue_mutex.unlock();
    return true;
}

bool FFmpegEncoder::append_audio(uint8_t* audio, size_t length)
{
    return append_audio_impl(audio, length, false);
}

bool write_pipe_checked(const HANDLE pipe, const char* buffer, const unsigned buffer_size)
{
    DWORD written{};
    const auto res = WriteFile(pipe, buffer, buffer_size, &written, nullptr);
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

            // We don't want to free our own silence buffer :P
            if (needs_free)
            {
                free(buf);
            }

            if (result)
            {
                this->m_audio_queue_mutex.lock();
                this->m_audio_queue.pop();
                this->m_audio_queue_mutex.unlock();
            }
            else
            {
                //g_view_logger->error("[FFmpegEncoder] write_audio_thread fail, queue: {}, error: {}\n", this->m_audio_queue.size(), GetLastError());
            }
        }
        else
        {
            Sleep(10);
        }
    }

    if (this->m_audio_queue.size() > 0)
    {
        FrontendService::show_warning(std::format("Capture stopped with {} audio seconds remaining in queue!\nThe capture might be corrupted.", this->m_audio_queue.size()).c_str());
    }
}

void FFmpegEncoder::write_video_thread()
{
    while (!this->m_stop_thread)
    {
        if (!this->m_video_queue.empty())
        {
            this->m_video_queue_mutex.lock();
            auto video_buf = this->m_video_queue.front();
            this->m_video_queue_mutex.unlock();

            auto result = write_pipe_checked(m_video_pipe, (char*)video_buf, m_params.width * m_params.height * 3);

            if (result)
            {
                this->m_video_queue_mutex.lock();
                this->m_video_queue.pop();
                this->m_video_queue_mutex.unlock();
            }
            else
            {
                //g_view_logger->error("[FFmpegEncoder] write_video_thread fail, queue: {}, error: {}\n", this->m_video_queue.size(), GetLastError());
            }
        }
        else
        {
            Sleep(10);
        }
    }

    if (this->m_video_queue.size() > 0)
    {
        FrontendService::show_warning(std::format("Capture stopped with {} video frames remaining in queue!\nThe capture might be corrupted.", this->m_video_queue.size()).c_str());
    }
}
