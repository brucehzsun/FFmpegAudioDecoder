#include "AudioDecoder.h"

#include <iostream>
#include <chrono>

using namespace std;

//Read File
int read_buffer(void *opaque, uint8_t *buf, int buf_size) {
    FILE *fp_open = (FILE *) opaque;
    if (!feof(fp_open)) {
        int true_size = fread(buf, 1, buf_size, fp_open);
        return true_size;
    } else {
        printf("read_buffer end of file\n");
        return AVERROR_EOF;
    }
}

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

    const char *inputFileName = "../data/music.mp3";
    const char *out_filename = "../data/music-out.pcm";
    FILE *fp_open = fopen(inputFileName, "rb");    //视频源文件
    FILE *fp_write = fopen(out_filename, "wb+"); //输出文件

    AudioDecoder *decoder;
//    decoder->start(read_buffer, fp_open, write_buffer, fp_write, 16000);


    std::chrono::milliseconds endms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
    );
//    delete decoder;
    cout << "C++ finish, time = " << (endms.count() - startTime) << endl;
    return 0;
}