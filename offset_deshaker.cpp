#include "offset_deshaker.hpp"
#include <iostream>
#include <stdexcept>
#include "compare_rgb_blocks.hpp"
using namespace std;

void OffsetDeshaker::get_frame_offset( double &dx, double &dy )
{
  dx = dx_accum;
  dy = dy_accum;
}

OffsetDeshaker::OffsetDeshaker( int x0_, int y0_, int dx_, int dy_, int w, int h, int half_return_time )
  :x0(x0_), y0(y0_), dx(dx_), dy(dy_)
  ,width(w), height(h)
{
  dx_accum = 0;
  dy_accum = 0;
  if (half_return_time == 0){
    dissipation_rate = 1;
  }else{
    dissipation_rate = pow( .5, 1.0 / half_return_time );
  }
}

bool OffsetDeshaker::handle(AVFrame *pFrame, AVFrame *pFrameOld, int fwidth, int fheight, int iFrame)
{
  if (pFrameOld == NULL) return true;

  uint8_t * pBlock1 = pFrame->data[0] + x0 * 3 + y0 * pFrame->linesize[0];
  uint8_t * pBlock2 = pFrameOld->data[0] + x0 * 3 + y0 * pFrameOld->linesize[0];

  if (x0-dx < 0 || y0-dy < 0 ||
      x0+width+dx >=fwidth || y0+height+dy >= fheight){
    throw std::logic_error("Search block outside of frame");
  }
  
  int deltaX, deltaY;
  double d, dworst;
  match_blocks( pBlock1, pFrame->linesize[0],
		pBlock2, pFrameOld->linesize[0],
		width, height,
		dx, dy,
		deltaX, deltaY,
		d, dworst);

  dx_accum *= dissipation_rate;
  dy_accum *= dissipation_rate;
  if (true){//add shift condition here
    dx_accum += deltaX;
    dy_accum += deltaY;
  }
  cout <<iFrame<< "Rate:"<<dworst / d
       << " dxa = "<<dx_accum<<" dya = "<<dy_accum<<endl;
  return true;
}


