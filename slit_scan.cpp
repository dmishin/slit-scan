#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <sstream>
#include <cassert>
#include <vector>

#include "optionparser.h"
#include "ffmpeg_decoder.hpp"
//#include "keyframe_deshaker.hpp"
#include "offset_deshaker.hpp"

const int default_stab_range_y = 10;
const int default_stab_range_x = 10;
const int default_relaxation_frames = 200;


using namespace std;

enum SlitOrientation{
  SlitVertical, SlitHorizontal
};

class SlitExtractor: public FrameHandler{
public:
  double slit_position_rel;
  size_t slit_position;
  SlitOrientation orientation;
  std::ostream &output;
  int skip_frames;
  size_t width, height;
  size_t frames;
  uint8_t *buffer;
  AbstractOffsetDeshaker *deshaker;
public:
  SlitExtractor( double pos, SlitOrientation o, std::ostream &output_, int skip_frames );
  ~SlitExtractor();
  virtual bool handle(AVFrame *pFrame, int width, int height, int iFrame);
  int slit_width()const;
  int frames_processed()const{ return frames; };
  void set_deshaker( AbstractOffsetDeshaker *d){ deshaker = d; };
private:
  void extract_vertical(AVFrame *pFrame);
  void extract_horizontal(AVFrame *pFrame);
  int get_perpendicular_size()const; //size of the frame in direction, perpendicular to the slit
};


/** Extract file name without extension from the path*/
std::string base_name( const std::string &path );

////////////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////////////
SlitExtractor::SlitExtractor(double pos, SlitOrientation o, std::ostream &output_, int skip_frames_ )
    :slit_position_rel(pos)
    ,orientation(o)
    ,output(output_)
    ,skip_frames(skip_frames_)
    ,deshaker(NULL)
{
  assert( pos >=0 && pos <= 1.0 );
  assert( skip_frames >= 0 );
  frames = 0;
  slit_position = 0;
  buffer = NULL;
  deshaker = NULL;
}

SlitExtractor::~SlitExtractor()
{
  delete[] buffer;
  buffer = NULL;
}

bool SlitExtractor::handle(AVFrame *pFrame, int width_, int height_, int iFrame)
{
  if (iFrame < skip_frames) return true;//skipping
  if (deshaker) deshaker->handle( pFrame, width_, height_, iFrame);
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
    dx = -(int)round( ddx );
    dy = -(int)round( ddy );
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
    dx = -(int)round( ddx );
    dy = -(int)round( ddy );
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
/*
void read_interpolated( AVFrame * rgb_frame, double x, double y,
			uint8_t *rgb )
{
  int ix = (int)floor(x);
  int iy = (int)floor(y);
  double px = x - floor(x);
  double py = y - floor(y);
  
}
*/
enum  optionIndex { UNKNOWN, HELP, OUTPUT, RAW_OUTPUT, ORIENTATION, POSITION, STABILIZE, SCALE, SKIP_FRAMES };

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
  "  --stabilize, -s \tUse simple image stabilization to reduce shaking. Format: x:y:w:h[:rx:ry:relax_frames]"},
 {SCALE, 0, "-k", "scale", option::Arg::Optional,
  "  --scale, -k \tScale image before applying deshake. Integer 1..4"},
 {SKIP_FRAMES, 0, "-S", "skip-frames", option::Arg::Optional,
  "  --skip-frames=N, -S \tSkip N first frames from the video (useful for skipping iitial shake)" },

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
  struct Box{
    int x, y, w, h;
  };
  SlitOrientation orientation;
  string output;
  string raw_output;
  double position;
  string input_file;
  int scale;
  int skip_frames;

  //stabilization options
  bool stabilize; //enable or disable
  Box stabilize_box; //box
  int stabilize_search_range_x, stabilize_search_range_y; //search range
  int stabilize_relaxation_frames; //

  Options()
    :orientation(SlitVertical)
    ,raw_output("output.raw")
    ,position(50.0)
    ,stabilize(false)
    ,stabilize_search_range_x(10)
    ,stabilize_search_range_y(10)
    ,stabilize_relaxation_frames(200)
    ,scale(1)
    ,skip_frames(0)
  {}
  bool parse( int argc, char *argv[] );
