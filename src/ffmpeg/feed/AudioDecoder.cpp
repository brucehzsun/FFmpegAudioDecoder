//
// Created by bruce sun on 2021/9/1.
//

#include "AudioDecoder.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static int get_format_from_sample_fmt(const char **fmt, enum AVSampleFormat sample_fmt) {
    int i;
    struct sample_fmt_entry {
        enum AVSampleFormat sample_fmt;
        const char *fmt_be, *fmt_le;
    } sample_fmt_entries[] = {
            {AV_SAMPLE_FMT_U8,  "u8",    "u8"},
            {AV_SAMPLE_FMT_S16, "s16be", "s16le"},
            {AV_SAMPLE_FMT_S32, "s32be", "s32le"},
            {AV_SAMPLE_FMT_FLT, "f32be", "f32le"},
            {AV_SAMPLE_FMT_DBL, "f64be", "f64le"},
    };
    *fmt = nullptr;

    for (i = 0; i < FF_ARRAY_ELEMS(sample_fmt_entries); i++) {
        struct sample_fmt_entry *entry = &sample_fmt_entries[i];
        if (sample_fmt == entry->sample_fmt) {
            *fmt = AV_NE(entry->fmt_be, entry->fmt_le);
            return 0;
        }
    }

    fprintf(stderr, "sample format %s is not supported as output format\n", av_get_sample_fmt_name(sample_fmt));
    return -1;
}

int AudioDecoder::decodeFrame(AVCodecContext *dec_ctx, AVPacket *pkt, AVFrame *frame, uint8_t **out_buffer,
                              int out_buffer_size) {

    /* send the packet with the compressed data to the decoder */
    int ret = avcodec_send_packet(dec_ctx, pkt);
    if (ret < 0) {
        fprintf(stdout, "Error submitting the packet to the decoder\n");
        return -100;
    }

    if (ret == 0 && swr_context == nullptr) {
        initSwrContext();
    }

    /* read all the output frames (in general there may be any number of them */
    while (ret >= 0) {
        ret = avcodec_receive_frame(dec_ctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return out_buffer_size;
        else if (ret < 0) {
            fprintf(stderr, "Error during decoding\n");
            return ret;
        }

        //3、类型转换(音频采样数据格式有很多种类型)
        if ((ret = swr_convert(swr_context, &swr_buffer, IO_BUF_SIZE, (const uint8_t **) frame->data,
                               frame->nb_samples)) < 0) {
            printf("C++ swr_convert error \n");
            return ret;
        }

        //5、写入文件(你知道要写多少吗？)
        int resampled_data_size = av_samples_get_buffer_size(nullptr, out_nb_channels, ret, AV_SAMPLE_FMT_S16, 1);
        if (resampled_data_size < 0) {
            printf("C++ av_samples_get_buffer_size error:%d\n", resampled_data_size);
            return resampled_data_size;
        }
//        (*_write_buffer)(outfile, swr_buffer, resampled_data_size);
        //从 data 复制 data_size 个字符到 inbuf
        memmove((*out_buffer) + out_buffer_size, swr_buffer, resampled_data_size);
        out_buffer_size += resampled_data_size;
    }
    return out_buffer_size;
}

int AudioDecoder::initSwrContext() {
    printf("init swr_context\n");
    /** 开始设置转码信息**/
    // 打开转码器
    if ((swr_context = swr_alloc()) == nullptr) {
        printf("C++ swr_alloc failed\n");
        return -200;
    }

    /**获取输入参数*/
    //输入声道布局类型
    int64_t in_ch_layout = av_get_default_channel_layout(context->channels);

    //设置转码参数
    swr_context = swr_alloc_set_opts(swr_context, AV_CH_LAYOUT_MONO, AV_SAMPLE_FMT_S16, _output_sample_rate,
                                     in_ch_layout, context->sample_fmt, context->sample_rate, 0, nullptr);
    if (swr_context == nullptr) {
        printf("C++ swr_alloc_set_opts failed\n");
        return -201;
    }

    int ret;
    //初始化音频采样数据上下文;初始化转码器
    if ((ret = swr_init(swr_context) < 0)) {
        printf("C++ swr_init failed:%d\n", ret);
        return ret;
    }
    return 0;
}

AudioDecoder::AudioDecoder(int output_sample_rate) {

    this->_output_sample_rate = output_sample_rate;
    pkt = av_packet_alloc();

    //缓冲区大小 = 采样率(44100HZ) * 采样精度(16位 = 2字节)
    if ((swr_buffer = (uint8_t *) av_malloc(IO_BUF_SIZE)) == nullptr) {
        printf("C++ av_malloc(IO_BUF_SIZE) failed\n");
        return;
    }

    /**设置输出参数*/
    //输出声道数量 上面设置为立体声
    this->out_nb_channels = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_MONO);

    /* find the MPEG audio decoder */
    const AVCodec *codec = avcodec_find_decoder(AV_CODEC_ID_MP3);
    if (!codec) {
        fprintf(stderr, "Codec not found\n");
        return;
    }

    parser = av_parser_init(codec->id);
    if (!parser) {
        fprintf(stderr, "Parser not found\n");
        return;
    }

    context = avcodec_alloc_context3(codec);
    if (!context) {
        fprintf(stderr, "Could not allocate audio codec context\n");
        return;
    }

    /* open it */
    if (avcodec_open2(context, codec, nullptr) < 0) {
        fprintf(stderr, "Could not open codec\n");
        return;
    }
}

