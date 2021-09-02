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

class AudioDecoder {
public:
    AudioDecoder(int output_sample_rate, FILE *fp_open);

    ~AudioDecoder();

    void feed2(uint8_t *inbuf, int data_size);

    void stop();

private:
    const char *outfilename, *filename;
    const AVCodec *codec;
    AVCodecContext *c = nullptr;
    AVCodecParserContext *parser = nullptr;
    int len, ret;
    FILE *f, *outfile;
    uint8_t inbuf[AUDIO_INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
    uint8_t *data;
    size_t data_size;
    AVPacket *pkt;
    AVFrame *decoded_frame = nullptr;
    enum AVSampleFormat sfmt;
    int n_channels = 0;
    const char *fmt;
};

#endif //FFMPEGAUDIODECODER_AUDIODECODER_H
