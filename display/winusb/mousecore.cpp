#ifdef USE_MOUSECORE
#include "mousecore.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <libusb.h>
#include "libusb_utils.h"

using namespace npnx;

MouseCore::MouseCore() {
  memset(devs, 0x00, NUM_MOUSE_MAXIMUM * sizeof(libusb_device *));
  memset(devs_handle, 0x00, NUM_MOUSE_MAXIMUM * sizeof(libusb_device_handle *));
  mouseReportCallbackFunc = NULL;
}

MouseCore::~MouseCore() {
  shouldStop = true;
  for (int i=0; i< NUM_MOUSE_MAXIMUM; i++) {
    if (pollThread[i].joinable()) pollThread[i].join();
    if (devs_handle[i] != NULL) {
      libusb_close(devs_handle[i]);
      devs_handle[i] = NULL;
    }
  }
}

//return num_mouse
int MouseCore::Init(uint16_t vid, uint16_t pid, MOUSEREPORTCALLBACKFUNC func) {
  mouseReportCallbackFunc = func;

  libusb_device **dev_list;
  int cnt;
  int ret; // return value;
  unsigned char buf[4096]; //WinUSB only support up to 4kb buffer. Larger is useless.

  LIBUSB_ASSERTCALL(libusb_init(NULL));
  
  cnt = (int) libusb_get_device_list(NULL, &dev_list);
  LIBUSB_CHECK_RET(libusb_get_device_list, cnt);

  for (int i = 0; i < cnt; i++) {
    print_device(dev_list[i], 0);
    libusb_device_descriptor desc;
    LIBUSB_ASSERTCALL(libusb_get_device_descriptor(dev_list[i], &desc));
    if ((desc.idVendor & 0xfff0) == 0x0460) {
      printf("found target\n");
      devs[num_mouse++] = dev_list[i];
      LIBUSB_ASSERTCALL(libusb_open(devs[num_mouse - 1], &devs_handle[num_mouse - 1]));
      if (num_mouse > NUM_MOUSE_MAXIMUM) break;
    }
  } 
  
  libusb_free_device_list(dev_list, 1);

  for (int i = 0; i < num_mouse; i++) {
    printf("device %d: \n", i);

    cnt = libusb_get_descriptor(devs_handle[i], LIBUSB_DT_DEVICE, 0, buf, 4096);
    LIBUSB_CHECK_RET_BUFFER(mousecore_device_descriptor, cnt, buf);

    cnt = libusb_get_descriptor(devs_handle[i], LIBUSB_DT_CONFIG, 0, buf, 4096);
    LIBUSB_CHECK_RET_BUFFER(mousecore_config_descriptor, cnt, buf);
    
    cnt = libusb_control_transfer(devs_handle[i], 0x81, 0x06, 0x2100, 0, buf, 1024, 1000);
    LIBUSB_CHECK_RET_BUFFER(mousecore_hid_descriptor, cnt, buf);

    cnt = libusb_control_transfer(devs_handle[i], 0x81, 0x06, 0x2200, 0, buf, 1024, 1000);
    LIBUSB_CHECK_RET_BUFFER(mousecore_hid_report_descriptor, cnt, buf);
    hid_report_size = get_hid_record_size(buf, cnt, HID_REPORT_TYPE_INPUT);

    printf("\n");

    LIBUSB_ASSERTCALL(libusb_set_configuration(devs_handle[i], target_report_configuration));
  }
  
  return num_mouse;
}

void MouseCore::Start() {
  for (int i=0; i < num_mouse; i++) {
    pollThread[i] = std::thread([&, this](int idx) { poll(idx); }, i);
  }
}

int MouseCore::ControlTransfer(int mouse_idx, uint8_t request_type, uint8_t bRequest, uint16_t wValue, uint16_t wIndex,
                    unsigned char *data, uint16_t wLength, unsigned int timeout) {
  if (mouse_idx >= num_mouse) return -1; 
  return libusb_control_transfer(devs_handle[mouse_idx], request_type, bRequest, wValue, wIndex, data, wLength > 4096 ? 4096: wLength, timeout);
}


void MouseCore::poll(int idx) {
  LIBUSB_ASSERTCALL(libusb_claim_interface(devs_handle[idx], target_report_interface));
  while (!shouldStop) {
    int ret, cnt;
    unsigned char buf[1024];
    ret = libusb_interrupt_transfer(devs_handle[idx], target_report_endpoint, buf, hid_report_size, NULL, 1000);
    if (ret == LIBUSB_ERROR_TIMEOUT || ret == LIBUSB_ERROR_PIPE) continue;
    if (ret < 0) {
      printf("%d device polling failed: %s\n", idx, libusb_error_name(ret));
      exit(-1);
    } else {
      mouseReportCallbackFunc(idx, raw_to_mousereport(buf, hid_report_size));
    }
  }
  libusb_release_interface(devs_handle[idx], target_report_interface);
}

MouseReport MouseCore::raw_to_mousereport(uint8_t *buffer, size_t size) {
  //our mouse input report format is 
  //bb xx yx yy ww
  //b for button, w for wheel.
  MouseReport result;
  memset(&result, 0, sizeof(MouseReport));
  result.flags = 0;
  result.button = MOUSECORE_EXTRACT_BUFFER(buffer, size, 0, uint8_t);
  uint8_t xxbyte = MOUSECORE_EXTRACT_BUFFER(buffer, size, 1, uint8_t);
  uint8_t xybyte = MOUSECORE_EXTRACT_BUFFER(buffer, size, 2, uint8_t);
  uint8_t yybyte = MOUSECORE_EXTRACT_BUFFER(buffer, size, 3, uint8_t);

  result.xTrans = ((xybyte & 0x0f) << 8) + xxbyte;
  if (result.xTrans & 0x0800) result.xTrans |= 0xf000; 
  
  result.yTrans = ((xybyte & 0xf0) >> 4) + (yybyte << 4);
  if (result.yTrans & 0x0800) result.yTrans |= 0xf000;

  result.wheel = MOUSECORE_EXTRACT_BUFFER(buffer, size, 4, int8_t);
  if (result.wheel & 0x80) result.wheel |= 0xff00;

  // printf("%02hhx %02hhx %02hhx\n", xxbyte, xybyte, yybyte);
  // printf("%04hx %04hx\n", result.xTrans, result.yTrans);
  return result;
}
#endif

// For commercial optical mice
#ifdef USE_MOUSECORE_NATIVE
  #include "mousecore_native.cpp"
#endif 

// For optical mice sensor
#ifdef USE_MOUSECORE_SP
  #include "mousecore_sp.cpp"
#endif 