#ifndef __OFFSED_DESHAKER_INCLUDED__
#define __OFFSED_DESHAKER_INCLUDED__
#include "abstract_offset_deshaker.hpp"

class OffsetDeshaker: public AbstractOffsetDeshaker{
  int x0, y0, dx, dy;
  int width, height;
  double dx_accum, dy_accum;
  double dissipation_rate;
  uint8_t * key_frame;
public:
  OffsetDeshaker( int x0, int y0, int dx, int dy, int w, int h, int half_return_time );
  virtual ~OffsetDeshaker(){dealloc_key_frame();};
  virtual bool handle(AVFrame *pFrame, int width, int height, int iFrame);
  virtual void get_frame_offset( double &dx, double &dy );
private:
  void alloc_key_frame(AVFrame *pFrame, int fheight);
  void dealloc_key_frame();
  void copy_key_frame(AVFrame *pFrame, int fheight);
  
};



#endif
