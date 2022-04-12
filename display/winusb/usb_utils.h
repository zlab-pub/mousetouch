#ifndef DISPLAY_WINUSB_USB_UTILS_H_
#define DISPLAY_WINUSB_USB_UTILS_H_

#include <cstdint>

#define HID_REPORT_TYPE_INPUT 0x01
#define HID_REPORT_TYPE_OUTPUT 0x02
#define HID_REPORT_TYPE_FEATURE 0x03

int get_hid_record_size(uint8_t *hid_report_descriptor, int size, int type);

#ifdef USE_MOUSECORE_NATIVE
#define LIBUSB_ASSERTCALL(A)                                 \
  do                                                       \
  {                                                        \
    int r = (A);                                           \
    if (r < 0)                                             \
    {                                                      \
      printf("%s failed: %d\n", #A, (r)); \
      exit(-1);                                            \
    }                                                      \
  } while (0)

#define LIBUSB_CHECK_RET(NAME, RET) do { \
  if ((RET) < 0) { \
    printf("%s: ", (#NAME)); \
    printf("failed with %d", (RET));\
  }  \
} while (0)

#define LIBUSB_CHECK_RET_BUFFER(NAME, RET, BUFFER) \
  do {                                                                         \
    printf("%s: ", (#NAME));                                                   \
    if ((RET) < 0)                                                            \
    {                                                                       \
      printf("failed with %d\n", (RET)); \
    }                                                                       \
    else                                                                    \
    {                                                                       \
      for (int i = 0; i < (RET); i++)                                         \
      {                                                                     \
        printf("%02x ", (BUFFER)[i]);                                       \
      }                                                                     \
      printf("\n");                                                         \
    }                                                                       \
  }                                                                         \
  while (0)

#define LIBUSB_CHECK_RET_BUFFER_SIZE(NAME, RET, BUFFER, SIZE) do {          \
  printf("%s: ", (#NAME));                                                  \
  if ((RET) < 0) {                                                         \
    printf("failed with %d\n", (RET));\
  } else {                                                               \
    for (int i = 0; i < (SIZE); i++) {                                      \
      printf("%02x ", (BUFFER)[i]);                                      \
    }                                                                    \
    printf("\n");                                                        \
  }                                                                      \
} while (0)

#else

#define LIBUSB_ASSERTCALL(A)                                 \
  do                                                       \
  {                                                        \
    int r = (A);                                           \
    if (r < 0)                                             \
    {                                                      \
      printf("%s failed: %s\n", #A, libusb_error_name(r)); \
      exit(-1);                                            \
    }                                                      \
  } while (0)

#define LIBUSB_CHECK_RET(NAME, RET) do { \
  if ((RET) < 0) { \
    printf("%s: ", (#NAME)); \
    printf("failed with %s", libusb_error_name(RET));\
  }  \
} while (0)

#define LIBUSB_CHECK_RET_BUFFER(NAME, RET, BUFFER) \
  do {                                                                         \
    printf("%s: ", (#NAME));                                                   \
    if ((RET) < 0)                                                            \
    {                                                                       \
      printf("failed with %s\n", libusb_error_name(RET)); \
    }                                                                       \
    else                                                                    \
    {                                                                       \
      for (int i = 0; i < (RET); i++)                                         \
      {                                                                     \
        printf("%02x ", (BUFFER)[i]);                                       \
      }                                                                     \
      printf("\n");                                                         \
    }                                                                       \
  }                                                                         \
  while (0)

#define LIBUSB_CHECK_RET_BUFFER_SIZE(NAME, RET, BUFFER, SIZE) do {          \
  printf("%s: ", (#NAME));                                                  \
  if ((RET) < 0) {                                                         \
    printf("failed with %s\n", libusb_error_name(RET));\
  } else {                                                               \
    for (int i = 0; i < (SIZE); i++) {                                      \
      printf("%02x ", (BUFFER)[i]);                                      \
    }                                                                    \
    printf("\n");                                                        \
  }                                                                      \
} while (0)

#endif



#endif // !DISPLAY_WINUSB_USB_UTILS_H_