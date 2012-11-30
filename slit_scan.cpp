#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <sstream>
#include <cassert>

#include "optionparser.h"
#include "ffmpeg_decoder.hpp"
#include "compare_rgb_blocks.hpp"

using namespace std;

enum SlitOrientation{
  SlitVertical, SlitHorizontal
};
class OffsetDeshaker;

class AbstractOffsetDeshaker: public FrameHandler{
public:
  virtual void get_frame_offset( double &dx, double &dy )=0;
};

class SlitExtractor: public FrameHandler{
public:
  double slit_position_rel;
  size_t slit_position;
  SlitOrientation orientation;
  std::ostream &output;
  size_t width, height;
  size_t frames;
  uint8_t *buffer;
  AbstractOffsetDeshaker *deshaker;
public:
  SlitExtractor( double pos, SlitOrientation o, std::ostream &output_ );
  ~SlitExtractor();
  virtual bool handle(AVFrame *pFrame, AVFrame *pFrameOld, int width, int height, int iFrame);
  int slit_width()const;
  int frames_processed()const{ return frames; };
  void set_deshaker( AbstractOffsetDeshaker *d){ deshaker = d; };
private:
  void extract_vertical(AVFrame *pFrame);
  void extract_horizontal(AVFrame *pFrame);
  int get_perpendicular_size()const; //size of the frame in direction, perpendicular to the slit
};

class OffsetDeshaker: public AbstractOffsetDeshaker{
  int x0, y0, dx, dy;
  int width, height;
  double dx_accum, dy_accum;
  double dissipation_rate;
public:
  OffsetDeshaker( int x0, int y0, int dx, int dy, int w, int h );
  virtual ~OffsetDeshaker(){};
  virtual bool handle(AVFrame *pFrame, AVFrame *pFrameOld, int width, int height, int iFrame);
  virtual void get_frame_offset( double &dx, double &dy );
};


/** Extract file name without extension from the path*/
std::string base_name( const std::string &path );

////////////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////////////
void OffsetDeshaker::get_frame_offset( double &dx, double &dy )
{
  dx = dx_accum;
  dy = dy_accum;
}

OffsetDeshaker::OffsetDeshaker( int x0_, int y0_, int dx_, int dy_, int w, int h )
  :x0(x0_), y0(y0_), dx(dx_), dy(dy_)
  ,width(w), height(h)
{
  dx_accum = 0;
  dy_accum = 0;
  dissipation_rate = pow( .5, 1.0 / 20 ); //half-return in 20 frames
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
  //cout << "Frame "<<iFrame<<" dx="<<deltaX<<" dy="<<deltaY<<endl;
  cout << "Dbest = "<<d<<" Dworst="<<dworst<<" Rate:"<<dworst / d<<endl;
  cout << " dxa = "<<dx_accum<<" dya = "<<dy_accum<<endl;
  return true;
}


SlitExtractor::SlitExtractor(double pos, SlitOrientation o, std::ostream &output_)
    :slit_position_rel(pos)
    ,orientation(o)
    ,output(output_)
{
  assert( pos >=0 && pos <= 1.0 );
  frames = 0;
  slit_position = 0;
  buffer = NULL;
}
SlitExtractor::~SlitExtractor()
{
  delete[] buffer;
  buffer = NULL;
}

