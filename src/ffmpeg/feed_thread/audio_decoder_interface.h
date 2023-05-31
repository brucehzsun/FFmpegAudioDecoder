//
// Created by bruce sun on 2023/5/31.
//

#ifndef FFMPEGAUDIODECODER_SRC_FFMPEG_FEED_THREAD_AUDIO_DECODER_INTERFACE_H_
#define FFMPEGAUDIODECODER_SRC_FFMPEG_FEED_THREAD_AUDIO_DECODER_INTERFACE_H_

#include <cstdint>

namespace Rokid {

#define IO_BUF_SIZE 81920

typedef int (*format_buffer_write)(void *opaque_out, uint8_t *buf, int buf_size);

class AudioDecoderInterface {
 public:
  virtual ~AudioDecoderInterface() = default; // 声明虚析构函数
  explicit AudioDecoderInterface(int output_sample_rate = 16000) : _output_sample_rate(output_sample_rate) {}
  virtual int start(format_buffer_write write_buffer, void *opaque_out) = 0;
  virtual int feed(uint8_t *inbuf, int data_size) const = 0;
  virtual int stop() = 0;
 protected:
  int _output_sample_rate;
  format_buffer_write write_buffer = nullptr;
  void *opaque_out = nullptr;

};

}
#endif //FFMPEGAUDIODECODER_SRC_FFMPEG_FEED_THREAD_AUDIO_DECODER_INTERFACE_H_
