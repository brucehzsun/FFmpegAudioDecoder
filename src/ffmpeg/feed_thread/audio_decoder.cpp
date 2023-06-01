//
// Created by bruce sun on 2023/4/27.
#include "audio_decoder.h"
#include "memory"
#include "string"
#include "iostream"
#include "cstring"
//

namespace Rokid {

#define IO_BUF_SIZE 81920

int read_buffer(void *opaque, uint8_t *buf, int buf_size) {
  auto *audio_decoder = (Rokid::AudioDecoder *) opaque;
  if (audio_decoder->eof) return AVERROR_EOF;

  std::string received_data;
  while (true) {
    if (audio_decoder->eof) return AVERROR_EOF;
    if (audio_decoder->input_queue->pop(received_data)) {
      if (received_data.empty()) {
        audio_decoder->eof = true;
        return AVERROR_EOF;
      }
      std::memcpy(buf, received_data.c_str(), received_data.size());
//      std::cout << "read_buffer: " << received_data.size() << ",buf_size=" << buf_size << ">>>>>>" << std::endl;
      return (int) received_data.size();
    } else {
      std::cout << "timeout" << std::endl;
      audio_decoder->eof = true;
      return AVERROR_EOF;
    }
  }
}

void AudioDecoder::DecodeThreadFunc() {
  std::cout << "audio_decoder_start" << std::endl;
  this->init_header();
  this->decode_frame();
  std::cout << "audio_decoder_finish" << std::endl;
}

AudioDecoder::AudioDecoder(int output_sample_rate) : Rokid::AudioDecoderInterface(output_sample_rate) {
  this->input_queue = std::make_shared<Rokid::TimeoutQueue<std::string>>(5000);
  this->audio_stream_index = -1;
}

int AudioDecoder::start(format_buffer_write write_buffer, void *opaque_out) {
  std::cout << "start" << std::endl;
  this->write_buffer = write_buffer;
  this->opaque_out = opaque_out;
  decode_thread_ = std::make_shared<std::thread>(&AudioDecoder::DecodeThreadFunc, this);
  return 0;
}

int AudioDecoder::feed(uint8_t *inbuf, int data_size) const {
  printf("feed,%d\n", data_size);
  std::string raw_data((char *) inbuf, data_size);
  this->input_queue->push(raw_data);
  return 0;
}

int AudioDecoder::stop() {
  printf("stop\n");
  std::string eof_str;
  this->input_queue->push(eof_str);
  decode_thread_->join();
  return 0;
}

int AudioDecoder::init_header() {

  std::cout << "init_header" << std::endl;
  int ret;

// 定义封装结构体,分配一个avformat
  this->format_ctx = avformat_alloc_context();
  if (format_ctx == nullptr) {
    printf("C++ avFormatContext alloc fail\n");
    return -3;
  }

  auto *inbuffer = (unsigned char *) av_malloc(IO_BUF_SIZE);
  if (inbuffer == nullptr) {
    printf("C++ av malloc failed\n");
    return -1;
  }

  AVIOContext *avio_cxt = avio_alloc_context(inbuffer, IO_BUF_SIZE, 0, this, read_buffer, nullptr, nullptr);
  if (avio_cxt == nullptr) {
    printf("C++ avio_alloc_context for input failed\n");
    return -2;
  }

  format_ctx->pb = avio_cxt; // 赋值自定义的IO结构体
  format_ctx->flags = AVFMT_FLAG_CUSTOM_IO; // 指定为自定义

  // 将输入流媒体流链接为封装结构体的句柄，将url句柄挂载至format_ctx结构里，之后ffmpeg即可对format_ctx进行操作
  ret = avformat_open_input(&format_ctx, nullptr, nullptr, nullptr);
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

  /* 穷举所有的音视频流信息，找出流编码类型为AVMEDIA_TYPE_AUDIO对应的索引 */
  for (unsigned int i = 0; i < format_ctx->nb_streams; ++i) {
    const AVStream *stream = format_ctx->streams[i];
    if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
      audio_stream_index = static_cast<int>(i);
      break;
    }
  }

  if (audio_stream_index == -1) {
    fprintf(stderr, "Cannot find a audio stream\n");
    return -1;
  }

  /* 查找audio解码器 */
  // 首先从输入的AVFormatContext中得到stream,然后从stream中根据编码器的CodeID获得对应的Decoder
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

  init_swr_context();

  /**设置输出参数*/
  //输出声道数量 单声道
  this->out_nb_channels = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_MONO);

  // 设置音频缓冲区间 16bit   16000  PCM数据, 单声道
  this->out_buffer = (uint8_t *) av_malloc(IO_BUF_SIZE);
  if (this->out_buffer == nullptr) {
    printf("C++ av_malloc(IO_BUF_SIZE) failed\n");
    return -10;
  }
  std::cout << "init_header success" << std::endl;
  return 0;
}

/** 开始设置转码信息**/
int AudioDecoder::init_swr_context() {
  /************ Audio Convert ************/
  // frame->16bit 16000 PCM 统一音频采样格式与采样率
  int ret;

  // 打开转码器
  this->swr_context = swr_alloc();
  if (this->swr_context == nullptr) {
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
  if ((this->swr_context =
           swr_alloc_set_opts(this->swr_context, AV_CH_LAYOUT_MONO, AV_SAMPLE_FMT_S16, _output_sample_rate,
                              in_ch_layout, av_codec_ctx->sample_fmt, inSampleRate, 0,
                              nullptr)) == nullptr) {
    printf("C++ swr_alloc_set_opts failed\n");
    return -9;
  }

  // 初始化
  //初始化音频采样数据上下文;初始化转码器
  if ((ret = swr_init(this->swr_context) < 0)) {
    printf("C++ swr_init failed:%d\n", ret);
    return ret;
  }
  return 0;
}

int AudioDecoder::decode_frame() {
  /************ Audio Convert ************/
  // 循环读取一份音频压缩数据
  int _data_size = 0;
//  while (av_read_frame(format_ctx, packet) == 0) {
  int ret;
  while (1) {
    ret = av_read_frame(format_ctx, packet); // 读取下一帧数据
    if (ret < 0) {
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
        const auto **frame_data = (const uint8_t **) frame->data;
        ret = swr_convert(swr_context, &this->out_buffer, IO_BUF_SIZE, frame_data, frame->nb_samples);
        if (ret < 0) {
          printf("C++ swr_convert error \n");
          return ret;
        }

        //5、写入文件, 获取实际的缓存大小（补充：第三个参数不应该是输入的样本数，而是重采样的样本数）
        int resampled_data_size = av_samples_get_buffer_size(nullptr, out_nb_channels, ret, AV_SAMPLE_FMT_S16, 1);
        if (resampled_data_size < 0) {
          printf("C++ av_samples_get_buffer_size error:%d\n", resampled_data_size);
//          return _data_size;
          break;
        } else {
          (*write_buffer)(opaque_out, this->out_buffer, resampled_data_size);
        }
      }
    }
    av_packet_unref(packet);
  }
}

}
