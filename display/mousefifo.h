#ifndef DISPLAY_MOUSEFIFO_H_
#define DISPLAY_MOUSEFIFO_H_

#include <deque>
#include <mutex>
#include "common.h"

namespace npnx {

struct MouseFifoReport {
  int hDevice, button, action;
  double screenX, screenY;
};

struct SensorFifoReport {
    unsigned char data[SP_REPORT_SIZE]; // pixel_1, pixel_2, pixel_3, pixel_4, pixelAvg
    SensorFifoReport() {};
    SensorFifoReport(std::string str) {
        for (int i = 0; i < SP_REPORT_SIZE; i++) {
            if (i < str.length()) {
                data[i] = str.at(i);
            }
            else {
                data[i] = 0x00;
            }
        }
    };
    SensorFifoReport(const char* buf) {
        for (int i = 0; i < SP_REPORT_SIZE; i++) {
            data[i] = buf[i];
        }
    };
    void PrintHex() {
        for (int i = 0; i < SP_REPORT_SIZE; i++) {
            printf("%.2X ", data[i] & 0xff);
        }
        printf("\n");
    }
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

  inline bool empty() {
      return data.empty();
  }

  inline int size() {
    return data.size();
  }

private:
  std::mutex objectMutex;
  std::deque<T> data;
};

typedef NpnxFifo<MouseFifoReport> MouseFifo;
typedef NpnxFifo<SensorFifoReport> PositionRawDataFifo;
}

#endif // !DISPLAY_MOUSEFIFO_H_