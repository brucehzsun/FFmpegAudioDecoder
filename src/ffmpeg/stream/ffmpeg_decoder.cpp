/*
 * Copyright (c) 2001 Fabrice Bellard
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "ffmpeg_decoder.h"
#include <iostream>

#define IO_BUF_SIZE (32768*4)

using namespace std;

FFmpegDecoder::FFmpegDecoder() {
  cout << "C++ FFmpegDecoder()" << endl;
}

FFmpegDecoder::~FFmpegDecoder() {
  cout << "C++ ~FFmpegDecoder()" << endl;
//  av_packet_free(&avpacket);
//  swr_free(&swr_context);
//  av_free(out_buffer);
//  av_frame_free(&avframe);
//  avcodec_close(avCodecContex);
//  avcodec_free_context(&avCodecContex);
//  avformat_close_input(&avFormatContext);
//  av_free(avio_in->buffer);
//  avio_context_free(&avio_in);
}

int FFmpegDecoder::start(format_buffer_read read_buffer, void *opaque_in, format_buffer_write write_buffer,
                         void *opaque_out, int output_sample_rate) {
  printf("C++ start\n");
  int ret;
  if (output_sample_rate <= 0) {
    output_sample_rate = 16000;
  }

  // 定义封装结构体,分配一个avformat
  this->format_ctx = avformat_alloc_context();
  if (format_ctx == nullptr) {
    printf("C++ avFormatContext alloc fail\n");
    return -3;
  }

  this->inbuffer = (unsigned char *) av_malloc(IO_BUF_SIZE);
  if (this->inbuffer == nullptr) {
    printf("C++ av malloc failed\n");
    return -1;
  }

  this->avio_cxt = avio_alloc_context(this->inbuffer, IO_BUF_SIZE, 0, opaque_in, read_buffer, nullptr, nullptr);
  if (avio_cxt == nullptr) {
    printf("C++ avio_alloc_context for input failed\n");
    return -2;
  }

  format_ctx->pb = avio_cxt; // 赋值自定义的IO结构体
  format_ctx->flags = AVFMT_FLAG_CUSTOM_IO; // 指定为自定义

  // 将输入流媒体流链接为封装结构体的句柄，将url句柄挂载至format_ctx结构里，之后ffmpeg即可对format_ctx进行操作
  ret = avformat_open_input(&format_ctx, nullptr, nullptr, nullptr);
//  ret = avformat_open_input(&format_ctx, input_file, nullptr, nullptr);
  if (ret < 0) {
    printf("C++ open fail:%d\n", ret);
    return ret;
  }

  // 查找音视频流信息，从format_ctx中建立输入文件对应的流信息
  if ((ret = avformat_find_stream_info(format_ctx, nullptr)) < 0) {
    printf("C++ find stream fail:%d\n", ret);
    return ret;
  }

  if (format_ctx->nb_streams < 1) {
    printf("C++ format streams number is 0\n");
    return -4;
  }

  int audio_stream_index = -1;
  /* 穷举所有的音视频流信息，找出流编码类型为AVMEDIA_TYPE_AUDIO对应的索引 */
  for (unsigned int i = 0; i < format_ctx->nb_streams; ++i) {
    const AVStream *stream = format_ctx->streams[i];
    if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
      audio_stream_index = i;
      break;
    }
  }

  if (audio_stream_index == -1) {
    fprintf(stderr, "Cannot find a audio stream\n");
    return -1;
  }

  // Dump valid information onto standard error
//  av_dump_format(format_ctx, 0, input_file, false);

  /* 查找audio解码器 */
  /* 首先从输入的AVFormatContext中得到stream,然后从stream中根据编码器的CodeID获得对应的Decoder */
  AVCodecParameters *ad_codec_params = format_ctx->streams[audio_stream_index]->codecpar;
  const AVCodec *av_codec = avcodec_find_decoder(ad_codec_params->codec_id);
  if (!av_codec) {
    fprintf(stderr, "fail to avcodec_find_decoder\n");
    return -1;
  }

  this->av_codec_ctx = avcodec_alloc_context3(av_codec);
  if (av_codec_ctx == nullptr) {
    printf("C++ avcodec_alloc_context3 failed\n");
    return -6;
  }
  ret = avcodec_parameters_to_context(av_codec_ctx, ad_codec_params);
  if (ret < 0) {
    printf("C++ avcodec_parameters_to_context failed, ret:%d\n", ret);
    return ret;
  }

  /* 打开audio解码器 */
  /**第五步：打开音频解码器*/
  ret = avcodec_open2(av_codec_ctx, av_codec, nullptr);
  if (ret < 0) {
    fprintf(stderr, "Could not open av_codec: %d\n", ret);
    return -1;
  }
  printf("av_codec name: %s\n", av_codec->name);

  //用于存放解码后的数据,创建音频采样数据帧
  this->frame = av_frame_alloc();
  if (frame == nullptr) {
    printf("C++ av_fram_alloc failed\n");
    return -1;
  }

  /**第六步：读取音频压缩数据->循环读取*/
  //创建音频压缩数据帧->acc格式、mp3格式
  this->packet = (AVPacket *) av_malloc(sizeof(AVPacket));
  if (!packet) {
    fprintf(stderr, "Could not allocate video packet\n");
    return -1;
  }


  /************ Audio Convert ************/
  // frame->16bit 16000 PCM 统一音频采样格式与采样率
  /** 开始设置转码信息**/
  // 打开转码器
  this->swr_context = swr_alloc();
  if (swr_context == nullptr) {
    printf("C++ swr_alloc failed\n");
    return -8;
  }

  // 音频格式  输入的采样设置参数
  AVSampleFormat inFormat = av_codec_ctx->sample_fmt;

  // 输入采样率
  int inSampleRate;
  if (av_codec_ctx->sample_rate) {
    inSampleRate = av_codec_ctx->sample_rate;
  } else {
    inSampleRate = format_ctx->streams[audio_stream_index]->codecpar->sample_rate;
  }

  // 输入声道布局
