#include "AudioDecoder.h"

#include <iostream>
#include <chrono>
#include <fstream>

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
int write_buffer(void *opaque, uint8_t *buf, int buf_size) {
    FILE *fp_write = (FILE *) opaque;
    if (fp_write != nullptr) {
        if (!feof(fp_write)) {
            int true_size = fwrite(buf, 1, buf_size, fp_write);
            return true_size;
        } else {
            return -1;
        }
    }
    return buf_size;
}


int main() {
    std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
    );
    long startTime = ms.count();
    std::cout << ms.count() << std::endl;

    const char *inputFileName = "../data/music.mp3";
    const char *out_filename = "../data/music-out.pcm";
    FILE *fp_open = fopen(inputFileName, "rb");    //视频源文件

    auto *pDecoder = new AudioDecoder(16000, write_buffer, true);

    int buf_size = 1024 * 1;
    uint8_t inbuffer[buf_size];
    while (!feof(fp_open)) {
        int true_size = fread(inbuffer, 1, buf_size, fp_open);
        pDecoder->feed(inbuffer, true_size);
    }

    pDecoder->stop();
    delete pDecoder;

    std::chrono::milliseconds endms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
    );
    cout << "C++ finish, time = " << (endms.count() - startTime) << endl;
    return 0;
}