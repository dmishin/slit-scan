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

void KeyframeDeshaker::alloc_keyframe()
{
  key_frame_ptr = new uint8_t[width*height*3];
}
void KeyframeDeshaker::update_keyframe( AVFrame *pFrame )
{
  uint8_t * block_start = pFrame->data[0] + x0 * 3 + y0 * pFrame->linesize[0];
  for (int y = 0; y < height; ++y){
    memcpy( key_frame_ptr + width*3*y,
	    block_start + y*pFrame->linesize[0],
	    width*3 );
  }
}
