#include "audio_decoder.h"

#include <iostream>
#include <chrono>

using namespace std;

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

//  const char *inputFileName = "data/test.ogg";
//  const char *out_filename = "data/test_out_opus.pcm";

  const char *inputFileName = "data/test.mp3";
  const char *out_filename = "data/test_out_mp3.pcm";

//  const char *inputFileName = "data/test.m4a";
//  const char *out_filename = "data/test_out_m4a.pcm";

  FILE *fp_open = fopen(inputFileName, "rb");    //视频源文件
  FILE *fp_write = fopen(out_filename, "wb+"); //输出文件

  Rokid::AudioDecoder *decoder = new Rokid::AudioDecoder();
  decoder->start();
  int buf_size = 1024 * 8;
  char buf[buf_size];
  uint8_t *out_buf = new uint8_t[buf_size * 8];

  while (!feof(fp_open)) {
    int true_size = fread(buf, 1, buf_size, fp_open);
    printf("read from file,len=%d\n", true_size);
    int pcm_size = decoder->feed((uint8_t *) buf, true_size, &out_buf);
    int writer_size = fwrite(out_buf, 1, pcm_size, fp_write);
    printf("read_buffer raw_size=%d,pcm_size=%d,writer_size=%d\n", true_size, pcm_size, writer_size);
  }

  std::chrono::milliseconds endms = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch()
  );
//    delete decoder;
  cout << "C++ finish, time = " << (endms.count() - startTime) << endl;
  return 0;
}