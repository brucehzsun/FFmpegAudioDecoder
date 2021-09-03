//
// Created by bruce sun on 2021/9/1.
//

#ifndef FFMPEGAUDIODECODER_AUDIODECODER_H
#define FFMPEGAUDIODECODER_AUDIODECODER_H

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libavutil/frame.h>
#include <libavutil/mem.h>
}

#define AUDIO_INBUF_SIZE 20480
#define AUDIO_REFILL_THRESH 4096
#define IO_BUF_SIZE (32768*1)

class AudioDecoder {
public:
    explicit AudioDecoder(int output_sample_rate);

    ~AudioDecoder();

    int feed(uint8_t *inbuf, int data_size, uint8_t **out_buffer);

    int decodeFrame(uint8_t **out_buffer, int out_buffer_size);

    int initSwrContext();

    int stop(uint8_t **out_buffer);

private:
    //输入数据的buffer
    uint8_t *_data_buffer = nullptr;
    int _data_size = 0;
    uint8_t *_data = nullptr;

    //缓冲区大小
    uint8_t *swr_buffer = nullptr;

    AVCodecContext *context = nullptr;
    AVCodecParserContext *parser = nullptr;
    AVPacket *pkt;
    AVFrame *decoded_frame = nullptr;
    SwrContext *swr_context = nullptr;

    //测试使用
    int out_nb_channels;
    int _output_sample_rate;
};

#endif //FFMPEGAUDIODECODER_AUDIODECODER_H
