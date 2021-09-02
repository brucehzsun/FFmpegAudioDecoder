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

typedef int (*format_buffer_write)(void *opaque, uint8_t *buf, int buf_size);

class AudioDecoder {
public:
    AudioDecoder(int output_sample_rate, format_buffer_write write_buffer, bool localTest);

    ~AudioDecoder();

    int feed(uint8_t *inbuf, int data_size);

    void stop();

private:
    //输入数据的buffer
    uint8_t _data_buffer[AUDIO_INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
    int _data_size;
    uint8_t *_data;

    AVCodecContext *c = nullptr;
    AVCodecParserContext *parser = nullptr;
    AVPacket *pkt;
    AVFrame *decoded_frame = nullptr;
    enum AVSampleFormat sfmt;
    format_buffer_write _write_buffer;

    //测试使用
    FILE *outfile = nullptr;
};

#endif //FFMPEGAUDIODECODER_AUDIODECODER_H
