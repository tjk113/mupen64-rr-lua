#include <string>

const std::string defaultOptions = "out.mp4";

enum initErrors
{
	UNKNOWN,
	INIT_SUCCESS,
	INIT_CREATEPROCESS_ERROR,
	INIT_PIPE_ERROR,
};

initErrors InitFFMPEGTest();

//class FFMPEGManager;