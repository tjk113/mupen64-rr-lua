#include <string>

const std::string defaultOptions = "out.mp4";

enum initErrors
{
	UNKNOWN,
	INIT_SUCCESS,
	INIT_CREATEPROCESS_ERROR,
	INIT_PIPE_ERROR,
    INIT_ALREADY_RUNNING,
};

initErrors InitFFMPEGTest();

class FFmpegManager
{
public:

    /// <summary>
    /// Object used to communicate with ffmpeg process
    /// </summary>
    /// <param name="videoX">input video resolution</param>
    /// <param name="videoY">input video resolution</param>
    /// <param name="framerate">framerate (depends whether PAL or not)</param>
    /// <param name="cmdOptions">additional ffmpeg options (compression, output name, effects and shit)</param>
    FFmpegManager(unsigned videoX, unsigned videoY, unsigned framerate, std::string cmdOptions = defaultOptions);

    ~FFmpegManager();

    int WriteVideoFrame(unsigned char* buffer, unsigned bufferSize);

    int WriteAudioFrame(unsigned char* buffer, unsigned bufferSize);

    // don't copy this object (why would you want to???)
    FFmpegManager(const FFmpegManager&) = delete;
    FFmpegManager& operator=(const FFmpegManager&) = delete;

    initErrors initError{};

private:
    int WritePipe(HANDLE pipe, unsigned char* buffer, unsigned bufferSize);

    STARTUPINFO si{};
    PROCESS_INFORMATION pi{};
    HANDLE videoPipe{};
    HANDLE audioPipe{};
};