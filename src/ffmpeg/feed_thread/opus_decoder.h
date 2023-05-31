#pragma once

#include <stdint.h>
#include <opus/opus.h>
#include "audio_decoder_interface.h"

namespace Rokid {

// tts  opus -->  pcm
class RKOpusDecoder : public AudioDecoderInterface {
 public:
  RKOpusDecoder(int output_sample_rate = 16000, int duration = 20);

  ~RKOpusDecoder();

  // params:
  //   sample_rate: pcm sample rate
  //   duration: duration(ms) per opus frame
  //   bitrate: opus bitrate override
//  bool init(uint32_t bitrate, uint32_t duration);
  int start(format_buffer_write write_buffer, void *opaque_out) override;
  int feed(uint8_t *inbuf, int data_size) const override;
  int stop() override;

 private:

  OpusDecoder *_opus_decoder;

  uint32_t _pcm_frame_size;
  uint32_t _opu_frame_size;
  int _duration;
  uint16_t* _pcm_buffer;
};

} // namesapce rokid
