//
// Created by bruce sun on 2021/9/1.
//

#ifndef FFMPEGAUDIODECODER_AUDIODECODER_H
#define FFMPEGAUDIODECODER_AUDIODECODER_H


class AudioDecoder {
public:
    AudioDecoder(unsigned int output_sample_rate)

    ~AudioDecoder()

private:
    const char *outfilename, *filename;
    const AVCodec *codec;
    AVCodecContext *c = NULL;
    AVCodecParserContext *parser = NULL;
    int len, ret;
    FILE *f, *outfile;
    uint8_t inbuf[AUDIO_INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
    uint8_t *data;
    size_t data_size;
    AVPacket *pkt;
    AVFrame *decoded_frame = NULL;
    enum AVSampleFormat sfmt;
    int n_channels = 0;
    const char *fmt;
};


#endif //FFMPEGAUDIODECODER_AUDIODECODER_H
