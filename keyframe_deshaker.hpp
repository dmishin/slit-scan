#ifndef __KEYFRAME_DESHAKER_INCLUDED__
#define __KEYFRAME_DESHAKER_INCLUDED__
#include "abstract_offset_deshaker.hpp"

class KeyframeDeshaker: public FrameHandler{
  int x0, y0, dx, dy;
  int width, height;
  double dx_accum, dy_accum;
  uint8 *key_frame_ptr;
public:
  KeyframeDeshaker( int x0, int y0, int dx, int dy, int w, int h );
  virtual ~KeyframeDeshaker();
  virtual bool handle(AVFrame *pFrame, AVFrame *pFrameOld, int width, int height, int iFrame);
  virtual void get_frame_offset( double &dx, double &dy );
private:
  void update_keyframe( AVFrame *pFrame );
};

#endif
