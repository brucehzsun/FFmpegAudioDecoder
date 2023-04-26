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

#define IO_BUF_SIZE (32768*1)

using namespace std;

FFmpegDecoder::FFmpegDecoder() {
  cout << "C++ FFmpegDecoder()" << endl;
}

FFmpegDecoder::~FFmpegDecoder() {
  cout << "C++ ~FFmpegDecoder()" << endl;
  av_packet_free(&avpacket);
  swr_free(&swr_context);
  av_free(out_buffer);
  av_frame_free(&avframe);
  avcodec_close(avCodecContex);
  avcodec_free_context(&avCodecContex);
  avformat_close_input(&avFormatContext);
  av_free(avio_in->buffer);
  avio_context_free(&avio_in);
}

int FFmpegDecoder::start(format_buffer_read read_buffer, void *opaque_in, format_buffer_write write_buffer,
                         void *opaque_out, int output_sample_rate) {
  printf("C++ start\n");
  int ret;
  if (output_sample_rate <= 0) {
    output_sample_rate = 16000;
  }

  if ((inbuffer = (char *) av_malloc(IO_BUF_SIZE)) == nullptr) {
    printf("C++ av malloc failed\n");
    return -1;
  }

  if ((avio_in = avio_alloc_context((unsigned char *)inbuffer, IO_BUF_SIZE, 0, opaque_in, read_buffer, nullptr, nullptr)) == nullptr) {
    printf("C++ avio_alloc_context for input failed\n");
    return -2;
  }

  //分配一个avformat
  if ((avFormatContext = avformat_alloc_context()) == nullptr) {
    printf("C++ avFormatContext alloc fail\n");
    return -3;
  }

  avFormatContext->pb = avio_in; // 赋值自定义的IO结构体
  avFormatContext->flags = AVFMT_FLAG_CUSTOM_IO; // 指定为自定义

  //打开文件，解封装
  if ((ret = avformat_open_input(&avFormatContext, "wtf", nullptr, nullptr)) < 0) {
    printf("C++ open fail:%d\n", ret);
    return ret;
  }

  //查找文件的相关流信息
  if ((ret = avformat_find_stream_info(avFormatContext, nullptr)) < 0) {
    printf("C++ find stream fail:%d\n", ret);
    return ret;
  }
  if (avFormatContext->nb_streams < 1) {
    printf("C++ format streams number is 0\n");
    return -4;
  }

  //获得音频解码器
  enum AVCodecID codec_id = avFormatContext->streams[0]->codecpar->codec_id;
  if ((avcodec = avcodec_find_decoder(codec_id)) == nullptr) {
    printf("C++ avcodec_find_decoder failed\n");
    return -5;
  }

  ;
  if ((avCodecContex = avcodec_alloc_context3(avcodec)) == nullptr) {
    printf("C++ avcodec_alloc_context3 failed\n");
    return -6;
  }

  if ((ret = avcodec_parameters_to_context(avCodecContex, avFormatContext->streams[0]->codecpar)) < 0) {
    printf("C++ avcodec_parameters_to_context failed, ret:%d\n", ret);
    return ret;
  }

  /**第五步：打开音频解码器*/
  if ((ret = avcodec_open2(avCodecContex, avcodec, nullptr)) < 0) {
    printf("C++ 打开音频解码器失败:%d\n", ret);
    return ret;
  }

  /**第六步：读取音频压缩数据->循环读取*/

  //创建音频压缩数据帧->acc格式、mp3格式
  if ((avpacket = (AVPacket *) av_malloc(sizeof(AVPacket))) == nullptr) {
    printf("C++ av_malloc failed\n");
    return -7;
  }

  /**设置输出参数*/
  //输出声道数量 上面设置为立体声
  int out_nb_channels = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_MONO);

  /**获取输入参数*/
  //输入声道布局类型
  int64_t in_ch_layout = av_get_default_channel_layout(avCodecContex->channels);

  /** 开始设置转码信息**/
  // 打开转码器
  if ((swr_context = swr_alloc()) == nullptr) {
    printf("C++ swr_alloc failed\n");
    return -8;
  }

  //设置转码参数
  if ((swr_context = swr_alloc_set_opts(swr_context, AV_CH_LAYOUT_MONO, AV_SAMPLE_FMT_S16, output_sample_rate,
                                        in_ch_layout, avCodecContex->sample_fmt, avCodecContex->sample_rate, 0,
                                        nullptr)) == nullptr) {
    printf("C++ swr_alloc_set_opts failed\n");
    return -9;
  }

  //初始化音频采样数据上下文;初始化转码器
  if ((ret = swr_init(swr_context) < 0)) {
    printf("C++ swr_init failed:%d\n", ret);
    return ret;
  }

  //缓冲区大小 = 采样率(44100HZ) * 采样精度(16位 = 2字节)
  if ((out_buffer = (uint8_t *) av_malloc(IO_BUF_SIZE)) == nullptr) {
    printf("C++ av_malloc(IO_BUF_SIZE) failed\n");
    return -10;
  }

  //创建音频采样数据帧
  if ((avframe = av_frame_alloc()) == nullptr) {
    printf("C++ av_fram_alloc failed\n");
    return -11;
  }

  // 循环读取一份音频压缩数据
  while (av_read_frame(avFormatContext, avpacket) == 0) {
    //判定是否是音频流
    /** 第七步音频解码 */
    //1、发送一帧音频压缩数据包->音频压缩数据帧
    if ((ret = avcodec_send_packet(avCodecContex, avpacket)) != 0) {
      printf("C++ avcodec_send_packet ret = %d\n", ret);
      return ret;
    }

    //2、解码一帧音频压缩数据包->得到->一帧音频采样数据->音频采样数据帧
    if (avcodec_receive_frame(avCodecContex, avframe) == 0) {
      //3、类型转换(音频采样数据格式有很多种类型)
      if ((ret = swr_convert(swr_context, &out_buffer, IO_BUF_SIZE, (const uint8_t **) avframe->data,
                             avframe->nb_samples)) < 0) {
        printf("C++ swr_convert error \n");
        return ret;
      }

      //5、写入文件(你知道要写多少吗？)
      int resampled_data_size = av_samples_get_buffer_size(nullptr, out_nb_channels, ret, AV_SAMPLE_FMT_S16, 1);
      if (resampled_data_size < 0) {
        printf("C++ av_samples_get_buffer_size error:%d\n", resampled_data_size);
//                return resampled_data_size;
      } else {
        (*write_buffer)(opaque_out, out_buffer, resampled_data_size);
      }
    }
  }

  printf("C++ decoder finish\n");
  return 0;
}