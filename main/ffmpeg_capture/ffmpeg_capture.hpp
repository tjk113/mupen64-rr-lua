#include <string>
#include "vcr_compress.h"

#ifdef _DEBUG
#define FFMPEG_BENCHMARK
#endif

const std::string defaultOptions = "out.mp4";
// this is C string for now
// rawvideo demuxer,
// video size and framerate prepared in manager constructor when values are known
// pixel format is bgr24, because that's what windows uses
// input pipes are opened in manager
const char baseOptions[] = " -f rawvideo -video_size %dx%d -framerate %d -pixel_format bgr24 -i \\\\.\\pipe\\mupenvideo -i \\\\.\\pipe\\mupenaudio ";

void InitReadScreenFFmpeg(const SWindowInfo& info);

enum initErrors
{
	UNKNOWN,
	INIT_SUCCESS,
	INIT_CREATEPROCESS_ERROR,
	INIT_PIPE_ERROR,
    INIT_ALREADY_RUNNING,
    INIT_EMU_NOT_LAUNCHED,
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
    FFmpegManager(unsigned videoX, unsigned videoY, unsigned framerate,
        std::string cmdOptions = defaultOptions);

    ~FFmpegManager();

    int WriteVideoFrame(unsigned char* buffer, unsigned bufferSize);

    int WriteAudioFrame(unsigned char* buffer, unsigned bufferSize);

    // don't copy this object (why would you want to???)
    FFmpegManager(const FFmpegManager&) = delete;
    FFmpegManager& operator=(const FFmpegManager&) = delete;

    initErrors initError{};
    unsigned videoX, videoY;

private:
    int WritePipe(HANDLE pipe, unsigned char* buffer, unsigned bufferSize);

    STARTUPINFO si{};
    PROCESS_INFORMATION pi{};
    HANDLE videoPipe{};
    HANDLE audioPipe{};
    
};