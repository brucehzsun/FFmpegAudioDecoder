//
// Created by bruce sun on 2023/4/27.
//
#ifndef FFMPEGAUDIODECODER_SRC_FFMPEG_FEED2_AUDIO_DECODER_H_
#define FFMPEGAUDIODECODER_SRC_FFMPEG_FEED2_AUDIO_DECODER_H_

#include "queue"
#include "string"
#include "memory"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
#include<libswresample/swresample.h>
}

typedef int (*format_buffer_read)(void *opaque_in, uint8_t *buf, int buf_size);

namespace Rokid {
class AudioDecoder {
 public:
  AudioDecoder();
  int start(int output_sample_rate = 16000);
  int feed(uint8_t *inbuf, int data_size, uint8_t **out_buffer);
  int stop(uint8_t **pcm_buffer);
 private:
  int init_header();
  int init_swr_context();
  int decode_frame(uint8_t **out_buffer);

 private:
  AVFormatContext *format_ctx = nullptr;
  AVPacket *packet = nullptr;
  AVCodecContext *av_codec_ctx = nullptr;
  AVFrame *frame = nullptr;
  SwrContext *swr_context = nullptr;
  uint8_t *out_buffer = nullptr;
  int audio_stream_index = -1;
  int out_nb_channels;

 private:
  bool is_init_header;
  int output_sample_rate = 16000;

  const char *out_file = "data/test_output_m4a.pcm";
  FILE *fp_pcm = fopen(out_file, "wb");
 public:
  std::shared_ptr<std::queue<std::string> > audio_queue = nullptr;

};
}

#endif //FFMPEGAUDIODECODER_SRC_FFMPEG_FEED2_AUDIO_DECODER_H_
