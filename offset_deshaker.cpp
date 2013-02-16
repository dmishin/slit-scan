#include "offset_deshaker.hpp"
#include <iostream>
#include <stdexcept>
#include "compare_rgb_blocks.hpp"
using namespace std;

void OffsetDeshaker::get_frame_offset( double &dx, double &dy )
{
  x_filter.put(dx_accum);
  y_filter.put(dy_accum);
  dx = x_filter.get();
  dy = y_filter.get();
  //  dx = dx_accum;
  //  dy = dy_accum;
}

OffsetDeshaker::OffsetDeshaker( int x0_, int y0_, int dx_, int dy_, int w, int h, int half_return_time )
  :x0(x0_), y0(y0_), dx(dx_), dy(dy_)
  ,width(w), height(h), key_frame(NULL)
{
  dx_accum = 0;
  dy_accum = 0;
  dx_old = 0;
  dy_old = 0;
  if (half_return_time == 0){
    dissipation_rate = 1;
  }else{
    dissipation_rate = pow( .5, 1.0 / half_return_time );
  }
}
void OffsetDeshaker::alloc_key_frame(AVFrame *pFrame, int fheight)
{
  dealloc_key_frame();
  key_frame = new uint8_t[ pFrame->linesize[0] * fheight ];
}
void OffsetDeshaker::dealloc_key_frame()
{
  delete[] key_frame;
  key_frame = NULL;
}
void OffsetDeshaker::copy_key_frame(AVFrame *pFrame, int fheight)
{
  uint8_t *pSrc = pFrame->data[0];
  //cout << "Frame height: "<<fheight<<" line size:"<<pFrame->linesize[0]<<endl;
  std::copy( pSrc, pSrc + (pFrame->linesize[0] * fheight), key_frame );
}
bool OffsetDeshaker::handle(AVFrame *pFrame, int fwidth, int fheight, int iFrame)
{
  if (key_frame == NULL){
    alloc_key_frame(pFrame, fheight);
    copy_key_frame(pFrame, fheight);
    return true;
  }

  uint8_t * pBlock1 = pFrame->data[0] + x0 * 3 + y0 * pFrame->linesize[0];
  uint8_t * pBlock2 = key_frame + x0 * 3 + y0 * pFrame->linesize[0];

  if (x0-dx < 0 || y0-dy < 0 ||
      x0+width+dx >=fwidth || y0+height+dy >= fheight){
    throw std::logic_error("Search block outside of frame");
  }
  
  int deltaX, deltaY;
  double d, dworst;
  match_blocks( pBlock1, pFrame->linesize[0],
		pBlock2, pFrame->linesize[0],
		width, height,
		dx, dy,
		deltaX, deltaY,
		d, dworst);
  
  dx_accum *= dissipation_rate;
  dy_accum *= dissipation_rate;
  if (true){//add shift condition here
    dx_accum += (deltaX - dx_old);
    dy_accum += (deltaY - dy_old);
  }
  if (abs(deltaX) + 1 >= dx || abs(deltaY) + 1 >= dy ){
    cout << "Frame copy!" <<endl;
    copy_key_frame(pFrame, fheight);
    dx_old = 0;
    dy_old = 0;
  }else{
    dx_old = deltaX;
    dy_old = deltaY;
  }
  cout << iFrame 
       << " Rate:" << dworst / d
       << " dx="<<deltaX <<" dy="<<deltaY
       << " dxa = "<< dx_accum
       << " dya = "<< dy_accum<<endl;
  return true;
}


