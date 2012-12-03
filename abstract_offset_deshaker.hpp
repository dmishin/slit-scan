#ifndef __ABSTRACT_OFFSET_DESHAKER_INCLUDED__
#define __ABSTRACT_OFFSET_DESHAKER_INCLUDED__
#include "ffmpeg_decoder.hpp"

class AbstractOffsetDeshaker: public FrameHandler{
public:
  virtual void get_frame_offset( double &dx, double &dy )=0;
};

#endif
