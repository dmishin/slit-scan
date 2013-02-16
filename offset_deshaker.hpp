#ifndef __OFFSED_DESHAKER_INCLUDED__
#define __OFFSED_DESHAKER_INCLUDED__
#include <algorithm>
#include "abstract_offset_deshaker.hpp"


struct median3_filter{
  double x[3];
  int pos;
  bool initialized;
  median3_filter(): initialized(false){};
  void put(double xx){
    if (! initialized){
      x[0]=x[1]=x[2]=xx;
      pos = 1;
    }else{
      x[pos]=xx;
      pos = (pos +1)%3;
    };
  };
  double get()const{
    double a=x[0], b=x[1], c=x[2];
    if (a>b){
      if (a>c) return std::max(b,c);
      else     return a;
    }else{ //a < b
      if (a < c) return std::min(b,c);
      else      return a;
    }
  };
};

class OffsetDeshaker: public AbstractOffsetDeshaker{
  int x0, y0, dx, dy;
  int width, height;
  int dx_old, dy_old;
  double dx_accum, dy_accum;
  double dissipation_rate;
  uint8_t * key_frame;
  median3_filter x_filter, y_filter;
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
