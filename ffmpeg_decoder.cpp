//Based on sample code from https://github.com/mpenkov/ffmpeg-tutorial

#include "ffmpeg_decoder.hpp"

#include <stdio.h>

int process_ffmpeg_file( const char *fname, FrameHandler &handler, int scale )
{
  AVFormatContext *pFormatCtx = NULL;
  int             i, videoStream;
  AVCodecContext  *pCodecCtx = NULL;
  AVCodec         *pCodec = NULL;
  AVFrame         *pFrame = NULL; 
  AVFrame         *pFrameRGB0 = NULL, *pFrameRGB1 = NULL, *pFrameRGBOld = NULL;
  AVPacket        packet;
  int             frameFinished;
  int             numBytes;
  uint8_t         *buffer0 = NULL, *buffer1 = NULL;
  AVDictionary    *optionsDict = NULL;
  struct SwsContext      *sws_ctx = NULL;
  
  // Open video file
  if(avformat_open_input(&pFormatCtx, fname, NULL, NULL)!=0)
    return -1; // Couldn't open file
  
  // Retrieve stream information
  if(avformat_find_stream_info(pFormatCtx, NULL)<0)
    return -1; // Couldn't find stream information
  
  // Dump information about file onto standard error
  av_dump_format(pFormatCtx, 0, fname, 0);
  
  // Find the first video stream
  videoStream=-1;
  for(i=0; i<pFormatCtx->nb_streams; i++)
    if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO) {
      videoStream=i;
      break;
    }
  if(videoStream==-1)
    return -1; // Didn't find a video stream
  
  // Get a pointer to the codec context for the video stream
  pCodecCtx=pFormatCtx->streams[videoStream]->codec;
  
  // Find the decoder for the video stream
  pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
  if(pCodec==NULL) {
    fprintf(stderr, "Unsupported codec!\n");
    return -1; // Codec not found
  }
  // Open codec
  if(avcodec_open2(pCodecCtx, pCodec, &optionsDict)<0)
    return -1; // Could not open codec
  
  // Allocate video frame
  pFrame=av_frame_alloc();
  
  // Allocate an AVFrame structure
  pFrameRGB0=av_frame_alloc();
  pFrameRGB1=av_frame_alloc();
  if(pFrameRGB0==NULL || pFrameRGB1==NULL || pFrameRGB1 == pFrameRGB0)
    return -1;
  
  // Determine required buffer size and allocate buffer
  numBytes=avpicture_get_size(AV_PIX_FMT_RGB24, pCodecCtx->width*scale,
			      pCodecCtx->height*scale);
  buffer0=(uint8_t *)av_malloc(numBytes*sizeof(uint8_t));
  buffer1=(uint8_t *)av_malloc(numBytes*sizeof(uint8_t));

  sws_ctx =
    sws_getContext
    (
        pCodecCtx->width,
        pCodecCtx->height,
        pCodecCtx->pix_fmt,
        pCodecCtx->width*scale,
        pCodecCtx->height*scale,
        AV_PIX_FMT_RGB24,
        SWS_BILINEAR,
        NULL,
        NULL,
        NULL
    );
  
  // Assign appropriate parts of buffer to image planes in pFrameRGB
  // Note that pFrameRGB is an AVFrame, but AVFrame is a superset
  // of AVPicture
  avpicture_fill((AVPicture *)pFrameRGB0, buffer0, AV_PIX_FMT_RGB24,
		 pCodecCtx->width*scale, pCodecCtx->height*scale);
  avpicture_fill((AVPicture *)pFrameRGB1, buffer1, AV_PIX_FMT_RGB24,
		 pCodecCtx->width*scale, pCodecCtx->height*scale);
  
  // Read frames
  i=0;
  while(av_read_frame(pFormatCtx, &packet)>=0) {
    // Is this a packet from the video stream?
    if(packet.stream_index==videoStream) {
      // Decode video frame
      avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, 
			   &packet);
      
      // Did we get a video frame?
      if(frameFinished) {
	// Convert the image from its native format to RGB
        sws_scale
        (
            sws_ctx,
            (uint8_t const * const *)pFrame->data,
            pFrame->linesize,
            0,
            pCodecCtx->height,
            pFrameRGB0->data,
            pFrameRGB0->linesize
        );
	// handle the frame
	if (pFrameRGBOld && pFrameRGB0->data[0] == pFrameRGBOld->data[0]){
	  return -1;
	}
	if (! handler.handle(pFrameRGB0, pCodecCtx->width*scale, pCodecCtx->height*scale, i++) )
	  break;
	pFrameRGBOld = pFrameRGB0;
	pFrameRGB0 = pFrameRGB1;
	pFrameRGB1 = pFrameRGBOld;
      }
    }
    
    // Free the packet that was allocated by av_read_frame
    av_free_packet(&packet);
  }
  
  // Free the RGB image
  av_free(buffer0);
  av_free(buffer1);
  av_free(pFrameRGB1);
  av_free(pFrameRGB0);
  
  // Free the YUV frame
  av_free(pFrame);
  
  // Close the codec
  avcodec_close(pCodecCtx);
  
  // Close the video file
  avformat_close_input(&pFormatCtx);
  return 1;
}
			    