int AudioDecoder::feed(uint8_t *raw_data, int raw_data_size, uint8_t **out_buffer) {

    memmove(_data_buffer + _data_size, raw_data, raw_data_size);
    _data_size += raw_data_size;
    //使用双指针
    _data = _data_buffer;
    int ret;
    int out_buffer_size = 0;

    while (_data_size > 0) {
        if (!decoded_frame) {
            if (!(decoded_frame = av_frame_alloc())) {
                fprintf(stderr, "Could not allocate audio frame\n");
                return -1;
            }
        }

        //ret是输入数据inbuf中已经使用的数据长度
        ret = av_parser_parse2(parser, context, &pkt->data, &pkt->size, _data, _data_size, AV_NOPTS_VALUE,
                               AV_NOPTS_VALUE, 0);
        if (ret < 0) {
            fprintf(stderr, "Error while parsing\n");
            return -2;
        }
        _data += ret; //指针偏移量
        _data_size -= ret; // 剩余数据的大小

        if (pkt->size) {
            out_buffer_size = decodeFrame(context, pkt, decoded_frame, out_buffer, out_buffer_size);
            if (out_buffer_size < 0) {
                fprintf(stderr, "decodeFrame failed ret = %d\n", out_buffer_size);
            }
        }

        if (_data_size < AUDIO_REFILL_THRESH) {
//            printf("data less AUDIO_REFILL_THRESH,_data_size=%d\n", _data_size);
            //从 data 复制 data_size 个字符到 inbuf
            memmove(_data_buffer, _data, _data_size);
            _data = _data_buffer;
            break;
//            len = fread(data + data_size, 1,
//                        AUDIO_INBUF_SIZE - data_size, f);
//            if (len > 0)
//                data_size += len;
        }
    }
    return out_buffer_size;
}

int AudioDecoder::stop(uint8_t **out_buffer) {
    /* flush the decoder */
    int out_buffer_size = 0;
    pkt->data = nullptr;
    pkt->size = 0;
    out_buffer_size = decodeFrame(context, pkt, decoded_frame, out_buffer, out_buffer_size);

    /* print output pcm infomations, because there have no metadata of pcm */
    sfmt = context->sample_fmt;

    if (av_sample_fmt_is_planar(sfmt)) {
        const char *packed = av_get_sample_fmt_name(sfmt);
        printf("Warning: the sample format the decoder produced is planar "
               "(%s). This example will output the first channel only.\n",
               packed ? packed : "?");
        sfmt = av_get_packed_sample_fmt(sfmt);
    }

    const char *fmt;
    int ret;
    if ((ret = get_format_from_sample_fmt(&fmt, sfmt)) < 0) {
        printf("get_format_from_sample_fmt error\n");
        return ret;
    }

    printf("Play the output audio file with the command:\n"
           "ffplay -f %s %d %d \n",
           fmt, context->channels, context->sample_rate);
    return out_buffer_size;
}

AudioDecoder::~AudioDecoder() {


    av_free(swr_buffer);
    swr_free(&swr_context);
    avcodec_free_context(&context);
    av_parser_close(parser);
    av_frame_free(&decoded_frame);
    av_packet_free(&pkt);
}