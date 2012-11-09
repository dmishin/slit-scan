#!/bin/bash
g++ `pkg-config --libs libavformat libavcodec libswscale libavutil` ffmpeg_decoder.cpp frame_saver.cpp -o a.bin
