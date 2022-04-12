#include "hcicontroller.h"
#include <cstdlib>
#include <ctime>
#include <ratio>
#include <chrono>


#include "multimouse.h"

using namespace npnx;

HCIInstance::HCIInstance(HCIController *parent, int mouseidx):
  mouseIdx(mouseidx),
  mParent(parent)
{
  wsprintf(senderPipePath, TEXT("%ls_%d"), HCICONTROLLER_PIPE_SENDER_PATH, mouseidx);
  printf("%ls\n", senderPipePath);
  wsprintf(receiverPipePath, TEXT("%ls_%d"), HCICONTROLLER_PIPE_RECEIVER_PATH, mouseidx);
  printf("%ls\n", receiverPipePath);
}

void HCIInstance::Start() {
  HANDLE hSenderPipe = CreateNamedPipe(senderPipePath, PIPE_ACCESS_DUPLEX,
                                 PIPE_TYPE_MESSAGE | PIPE_WAIT | PIPE_READMODE_MESSAGE,
                                 PIPE_UNLIMITED_INSTANCES,
                                 1,
                                 1, 1000, NULL);
  HANDLE hReceiverPipe = CreateNamedPipe(receiverPipePath, PIPE_ACCESS_DUPLEX,
                                        PIPE_TYPE_MESSAGE | PIPE_WAIT | PIPE_READMODE_MESSAGE,
                                        PIPE_UNLIMITED_INSTANCES,
                                        1,
                                        1, 1000, NULL);
  STARTUPINFO si;
  PROCESS_INFORMATION pi;
  memset(&si, 0, sizeof(STARTUPINFO));
  si.cb = sizeof(STARTUPINFO);
  memset(&pi, 0, sizeof(PROCESS_INFORMATION));
  WCHAR processCmd[8192];
  
  //if use python, add python at the beginning like this
  wsprintf(processCmd, TEXT("python %ls/%ls %d %ls %ls"), TEXT(NPNX_PY_PATH), TEXT(NPNX_PY_EXECUTABLE), mouseIdx, senderPipePath, receiverPipePath);
  
  // // wsprintf(processCmd, TEXT("%ls/%ls %d %ls %ls"), TEXT(NPNX_PY_PATH), TEXT(NPNX_PY_EXECUTABLE), mouseIdx, senderPipePath, receiverPipePath);
  printf("processCmd %ls\n", processCmd);
  if (!CreateProcess(NULL,
                     processCmd,
                     NULL,
                     NULL,
                     FALSE,
                     0,
                     NULL,
                     NULL,
                     &si,
                     &pi)) {
    NPNX_ASSERT_LOG(false && "createPyProcessFailed", GetLastError());
  } else {
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
  }

  bool fConnected = ConnectNamedPipe(hSenderPipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
  NPNX_ASSERT(fConnected && "sender");
  fConnected = ConnectNamedPipe(hReceiverPipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
  NPNX_ASSERT(fConnected && "receiver");

  senderThread = std::thread([&, this](HANDLE hPipe){this->senderEntry(hPipe);}, hSenderPipe);
  receiverThread = std::thread([&, this](HANDLE hPipe){this->receiverEntry(hPipe);}, hReceiverPipe);
}

void HCIInstance::senderEntry(HANDLE hPipe) {
  int counter = 0;
  int64_t lll = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
  int lastState = HCIMESSAGEINTYPE_POSITION;
  while (true) {
    int nowState = messageType.load();
    switch (nowState) {
      case HCIMESSAGEINTYPE_POSITION:
        {
          unsigned char buf[255];
          int64_t last_time = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
          mParent->core->ControlTransfer(mouseIdx, 0x40, 0x01, 0x0000, 0x0D, buf+1, 1, 1000);
          buf[0] = HCIMESSAGEINTYPE_POSITION;
          int cnt = mParent->core->ControlTransfer(mouseIdx, 0xC0, 0x01, 0x0000, 0x0D, buf+1, 1, 1000);
          int64_t nowtime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
          *(int64_t *)(buf+3) = nowtime;
          LIBUSB_CHECK_RET(hcisender_position_controlTrans, cnt);

          if (buf[1]<128) buf[1]+=128;
          cnt = mParent->core->ControlTransfer(mouseIdx, 0xC0, 0x01, 0x0000, 0x0B, buf+2, 1, 1000);
          int64_t nowtime2 = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
          *(int64_t *)(buf+11) = nowtime2;
          LIBUSB_CHECK_RET(hcisender_position_controlTrans_1, cnt);
          // printf("%lf\n", (nowtime-last_time) / 1000000.0);
          DWORD length = 0;
          NPNX_ASSERT_LOG(WriteFile(hPipe, buf, 3 + 8 + 8, &length, NULL), GetLastError());
          counter++;
          if (nowtime - lll > 10000000) {
            NPNX_LOG(counter);
            counter = 0;
            lll = nowtime;
          }
        }
        break;
      case HCIMESSAGEINTYPE_ANGLE_1:
      case HCIMESSAGEINTYPE_ANGLE_2:
        {
          unsigned char buf[4096];
          buf[0] = nowState;
          unsigned char tempbuf[255];
          if (lastState != nowState) {
            std::chrono::duration<double, std::milli> wait_ms(10);
            std::this_thread::sleep_for(wait_ms);
          }
          mParent->core->ControlTransfer(mouseIdx, 0x40, 0x01, 0x0000, 0x0D, tempbuf, 1, 1000);
          for (int i = 0; i < 19*19; i++) {
            int cnt = mParent->core->ControlTransfer(mouseIdx, 0xC0, 0x01, 0x0000, 0x0D, buf + 1 + i, 1, 1000);
            LIBUSB_CHECK_RET(hcisender_position_controlTrans, cnt);
            std::chrono::duration<double, std::milli> wait_ms(0.5);
            std::this_thread::sleep_for(wait_ms);
            int currentState = messageType.load();
            if (currentState != nowState) break;
          }
          DWORD length = 0;
          int currentState = messageType.load();
          if (nowState == currentState) {
            NPNX_ASSERT_LOG(WriteFile(hPipe, buf, 19 * 19 + 1, &length, NULL), GetLastError());
          }
        }
        break;
      case HCIMESSAGEINTYPE_ANGLE_HALT:
        //reserved.
        break;
      case HCIMESSAGEINTYPE_EXIT:
        DisconnectNamedPipe(hPipe);
        CloseHandle(hPipe);
        return;
        break;
      default:
        NPNX_ASSERT(false && "HCIInstance send status error");
        break;
    }
    std::this_thread::yield();
    lastState = nowState;
  }
}

void HCIInstance::receiverEntry(HANDLE hPipe) {
  HCIMessageReport thisReport;
  memset(&thisReport, 0, sizeof(HCIMessageReport));
  while (true) {
    unsigned char buf[4096];
    DWORD length = 0;
    NPNX_ASSERT_LOG(ReadFile(hPipe, buf, 1024, &length, NULL), GetLastError());
    LIBUSB_CHECK_RET_BUFFER_SIZE(receive_hci_report, 0, buf, length);
    thisReport.index = buf[0];
    thisReport.type = buf[1];
    switch (thisReport.type) {
      case HCIMESSAGEOUTTYPE_POSITION:
        // (32,0)
        //  |
        // (0,0) -- (0,32)
        
        thisReport.param1 = *(int16_t *)&(buf[2]);
        thisReport.param2 = *(int16_t *)&(buf[4]);
        
        break;
      case HCIMESSAGEOUTTYPE_ANGLE:
        {
          float ff = (540 - *(float *)&(buf[2])) / 180.0f * 3.1415926535897f;
          thisReport.param1 = *(uint32_t *) &ff;
        }
        break;
      case HCIMESSAGEOUTTYPE_ANGLE_SIGNAL:
        break;
      default:
        LIBUSB_CHECK_RET_BUFFER_SIZE(hci_output_type_error, -1, buf, length);
        CloseHandle(hPipe);
        exit(-1);
        break;    
    }
    mParent->messageFifo.Push(thisReport);
    std::this_thread::yield();
  }
}

HCIController::HCIController(){
  instances.clear();
}

HCIController::~HCIController(){
  for(auto it: instances){
    delete it;
  }
  instances.clear();
}

void HCIController::Init(MultiMouseSystem *parent, int num_mouse) {
  core = &parent->core;
  mNumMouse = num_mouse;
  for (int i=0; i<num_mouse; i++) {
    instances.push_back(new HCIInstance(this, i));
  }
  mInitialized = true;
}

void HCIController::Start() {
  NPNX_ASSERT(mInitialized);
  for (auto it: instances) {
    it->Start();
  }
}