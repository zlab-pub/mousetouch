#ifndef DISPLAY_HCICONTROLLER_H_
#define DISPLAY_HCICONTROLLER_H_

#include "common.h"
#include "winusb/mousecore.h"
#include "mousefifo.h"
#include <thread>
#include <vector>
#include <mutex>
#include <atomic>
#include <functional>

namespace npnx {

#define HCICONTROLLER_PIPE_SENDER_PATH TEXT("\\\\.\\pipe\\hci_pipe_cpp_to_py")
#define HCICONTROLLER_PIPE_RECEIVER_PATH TEXT("\\\\.\\pipe\\hci_pipe_py_to_cpp")

enum HCIMessageInType {
  HCIMESSAGEINTYPE_POSITION = 0x00,
  HCIMESSAGEINTYPE_ANGLE_1 = 0x01,
  HCIMESSAGEINTYPE_ANGLE_2 = 0x02,
  HCIMESSAGEINTYPE_ANGLE_HALT = 0x03,
  HCIMESSAGEINTYPE_EXIT = 0xff
};

enum HCIMessageOutType {
  HCIMESSAGEOUTTYPE_POSITION = 0x00,
  HCIMESSAGEOUTTYPE_ANGLE = 0x01,
  HCIMESSAGEOUTTYPE_ANGLE_SIGNAL = 0x02,
  HCIMESSAGEOUTTYPE_ANGLE_HALT = 0x03,
};

struct HCIMessageReport {
  int index;
  unsigned char type;
  uint32_t param1, param2;
};

struct HCIReport {
	uint32_t x = 0;
  uint32_t y = 0;
	float angle = 0;
	bool isCompleted = false;
};

typedef NpnxFifo<HCIMessageReport> HCIMessageFifo;


class HCIController;
class MultiMouseSystem;

class HCIInstance {
public:
  HCIInstance(HCIController *parent, int mouseidx);
  ~HCIInstance() = default;

  void Start();

  //should be private but std::thread need a bind or lambda for a member function,
  //or a non-member friend wrapper for this func.
  void senderEntry(HANDLE hSenderPipe);
  void receiverEntry(HANDLE hReceiverPipe);

public:
  std::atomic<int> messageType = HCIMESSAGEINTYPE_POSITION; 
  std::thread senderThread, receiverThread;
  HCIReport tokenReport;

private:
  WCHAR senderPipePath[255], receiverPipePath[255];
  HCIController *mParent;
  int mouseIdx;
  
};

class HCIController {
public:
  HCIController();
  ~HCIController();

  void Init(MultiMouseSystem *parent, int num_mouse);
  void Start();

public:
  MultiMouseSystem *mParent;
  MouseCore *core;
  std::vector<HCIInstance *> instances;
  int mNumMouse = 0;
  HCIMessageFifo messageFifo;

private:
  bool mInitialized = false;
};

}

#endif //!DISPLAY_HCICONTROLLER_H_
