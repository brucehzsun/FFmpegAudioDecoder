#include "audio_decoder.h"

#include <iostream>
#include <chrono>

using namespace std;

//Write File
int write_buffer(void *outObj, uint8_t *buf, int buf_size) {
//    printf("write_buffer\n");
  FILE *fp_write = (FILE *) outObj;
  if (!feof(fp_write)) {
    int true_size = fwrite(buf, 1, buf_size, fp_write);
    return true_size;
  } else {
    return -1;
  }
}

int main() {
  std::cout << "Hello, World!" << std::endl;

  std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch()
  );

  long startTime = ms.count();
  std::cout << ms.count() << std::endl;

//    const char *inputFileName = "../data/music.mp3";
//    const char *out_filename = "../data/music-out.pcm";
//  const char *inputFileName = "data/test.amr";
//  const char *out_filename = "data/test_out_amr.pcm";

  const char *inputFileName = "data/test.ogg";
  const char *out_filename = "data/test_out_opus.pcm";

//  const char *inputFileName = "data/test.mp3";
//  const char *out_filename = "data/test_out_mp3.pcm";

//  const char *inputFileName = "data/test.amr";
//  const char *out_filename = "data/test_out_mp3.pcm";

//  const char *inputFileName = "data/test.m4a";
//  const char *out_filename = "data/test_out_m4a.pcm";

  FILE *fp_open = fopen(inputFileName, "rb");    //视频源文件
  FILE *fp_write = fopen(out_filename, "wb+"); //输出文件

  std::shared_ptr<std::thread> decode_thread_ = nullptr;
  Rokid::AudioDecoder *decoder = new Rokid::AudioDecoder();
  decoder->start(write_buffer, fp_write);
  int buf_size = 1024 * 8;
  char buf[buf_size];

  while (!feof(fp_open)) {
    int true_size = fread(buf, 1, buf_size, fp_open);
//    printf("read from file,len=%d\n", true_size);
    int ret = decoder->feed((uint8_t *) buf, true_size);
  }
  int ret = decoder->stop();
  std::cout << "stop,ret=" << ret << std::endl;

  std::chrono::milliseconds endms = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch()
  );
//    delete decoder;
  cout << "C++ finish, time = " << (endms.count() - startTime) << endl;
  return 0;
}