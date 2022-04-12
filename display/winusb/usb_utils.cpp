#include "usb_utils.h"

int get_hid_record_size(uint8_t *hid_report_descriptor, int size, int type)
{
  uint8_t i, j = 0;
  uint8_t offset;
  int record_size[3] = {0, 0, 0};
  int nb_bits = 0, nb_items = 0;
  bool found_record_marker;

  found_record_marker = false;
  for (i = hid_report_descriptor[0] + 1; i < size; i += offset)
  {
    offset = (hid_report_descriptor[i] & 0x03) + 1;
    if (offset == 4)
      offset = 5;
    switch (hid_report_descriptor[i] & 0xFC)
    {
    case 0x74: // bitsize
      nb_bits = hid_report_descriptor[i + 1];
      break;
    case 0x94: // count
      nb_items = 0;
      for (j = 1; j < offset; j++)
      {
        nb_items = ((uint32_t)hid_report_descriptor[i + j]) << (8 * (j - 1));
      }
      break;
    case 0x80: // input
      found_record_marker = true;
      j = 0;
      break;
    case 0x90: // output
      found_record_marker = true;
      j = 1;
      break;
    case 0xb0: // feature
      found_record_marker = true;
      j = 2;
      break;
    case 0xC0: // end of collection
      nb_items = 0;
      nb_bits = 0;
      break;
    default:
      continue;
    }
    if (found_record_marker)
    {
      found_record_marker = false;
      record_size[j] += nb_items * nb_bits;
    }
  }
  if ((type < HID_REPORT_TYPE_INPUT) || (type > HID_REPORT_TYPE_FEATURE))
  {
    return 0;
  }
  else
  {
    return (record_size[type - HID_REPORT_TYPE_INPUT] + 7) / 8;
  }
}