//#include "../config.h"

#ifdef VCR_SUPPORT

#ifndef __VCR_COMPRESS_H__
#define __VCR_COMPRESS_H__

#if defined(__cplusplus) && !defined(_MSC_VER)
extern "C" {
#endif

#define ATTRIB_INTEGER		1
#define ATTRIB_STRING		2
#define ATTRIB_SELECT		3
#define ATTRIB_FLOAT		4


//real width and height of emulated area might not be a multiple of 4, which is apparently important for avi
typedef struct {
	long height;
	long width;
	long toolbarHeight;
	long statusbarHeight;
} SWindowInfo;

extern SWindowInfo sInfo;

void VCRComp_init();

void VCRComp_startFile( const char *filename, long width, long height, int fps, int New);
void VCRComp_finishFile(int split);
BOOL VCRComp_addVideoFrame( unsigned char *data );
BOOL VCRComp_addAudioData( unsigned char *data, int len );

int         VCRComp_numVideoCodecs();
const char *VCRComp_videoCodecName( int index );
int         VCRComp_numVideoCodecAttribs( int index );
const char *VCRComp_videoCodecAttribName( int cindex, int aindex );
int         VCRComp_videoCodecAttribKind( int cindex, int aindex );
const char *VCRComp_videoCodecAttribValue( int cindex, int aindex );
void        VCRComp_videoCodecAttribSetValue( int cindex, int aindex, const char *val );
int         VCRComp_numVideoCodecAttribOptions( int cindex, int aindex );
const char *VCRComp_videoCodecAttribOption( int cindex, int aindex, int oindex );
void        VCRComp_selectVideoCodec( int index );

int         VCRComp_numAudioCodecs();
const char *VCRComp_audioCodecName( int index );
int         VCRComp_numAudioCodecAttribs( int index );
const char *VCRComp_audioCodecAttribName( int cindex, int aindex );
int         VCRComp_audioCodecAttribKind( int cindex, int aindex );
const char *VCRComp_audioCodecAttribValue( int cindex, int aindex );
void        VCRComp_audioCodecAttribSetValue( int cindex, int aindex, const char *val );
int         VCRComp_numAudioCodecAttribOptions( int cindex, int aindex );
const char *VCRComp_audioCodecAttribOption( int cindex, int aindex, int oindex );
void        VCRComp_selectAudioCodec( int index );
unsigned int VCRComp_GetSize ( );
void CalculateWindowDimensions(HWND hWindow, SWindowInfo& infoStruct);

#if defined(__cplusplus) && !defined(_MSC_VER)
} // extern "C"
#endif

#endif // __VCR_COMPRESS_H__

#endif // VCR_SUPPORT
