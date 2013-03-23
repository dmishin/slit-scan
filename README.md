Slit Scan
=========

Program for making slit-scan photos from video files.
Uses FFMPEG for decoding video, and ImageMagic to encode image files.

Build
-----

Run make.sh


Install
-------

Not yet implemented. Put the executable somewhere.


Sample usage
------------

./slit-scan video.mp4

Will generate video.png in the current directory.

See ./slit-scan --help for more information about keys


Tools
=====

vertical_jitter_fix.py
   Tool for removing vertical jitter from the images.
   Useful for post-processing slit scan photos, made from shaky videos.
   Requires Python2, PIL and Numpy.
   Usage: 
   $ python vertical_jitter_fix.py --help
