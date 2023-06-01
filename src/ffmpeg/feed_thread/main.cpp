#include "audio_decoder.h"
#include <iostream>
#include <chrono>
#include "fstream"
#include "opus_decoder.h"

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

int test_ffmpeg() {
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

//  const char *inputFileName = "data/test.amr";
//  const char *out_filename = "data/test_out_amr.pcm";

//  const char *inputFileName = "data/test.m4a";
//  const char *out_filename = "data/test_out_m4a.pcm";

  FILE *fp_open = fopen(inputFileName, "rb");    //视频源文件
  FILE *fp_write = fopen(out_filename, "wb+"); //输出文件

  std::shared_ptr<std::thread> decode_thread_ = nullptr;
  auto *decoder = new Rokid::AudioDecoder(16000);
  decoder->start(write_buffer, fp_write);
  int buf_size = 1024 * 8;
  char buf[buf_size];

  while (!feof(fp_open)) {
    int true_size = fread(buf, 1, buf_size, fp_open);
//    printf("read from file,len=%d\n", true_size);
    int ret = decoder->feed((uint8_t *) buf, true_size);
  }
  int ret = decoder->stop();
//  std::cout << "stop,ret=" << ret << std::endl;

  std::chrono::milliseconds endms = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch()
  );
//    delete decoder;
  cout << "test ffmpeg finish, time = " << (endms.count() - startTime) << endl;
  return 0;
}

int test_opus() {

  const char *out_filename = "data/test_out_opu.pcm";
  FILE *fp_write = fopen(out_filename, "wb+"); //输出文件

  Rokid::RKOpusDecoder decoder;

  int ret = decoder.start(write_buffer, fp_write);
  if (ret != 0) { // 根据题目需求初始化RKOpusDecoder类
    cerr << "Failed to start decoder" << endl;
    return -1;
  }

  std::ifstream input("data/in.opu", std::ios::binary); // 打开输入文件流
  if (!input.is_open()) {
    cerr << "Failed to open input file" << endl;
    return -1;
  }

  char length_char;
  while (input.get(length_char)) { // 读取输入文件每个字节，直到文件结束
    std::vector<char> buffer_vector; // 存储读入的数据
    uint8_t length = static_cast<uint8_t>(length_char);
    buffer_vector.push_back(length_char);
    char *buffer = new char[length];
    input.read(buffer, length); // 读取对应长度的内容到缓冲区
    buffer_vector.insert(buffer_vector.end(), buffer, buffer + length); // 将缓冲区内容追加到 vector 中
    ret = decoder.feed((uint8_t *) buffer_vector.data(), buffer_vector.size()); // 解码缓冲区内容
//    std::cout << "feed = " << length << ",ret=" << ret << std::endl;
    delete[] buffer;
  }

  decoder.stop(); // 关闭解码器

  input.close(); // 关闭输入文件流
  fclose(fp_write);
  std::cout << "test opus finish" << std::endl;
  return 0;
}

int main() {
//  test_ffmpeg();
  test_opus();
}