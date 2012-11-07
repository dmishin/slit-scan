#include <iostream>
#include <fstream>
#include <vector>
#include <cstdlib>
#include <sstream>
//pacman -S pstreams
#include <pstreams/pstream.h>
#include <stdexcept>
#include <sstream>
#include "optionparser.h"

using namespace std;


struct PixelT{
  unsigned char R, G, B;
};

int process_raw_data( int max_frames, int w, int h, int pos, std::istream &raw_video, std::ostream &ostream );
int extract_column_simple( int max_frames, int w, int h, int pos, std::istream &raw_video, std::ostream &ostream );
int extract_row_simple   ( int max_frames, int w, int h, int pos, std::istream &raw_video, std::ostream &ostream );


enum  optionIndex { UNKNOWN, HELP, OUTPUT, RAW_OUTPUT, ORIENTATION, POSITION };

const option::Descriptor usage[] =
{
 {UNKNOWN, 0, "", "",option::Arg::None, "USAGE: converter [options] width height source\n\n"
                                        "Options:" },
 {HELP, 0,"", "help",option::Arg::None, 
  "  --help  \tPrint usage and exit." },
 {OUTPUT, 0,"o","output",option::Arg::Optional, 
  "  --output, -o  \tSpecify output image file (default is output.png)." },
 {RAW_OUTPUT, 0,"","raw-output",option::Arg::Optional, 
  "  --raw-output \tSpecify raw output image file (default is output.raw)." },
 {POSITION, 0,"p","position",option::Arg::Optional, 
  "  --position, -p  \tSpecify position of the slit (default is 50%)" },
 {ORIENTATION, 0,"-r","orientation",option::Arg::Optional, 
  "  --orientation, -r  \tSlit orientation: vertical | v | horizontal | h." },

 {0,0,0,0,0,0}
};

enum SlitOrientation{
  SlitVertical, SlitHorizontal
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
  int w, h;
  string input_file;
  Options()
    :orientation(SlitVertical)
    ,output("output.png")
    ,raw_output("output.raw")
    ,position(50.0)
    ,w(0), h(0)
  {}
  bool parse( int argc, char *argv[] );
};
int parse_int( const char *s )
{
  stringstream ss(s);
  int rval=0;
  if ( !(ss>>rval) ) throw invalid_argument("Failed to parse integer");
  return rval;
}
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
    throw std::invalid_argument("Options parsing failed");

  if (options[HELP] || argc == 0) {
    option::printUsage(cout, usage);
    return false;
  }

  if (options[ORIENTATION])
    orientation = parse_orientation(options[ORIENTATION].last()->arg);

  if (options[OUTPUT])
    output = null_to_empty(null_to_empty(options[OUTPUT].last()->arg));

  if (options[RAW_OUTPUT])
    raw_output = null_to_empty(null_to_empty(options[RAW_OUTPUT].last()->arg));

  if (options[POSITION]){
    stringstream ss(null_to_empty(options[POSITION].last()->arg));
    if (! (ss >> position) ) throw std::invalid_argument("Invalid number");
    if (position < 0 || position > 100)
      throw std::invalid_argument("Postion must be in range [0..100]");
  }

  if (parse.nonOptionsCount() != 3){
    stringstream ss; ss<<"Must have 3 arguemnts: width, height and input file, but have "<<parse.nonOptionsCount()<<" :";
    for (int i = 0; i < parse.nonOptionsCount(); ++i)
      ss << "#" << i+1 << ": [" << parse.nonOption(i) << "] ";
    throw std::invalid_argument(ss.str());
  }
  w = parse_int(parse.nonOption(0));
  h = parse_int(parse.nonOption(1));
  input_file = parse.nonOption(2);
  //PARSE positional arguments
  if ( w <=0 || w > 10000 ){
    stringstream ss; ss<< "Width "<<w<<" is incorrect"<<endl;
    throw std::invalid_argument(ss.str());
  }
  if ( h <=0 || h > 10000 ){
    stringstream ss; ss<< "Height "<<h<<" is incorrect"<<endl;
    throw std::invalid_argument(ss.str());
  }
  cout << "Passed options:\n"
       << " Orientaion:"<<orientation<<endl
       << " Position:"<<position<<endl
       << " Input:"<<input_file<<endl
       << " Raw output:"<<raw_output<<endl
       << " Output:"<<output<<endl;
  return true;
}

