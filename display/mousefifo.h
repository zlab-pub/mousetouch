#ifndef DISPLAY_MOUSEFIFO_H_
#define DISPLAY_MOUSEFIFO_H_

#include <deque>
#include <mutex>
#include "winusb/mousecore.h"

namespace npnx {

struct MouseFifoReport {
  int hDevice, button, action;
  double screenX, screenY;
};

template <class T>
class NpnxFifo {
public:
  NpnxFifo() {data.clear();}
  ~NpnxFifo() {}

  inline void Push(const T & report) {
    std::lock_guard<std::mutex> lck(objectMutex);
    data.push_back(report);
  } 

  inline bool Pop(T *report) {
    std::lock_guard<std::mutex> lck(objectMutex);
    if (data.empty()) return false;
    *report = data.front();
    data.pop_front();
    return true;
  }

private:
  std::mutex objectMutex;
  std::deque<T> data;
};

typedef NpnxFifo<MouseFifoReport> MouseFifo;

}

#endif // !DISPLAY_MOUSEFIFO_H_