private:
  void parse_stabilization_options( const char *opts );
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
  if (options[SCALE]){
    stringstream ss(null_to_empty(options[SCALE].last()->arg)); 
    if (! (ss >> scale) )
      throw invalid_argument("Scale incorrect");
    if (scale < 1 || scale > 4)
      throw invalid_argument("Scale must be in range 1..4");
  }
  if (options[SKIP_FRAMES]){
    stringstream ss(null_to_empty(options[SKIP_FRAMES].last()->arg)); 
    if (! (ss >> skip_frames)) throw invalid_argument("Failed to parse frame number");
    if (skip_frames < 0) throw invalid_argument("Number of frames must be positive");
  }
  if (options[STABILIZE]){
    parse_stabilization_options(null_to_empty(options[STABILIZE].last()->arg));
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
       << " Output:"<<output<<endl
       << " Scale:"<<scale<<endl
       << " Skip frames:"<<skip_frames<<endl;
  
  if (stabilize){
    cout << " Stabilization enabled. Box:"<<endl
	 << " x0:"<<stabilize_box.x<<" y0:"<<stabilize_box.y<<" w:"<<stabilize_box.w<<" h:"<<stabilize_box.h<<endl
	 << " range x:"<<stabilize_search_range_x<<" y:"<<stabilize_search_range_y<<endl
	 << " Relax in "<<stabilize_relaxation_frames<<" frames"<<endl;
  }
  return true;
}

int str2int( const std::string &s, int default_, bool raise_error )
{
  std::stringstream ss(s);
  int rval;
  if (s.empty()){
    if (raise_error)
      throw std::invalid_argument("Numeric value is required");
    else
      return default_;
  }
  if (! (ss >> rval)){
    throw std::invalid_argument("Failed to parse numeric value");
  }
  return rval;
}

size_t split_by( const std::string s, char sep, std::vector<std::string> &parts )
{
  using namespace std;
  size_t count = 0;
  size_t pos = 0;
  while (pos < s.size()){
    size_t sep_pos = s.find(sep, pos);
    count ++;
    if (sep_pos == s.npos){
      parts.push_back( s.substr( pos ) );
      break;
    }else{
      parts.push_back( s.substr( pos, sep_pos - pos ) );
      pos = sep_pos + 1;
    }
  }
  return count;
}
void Options::parse_stabilization_options( const char * sopts )
{
  //sopts is a :-separated list of integers:
  // x0:y0:w:h:rx:ry:reltime
  //
  // first 4 are required; other are optional
  std::vector<std::string> parts;
  split_by( sopts, ':', parts );
  if (parts.size() > 7)
    throw std::invalid_argument( "Stabilization options must have at most 7 values" );
  if (parts.size() < 4){
    stringstream msg;
    msg << "Stabilization options must have at least 4 values: x0:y0:w:h:range_x:range_y ";
    msg << parts.size() << " parsed instead: " << sopts;
    throw std::invalid_argument( msg.str() );
  }
  stabilize = true;

  stabilize_box.x = str2int( parts[0], 0, true );
  stabilize_box.y = str2int( parts[1], 0, true );
  stabilize_box.w = str2int( parts[2], 0, true );
  stabilize_box.h = str2int( parts[3], 0, true );

  if (parts.size() >= 5)
  stabilize_search_range_x = str2int( parts[4], default_stab_range_x, false );
  if (parts.size() >= 6)
  stabilize_search_range_y = str2int( parts[5], default_stab_range_y, false );
  if (parts.size() >= 7)
  stabilize_relaxation_frames = str2int( parts[6], default_relaxation_frames, false );
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

  SlitExtractor extractor( options.position*0.01, options.orientation, ostream, options.skip_frames );
  OffsetDeshaker * deshaker = NULL;
  if ( options.stabilize ){
    int s = options.scale;
    deshaker = new 
      OffsetDeshaker(options.stabilize_box.x*s, options.stabilize_box.y*s, 
		     options.stabilize_search_range_x*s, options.stabilize_search_range_y*s, 
		     options.stabilize_box.w*s, options.stabilize_box.h*s,
		     options.stabilize_relaxation_frames );
    extractor.set_deshaker( deshaker );
  }
  // Register all formats and codecs
  av_register_all();
  try{
    process_ffmpeg_file( options.input_file.c_str(), extractor, options.scale );
  }catch(std::exception &err){
    cerr << "Error processing file:"<<err.what()<<endl;
    return 1;
  }

  if (extractor.frames_processed() == 0){
    cerr << "No frames were processed"<<endl;
    return 1;
  }
  delete deshaker; deshaker = NULL;
  //convert -size 360x1072 -depth 8 rgb:output_raw.data -transpose  image.jpg
  stringstream command;
  command << "convert -size "<<extractor.slit_width()<<"x"<<extractor.frames_processed()
	  <<" -depth 8 rgb:\""<<options.raw_output<<"\""
	  <<" -transpose "
	  << "\""<<options.output<<"\"";
  return system( command.str().c_str() );
}