//  int64_t in_ch_layout = av_get_default_channel_layout(av_codec_ctx->channels);
  uint64_t in_ch_layout;
  if (av_codec_ctx->channel_layout) {
    in_ch_layout = av_codec_ctx->channel_layout;
  } else {
    in_ch_layout = format_ctx->streams[audio_stream_index]->codecpar->channel_layout;
  }
  /**获取输入参数*/
  //输入声道布局类型

  //设置转码参数
  if ((swr_context = swr_alloc_set_opts(swr_context, AV_CH_LAYOUT_MONO, AV_SAMPLE_FMT_S16, output_sample_rate,
                                        in_ch_layout, av_codec_ctx->sample_fmt, inSampleRate, 0,
                                        nullptr)) == nullptr) {
    printf("C++ swr_alloc_set_opts failed\n");
    return -9;
  }

  // 初始化
  //初始化音频采样数据上下文;初始化转码器
  if ((ret = swr_init(swr_context) < 0)) {
    printf("C++ swr_init failed:%d\n", ret);
    return ret;
  }

  /**设置输出参数*/
  //输出声道数量 单声道
  int out_nb_channels = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_MONO);

  // 设置音频缓冲区间 16bit   16000  PCM数据, 单声道
  this->out_buffer = (uint8_t *) av_malloc(IO_BUF_SIZE);
  if (this->out_buffer == nullptr) {
    printf("C++ av_malloc(IO_BUF_SIZE) failed\n");
    return -10;
  }


  /************ Audio Convert ************/
  // 循环读取一份音频压缩数据
//  while (av_read_frame(format_ctx, packet) == 0) {
  while (1) {
    ret = av_read_frame(format_ctx, packet); // 读取下一帧数据
    if (ret < 0) {
      fprintf(stderr, "Error submitting the packet to the decoder\n");
      return ret;
    }
    if (packet->stream_index == audio_stream_index) {
      /** 第七步音频解码 */
      //1、发送一帧音频压缩数据包->音频压缩数据帧
      ret = avcodec_send_packet(av_codec_ctx, packet); // 解码
      if (ret != 0) {
        printf("C++ avcodec_send_packet ret = %d\n", ret);
        av_packet_unref(packet);
        return ret;
      }

      while ((ret = avcodec_receive_frame(av_codec_ctx, frame)) >= 0) {
        //2、解码一帧音频压缩数据包->得到->一帧音频采样数据->音频采样数据帧
        // 接收获取解码收的数据
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
          av_packet_unref(packet);
          break;
        } else if (ret < 0) {
          fprintf(stderr, "Error during decoding: %d\n", ret);
          av_packet_unref(packet);
          break;
        }

        //3、类型转换(音频采样数据格式有很多种类型),将每一帧数据转换成pcm
        ret = swr_convert(swr_context, &out_buffer, IO_BUF_SIZE, (const uint8_t **) frame->data, frame->nb_samples);
        if (ret < 0) {
          printf("C++ swr_convert error \n");
          return ret;
        }

        //5、写入文件(你知道要写多少吗？)
        // 获取实际的缓存大小（补充：第三个参数不应该是输入的样本数，而是重采样的样本数）
        //ret = frame->nb_samples * outSampleRate / inSampleRate,
        int resampled_data_size = av_samples_get_buffer_size(nullptr, out_nb_channels, ret, AV_SAMPLE_FMT_S16, 1);
        if (resampled_data_size < 0) {
          printf("C++ av_samples_get_buffer_size error:%d\n", resampled_data_size);
          //                return resampled_data_size;
          continue;
        } else {
          printf("C++ write_buffer resampled_data_size:%d\n", resampled_data_size);
          (*write_buffer)(opaque_out, out_buffer, resampled_data_size);
          // 写入文件
//          fwrite(out_buffer, 1, resampled_data_size, fp_pcm);
        }
      }
    }

    av_packet_unref(packet);
  }

  printf("C++ decoder finish\n");
  return 0;
}

int FFmpegDecoder::stop() {
  /* Release resources */
//  av_free(this->inbuffer);
  av_frame_free(&frame);
  av_packet_free(&packet);
  swr_free(&swr_context);
  avcodec_free_context(&av_codec_ctx); // unless avcodec_alloc_context3() is used
  avformat_close_input(&format_ctx);

//  av_packet_free(&avpacket);
//  swr_free(&swr_context);
//  av_frame_free(&avframe);
//  avcodec_free_context(&avCodecContex);
//  avformat_close_input(&avFormatContext);
  avcodec_close(av_codec_ctx);
  av_free(avio_cxt->buffer);
  av_free(this->out_buffer);
  avio_context_free(&avio_cxt);
  return 0;
}