//
// Created by bruce sun on 2023/4/27.
//
#ifndef FFMPEGAUDIODECODER_SRC_FFMPEG_FEED2_AUDIO_DECODER_H_
#define FFMPEGAUDIODECODER_SRC_FFMPEG_FEED2_AUDIO_DECODER_H_

#include "queue"
#include "string"
#include "memory"
#include "timeout_queue.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
#include<libswresample/swresample.h>
}

typedef int (*format_buffer_read)(void *opaque_in, uint8_t *buf, int buf_size);
typedef int (*format_buffer_write)(void *opaque_out, uint8_t *buf, int buf_size);

namespace Rokid {
class AudioDecoder {
 public:
  AudioDecoder(int output_sample_rate = 16000);
  int start(format_buffer_write write_buffer, void *opaque_out);
  int feed(uint8_t *inbuf, int data_size);
  int stop();
 private:
  int init_header();
  int init_swr_context();
  int decode_frame();

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
  std::shared_ptr<std::thread> decode_thread_ = nullptr;
  format_buffer_write write_buffer = nullptr;
  void *opaque_out = nullptr;

  void DecodeThreadFunc();
 public:
  bool eof = false;
  std::shared_ptr<Rokid::TimeoutQueue<std::vector<unsigned char>>> input_queue = nullptr;
  std::shared_ptr<Rokid::TimeoutQueue<std::string>> output_queue = nullptr;
//  std::shared_ptr<std::queue<std::vector<unsigned char>>> output_queue = nullptr;

};
}

#endif //FFMPEGAUDIODECODER_SRC_FFMPEG_FEED2_AUDIO_DECODER_H_
