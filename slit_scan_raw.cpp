#include <iostream>
#include <fstream>
#include <vector>
#include <cstdlib>
#include <sstream>
//pacman -S pstreams
#include <pstreams/pstream.h>

using namespace std;

const int pixel_octets = 3;

int process_raw_data( int max_frames, int w, int h, int pos, std::istream &raw_video, std::ostream &ostream );

int main( int argc, char * argv[] )
{
  if (argc != 4){
    cerr << "Arguments must be: width height slit_position source_raw" <<endl;
    return 1;
  }

  int w, h, pos;
  w = atoi( argv[1] );
  h = atoi( argv[2] );
  //pos = atoi( argv[3] );
  pos = w / 2;
  string input = argv[3];

  if ( w <=0 || w > 10000 ){
    cerr << "Width "<<w<<" is incorrect"<<endl;
    return 1;
  }
  if ( h <=0 || h > 10000 ){
    cerr << "Height "<<h<<" is incorrect"<<endl;
    return 1;
  }
  if ( pos < 0 || pos >= w ){
    cerr << "Position must be between 0 and "<<w<<endl;
    return 1;
  }
  string output = "output_raw.data";
  string converted_name = "output.png";
  ofstream ostream( output.c_str(), ios_base::binary );
  ifstream istream( input.c_str(), ios_base::binary );

  int frames = process_raw_data( 10000, w, h, pos, istream, ostream );
  //convert -size 360x1072 -depth 8 rgb:output_raw.data -transpose  image.jpg

  
  stringstream command;
  command << "convert -size "<<h<<"x"<<frames<<" -depth 8 rgb:"<<output<<" -transpose "<<converted_name;
  ostream.close();
  system( command.str().c_str() );
}

int process_raw_data( int max_frames, int w, int h, int pos, std::istream &raw_video, std::ostream &ostream )
{
  std::vector<char> column;
  std::vector<char> row_buffer;
  row_buffer.resize( w*pixel_octets );
  column.resize(h*pixel_octets);

  int frame = 0;
  do{
    for (int row=0; row < h; ++ row){
      raw_video.read( &(row_buffer[0]), row_buffer.size() );
      for(int i = 0; i < pixel_octets; ++i){
	column[row*pixel_octets + i] = row_buffer[ pos*pixel_octets + i ];
      }
    }
    ostream.write( &(column[0]), column.size() );
    frame ++;
  }while (raw_video && (max_frames<=0 || frame < max_frames));
  cout << "Done reading "<<frame<<" frames"<<endl;
  return frame;
}


void process_pstream( const std::string ifile )
{
  // run a process and create a streambuf that reads its stdout and stderr
  redi::ipstream proc("./some_command", redi::pstreams::pstderr);

  //ffmpeg -i "$source"  -vcodec rawvideo -f rawvideo -pix_fmt rgb24 -
  

}
