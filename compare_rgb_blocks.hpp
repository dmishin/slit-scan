#ifndef __COMPARE_RGB_BLOCKS_INCUDED__
#define __COMPARE_RGB_BLOCKS_INCUDED__

#include <cstdlib>

typedef unsigned char uint8;

void match_blocks( uint8 *block1, size_t interline1,
		   uint8 *block2, size_t interline2,
		   size_t width, size_t height,
		   int dx_range, int dy_range,
		   int &dx_best, int &dy_best);

#endif
