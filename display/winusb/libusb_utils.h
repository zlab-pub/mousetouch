#ifndef DISPLAY_WINUSB_LIBUSB_UTILS_H_
#define DISPLAY_WINUSB_LIBUSB_UTILS_H_

#include <libusb.h>
#include <stdio.h>
#include "usb_utils.h"

#if defined(_MSC_VER) && (_MSC_VER < 1900)
#define snprintf _snprintf
#endif

void print_endpoint_comp(const struct libusb_ss_endpoint_companion_descriptor *ep_comp);
void print_endpoint(const struct libusb_endpoint_descriptor *endpoint);
void print_altsetting(const struct libusb_interface_descriptor *interface);
void print_2_0_ext_cap(struct libusb_usb_2_0_extension_descriptor *usb_2_0_ext_cap);
void print_ss_usb_cap(struct libusb_ss_usb_device_capability_descriptor *ss_usb_cap);
void print_bos(libusb_device_handle *handle);
void print_interface(const struct libusb_interface *interface);
void print_configuration(struct libusb_config_descriptor *config);
int print_device(libusb_device *dev, int level);

#endif // !DISPLAY_LIBUSB_UTILS_H_