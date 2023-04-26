#ifndef __AILABS_AUDIODEC_FFMPEG_DECODER_H__
#define __AILABS_AUDIODEC_FFMPEG_DECODER_H__

#include<stdio.h>
#include<stdlib.h>
#include<string.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
#include<libswresample/swresample.h>
}

typedef int (*format_buffer_read)(void *opaque_in, uint8_t *buf, int buf_size);

typedef int (*format_buffer_write)(void *opaque_out, uint8_t *buf, int buf_size);

class FFmpegDecoder {
 public:
  FFmpegDecoder();

  ~FFmpegDecoder();

  int start(format_buffer_read read_buffer,
            void *opaque_in,
            format_buffer_write write_buffer,
            void *opaque_out,
            int output_sample_rate);

 private:

 private:

};

#endif /*__AILABS_AUDIODEC_FFMPEG_DECODER_H__*/