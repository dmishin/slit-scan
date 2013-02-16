#include "compare_rgb_blocks.hpp"
#include <iostream>
#include <cmath>

using namespace std;
inline int sqr( int x ){
  return x*x;
}

double compare_blocks( uint8 *block1, size_t interline1,
		       uint8 *block2, size_t interline2,
		       size_t width, size_t height)
{
  double s = 0;
  for (size_t y = 0; y < height; ++y ){
    double row_s = 0;
    uint8 *row1, *row2;
    row1 = block1 + (interline1 * y);
    row2 = block2 + (interline2 * y);
    for( size_t x = 0; x < width*3; x+=3 ){
      row_s += sqr( (int)row1[x  ] - (int)row2[x  ]); //R
      row_s += sqr( (int)row1[x+1] - (int)row2[x+1]); //G
      row_s += sqr( (int)row1[x+2] - (int)row2[x+2]); //B
    }
    s += row_s;
  }
  return sqrt(s / width / height);
}

void match_blocks( uint8 *block1, size_t interline1,
		   uint8 *block2, size_t interline2,
		   size_t width, size_t height,
		   int dx_range, int dy_range,
		   int &dx_best, int &dy_best,
		   double &dbest, double &dworst)
{
  if (block1 == block2){
    cerr << "Blocks refer the same area"<<endl;
  };
  bool first = true;
  double diff_best, diff_worst;
  for ( int x = -dx_range; x <= dx_range; ++x ){
    for ( int y = -dy_range; y <= dy_range; ++y ){
      int offset2 = x*3 + y * interline2;
      double diff = compare_blocks(  block1, interline1,
				     block2 + offset2, interline2,
				     width, height );
      if (first || diff < diff_best){
	dx_best = x;
	dy_best = y;
	diff_best = diff;
      }
      if (first || diff > diff_worst)
	diff_worst = diff;
      first = false;
    }
  }
  dbest = diff_best;
  dworst = diff_worst;
}
