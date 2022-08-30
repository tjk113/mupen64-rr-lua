// manager_win.cpp
// windows definition of the manager class
#include <string>
#include <Windows.h>

#include "ffmpeg_capture.hpp"

class FFMPEGManager
{
public:
    FFMPEGManager(std::string cmdOptions = defaultOptions) //wide char version can modify it so copy it first
    {
        // partially copied from msdn
#ifdef _DEBUG
        char buf[MAX_PATH];
        GetCurrentDirectory(sizeof(buf), buf);
        printf("Looking for ffmpeg.exe in %s", buf);
#endif
        si.cb = sizeof(si);

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

        // Create named pipes
        //@TODO: calculate good bufsize? how
        unsigned int BUFSIZE = 1234;

        videoPipe = CreateNamedPipe(
            "\\\\.\\pipe\\mupenvideo",             // pipe name 
            PIPE_ACCESS_OUTBOUND,       // read/write access 
            PIPE_TYPE_BYTE |       // message type pipe 
            PIPE_READMODE_BYTE |   // message-read mode 
            PIPE_NOWAIT,                // blocking mode @TODO: what to choose here?
            PIPE_UNLIMITED_INSTANCES, // max. instances  
            BUFSIZE,                  // output buffer size 
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
            BUFSIZE,                  // output buffer size 
            0,                  // input buffer size 
            0,                        // client time-out 
            NULL);

        if (!audioPipe)
        {
            initError = INIT_PIPE_ERROR;
            return;
        }

        initError = INIT_SUCCESS;
    }
    ~FFMPEGManager()
    {
        TerminateProcess(pi.hProcess, 0);
        //WaitForSingleObject(pi.hProcess, INFINITE); // I don't really care if the process is a zombie at this point
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
    // don't copy this object (why would you want to???)
    FFMPEGManager(const FFMPEGManager&) = delete;
    FFMPEGManager& operator=(const FFMPEGManager&) = delete;

    initErrors initError{};

private:
    STARTUPINFO si{};
    PROCESS_INFORMATION pi{};
    HANDLE videoPipe{};
    HANDLE audioPipe{};
};

// test default constructor, also this can be externed to C
initErrors InitFFMPEGTest()
{
    FFMPEGManager manager;
    return manager.initError;
}