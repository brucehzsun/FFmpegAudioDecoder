//
// Created by bruce sun on 2021/9/1.
//

#include "AudioDecoder.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IO_BUF_SIZE (32768*1)


static void decode(AVCodecContext *dec_ctx, AVPacket *pkt, AVFrame *frame,
                   FILE *outfile) {
    int i, ch;
    int ret, data_size;

    /* send the packet with the compressed data to the decoder */
    ret = avcodec_send_packet(dec_ctx, pkt);
    if (ret < 0) {
        fprintf(stderr, "Error submitting the packet to the decoder\n");
        exit(1);
    }

    /* read all the output frames (in general there may be any number of them */
    while (ret >= 0) {
        ret = avcodec_receive_frame(dec_ctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return;
        else if (ret < 0) {
            fprintf(stderr, "Error during decoding\n");
            exit(1);
        }
        data_size = av_get_bytes_per_sample(dec_ctx->sample_fmt);
        if (data_size < 0) {
            /* This should not occur, checking just for paranoia */
            fprintf(stderr, "Failed to calculate data size\n");
            exit(1);
        }
        for (i = 0; i < frame->nb_samples; i++)
            for (ch = 0; ch < dec_ctx->channels; ch++)
                fwrite(frame->data[ch] + data_size * i, 1, data_size, outfile);
    }
}


AudioDecoder::AudioDecoder(int output_sample_rate) {
    printf("C++ AudioDecoder\n");
    outfilename = "test.pcm";

    pkt = av_packet_alloc();

    /* find the MPEG audio decoder */
    codec = avcodec_find_decoder(AV_CODEC_ID_MP3);
    if (!codec) {
        fprintf(stderr, "Codec not found\n");
        exit(1);
    }

    parser = av_parser_init(codec->id);
    if (!parser) {
        fprintf(stderr, "Parser not found\n");
        exit(1);
    }

    c = avcodec_alloc_context3(codec);
    if (!c) {
        fprintf(stderr, "Could not allocate audio codec context\n");
        exit(1);
    }

    /* open it */
    if (avcodec_open2(c, codec, NULL) < 0) {
        fprintf(stderr, "Could not open codec\n");
        exit(1);
    }

    outfile = fopen(outfilename, "wb");
    if (!outfile) {
        av_free(c);
        exit(1);
    }
}
//
//int AudioDecoder::feed(char *buf) {
//
//    // 循环读取一份音频压缩数据
//    while (av_read_frame(avFormatContext, avpacket) == 0) {
//        //判定是否是音频流
//        /** 第七步音频解码 */
//        //1、发送一帧音频压缩数据包->音频压缩数据帧
//        if ((ret = avcodec_send_packet(avCodecContex, avpacket)) != 0) {
//            printf("C++ avcodec_send_packet ret = %d\n", ret);
//            return ret;
//        }
//
//        //2、解码一帧音频压缩数据包->得到->一帧音频采样数据->音频采样数据帧
//        if (avcodec_receive_frame(avCodecContex, avframe) == 0) {
//            //3、类型转换(音频采样数据格式有很多种类型)
//            if ((ret = swr_convert(swr_context, &out_buffer, IO_BUF_SIZE, (const uint8_t **) avframe->data,
//                                   avframe->nb_samples)) < 0) {
//                printf("C++ swr_convert error \n");
//                return ret;
//            }
//
//            //5、写入文件(你知道要写多少吗？)
//            int resampled_data_size = av_samples_get_buffer_size(nullptr, out_nb_channels, ret, AV_SAMPLE_FMT_S16, 1);
//            if (resampled_data_size < 0) {
//                printf("C++ av_samples_get_buffer_size error:%d\n", resampled_data_size);
//                return resampled_data_size;
//            }
//            (*write_buffer)(opaque_out, out_buffer, resampled_data_size);
//        }
//    }
//}

void AudioDecoder::feed2(char *inbuf, int data_size) {
    /* decode until eof */
    data = inbuf;
    int ret;

    while (data_size > 0) {
        if (!decoded_frame) {
            if (!(decoded_frame = av_frame_alloc())) {
                fprintf(stderr, "Could not allocate audio frame\n");
                exit(1);
            }
        }

        ret = av_parser_parse2(parser, c, &pkt->data, &pkt->size, data, data_size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
        if (ret < 0) {
            fprintf(stderr, "Error while parsing\n");
            exit(1);
        }
        data += ret;
        data_size -= ret;

        if (pkt->size)
            decode(c, pkt, decoded_frame, outfile);

        if (data_size < AUDIO_REFILL_THRESH) {
//            memmove(inbuf, data, data_size);
//            data = inbuf;
//            len = fread(data + data_size, 1, AUDIO_INBUF_SIZE - data_size, f);
//            if (len > 0)
//                data_size += len;
            break;
        }
    }

    /* flush the decoder */
    pkt->data = NULL;
    pkt->size = 0;
    decode(c, pkt, decoded_frame, outfile);

    /* print output pcm infomations, because there have no metadata of pcm */
    sfmt = c->sample_fmt;

    if (av_sample_fmt_is_planar(sfmt)) {
        const char *packed = av_get_sample_fmt_name(sfmt);
        printf("Warning: the sample format the decoder produced is planar "
               "(%s). This example will output the first channel only.\n",
               packed ? packed : "?");
        sfmt = av_get_packed_sample_fmt(sfmt);
    }

    n_channels = c->channels;
    if ((ret = get_format_from_sample_fmt(&fmt, sfmt)) < 0)
        goto end;
}

void AudioDecoder::stop() {
    /* flush the decoder */
    pkt->data = NULL;
    pkt->size = 0;
    decode(c, pkt, decoded_frame, outfile);

    /* print output pcm infomations, because there have no metadata of pcm */
    sfmt = c->sample_fmt;

    if (av_sample_fmt_is_planar(sfmt)) {
        const char *packed = av_get_sample_fmt_name(sfmt);
        printf("Warning: the sample format the decoder produced is planar "
               "(%s). This example will output the first channel only.\n",
               packed ? packed : "?");
        sfmt = av_get_packed_sample_fmt(sfmt);
    }

    n_channels = c->channels;
    if ((ret = get_format_from_sample_fmt(&fmt, sfmt)) < 0)
        goto end;

    printf("Play the output audio file with the command:\n"
           "ffplay -f %s -ac %d -ar %d %s\n",
           fmt, n_channels, c->sample_rate,
           outfilename);

    fclose(outfile);
    fclose(f);

    avcodec_free_context(&c);
    av_parser_close(parser);
    av_frame_free(&decoded_frame);
    av_packet_free(&pkt);
}