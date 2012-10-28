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

if __name__=="__main__":
    if len(sys.argv) !=2:
        print ("Must have 1 argument")
        exit(1)

    fname = sys.argv[1]
    jdata = ffprobe_video(fname)
    for strm in jdata["streams"]:
        if strm["codec_type"] == "video":
            print (strm["width"], strm["height"])
            break
            
