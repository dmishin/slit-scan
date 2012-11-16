#!/bin/bash
g++ `pkg-config --libs libavformat libavcodec libswscale libavutil` \
    -DNDEBUG -O3 \
    ffmpeg_decoder.cpp slit_scan.cpp \
    -o slit-scan