int main( int argc, char *argv[] )
{
  Options options;
  try{
    if ( !options.parse(argc, argv) )
      return 0;
  }catch(std::invalid_argument err){
    cerr << "Failed to parse options:"<<err.what()<<endl;
    return 1;
  }catch(std::exception err){
    cerr << "Failed to parse options:"<<err.what()<<endl;
    return 1;
  }

  ofstream ostream( options.raw_output.c_str(), ios_base::binary );
  ifstream istream( options.input_file.c_str(), ios_base::binary );

  int frames;
  int row_size;
  switch (options.orientation){
  case SlitVertical:
    frames = extract_column_simple( 1000000, options.w, options.h, 
				    int(options.position*options.w/100), istream, ostream );
    row_size = options.h;
    break;
  case SlitHorizontal:
    frames = extract_row_simple   ( 1000000, options.w, options.h, 
				    int(options.position*options.h/100), istream, ostream );
    row_size = options.w;
    break;
  }
  ostream.close();

  //convert -size 360x1072 -depth 8 rgb:output_raw.data -transpose  image.jpg
  stringstream command;
  command << "convert -size "<<row_size<<"x"<<frames
	  <<" -depth 8 rgb:\""<<options.raw_output<<"\""
	  <<" -transpose "
	  << "\""<<options.output<<"\"";
  system( command.str().c_str() );
  return 0;
}

void read_frame( std::vector<PixelT> &frame, int w, int h, std::istream &raw_video)
{
  frame.resize(w*h);
  unsigned char buf[3];
  int pos = 0;
  for (int y = 0; y < h; ++y ){
    for (int x = 0; x < w; ++ x, ++pos){
      raw_video.read( (char*)buf, 3 );
      PixelT &pix(frame[pos]);
      pix.R = buf[0];
      pix.G = buf[1];
      pix.B = buf[2];
    }
  }
}

void extract_column( const std::vector<PixelT> &frame, int w, int col, std::vector<PixelT> &column )
{
  int pos = col;
  int idx = 0;
  while ( pos < frame.size() ){
    column[idx] = frame[pos];
    idx ++;
    pos += w;
  }
}
void write_column( const std::vector<PixelT> &column, std::ostream &ostream )
{
  std::vector<PixelT>::const_iterator i, e = column.end();
  unsigned char buf[3];
  for( i = column.begin(); i != e; ++ i){
    buf[0] = i->R;
    buf[1] = i->G;
    buf[2] = i->B;
    ostream.write( (char *)buf, 3 );
  }
}
int process_raw_data( int max_frames, int w, int h, int pos, std::istream &raw_video, std::ostream &ostream )
{
  std::vector<PixelT> frame;
  frame.resize(w*h);

  std::vector<PixelT> column;
  column.resize(h);

  int frames = 0;
  do{
    read_frame( frame, w, h, raw_video );
    extract_column( frame, w, pos, column );
    write_column( column, ostream );
    frames ++;
  }while (raw_video && (max_frames<=0 || frames < max_frames));
  cout << "Done reading "<<frames<<" frames"<<endl;
  return frames;
}
int extract_column_simple( int max_frames, int w, int h, int pos, std::istream &raw_video, std::ostream &ostream )
{
  std::vector<char> row_buffer;
  row_buffer.resize(w*3);
  int frame = 0;
  while ( raw_video && (max_frames <= 0 || frame < max_frames) ){
    for (int y = 0; y < h; ++y ){ 
      raw_video.read( &(row_buffer[0]), row_buffer.size() );
      ostream.write( &(row_buffer[pos*3]), 3 );
    }
    frame ++;
  };
  return frame;
}
int extract_row_simple( int max_frames, int w, int h, int pos, std::istream &raw_video, std::ostream &ostream )
{
  std::vector<char> row_buffer;
  row_buffer.resize(w*3);
  int frame = 0;
  while ( raw_video && (max_frames <= 0 || frame < max_frames) ){
    for (int y = 0; y < h; ++y ){ 
      raw_video.read( &(row_buffer[0]), row_buffer.size() );
      if (y == pos)
	ostream.write( &(row_buffer[0]), row_buffer.size() );
    }
    frame ++;
  };
  return frame;
}

void process_pstream( const std::string ifile )
{
  // run a process and create a streambuf that reads its stdout and stderr
  redi::ipstream proc("./some_command", redi::pstreams::pstderr);

  //ffmpeg -i "$source"  -vcodec rawvideo -f rawvideo -pix_fmt rgb24 -
  

}
