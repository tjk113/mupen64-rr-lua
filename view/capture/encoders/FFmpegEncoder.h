#pragma once

#include "Encoder.h"

#include <mutex>
#include <queue>
#include <thread>
#include <Windows.h>

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

    bool m_stop_thread = false;
    bool m_last_write_was_video = false;

    std::thread m_audio_thread;
    std::mutex m_audio_queue_mutex{};
	std::condition_variable m_audio_cv{};
    std::queue<std::tuple<uint8_t*, size_t, bool>> m_audio_queue;

    std::thread m_video_thread;
    std::mutex m_video_queue_mutex{};
	std::condition_variable m_video_cv{};
    std::queue<uint8_t*> m_video_queue;
};
