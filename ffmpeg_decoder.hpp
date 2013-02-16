#ifndef __FFMPEG_DECODER_INCLUDED__
#define __FFMPEG_DECODER_INCLUDED__

#define __STDC_CONSTANT_MACROS
#include <stdint.h>

extern "C"{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}
class FrameHandler{
public:
  virtual bool handle(AVFrame *pFrame, int width, int height, int iFrame)=0;
};

int process_ffmpeg_file( const char *fname, FrameHandler &handler );

#endif
