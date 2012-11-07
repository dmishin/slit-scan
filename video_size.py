#!/usr/bin/env python
import os
import sys
import subprocess
import json


def ffprobe_video( fname ):
    cmd_line = ["ffprobe", fname, 
                "-v", "quiet", 
                "-print_format", "json", 
                "-show_format", 
                "-show_streams"]
    data = subprocess.check_output(cmd_line)
    return json.loads(data.decode("utf-8"))

def get_video_size( video_data ):
    for strm in video_data["streams"]:
        if strm["codec_type"] == "video":
            return (strm["width"], strm["height"])
    raise ValueError("No video stream")

def convert_video( fname, fifo_name="raw_data.fifo" ):
    """
    mkfifo raw_data.fifo
    ffmpeg -i "$1"  -vcodec rawvideo -f rawvideo -pix_fmt rgb24 -y ./raw_data.fifo &
    ./converter `./video_size.py "$source"` raw_data.fifo
    rm raw_data.fifo
    """
    if os.path.exists( fifo_name ):
        raise ValueError("FIFO already exists. Another process is running?")
    os.mkfifo( fifo_name )

    w, h = get_video_size( ffprobe_video( fname ) )
    
    ffmpeg_cmd = ["ffmpeg",  "-i",  fname,
                  "-vcodec",  "rawvideo",
                  "-f", "rawvideo",
                  "-pix_fmt", "rgb24"
                  "-y", fifo_name ]

    converter_cmd = ["./converter", str(w), str(h), fifo_name]
    #Start the decoder in parallel
    #TODO

    #Start slit-scanner and wait for result
    #TODO
    
    #If needed, kill the converter process, collect output files
    #TODO
    #Delete fifo
    os.remove( fifo_name )

if __name__=="__main__":
    if len(sys.argv) !=2:
        print ("Must have 1 argument")
        exit(1)

    fname = sys.argv[1]
    w,h = get_video_size(ffprobe_video(fname))
    print (w, h)

            
