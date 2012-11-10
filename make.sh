#!/bin/bash
g++ `pkg-config --libs libavformat libavcodec libswscale libavutil` \
    -DNDEBUG -O3 \
    ffmpeg_decoder.cpp slit_scan_raw.cpp \
    -o converter.bin
