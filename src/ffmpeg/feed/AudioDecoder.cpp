//
// Created by bruce sun on 2021/9/1.
//

#include "AudioDecoder.h"
#include "iostream"
#include "libavformat/avformat.h"
#include "libavformat/avio.h"

#include "libavcodec/avcodec.h"

#include "libavutil/audio_fifo.h"
#include "libavutil/avassert.h"
#include "libavutil/avstring.h"
#include "libavutil/channel_layout.h"
#include "libavutil/frame.h"
#include "libavutil/opt.h"

#include "libswresample/swresample.h"

AudioDecoder::AudioDecoder(int output_sample_rate) {

  this->_data_buffer = new uint8_t[AUDIO_INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
  if (output_sample_rate > 0) {
    this->_output_sample_rate = output_sample_rate;
  } else {
    this->_output_sample_rate = 16000;
  }
  pkt = av_packet_alloc();

  //缓冲区大小 = 采样率(44100HZ) * 采样精度(16位 = 2字节)
  if ((this->swr_buffer = (uint8_t *) av_malloc(IO_BUF_SIZE)) == nullptr) {
    printf("C++ av_malloc(IO_BUF_SIZE) failed\n");
    return;
  }

  /**设置输出参数*/
  //输出声道数量 上面设置为立体声
  this->out_nb_channels = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_MONO);

  /* find the MPEG audio decoder */
//  const AVCodec *codec = avcodec_find_decoder(AV_CODEC_ID_MP3);
//  const AVCodec *codec = avcodec_find_decoder(AV_CODEC_ID_OPUS);
  const AVCodec *codec = avcodec_find_decoder(AV_CODEC_ID_AMR_NB);
//  const AVCodec *codec = avcodec_find_decoder(AV_CODEC_ID_AAC);
  if (!codec) {
    fprintf(stderr, "Codec not found\n");
    return;
  }

  this->parser = av_parser_init(codec->id);
  if (!this->parser) {
    fprintf(stderr, "Parser not found\n");
    return;
  }

  this->context = avcodec_alloc_context3(codec);
  if (!this->context) {
    fprintf(stderr, "Could not allocate audio codec context\n");
    return;
  }

  /* open it */
  if (avcodec_open2(this->context, codec, nullptr) < 0) {
    fprintf(stderr, "Could not open codec\n");
    return;
  }
}

int AudioDecoder::feed(uint8_t *raw_data, int raw_data_size, uint8_t **out_buffer) {
  std::cout << "feed:" << raw_data_size << std::endl;
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
    } else if (ret > 0) {
      _data += ret; //指针偏移量
      _data_size -= ret; // 剩余数据的大小
    }

    if (pkt->size) {
      out_buffer_size = decodeFrame(out_buffer, out_buffer_size);
      if (out_buffer_size < 0) {
        fprintf(stderr, "decodeFrame failed ret = %d\n", out_buffer_size);
      }
    }

    //本次feed的数据处理完毕，可能还有部分剩余数据
    if (_data_size < AUDIO_REFILL_THRESH) {
      //从 data 复制 data_size 个字符到 inbuf
      memmove(_data_buffer, _data, _data_size);
      _data = _data_buffer;
      break;
    }
  }
  return out_buffer_size;
}

int AudioDecoder::stop(uint8_t **out_buffer) {
  /* flush the decoder */
  int out_buffer_size = 0;
  pkt->data = nullptr;
  pkt->size = 0;
  out_buffer_size = decodeFrame(out_buffer, out_buffer_size);
  return out_buffer_size;
}

int AudioDecoder::decodeFrame(uint8_t **out_buffer, int out_buffer_size) {

  /* send the packet with the compressed data to the decoder */
  int ret = avcodec_send_packet(context, pkt);
  if (ret < 0) {
    fprintf(stdout, "Error submitting the packet to the decoder\n");
    return -100;
  }

  if (ret == 0 && swr_context == nullptr) {
    initSwrContext();
  }

  /* read all the output frames (in general there may be any number of them */
  while (ret >= 0) {
    ret = avcodec_receive_frame(context, decoded_frame);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
      return out_buffer_size;
    else if (ret < 0) {
      fprintf(stderr, "Error during decoding\n");
      return ret;
    }

    //3、类型转换(音频采样数据格式有很多种类型)
    if ((ret = swr_convert(swr_context, &swr_buffer, IO_BUF_SIZE, (const uint8_t **) decoded_frame->data,
                           decoded_frame->nb_samples)) < 0) {
      printf("C++ swr_convert error \n");
      return ret;
    }

    //5、写入文件(你知道要写多少吗？)
    int resampled_data_size = av_samples_get_buffer_size(nullptr, out_nb_channels, ret, AV_SAMPLE_FMT_S16, 1);
    if (resampled_data_size < 0) {
      printf("C++ av_samples_get_buffer_size error:%d\n", resampled_data_size);
      return resampled_data_size;
    }

    // (*_write_buffer)(outfile, swr_buffer, resampled_data_size);
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
//  swr_context = swr_alloc_set_opts(swr_context, AV_CH_LAYOUT_MONO, AV_SAMPLE_FMT_S16, _output_sample_rate,
//                                   in_ch_layout, context->sample_fmt, 44100, 0, nullptr);
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

static int init_resampler(AVCodecContext *input_codec_context,
                          AVCodecContext *output_codec_context,
                          SwrContext **resample_context) {
  int error;

  /*
   * Create a resampler context for the conversion.
   * Set the conversion parameters.
   */
  error = swr_alloc_set_opts2(resample_context,
                              &output_codec_context->ch_layout,
                              output_codec_context->sample_fmt,
                              output_codec_context->sample_rate,
                              &input_codec_context->ch_layout,
                              input_codec_context->sample_fmt,
                              input_codec_context->sample_rate,
                              0, NULL);
  if (error < 0) {
    fprintf(stderr, "Could not allocate resample context\n");
    return error;
  }
  /*
  * Perform a sanity check so that the number of converted samples is
  * not greater than the number of samples to be converted.
  * If the sample rates differ, this case has to be handled differently
  */
  av_assert0(output_codec_context->sample_rate == input_codec_context->sample_rate);

  /* Open the resampler with the specified parameters. */
  if ((error = swr_init(*resample_context)) < 0) {
    fprintf(stderr, "Could not open resample context\n");
    swr_free(resample_context);
    return error;
  }
  return 0;
}

AudioDecoder::~AudioDecoder() {

  _data = nullptr;
  delete[] _data_buffer;
  av_free(swr_buffer);
  swr_free(&swr_context);
  avcodec_free_context(&context);
  av_parser_close(parser);
  av_frame_free(&decoded_frame);
  av_packet_free(&pkt);
}