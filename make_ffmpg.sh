#!/bin/bash
g++ `pkg-config --libs libavformat libavcodec libswscale libavutil` ffmpeg_decoder.cpp -o a.bin
