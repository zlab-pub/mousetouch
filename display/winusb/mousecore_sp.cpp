#ifndef USE_MOUSECORE_SP
# error "DO NOT COMPILE THIS FILE DIRECTLY. Compile mousecore.cpp instead."
#endif

#include "mousecore.h"
#include "../mousefifo.h"
#include "SerialPort.h"
#include <windows.h>
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
    static void printf_guid(GUID guid) {
        printf("{%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX}\n",
            guid.Data1, guid.Data2, guid.Data3,
            guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
            guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
    }
}

using namespace npnx;

#define BAUDRATE 115200

MouseCore::MouseCore() {
    serial_interfaces.clear();
    position_raw_data_init_time.clear();
    mouseReportCallbackFunc = NULL;
}

MouseCore::~MouseCore() {
    for (int i = 0; i < num_mouse; i++)
    {
        serial_interfaces[i]->closeSerial();
    }
    serial_interfaces.clear();
    position_raw_data_init_time.clear();
}

int MouseCore::Init(uint16_t vid, uint16_t pid, MOUSEREPORTCALLBACKFUNC func) {
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

        WCHAR* buff = pInterface_detail->DevicePath;
        if (wcsstr(buff, L"bthenum") != NULL && wcsstr(buff, L"08e7") != NULL) { // filte Bluetooth (bthenum) Serial Port Profile Device (08e7)
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
                std::string tmp_str(m[2]);
                tmp_str = "\\\\.\\" + tmp_str;
                std::cout << "Find " << tmp_str << std::endl;
                serial_names.push_back(tmp_str);
                serial_interfaces.push_back(new SerialPort(tmp_str.data()));
                position_raw_data_fifos.push_back(new PositionRawDataFifo);
                position_raw_data_init_time.push_back(0);
                delete[] friendlyName;
            }
            pInterface_details.push_back(pInterface_detail);
            mouse_cnt++;
        }
        else {
            free(pInterface_detail);
        }
        dev_count++;
        }
    NPNX_ASSERT(GetLastError() == ERROR_NO_MORE_ITEMS);
    SetupDiDestroyDeviceInfoList(h_info);
    printf("Find %d Devices\n", mouse_cnt);
    num_mouse = mouse_cnt;
    return num_mouse;

    //char* port = "\\\\.\\COM3";
    //SerialPort* sp;
    //sp = new SerialPort(port);
    ////my_serial.setTimeout(serial::Timeout::max(), 250, 0, 250, 0);
    //serial_names.push_back(port);
    //serial_interfaces.push_back(sp);
    //position_raw_data_fifos.push_back(new PositionRawDataFifo);
    //position_raw_data_init_time.push_back(0);
    //num_mouse = 1;
    //return num_mouse;
}

void MouseCore::Start() {
    for (int i = 0; i < num_mouse; i++) {
        pollThread[i] = std::thread([&, this](int idx) { poll(idx); }, i);
    }
}

void MouseCore::Stop() {
    shouldStop = true;
}

void PrintStringHex(std::string str) {
    const char* buf = str.data();
    for (int i = 0; i < str.length(); i++) {
        printf("%.2X ", buf[i] & 0xff);
    }
    printf("\n");
}

void PrintHex(const unsigned char* data, int size) {
    for (int i = 0; i < size; i++) {
        printf("%.2X ", data[i] & 0xff);
    }
    printf("\n");
}

void PrintHex(const char* data, int size) {
    for (int i = 0; i < size; i++) {
        printf("%.2X ", data[i] & 0xff);
    }
    printf("\n");
}

void PrintFrameHex(const unsigned char* data, int size) {
    for (int i = 0; i < size; i++) {
        printf("%.2X ", data[i] & 0xff);
        if (i % 30 == 29) printf("\n");
    }
    printf("\n");
}

// return the data size
int MouseCore::GetPositionRawData(int idx, unsigned char* buf) {
    SensorFifoReport tmp_report;
    int total_report_count = position_raw_data_fifos[idx]->size();
    if (total_report_count > 60) {
        position_raw_data_fifos[idx]->Pop(&tmp_report);
        for (int i = 0; i < SP_REPORT_SIZE; i++) {
            buf[i] = tmp_report.data[i];
        }
        *(int64_t*)(buf + SP_REPORT_SIZE) = position_raw_data_init_time[idx];
        int64_t nowtime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        position_raw_data_init_time[idx] += (nowtime - position_raw_data_init_time[idx]) / total_report_count;
        return SP_REPORT_SIZE + 8;
    }
    else {
        return 0;
    }
}

void MouseCore::StopOrStartPositionDataReport(int idx) {
    while (!serial_interfaces[idx]->isConnected()) {
        Sleep(300);
        serial_interfaces[idx] = new SerialPort(serial_names[idx].data());
    }
    char w_buf[1] = { 0xfe };
    serial_interfaces[idx]->writeSerialPort(w_buf, 1);
}

