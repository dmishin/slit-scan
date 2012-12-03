#include "keyframe_deshaker.hpp"

KeyframeDeshaker::KeyframeDeshaker( int x0, int y0, int dx, int dy, int w, int h )
  :x0(x0_), y0(y0_), dx(dx_), dy(dy_)
  ,width(w), height(h)
{
  dx_accum = 0;
  dy_accum = 0;
  key_frame_ptr = NULL;
}

KeyframeDeshaker::~KeyframeDeshaker()
{
  delete key_frame_ptr;
}

bool KeyframeDeshaker::handle(AVFrame *pFrame, AVFrame *pFrameOld, int width, int height, int iFrame)
{
}

void KeyframeDeshaker::get_frame_offset( double &dx, double &dy )
{
  dx = dx_accum; dy = dy_accum;
}

void KeyframeDeshaker::update_keyframe( AVFrame *pFrame )
{
  uint8_t * row = 
}
