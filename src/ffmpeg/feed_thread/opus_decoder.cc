#include <stdio.h>
#include <sys/mman.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include "opus_decoder.h"
#include "vector"
#include <cstring>

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

//  int duration = 20;
//  _opu_frame_size = bitrate * duration / 8000;
//  _pcm_frame_size = this->_output_sample_rate * 2 * duration / 1000;
  _pcm_buffer = new uint16_t[IO_BUF_SIZE];

  return 0;
}

int RKOpusDecoder::decode_frame(uint8_t *data, int data_size) {
  // 读取第一个字节并转换为int类型
  uint8_t first_byte = *data;
  int num_bytes_to_read = static_cast<int>(first_byte);

  // 指向需要读取的字节的指针
  uint8_t *bytes_to_read = data + 1;

//  std::cout << "num_bytes_to_read=" << num_bytes_to_read << ",data_size=" << data_size << std::endl;

  // 分配缓冲区并读取数据
  std::vector<uint8_t> buffer(num_bytes_to_read);
  std::copy(bytes_to_read, bytes_to_read + num_bytes_to_read, buffer.begin());

  int total_samples = 0;
  while (num_bytes_to_read > 0) {
    int num_samples = opus_decode(_opus_decoder, buffer.data(), num_bytes_to_read,
                                  reinterpret_cast<opus_int16 *>(_pcm_buffer), IO_BUF_SIZE, 0);
    if (num_samples < 0) {
      return num_samples;
    }

    (*write_buffer)(opaque_out, reinterpret_cast<uint8_t *>(_pcm_buffer), num_samples * sizeof(uint16_t));

    total_samples += num_samples;
    if (total_samples > data_size) {
      break;
    }

    // 计算剩余可读字节数
    bytes_to_read += num_bytes_to_read;
    num_bytes_to_read = static_cast<int>(*bytes_to_read);
    bytes_to_read++;

    // 重新分配缓冲区并读取数据
    buffer.resize(num_bytes_to_read);
    std::copy(bytes_to_read, bytes_to_read + num_bytes_to_read, buffer.begin());
  }

  return 0;
}

int RKOpusDecoder::feed(uint8_t *data, int data_size) {
  if (_opus_decoder == nullptr) {
    return -1;
  }
  if (data == nullptr) {
    return -2;
  }

  for (int i = 0; i < data_size; i++) {
    _opu_queue.push(data[i]);
  }

  uint8_t first_byte = _opu_queue.front();
  int num_bytes_to_read = static_cast<int>(first_byte);
  int ret;
  while (!_opu_queue.empty() && _opu_queue.size() > num_bytes_to_read) {
    std::vector<uint8_t> data_list;
    for (int i = 0; i < num_bytes_to_read + 1; i++) {
      uint8_t c = _opu_queue.front();
      _opu_queue.pop();
      data_list.push_back(c);
    }
    ret = decode_frame(data_list.data(), (int) data_list.size());

    if (!_opu_queue.empty()) {
      first_byte = _opu_queue.front();
      num_bytes_to_read = static_cast<int>(first_byte);
    } else {
      num_bytes_to_read = 0;
    }
  }

  return ret;
}

int RKOpusDecoder::stop() {
  if (_opus_decoder) {
    opus_decoder_destroy(_opus_decoder);
    _opus_decoder = nullptr;
  }

  if (_pcm_buffer) {
    delete[] _pcm_buffer;
    _pcm_buffer = nullptr;
  }
  return 0;
}