int MouseCore::GetAngleRawData(int idx, unsigned char* buf) {
    //printf("GetAngleRawData");
    StopOrStartPositionDataReport(idx);
    Sleep(50);
    state = RECEIVING_ANGLE_DATA;
    char preamble[] = { 0xff, 0x00, 0xff, 0x00, 0xff, 0xff };
    while (!serial_interfaces[idx]->isConnected()) {
        Sleep(300);
        serial_interfaces[idx] = new SerialPort(serial_names[idx].data());
    }

    char w_buf[1] = { 0xff };
    char tmp_buf[4096];
    int buf_size = 2048;
    const int required_data_size = 30 * 30;
    int received_frame_data_size = 0;
    int buf_data_size = 0;
    bool half_frame = false;

    int cnt = 0;
    int64_t last_zero_data_cnt_time = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    serial_interfaces[idx]->writeSerialPort(w_buf, 1); // Start angle data report
    while (received_frame_data_size < required_data_size) {
        if (buf_data_size <= buf_size / 2) {
            cnt = serial_interfaces[idx]->readSerialPort(tmp_buf + buf_data_size, buf_size / 2);
        }
        else {
            cnt = serial_interfaces[idx]->readSerialPort(tmp_buf + buf_data_size, buf_size - buf_data_size);
        }
        if (cnt == 0 && serial_interfaces[idx]->isConnected()) {
            int64_t now_time = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
            if (now_time - last_zero_data_cnt_time > 1000000) {
                serial_interfaces[idx]->writeSerialPort(w_buf, 1);
                last_zero_data_cnt_time = now_time;
            }
        }
        else {
            //printf("Receive %d bytes\n", cnt);
            last_zero_data_cnt_time = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        }
        buf_data_size += cnt;

        int consumed_data_size = 0;
        if ((buf_data_size - consumed_data_size) >= 906) {
            int start_index = consumed_data_size;
            if (!memcmp(preamble, tmp_buf + start_index, sizeof(preamble))) {
                //printf("Detect Angle Data\n");
                consumed_data_size += sizeof(preamble);
                int remained_frame_data_size = required_data_size - (buf_data_size - consumed_data_size);
                int remained_buf_size = buf_size - buf_data_size;
                while (remained_frame_data_size > 0 &&
                    remained_buf_size >= remained_frame_data_size) {
                    remained_frame_data_size -= serial_interfaces[idx]->readSerialPort(tmp_buf + buf_data_size, remained_frame_data_size);
                }
                memcpy(buf, tmp_buf + consumed_data_size, required_data_size);
                //PrintFrameHex(buf, 900);
                state = RECEIVING_POSITION_DATA;
                StopOrStartPositionDataReport(idx);
                return required_data_size;

            }
            else {
                consumed_data_size++;
            }
        }
        if (consumed_data_size > 0) {
            buf_data_size -= consumed_data_size;
            std::memcpy(buf, buf + consumed_data_size, buf_data_size);
            memset(buf + buf_data_size, 0, buf_size - buf_data_size);
        }
    }
    return 0;
}

void MouseCore::poll(int idx) {
    char pos_pre[] = { 0xfe };
    char pos_suf[] = { 0x00, 0xfe };
    char preamble[] = { 0xfe, 0x00, 0xfe };
    char buf[1024];
    int data_size = 0;
    const int buf_size = 1024; // should be even
    std::cout << "Token " << idx << " Start.\n";
    int cnt = 0;
    int64_t last_zero_data_cnt_time = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    while (!shouldStop) {
        if (state != RECEIVING_POSITION_DATA) {
            Sleep(10);
            continue;
        }

        if (!serial_interfaces[idx]->isConnected()) {
            Sleep(300);
            serial_interfaces[idx] = new SerialPort(serial_names[idx].data());
        }

        if (data_size <= buf_size / 2) {
            cnt = serial_interfaces[idx]->readSerialPort(buf + data_size, buf_size / 2);
        }
        else {
            cnt = serial_interfaces[idx]->readSerialPort(buf + data_size, buf_size - data_size);
        }
        if (cnt == 0 && serial_interfaces[idx]->isConnected()) {
            int64_t now_time = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
            if (now_time - last_zero_data_cnt_time > 1000000) {
                //std::cout << "Send 0xfe\n";
                serial_interfaces[idx]->writeSerialPort(pos_pre, 1);
                last_zero_data_cnt_time = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
            }
        }
        else {
            last_zero_data_cnt_time = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        }
        data_size += cnt;

        int consumed_data_size = 0;
        while ( (data_size - consumed_data_size) > (SP_REPORT_SIZE + sizeof(preamble)) ) {
            int start_index = consumed_data_size;
            char* tmp_buf = buf + start_index;
            if (!memcmp(preamble, tmp_buf, sizeof(preamble))) {
                //PrintHex(tmp_buf, SP_REPORT_SIZE + sizeof(preamble));
                consumed_data_size += 3;
                if (position_raw_data_fifos[idx]->empty()) {
                    position_raw_data_init_time[idx] = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
                }
                SensorFifoReport report(tmp_buf + sizeof(preamble));
                //report.PrintHex();
                position_raw_data_fifos[idx]->Push(report);
                consumed_data_size += SP_REPORT_SIZE;
            }
            else {
                consumed_data_size++;
            }
        }

        if (consumed_data_size > 0) {
            data_size -= consumed_data_size;
            std::memcpy(buf, buf + consumed_data_size, data_size);
            memset(buf+data_size, 0, buf_size-data_size);
        }
    }
    std::cout << "MouseCore " << idx << " poll thread Stopped.\n";
}