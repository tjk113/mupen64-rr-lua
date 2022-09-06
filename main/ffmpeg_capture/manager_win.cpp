// manager_win.cpp
// windows definition of the manager class
#include <string>
//#include <format>
#include <Windows.h>

#include "ffmpeg_capture.hpp"


FFmpegManager::FFmpegManager(unsigned videoX, unsigned videoY, unsigned framerate, unsigned audiorate,
                             std::string cmdOptions) : videoX{ videoX }, videoY{ videoY }, audiorate{ audiorate }
{
    // partially copied from msdn
#ifdef _DEBUG
    {
        char buf[MAX_PATH];
        GetCurrentDirectory(sizeof(buf), buf);
        printf("Looking for ffmpeg.exe in %s", buf);
    }
#endif
    si.cb = sizeof(si);

    // Create named pipes
    //@TODO: calculate good bufsize? how
    unsigned int BUFSIZEvideo = videoX * videoY * 3;    //one frame size
    unsigned int BUFSIZEaudio = audiorate; //bytes per second, maybe good

    videoPipe = CreateNamedPipe(
        "\\\\.\\pipe\\mupenvideo",             // pipe name 
        PIPE_ACCESS_OUTBOUND,       // read/write access 
        PIPE_TYPE_BYTE |       // message type pipe 
        PIPE_READMODE_BYTE |   // message-read mode 
        PIPE_NOWAIT,                // blocking mode @TODO: what to choose here?
        PIPE_UNLIMITED_INSTANCES, // max. instances  
        BUFSIZEvideo,                  // output buffer size 
        0,                  // input buffer size 
        0,                        // client time-out 
        NULL);                    // default security attribute 

    if (!videoPipe)
    {
        initError = INIT_PIPE_ERROR;
        return;
    }

    audioPipe = CreateNamedPipe(
        "\\\\.\\pipe\\mupenaudio",             // pipe name 
        PIPE_ACCESS_OUTBOUND,       // read/write access 
        PIPE_TYPE_BYTE |       // message type pipe 
        PIPE_READMODE_BYTE |   // message-read mode 
        PIPE_NOWAIT,                // blocking mode @TODO: what to choose here?
        PIPE_UNLIMITED_INSTANCES, // max. instances  
        BUFSIZEaudio,                  // output buffer size 
        0,                  // input buffer size 
        0,                        // client time-out 
        NULL);

    if (!audioPipe)
    {
        initError = INIT_PIPE_ERROR;
        return;
    }

    // construct commandline
    // when micrisoft fixes c++20 this will use std::format instead
    {
        char buf[256];
        //@TODO: set audio format
        snprintf(buf, sizeof(buf), baseOptions, videoX, videoY, framerate, audiorate);

        //prepend
        cmdOptions = buf + cmdOptions;
    }

    printf("sleep before process start...\n");
    Sleep(1000);
    // Start the child process. 
    if (!CreateProcess("ffmpeg/bin/ffmpeg.exe",
        cmdOptions.data(),  //non-const
        NULL,           // Process handle not inheritable
        NULL,           // Thread handle not inheritable
        FALSE,          // Set handle inheritance to FALSE
        0,              // No creation flags
        NULL,           // Use parent's environment block
        NULL,           // Use parent's starting directory 
        &si,            // Pointer to STARTUPINFO structure
        &pi)           // Pointer to PROCESS_INFORMATION structure
        )
    {
        printf("CreateProcess failed (%d).\n", GetLastError());
        initError = INIT_CREATEPROCESS_ERROR;
        return;
    }
    printf("sleep after process start...\n");
    Sleep(1000);
    
    silenceBuffer = (char*)calloc(audiorate,1); // I like old C functions more
    initError = INIT_SUCCESS;
}

FFmpegManager::~FFmpegManager()
{
    free(silenceBuffer);
    DisconnectNamedPipe(videoPipe);
    DisconnectNamedPipe(audioPipe);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    TerminateProcess(pi.hProcess, 0);
    WaitForSingleObject(pi.hProcess, INFINITE); // I don't really care if the process is a zombie at this point
    FFMpegCleanup(); // free buffer used by readscreen (starting two captures at once would break this way because buffer is global)
}

int FFmpegManager::WriteVideoFrame(unsigned char* buffer, unsigned bufferSize)
{
    if (lastWriteWasVideo)
    {
        // write frame of silence (I don't know if there's a way to do this without buffer)
        //printf("Writing silence...\n");
        if (WriteAudioFrame(silenceBuffer, audiorate))
            return 1;
    }
     
    lastWriteWasVideo = true;
    return 0;
    //printf("writing video frame\n");
    return WritePipe(videoPipe, (char*)buffer, bufferSize);
}

int FFmpegManager::WriteAudioFrame(char* buffer, unsigned bufferSize) //samples are signed
{
    lastWriteWasVideo = false;
    //printf("writing audio frame: %d len\n", bufferSize);
    return WritePipe(audioPipe, buffer, bufferSize);
}

int FFmpegManager::WritePipe(HANDLE pipe, char* buffer, unsigned bufferSize)
{
    DWORD written{};
    auto res = WriteFile(pipe, buffer, bufferSize, &written, NULL);
    if (written != bufferSize or res != TRUE)
    {
        return 1;
    }
    return 0;
}

// test default constructor, also this can be externed to C
initErrors InitFFMPEGTest()
{
    FFmpegManager manager(100,100,60,32000);
    return manager.initError;
}