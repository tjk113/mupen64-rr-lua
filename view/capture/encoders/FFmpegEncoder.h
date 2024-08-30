#pragma once

#include "Encoder.h"

#include <mutex>
#include <queue>
#include <thread>
#include <Windows.h>
#include <Vfw.h>

class FFmpegEncoder : public Encoder
{
public:
    bool start(Params params) override;
    bool stop() override;
    bool append_video(uint8_t* image) override;
    bool append_audio(uint8_t* audio, size_t length) override;

private:
	bool append_audio_impl(uint8_t* audio, size_t length, bool needs_free);
    void write_video_thread();
    void write_audio_thread();
    
    Params m_params{};

    STARTUPINFO m_si{};
    PROCESS_INFORMATION m_pi{};
    HANDLE m_video_pipe{};
    HANDLE m_audio_pipe{};

    uint8_t* m_silence_buffer{};

    // (following comment functionality is disabled now) if you REALLY want to stop both threads, clear their queues. Otherwise this just tells thread to stop waiting for new stuff
    bool m_stop_thread = false;

    std::thread m_audio_thread;
    std::mutex m_audio_queue_mutex{};
    std::queue<std::tuple<uint8_t*, size_t, bool>> m_audio_queue; // queue for sound frames to write to ffmpeg

    std::thread m_video_thread;
    std::mutex m_video_queue_mutex{};
    std::queue<std::tuple<uint8_t*, bool>> m_video_queue; // queue for sound frames to write to ffmpeg
};
