#include <string>

const std::string defaultOptions = "test"; // idk what default is good

enum initErrors
{
	UNKNOWN,
	INIT_SUCCESS,
	INIT_CREATEPROCESS_ERROR,
	INIT_PIPE_ERROR,
};

initErrors InitFFMPEGTest();

//class FFMPEGManager;