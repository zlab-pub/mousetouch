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
#include <wchar.h>
#include <regex>

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

 }
 pInterface_details.clear();

 h_files.clear();
 h_winusbs.clear();
 h_hidinterfaces.clear();

}

int MouseCore::Init(uint16_t vid, uint16_t pid, MOUSEREPORTCALLBACKFUNC func)
{
   mouseReportCallbackFunc = func;

   GUID winDeviceInterfaceGuid = { 0x86E0D1E0, 0x8089, 0x11D0, 0x9C, 0xE4, 0x08, 0x00, 0x3E, 0x30, 0x1F, 0x73 };

   HDEVINFO h_info = SetupDiGetClassDevs(&winDeviceInterfaceGuid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
   NPNX_ASSERT(h_info && h_info != INVALID_HANDLE_VALUE);

   int mouse_cnt = 0;
   SP_DEVINFO_DATA devinfo_data;
   SP_DEVICE_INTERFACE_DATA interface_data;
   interface_data.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
   devinfo_data.cbSize = sizeof(SP_DEVINFO_DATA);
   int dev_count = 0;
   while (SetupDiEnumDeviceInterfaces(h_info, NULL, &winDeviceInterfaceGuid, dev_count, &interface_data)) {
       DWORD length;
       bool suc = SetupDiGetDeviceInterfaceDetail(h_info, &interface_data, NULL, 0, &length, NULL);
       NPNX_ASSERT(!suc && GetLastError() == ERROR_INSUFFICIENT_BUFFER);


       PSP_DEVICE_INTERFACE_DETAIL_DATA pInterface_detail = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(length);
       pInterface_detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
       NPNX_ASSERT_LOG(SetupDiGetDeviceInterfaceDetail(h_info, &interface_data, pInterface_detail, length, NULL, &devinfo_data), GetLastError());
       printf("%ls\n", pInterface_detail->DevicePath);

       DEVPROPTYPE valuetype;
       suc = SetupDiGetDeviceProperty(h_info, &devinfo_data, &DEVPKEY_Device_HardwareIds, &valuetype, NULL, 0, &length, 0);
       NPNX_ASSERT(!suc && GetLastError() == ERROR_INSUFFICIENT_BUFFER);

       WCHAR* property_buffer = (WCHAR*)malloc(length);

       NPNX_ASSERT_LOG(SetupDiGetDeviceProperty(h_info, &devinfo_data, &DEVPKEY_Device_HardwareIds, &valuetype, (PBYTE)property_buffer, length, NULL, 0), GetLastError());

       WCHAR* buff = pInterface_detail->DevicePath;
       if (wcsstr(buff, L"bthenum") != NULL && wcsstr(buff, L"08e7") != NULL) { // filte Bluetooth (bthenum) Serial Port Profile Device (09e7)
           BYTE PropertyVal[300];
           if (SetupDiGetDeviceRegistryProperty(h_info, &devinfo_data, SPDRP_FRIENDLYNAME, NULL, PropertyVal, sizeof(PropertyVal), NULL))
           {
               int cch = WideCharToMultiByte(CP_ACP, 0, (LPCTSTR)PropertyVal, -1, 0, 0, NULL, NULL);
               char* friendlyName = new char[cch];
               WideCharToMultiByte(CP_ACP, 0, (LPCTSTR)PropertyVal, -1, friendlyName, cch, NULL, NULL);
               std::cout << friendlyName << std::endl;
               std::regex com_reg("(.*)(COM[0-9]+)(.*)");
               std::cmatch m;
               auto ret = std::regex_match(friendlyName, m, com_reg);

               delete[] friendlyName;
           }
           pInterface_details.push_back(pInterface_detail);
           mouse_cnt++;
       }
       else {
           free(pInterface_detail);
       }
       free(property_buffer);
       dev_count++;
   }
   NPNX_ASSERT(GetLastError() == ERROR_NO_MORE_ITEMS);
   SetupDiDestroyDeviceInfoList(h_info);
   printf("Find %d Devices\n", mouse_cnt);
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