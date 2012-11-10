#!/bin/bash
g++ `pkg-config --libs libavformat libavcodec libswscale libavutil` \
    -O2 \
    ffmpeg_decoder.cpp slit_scan_raw.cpp \
    -o converter.bin
