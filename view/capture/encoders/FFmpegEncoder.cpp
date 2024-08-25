#include "FFmpegEncoder.h"

//format signed 16 bit samples little endian
//rate taken from m_audioFreq
//two channels (always like that on n64)
#define AUDIO_OPTIONS " -thread_queue_size 64 -f s16le -ar %d -ac 2 -channel_layout stereo "

// rawvideo demuxer,
// video size and framerate prepared in manager constructor when values are known
// pixel format is bgr24, because that's what windows uses
#define VIDEO_OPTIONS " -thread_queue_size 4 -f rawvideo -video_size %dx%d -framerate %d -pixel_format bgr24 "

constexpr char BASE_OPTIONS[] = VIDEO_OPTIONS "-i \\\\.\\pipe\\mupenvideo" AUDIO_OPTIONS "-i \\\\.\\pipe\\mupenaudio ";

bool FFmpegEncoder::start(Params params)
{
    m_params = params;
    m_params.path.replace_extension(".mp4");
    
    constexpr auto bufsize_video = 2048;
    const auto bufsize_audio = m_params.arate * 8;

    m_video_pipe = CreateNamedPipe(
        "\\\\.\\pipe\\mupenvideo", // pipe name 
        PIPE_ACCESS_OUTBOUND, // read/write access 
        PIPE_TYPE_BYTE | // message type pipe 
        PIPE_READMODE_BYTE | // message-read mode 
        PIPE_WAIT, // blocking mode
        PIPE_UNLIMITED_INSTANCES, // max. instances  
        bufsize_video, // output buffer size 
        0, // input buffer size 
        0, // client time-out 
        NULL); // default security attribute 

    if (!m_video_pipe)
    {
        return false;
    }

    m_audio_pipe = CreateNamedPipe(
        "\\\\.\\pipe\\mupenaudio",
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

    std::string cmd_options = "-pixel_format yuv420p " + m_params.path.string();
    // construct commandline
    // when micrisoft fixes c++20 this will use std::format instead
    {
        char buf[256];
        //@TODO: set audio format
        snprintf(buf, sizeof(buf), BASE_OPTIONS, m_params.width, m_params.height, m_params.fps, m_params.arate);

        //prepend
        cmd_options = buf + cmd_options;
    }

    if (!CreateProcess("C:/ffmpeg/bin/ffmpeg.exe",
                       cmd_options.data(), //non-const
                       NULL, // Process handle not inheritable
                       NULL, // Thread handle not inheritable
                       FALSE, // Set handle inheritance to FALSE
                       0, // No creation flags
                       NULL, // Use parent's environment block
                       NULL, // Use parent's starting directory 
                       &m_si, // Pointer to STARTUPINFO structure
                       &m_pi) // Pointer to PROCESS_INFORMATION structure
    )
    {
        MessageBox(NULL, "Could not start ffmpeg process! Does ffmpeg exist on disk?", "Error", MB_ICONERROR);
        printf("CreateProcess failed (%d).\n", GetLastError());
        CloseHandle(m_video_pipe);
        CloseHandle(m_audio_pipe);
        return false;
    }

    m_silence_buffer = static_cast<uint8_t*>(calloc(params.arate, 1));

    m_video_thread = std::thread(&FFmpegEncoder::WriteVideoThread, this);
    m_audio_thread = std::thread(&FFmpegEncoder::WriteAudioThread, this);

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

bool FFmpegEncoder::append_video(uint8_t* image)
{
    const auto buf_size = m_params.width * m_params.height * 3;

    if (m_last_write_was_video)
    {
        // write frame of silence (I don't know if there's a way to do this without buffer)
        append_audio(m_silence_buffer, m_params.arate / 16);
    }
    m_last_write_was_video = true;
    m_video_queue_mutex.lock();
    this->m_video_queue.push({image, image + buf_size});
    m_video_queue_mutex.unlock();
    return true;
}

bool FFmpegEncoder::append_audio(uint8_t* audio, size_t length)
{
    m_last_write_was_video = false;
    m_audio_queue_mutex.lock();
    this->m_audio_queue.push({audio, audio + length});
    m_audio_queue_mutex.unlock();
    return true;
}

bool FFmpegEncoder::WritePipe(HANDLE pipe, char* buffer, unsigned bufferSize)
{
    DWORD written{};
    auto res = WriteFile(pipe, buffer, bufferSize, &written, NULL);
    if (written != bufferSize or res != TRUE)
    {
        return false;
    }
    return true;
}

bool FFmpegEncoder::_WriteVideoFrame(unsigned char* buffer, unsigned bufferSize)
{
    return WritePipe(m_video_pipe, (char*)buffer, bufferSize);
}

bool FFmpegEncoder::_WriteAudioFrame(char* buffer, unsigned bufferSize)
{
    return WritePipe(m_audio_pipe, buffer, bufferSize);
}

void FFmpegEncoder::WriteAudioThread()
{
    // a special variable is used, since it's hard to guarantee that, for example, VCR_isCapturing() will be correct.
    // waits until the queue is empty, then stops (this is commented out)
    while (!this->m_stop_thread) // || !this->audioQueue.empty()) 
    {
        if (!this->m_audio_queue.empty())
        {
            //printf("Writing audio (thread)\n");
            this->m_audio_queue_mutex.lock();
            auto frame = this->m_audio_queue.front();
            this->m_audio_queue_mutex.unlock();
            if (this->_WriteAudioFrame(frame.data(), frame.size())) // waits here
            {
                //printf(">writing audio success\n");
                this->m_audio_queue_mutex.lock();
                this->m_audio_queue.pop(); //only remove when write succesfull
                this->m_audio_queue_mutex.unlock();
            }
            else
            {
                printf(">>>>>>>>>>>>>>>writing audio fail, queue size: %d\n", this->m_audio_queue.size());
            }
        }
        else
        {
            // sleep?
            Sleep(10);
        }
    }

    printf(">>>Remaining audio queue stuff: %d\n", this->m_audio_queue.size());
}

void FFmpegEncoder::WriteVideoThread()
{
    // a special variable is used, since it's hard to guarantee that, for example, VCR_isCapturing() will be correct.
    // waits until the queue is empty, then stops
    while (!this->m_stop_thread) // || !this->audioQueue.empty())
    {
        if (!this->m_video_queue.empty())
        {
            //printf("Writing video (thread)\n");
            this->m_video_queue_mutex.lock();
            auto frame = this->m_video_queue.front();
            this->m_video_queue_mutex.unlock();
            if (this->_WriteVideoFrame(frame.data(), frame.size())) // waits here
            {
                // printf(">writing video success\n");
                this->m_video_queue_mutex.lock();
                this->m_video_queue.pop(); //only remove when write succesfull
                this->m_video_queue_mutex.unlock();
            }
            else
            {
                printf(">>>>>>>>>>>>>>>writing video fail, queue size: %d\n", this->m_video_queue.size());
            }
        }
        else
        {
            // sleep?
            Sleep(10);
        }
    }

    printf(">>>Remaining video queue stuff: %d\n", this->m_video_queue.size());
}
