#include "AudioDecoder.h"

#include <iostream>
#include <chrono>
#include <fstream>

using namespace std;

int main() {
  std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch()
  );
  long startTime = ms.count();
  std::cout << ms.count() << std::endl;

  const char *inputFileName = "data/test.m4a";
  const char *out_filename = "data/test_feed_m4a_111.pcm";
  FILE *fp_open = fopen(inputFileName, "rb");    //视频源文件
  FILE *fp_out = fopen(out_filename, "wb");

  auto *pDecoder = new AudioDecoder(16000);

  int buf_size = 1024;
  uint8_t inbuffer[buf_size];
  auto *p_out_buffer = new uint8_t[buf_size * 16];

  int out_buffer_size;
  while (!feof(fp_open)) {
    int true_size = fread(inbuffer, 1, buf_size, fp_open);
    out_buffer_size = pDecoder->feed(inbuffer, true_size, &p_out_buffer);
    if (out_buffer_size > 0) {
      fwrite(p_out_buffer, 1, out_buffer_size, fp_out);
    } else {
      fprintf(stderr, "feed failed,ret=%d\n", out_buffer_size);
    }
  }

  out_buffer_size = pDecoder->stop(&p_out_buffer);
  fwrite(p_out_buffer, 1, out_buffer_size, fp_out);

  delete pDecoder;
  delete[]p_out_buffer;
  fclose(fp_open);
  fclose(fp_out);
  std::chrono::milliseconds endms = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch()
  );
  cout << "C++ finish, time = " << (endms.count() - startTime) << endl;
  return 0;
}