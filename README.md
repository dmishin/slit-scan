Slit Scan
=========

Program for making slit-scan photos( Slit-Scan Photography : <http://en.wikipedia.org/wiki/Slit-scan_photography> ) from video files.

Uses FFMPEG for decoding video, and ImageMagick to encode image files.

Build
-----

Run make.sh


Build Instructions
-------

Make sure you have FFMPEG, for decoding video, and ImageMagic to enncode image files.

You can grab FFMPEG <https://trac.ffmpeg.org/>, you can find instructions for compiling and installing it under Ubuntu <https://trac.ffmpeg.org/wiki/UbuntuCompilationGuide>.

Make sure to install ImageMagick <http://www.imagemagick.org/script/index.php>, you can use MacPorts to install ImageMagick and its dependencies to compile under MacOSX.

If you have MacPorts, and are planning on compiling it under a MacOSX environment, you can the dependencies by running:

sudo port install FFMPEG
sudo port install ImageMagick

Before compiling, make sure your MacPorts libraries are set.

vim ~/.profile

Add the following lines to the end of your ~/.profile.

export MANPATH=/opt/local/share/man:$MANPATH
export C_INCLUDE_PATH=/opt/local/include
export CPLUS_INCLUDE_PATH=/opt/local/include
export LIBRARY_PATH=/opt/local/lib
alias gcc="gcc -I/opt/local/include -L/opt/local/lib"
alias g++="g++ -I/opt/local/include -L/opt/local/lib"

Once you have your dependencies install, simply run the make file.

./make.sh

Usage
------------

./slit-scan [yourvideo.mp4]

Example:

./slit-scan video.mp4

Will generate video.png in the current directory.

See ./slit-scan --help for more information about keys.


Tools
=====

vertical_jitter_fix.py

   Tool for removing vertical jitter from the images.
   Useful for post-processing slit scan photos, made from shaky videos.
   Requires Python2, PIL and Numpy.
   Usage: 
   $ python vertical_jitter_fix.py --help
