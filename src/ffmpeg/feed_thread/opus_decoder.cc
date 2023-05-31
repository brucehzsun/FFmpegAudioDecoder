#include <stdio.h>
#include <sys/mman.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include "opus_decoder.h"
#include "vector"

using namespace Rokid;
using namespace std;

static const uint32_t OPUS_BUFFER_SIZE = 16 * 1024;

RKOpusDecoder::RKOpusDecoder(int output_sample_rate, int duration) : _opus_decoder(NULL), _duration(duration),
                                                                     Rokid::AudioDecoderInterface(output_sample_rate) {
}

RKOpusDecoder::~RKOpusDecoder() {
}

int RKOpusDecoder::start(format_buffer_write write_buffer, void *opaque_out) {
  if (_opus_decoder) {
    return 0;
  }

  this->write_buffer = write_buffer;
  this->opaque_out = opaque_out;

  int err;
  _opus_decoder = opus_decoder_create(_output_sample_rate, 1, &err);
  if (err != OPUS_OK) {
    return -1;
  }
  int bitrate = 16000;

  opus_decoder_ctl(_opus_decoder, OPUS_SET_VBR(1));
  opus_decoder_ctl(_opus_decoder, OPUS_SET_COMPLEXITY(8));
  opus_decoder_ctl(_opus_decoder, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE));
  opus_decoder_ctl(_opus_decoder, OPUS_SET_BITRATE(bitrate));

  int duration = 20;
  _opu_frame_size = bitrate * duration / 8000;
  _pcm_frame_size = this->_output_sample_rate * duration / 1000;
  _pcm_buffer = new uint16_t[_pcm_frame_size];

  return 0;
}

//const uint16_t *RKOpusDecoder::decode_frame(const void *opu, uint32_t opu_len) {
int RKOpusDecoder::feed(uint8_t *data, int data_size) const {
  if (_opus_decoder == NULL) {
    return -1;
  }
  if (data == NULL) {
    return -2;
  }

  // 获取第一个字节
//  uint8_t opus_len = *data;

  // 指向后面所有字节的指针
//  uint8_t *rest_data = data + 1;

//  std::cout << "opus_len=" << opus_len << ",data_size=" << data_size << std::endl;

//  short decoded_audio[IO_BUF_SIZE];
//  uint16_t *pcm_buffer = new uint16_t[IO_BUF_SIZE];
  int num_samples = opus_decode(_opus_decoder, data, data_size, reinterpret_cast<opus_int16 *>(_pcm_buffer),
                                _pcm_frame_size, 0);
  if (num_samples < 0) {
    return num_samples;
  }
  // debug
  // printf("decode debug: %x %x %x %x\n", _pcm_buffer[240], _pcm_buffer[241], _pcm_buffer[242], _pcm_buffer[243]);
//  return _pcm_buffer;
  (*write_buffer)(opaque_out, reinterpret_cast<uint8_t *>(_pcm_buffer), num_samples * sizeof(uint16_t));
  return 0;
}

int RKOpusDecoder::stop() {
  if (_opus_decoder) {
    opus_decoder_destroy(_opus_decoder);
    _opus_decoder = NULL;
  }
  return 0;
}


