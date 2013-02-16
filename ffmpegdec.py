import ctypes

avformat = ctypes.CDLL("libavformat.so")
avcodec = ctypes.CDLL("libavcodec.so")
swscale = ctypes.CDLL("libswscale.so")
avutil = ctypes.CDLL("libavutil.so")


_av_dump_format
_av_free
_av_free_packet
_av_malloc

_avcodec_alloc_frame
_avcodec_close
_avcodec_decode_video2
_avcodec_find_decoder
_avcodec_open2

_avformat_close_input
_avformat_find_stream_info
_avformat_open_input

_avpicture_fill
_avpicture_get_size

_sws_getContext
_sws_scale


