#ifndef USE_MOUSECORE_NATIVE
# error "DO NOT COMPILE THIS FILE DIRECTLY. Compile mousecore.cpp instead."
#endif

#include "mousecore.h"

#include <windows.h>
#include <winusb.h>
#include <SetupAPI.h>
#include <Cfgmgr32.h>
#include <Devpkey.h>
#include <iostream>
#include <cstdio>
#include <cstdint>
#include <functional>
#include <chrono>
#include <thread>
#include <string>

namespace npnx {
static void printf_guid(GUID guid)
{
  printf("{%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX}\n",
         guid.Data1, guid.Data2, guid.Data3,
         guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
         guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
}
}

using namespace npnx;

MouseCore::MouseCore() 
{
  pInterface_details.clear();
  h_files.clear();
  h_winusbs.clear();
  h_hidinterfaces.clear();
  mouseReportCallbackFunc = NULL;
}

MouseCore::~MouseCore() {
  for (int i = 0; i < num_mouse; i++) 
  {
    free(pInterface_details[i]);
    if (h_winusbs[i] != h_hidinterfaces[i] && h_hidinterfaces[i] != NULL) {
      WinUsb_Free(h_hidinterfaces[i]);
    }
    WinUsb_Free(h_winusbs[i]);
    CloseHandle(h_files[i]);
  }
  pInterface_details.clear();
  h_files.clear();
  h_winusbs.clear();
  h_hidinterfaces.clear();
}

int MouseCore::Init(uint16_t vid, uint16_t pid, MOUSEREPORTCALLBACKFUNC func)
{
  mouseReportCallbackFunc = func;
  GUID winUSBSetupGuid = {0x88BAE032, 0x5A81, 0x49f0, 0xBC, 0x3D, 0xA4, 0xFF, 0x13, 0x82, 0x16, 0xD6};
  GUID winUSBInterfaceGuid = {0xA5DCBF10, 0x6530, 0x11D2, 0x90, 0x1F, 0x00, 0xC0, 0x4F, 0xB9, 0x51, 0xED};
  // GUID winUSBInterfaceGuid = {0x0A8FE9DC, 0x086F, 0x4BE7, 0xA6, 0x31, 0x4E, 0x44, 0x15, 0x86, 0x42, 0x2D};
  // printf_guid(winUSBSetupGuid);

  HDEVINFO h_info = SetupDiGetClassDevs(&winUSBInterfaceGuid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
  NPNX_ASSERT(h_info && h_info != INVALID_HANDLE_VALUE);

  int mouse_cnt = 0;
  SP_DEVINFO_DATA devinfo_data;
  SP_DEVICE_INTERFACE_DATA interface_data;
  interface_data.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
  devinfo_data.cbSize = sizeof(SP_DEVINFO_DATA);
  int dev_count = 0;
  while (SetupDiEnumDeviceInterfaces(h_info, NULL, &winUSBInterfaceGuid, dev_count, &interface_data)) {
    DWORD length;
    bool suc = SetupDiGetDeviceInterfaceDetail(h_info, &interface_data, NULL, 0, &length, NULL);
    NPNX_ASSERT(!suc && GetLastError() == ERROR_INSUFFICIENT_BUFFER);
    

    PSP_DEVICE_INTERFACE_DETAIL_DATA pInterface_detail = (PSP_DEVICE_INTERFACE_DETAIL_DATA) malloc(length);
    pInterface_detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
    NPNX_ASSERT_LOG(SetupDiGetDeviceInterfaceDetail(h_info, &interface_data, pInterface_detail, length, NULL, &devinfo_data), GetLastError());

    printf("%ls\n", pInterface_detail->DevicePath);
    
    // suc = SetupDiGetDevicePropertyKeys(h_info, &devinfo_data, NULL, 0, &length, 0);
    // NPNX_ASSERT(!suc && GetLastError() == ERROR_INSUFFICIENT_BUFFER);
    
    // DEVPROPKEY * keys = (DEVPROPKEY *) malloc(length * sizeof(DEVPROPKEY));

    // NPNX_ASSERT_LOG(SetupDiGetDevicePropertyKeys(h_info, &devinfo_data, keys, length * sizeof(DEVPROPKEY), NULL, 0), GetLastError()); 
    // for(int i=0; i<length; i++) {
    //   printf_guid(keys[i].fmtid);
    //   printf("%d\n", keys[i].pid);
    // }

    DEVPROPTYPE valuetype;
    suc = SetupDiGetDeviceProperty(h_info, &devinfo_data, &DEVPKEY_Device_HardwareIds, &valuetype, NULL, 0, &length, 0);
    NPNX_ASSERT(!suc && GetLastError() == ERROR_INSUFFICIENT_BUFFER);
    
    WCHAR *property_buffer = (WCHAR *) malloc(length);
    
    NPNX_ASSERT_LOG(SetupDiGetDeviceProperty(h_info, &devinfo_data, &DEVPKEY_Device_HardwareIds, &valuetype, (PBYTE)property_buffer, length, NULL, 0), GetLastError());
    // for(int i=0; i<length; i++) {
    //   printf(property_buffer != 0 ? "%lc" : "\\0", property_buffer[i]);
    // }

    // if (!memcmp(property_buffer, TEXT("USB\\VID_046"), 11 * sizeof(WCHAR)) && !memcmp(&devinfo_data.ClassGuid, &winUSBSetupGuid, sizeof(GUID))){
    if (!memcmp(&devinfo_data.ClassGuid, &winUSBSetupGuid, sizeof(GUID))){
      pInterface_details.push_back(pInterface_detail);
      mouse_cnt++;
    } else {
      free(pInterface_detail);
    }
    free(property_buffer);
    dev_count++;
  }
  NPNX_ASSERT(GetLastError() == ERROR_NO_MORE_ITEMS);
  SetupDiDestroyDeviceInfoList(h_info);
  
  printf("\n");
  for (int i = 0; i < mouse_cnt; i++) {
    printf("found mouse: %ls\n", GetDevicePath(i));
    HANDLE h_file = CreateFile(GetDevicePath(i),
                                GENERIC_WRITE | GENERIC_READ,
                                FILE_SHARE_WRITE | FILE_SHARE_READ,
                                NULL,
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                                NULL);
    if (h_file == INVALID_HANDLE_VALUE) {
      NPNX_ASSERT_LOG(!"h_file_invalid", GetLastError());
    }
    h_files.push_back(h_file);
    HANDLE h_winusb;
    NPNX_ASSERT_LOG(WinUsb_Initialize(h_file, &h_winusb), GetLastError());
    h_winusbs.push_back(h_winusb);

    //find the report length.
    WINUSB_SETUP_PACKET controltransfer = {
      0x81, 0x06, 0x2200, 0, 4096
    };
    UCHAR buf[4096];
    DWORD length;
    NPNX_ASSERT_LOG( 
      WinUsb_ControlTransfer(h_winusb, controltransfer, buf, 4096, &length, NULL),
      GetLastError()
    );
    for(int i = 0; i < (int)length; i++){
      printf("%02hhx ", buf[i]);
    }
    printf("\n");
    hid_report_size = get_hid_record_size(buf, length, HID_REPORT_TYPE_INPUT);
    printf("report size : %d\n", hid_report_size);


    if (target_report_interface != 0) {
      HANDLE h_hidinterface;
      NPNX_ASSERT_LOG(WinUsb_GetAssociatedInterface(h_winusb, target_report_interface - 1, &h_hidinterface),
        GetLastError()
      );
      h_hidinterfaces.push_back(h_hidinterface);
    } else {
      h_hidinterfaces.push_back(h_winusb);
    }
  }
  num_mouse = mouse_cnt;
  return num_mouse;
}

void MouseCore::Start() {
  for (int i=0; i < num_mouse; i++) {
    pollThread[i] = std::thread([&, this](int idx) { poll(idx); }, i);
  }
}

int MouseCore::ControlTransfer(int mouse_idx, uint8_t request_type, uint8_t bRequest, uint16_t wValue, uint16_t wIndex,
                    unsigned char *data, uint16_t wLength, unsigned int timeout) {
  uint16_t realwLength = 4096 < wLength? 4096 : wLength;
  NPNX_ASSERT(mouse_idx < num_mouse);
  WINUSB_SETUP_PACKET controlSetup = {
    request_type,
    bRequest,
    wValue,
    wIndex,
    realwLength
  };
  DWORD length;

#ifdef USE_MOUSECORE_NATIVE_ASYNC
  OVERLAPPED overlapped;
  memset(&overlapped, 0, sizeof(OVERLAPPED));
  HANDLE hEvent = CreateEvent(NULL, false, false, NULL);
  NPNX_ASSERT_LOG(hEvent != NULL, GetLastError());
  overlapped.hEvent = hEvent;
  NPNX_ASSERT_LOG( 
    WinUsb_ControlTransfer(h_winusbs[mouse_idx], controlSetup, data, realwLength, &length, &overlapped)
    || GetLastError() == ERROR_IO_PENDING,
    GetLastError()
  );
  GetOverlappedResult(h_winusbs[mouse_idx], &overlapped, &length, true);
  CloseHandle(overlapped.hEvent);
#else
  NPNX_ASSERT_LOG( 
    WinUsb_ControlTransfer(h_winusbs[mouse_idx], controlSetup, data, realwLength, &length, NULL),
    GetLastError()
  );
#endif

  return length;
}

void MouseCore::poll(int idx) {
  while (!shouldStop) {
    UCHAR buf[200];
    DWORD length;
#ifdef USE_MOUSECORE_NATIVE_ASYNC
  OVERLAPPED overlapped;
  memset(&overlapped, 0, sizeof(OVERLAPPED));
  HANDLE hEvent = CreateEvent(NULL, false, false, NULL);
  NPNX_ASSERT_LOG(hEvent != NULL, GetLastError());
  overlapped.hEvent = hEvent;
  NPNX_ASSERT_LOG( WinUsb_ReadPipe(h_hidinterfaces[idx], target_report_endpoint, buf, hid_report_size, &length, &overlapped)
    || GetLastError() == ERROR_IO_PENDING,
    GetLastError()
  );
  GetOverlappedResult(h_hidinterfaces[idx], &overlapped, &length, true);
  CloseHandle(overlapped.hEvent);

#else
    NPNX_ASSERT_LOG( WinUsb_ReadPipe(h_hidinterfaces[idx], target_report_endpoint, buf, hid_report_size, &length, NULL),
      GetLastError()
    );
#endif
    mouseReportCallbackFunc(idx, raw_to_mousereport(buf, hid_report_size));
  }
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