bool SlitExtractor::handle(AVFrame *pFrame, AVFrame *pFrameOld, int width_, int height_, int iFrame)
{
  if (deshaker) deshaker->handle( pFrame, pFrameOld, width_, height_, iFrame);
  if (frames == 0){
    //first frame: perform some initialization
    width = width_;
    height = height_;
    slit_position = (int)((get_perpendicular_size()-1)*slit_position_rel);
  }else{
    if (width_ != width || height_ != height )
      throw logic_error( "Frame size changed during playback. How this can be?" );
  }

  switch (orientation){
  case SlitVertical:     extract_vertical(pFrame);
    break;
  case SlitHorizontal:   extract_horizontal(pFrame);
    break;
  }
  frames ++;
  return true;
}
int SlitExtractor::get_perpendicular_size()const
{
  switch( orientation ){
  case SlitVertical:
    return width;
  case SlitHorizontal:
    return height;
  }
}
int SlitExtractor::slit_width()const
{
  switch( orientation ){
  case SlitVertical:
    return height;
  case SlitHorizontal:
    return width;
  }
}
void SlitExtractor::extract_vertical(AVFrame *pFrame)
{
  if (buffer == NULL){
    buffer = new uint8_t[ height * 3 ];
  }
  int dx=0, dy=0;
  if ( deshaker ){
    double ddx, ddy;
    deshaker->get_frame_offset(ddx, ddy);
    dx = (int)round( ddx );
    dy = (int)round( ddy );
  }
  for( int t = 0; t < height; ++t ){
    int bpos = t*3;
    int x = slit_position + dx;
    int y = t + dy;
    if ( x < 0 || x >= width || y < 0 || y >= height ){
      buffer[bpos] = 0;
      buffer[bpos+1] = 0;
      buffer[bpos+2] = 0;
    }else{
      int fpos = x * 3 + y * pFrame->linesize[0];

      buffer[bpos] = pFrame->data[0][fpos];
      buffer[bpos+1] = pFrame->data[0][fpos+1];
      buffer[bpos+2] = pFrame->data[0][fpos+2];
    }
  }
  output.write((const char*)buffer, height * 3);
}
void SlitExtractor::extract_horizontal(AVFrame *pFrame)
{
  if (buffer == NULL){
    buffer = new uint8_t[ width * 3 ];
  }
  int dx=0, dy=0;
  if ( deshaker ){
    double ddx, ddy;
    deshaker->get_frame_offset(ddx, ddy);
    dx = (int)round( ddx );
    dy = (int)round( ddy );
  }
  for( int t = 0; t < width; ++t ){
    int bpos = t*3;
    int x = t + dx;
    int y = slit_position + dy;
    if ( x < 0 || x >= width || y < 0 || y >= height ){
      buffer[bpos] = 0;
      buffer[bpos+1] = 0;
      buffer[bpos+2] = 0;
    }else{
      int fpos = x * 3 + y * pFrame->linesize[0];

      buffer[bpos] = pFrame->data[0][fpos];
      buffer[bpos+1] = pFrame->data[0][fpos+1];
      buffer[bpos+2] = pFrame->data[0][fpos+2];
    }
  }

  output.write((const char*)buffer, width * 3);
}

enum  optionIndex { UNKNOWN, HELP, OUTPUT, RAW_OUTPUT, ORIENTATION, POSITION, STABILIZE };

const option::Descriptor usage[] =
{
 {UNKNOWN, 0, "", "",option::Arg::None, "USAGE: converter [options] source.mpg\n\n"
                                        "Options:" },
 {HELP, 0,"", "help",option::Arg::None, 
  "  --help  \tPrint usage and exit." },
 {OUTPUT, 0,"o","output",option::Arg::Optional, 
  "  --output, -o  \tSpecify output image file (default is INPUT_FILE_NAME.png)." },
 {RAW_OUTPUT, 0,"","raw-output",option::Arg::Optional, 
  "  --raw-output \tSpecify intermediate raw output image file (default is output.raw)." },
 {POSITION, 0,"p","position",option::Arg::Optional, 
  "  --position, -p  \tSpecify position of the slit (default is 50%)" },
 {ORIENTATION, 0,"-r","orientation",option::Arg::Optional, 
  "  --orientation, -r  \tSlit orientation: vertical | v | horizontal | h." },
 {STABILIZE, 0, "-s", "stabilize", option::Arg::Optional,
  "  --stabilize, -s \tUse simple image stabilization to reduce shaking"},

 {0,0,0,0,0,0}
};


