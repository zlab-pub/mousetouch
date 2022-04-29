#ifndef DISPLAY_WINUSB_MOUSECORE_H_
#define DISPLAY_WINUSB_MOUSECORE_H_


#ifdef USE_MOUSECORE
#include <libusb.h>
#include "libusb_utils.h"
  #ifdef MOUSECORE_NATIVE_ASYNC
    #error "cannot use native_async without native enabled."
  #endif
#endif
#ifdef USE_MOUSECORE_NATIVE
#include "../common.h"
#include "usb_utils.h"
 #include <windows.h>
 #include <SetupAPI.h>
 #endif
 #ifdef USE_MOUSECORE_SP
 #include "../common.h"
#include "../mousefifo.h"
#include "SerialPort.h"
#include <windows.h>
#include <SetupAPI.h>
#endif

#include <stdint.h>
#include <functional>
#include <thread>
#include <vector>

struct MouseReport {
  uint8_t flags;
  uint8_t button;
  int16_t xTrans;
  int16_t yTrans;
  int16_t wheel;
};

//this callback must be thread safe.
typedef std::function<void(int, MouseReport)> MOUSEREPORTCALLBACKFUNC;

#define MOUSECORE_EXTRACT_BUFFER(buffer, size, offset, type) \
  (((offset) + sizeof(type) <= (size)) ? *(type *)((uint8_t *)(buffer) + (offset)): (type) 0)

#define NUM_MOUSE_MAXIMUM 10

// const uint16_t default_vid = 0x046D;
// const uint16_t default_pid = 0xC077;

// const uint16_t default_vid = 0x046D;
// const uint16_t default_pid = 0xC019;

const uint16_t default_vid = 0x046D;
const uint16_t default_pid = 0xC05B;

//this is for mouse hid report
const int target_report_configuration = 0x01;
const int target_report_interface = 0x00;
const unsigned char target_report_endpoint = 0x81;

namespace npnx {
    class MouseCore {
    public:
        MouseCore();
        ~MouseCore();

        //return num_mouse
        int Init(uint16_t vid, uint16_t pid, MOUSEREPORTCALLBACKFUNC func);
        int ControlTransfer(int mouse_idx, uint8_t request_type, uint8_t bRequest, uint16_t wValue, uint16_t wIndex,
            unsigned char* data, uint16_t wLength, unsigned int timeout);

        //start polling for all mouses.
        void Start();
        int GetPositionRawData(int mouse_idx, unsigned char* buf); // return data size
        int GetAngleRawData(int mouse_idx, unsigned char* buf); // return data size
        void StopOrStartPositionDataReport(int mouse_idx);
        void Stop();

    private:
        void poll(int idx);
        MouseReport raw_to_mousereport(uint8_t* buffer, size_t size);

    private:

#ifdef USE_MOUSERCORE
        libusb_device* devs[NUM_MOUSE_MAXIMUM];
        libusb_device_handle* devs_handle[NUM_MOUSE_MAXIMUM];
#endif
#ifdef USE_MOUSECORE_NATIVE
        std::vector<PSP_DEVICE_INTERFACE_DETAIL_DATA> pInterface_details;
        std::vector<HANDLE> h_files, h_winusbs, h_hidinterfaces;
        inline WCHAR* GetDevicePath(int hDevice) {
            return &(pInterface_details[hDevice]->DevicePath[0]);
        }
#endif
#ifdef USE_MOUSECORE_SP
#define RECEIVING_POSITION_DATA 0
#define RECEIVING_ANGLE_DATA 1
        std::vector<PSP_DEVICE_INTERFACE_DETAIL_DATA> pInterface_details;
        std::vector<std::string> serial_names;
        std::vector<SerialPort*> serial_interfaces;
        std::vector<PositionRawDataFifo*> position_raw_data_fifos;
        std::vector<int64_t> position_raw_data_init_time;
        int state;
#endif

        MOUSEREPORTCALLBACKFUNC mouseReportCallbackFunc;
        int num_mouse = 0;
        int hid_report_size = 0;

        bool shouldStop = false;
        std::thread pollThread[NUM_MOUSE_MAXIMUM];

    };
}

#endif // !DISPLAY_WINUSB_MOUSECORE_H_
