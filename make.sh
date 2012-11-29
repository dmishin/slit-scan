#!/bin/bash
g++ `pkg-config --libs libavformat libavcodec libswscale libavutil` \
    -DNDEBUG -O3 \
    ffmpeg_decoder.cpp slit_scan.cpp compare_rgb_blocks.cpp \
    -o slit-scan