SlitOrientation parse_orientation(const std::string &orientation)
{
  if (orientation.compare("vertical") == 0) return SlitVertical;
  if (orientation.compare("v") == 0) return SlitVertical;
  if (orientation.compare("horizontal") == 0) return SlitHorizontal;
  if (orientation.compare("h") == 0) return SlitHorizontal;
  throw invalid_argument(string("Unknown orientation: ")+orientation);
}
const char * null_to_empty( const char * s )
{
  if (! s) return "";
  else return s;
}
struct Options{
  SlitOrientation orientation;
  string output;
  string raw_output;
  double position;
  string input_file;
  Options()
    :orientation(SlitVertical)
    ,raw_output("output.raw")
    ,position(50.0)
  {}
  bool parse( int argc, char *argv[] );
};

bool Options::parse(int argc, char *argv[])
{
  if (argc > 0){ //skip program name
    argc --;
    argv ++;
  };
  option::Stats  stats(usage, argc, argv);
  option::Option* options = new option::Option[stats.options_max];
  option::Option* buffer  = new option::Option[stats.buffer_max];
  option::Parser parse(usage, argc, argv, options, buffer);
  if (parse.error())
    throw invalid_argument("Failed to parse options");

  if (options[HELP] || argc == 0) {
    option::printUsage(cout, usage);
    return false;
  }

  if (options[ORIENTATION])
    orientation = parse_orientation(options[ORIENTATION].last()->arg);

  if (options[OUTPUT])
    output = null_to_empty(options[OUTPUT].last()->arg);

  if (options[RAW_OUTPUT])
    raw_output = null_to_empty(options[RAW_OUTPUT].last()->arg);

  if (options[POSITION]){
    stringstream ss(null_to_empty(options[POSITION].last()->arg));
    if (! (ss >> position) ) 
      throw invalid_argument("Invalid numeric value");
    if (position < 0 || position > 100)
      throw invalid_argument("Position must be floating-point value in range [0..100] (percents)");
  }

  if (parse.nonOptionsCount() != 1){
    stringstream ss; ss<<"Must have 1 argument: input file";
    throw std::invalid_argument(ss.str());
  }
  input_file = parse.nonOption(0);

  if (!options[OUTPUT]){
    output = base_name( input_file ) + ".png";
    if (output.empty())
      output = "output.png";
  }

  cout << "Passed options:\n"
       << " Orientaion:"<<orientation<<endl
       << " Position:"<<position<<endl
       << " Input:"<<input_file<<endl
       << " Raw output:"<<raw_output<<endl
       << " Output:"<<output<<endl;
  return true;
}

/** Extract file name without extension from the path*/
std::string base_name( const std::string &path )
{
  size_t slash_pos = path.rfind('/');
  size_t dot_pos = path.rfind('.');
  size_t start_pos, end_pos;
  if (slash_pos == path.npos)
    start_pos = 0;
  else
    start_pos = slash_pos + 1;
  if (dot_pos == path.npos || dot_pos <= slash_pos )
    end_pos = path.size();
  else
    end_pos = dot_pos;
  return path.substr( start_pos, end_pos - start_pos );
}

int main( int argc, char *argv[] )
{
  Options options;
  try{
    if ( !options.parse(argc, argv) )
      return 0;
  }catch(std::exception &err){
    cerr << "Failed to parse options:"<<err.what()<<endl;
    return 1;
  }

  ofstream ostream( options.raw_output.c_str(), ios_base::binary );

  SlitExtractor extractor( options.position*0.01, options.orientation, ostream );
  OffsetDeshaker deshaker( 30, 30, 20, 20, 100, 100 );
  extractor.set_deshaker( &deshaker );
  // Register all formats and codecs
  av_register_all();
  try{
    process_ffmpeg_file( options.input_file.c_str(), extractor );
  }catch(std::exception &err){
    cerr << "Error processing file:"<<err.what()<<endl;
    return 1;
  }

  if (extractor.frames_processed() == 0){
    cerr << "No frames were processed"<<endl;
    return 1;
  }
  //convert -size 360x1072 -depth 8 rgb:output_raw.data -transpose  image.jpg
  stringstream command;
  command << "convert -size "<<extractor.slit_width()<<"x"<<extractor.frames_processed()
	  <<" -depth 8 rgb:\""<<options.raw_output<<"\""
	  <<" -transpose "
	  << "\""<<options.output<<"\"";
  return system( command.str().c_str() );
}

