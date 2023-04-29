//
// Created by bruce sun on 2023/4/28.
//

#ifndef FFMPEGAUDIODECODER_SRC_FFMPEG_FEED_THREAD_TIMEOUT_QUEUE_H_
#define FFMPEGAUDIODECODER_SRC_FFMPEG_FEED_THREAD_TIMEOUT_QUEUE_H_

#include <queue>
#include <condition_variable>
#include <mutex>
#include <chrono>

namespace Rokid {

template<typename T>
class TimeoutQueue {
 public:
  TimeoutQueue(int timeout_ms) : timeout_(timeout_ms) {}

  bool empty() {
    return queue_.empty();
  }

  void push(const T &item) {
    std::unique_lock<std::mutex> lock(mutex_);
    queue_.push(item);
    cond_.notify_one();
  }

  bool pop(T &item) {
    std::unique_lock<std::mutex> lock(mutex_);
    while (queue_.empty()) {
      if (cond_.wait_for(lock, std::chrono::milliseconds(timeout_)) == std::cv_status::timeout)
        return false;   // 超时返回false
    }
    item = queue_.front();
    queue_.pop();
    return true;
  }

  int size() {
    return queue_.size();
  }

 private:
  std::queue<T> queue_;
  std::condition_variable cond_;
  std::mutex mutex_;
  int timeout_;
};
}
#endif //FFMPEGAUDIODECODER_SRC_FFMPEG_FEED_THREAD_TIMEOUT_QUEUE_H_
