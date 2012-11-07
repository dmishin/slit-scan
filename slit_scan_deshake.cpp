#include "slit_scan_deshake.hpp"

struct PixelT{
  unsigned char R,G,B;
};

struct FrameT{
  int w, h;
  std::vector<PixelT> pixels;
  FrameT( int ww, int hh ):w(ww),h(hh){ pixels.resize(ww*hh); };
  size_t idx(int x, int y)const{ return x + y * w; };
};

float merge(float a,float b,float t){
  return a+(b-a)*t;
};

float merge2( float x00, float x01, float x10, float x11, float p, float q)
{
  return merge( merge( x00, x01, p),
		merge( x10, x11, p),
		q );
}

bool interpolate( const FrameT &f, float x, float y, float &(fRGB[3]) )
{
  int ix, iy;
  ix = (int)x; iy = (int)y;
  if (ix < 0 || iy < 0 ||
      ix+1 >= f.w || iy+1 >= f.h ) return false;
  float px, py;
  px = x - ix; py = y-iy;
  
  int idx = f.idx(ix,iy);
  const PixelT &p00(f.pixels[idx]);
  const PixelT &p01(f.pixels[idx+1]);
  const PixelT &p10(f.pixels[idx+f.w]);
  const PixelT &p11(f.pixels[idx+1+f.w]);

  fRGB[0] = merge2( p00.R, p01.R, p10.R, p11.R, px, py );
  fRGB[1] = merge2( p00.G, p01.G, p10.G, p11.G, px, py );
  fRGB[2] = merge2( p00.B, p01.B, p10.B, p11.B, px, py );
  return true;
}

float compare_pixels( const PixelT &p0, const FrameT &f, float x, float y )
{
  float fRGB[3];
  if (! interpolate( f, x, y, fRGB ))
    return -1;
  return 
    fabs( fRGB[0] - p0.R ) +
    fabs( fRGB[1] - p0.G) +
    fabs( fRGB[2] - p0.B);
}


float match_images( const FrameT &f1, const FrameT &f2,
		    float dx, float dy, float sin_f, float cos_f )
{
  float weight = 0;
  float dist = 0;

  int pix_idx=0;
  for( int y =0; y < f2.h; ++y ){
    for( int x =0; x < f2.w; ++x, ++pix_idx ){
      float xx = dx + x*cos_f - y * sin_f;
      float yy = dy + x*sin_f + x * cos_f;
      float cmp = compare_pixels( f2.pixels[pix_idx], f1, xx, yy );
      if (cmp > 0){
	weight += 1;
	dist += cmp;
      }
    }
  }
  return dist / weight;
}


/**Construct a transformation, that maps center of f2 to the center of f1, 
   rotating it by given angle
   and shifting by offset
*/
void make_transformation( const FrameT &f1, const FrameT &f2,
			  const float & (angle_dx_dy[3]),
			  float &dx1, float &dy1, float &sin_a, float &cos_a)
{
  sin_a = (float)sin(angle_dx_dy[0]);
  cos_a = (float)cos(angle_dx_dy[0]);
  // Must map center to center, if without dx
  
  float 
    cx2 = f2.w*0.5f,
    cy2 = f2.h*0.5f,
    cx1 = f1.w*0.5f,
    cy1 = f1.h*0.5f;
  dx1 = angle_dx_dy[1] + cx1 - (cx2*cos_a - cy2*sin_a);
  dy1 = angle_dx_dy[2] + cy1 - (cx2*sin_a + cy2*cos_a);
}


void dumb_minimizer( float &(axy0[3]), float &(axy1[3]),
		     const FrameT &f1, const FrameT &f2,
		     int steps
		     
