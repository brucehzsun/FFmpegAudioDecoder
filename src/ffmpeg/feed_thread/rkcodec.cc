#include <stdio.h>
#include <sys/mman.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include "rkcodec.h"


using namespace rokid;
using namespace std;


static const uint32_t OPUS_BUFFER_SIZE = 16 * 1024;

RKOpusDecoder::RKOpusDecoder() : _opu_frame_size(0),
		_pcm_frame_size(0), _opus_decoder(NULL), _pcm_buffer(NULL) {
}

RKOpusDecoder::~RKOpusDecoder() {
	close();
}

bool RKOpusDecoder::init(uint32_t sample_rate, uint32_t bitrate,
		uint32_t duration) {
	if (_opus_decoder) {
		return true;
	}
	int err;
	_opus_decoder = opus_decoder_create(sample_rate, 1, &err);
	if (err != OPUS_OK) {
		return false;
	}
	opus_decoder_ctl(_opus_decoder, OPUS_SET_VBR(1));
	opus_decoder_ctl(_opus_decoder, OPUS_SET_COMPLEXITY(8));
	opus_decoder_ctl(_opus_decoder, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE));
	opus_decoder_ctl(_opus_decoder, OPUS_SET_BITRATE(bitrate));

	_opu_frame_size = bitrate * duration / 8000;
	_pcm_frame_size = sample_rate * duration / 1000;
	_pcm_buffer = new uint16_t[_pcm_frame_size];
	return true;
}

const uint16_t* RKOpusDecoder::decode_frame(const void* opu, uint32_t opu_len) {
	if (_opus_decoder == NULL) {
		return NULL;
	}
	if (opu == NULL) {
		return NULL;
	}
	int c = opus_decode(_opus_decoder, reinterpret_cast<const uint8_t*>(opu),
			opu_len, reinterpret_cast<opus_int16*>(_pcm_buffer),
			_pcm_frame_size, 0);
	if (c < 0) {
		return NULL;
	}
	// debug
	// printf("decode debug: %x %x %x %x\n", _pcm_buffer[240], _pcm_buffer[241], _pcm_buffer[242], _pcm_buffer[243]);
	return _pcm_buffer;
}

void RKOpusDecoder::close() {
	if (_opus_decoder) {
		delete _pcm_buffer;
		_opu_frame_size = 0;
		_pcm_frame_size = 0;
		opus_decoder_destroy(_opus_decoder);
		_opus_decoder = NULL;
	}
}


