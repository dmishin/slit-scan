#include "ffmpeg_decoder.hpp"

#include <stdio.h>

class SavingFrameHandler: public FrameHandler{
public:
  int save_every;
  SavingFrameHandler( int every=1 ): save_every(every){};
  virtual bool handle(AVFrame *pFrame, int width, int height, int iFrame);
};

bool SavingFrameHandler::handle(AVFrame *pFrame, int width, int height, int iFrame)
{
  FILE *pFile;
  char szFilename[32];
  int  y;

  if ( iFrame % save_every != 0 ) return true;
  
  // Open file
  printf( "Saving frame %d\n", iFrame );
  sprintf(szFilename, "frame%d.ppm", iFrame);
  pFile=fopen(szFilename, "wb");
  if(pFile==NULL)
    return false;
  
  // Write header
  fprintf(pFile, "P6\n%d %d\n255\n", width, height);
  
  // Write pixel data
  for(y=0; y<height; y++)
    fwrite(pFrame->data[0]+y*pFrame->linesize[0], 1, width*3, pFile);
  
  // Close file
  fclose(pFile);

  return true;
}

int main(int argc, char *argv[]) {
  if(argc < 2) {
    printf("Please provide a movie file\n");
    return -1;
  }
  // Register all formats and codecs
  av_register_all();
  SavingFrameHandler frame_saver(5);
  int rval = process_ffmpeg_file( argv[1], frame_saver );
  return rval;
}
