// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * HID driver for Gigabyte Keyboards
 * Copyright (c) 2020 Hemanth Bollamreddi
*/

#include <linux/hid.h>
#include <linux/module.h>
#include "gigabytekbd_driver.h"

MODULE_AUTHOR("Hemanth Bollamreddi <blmhemu@gmail.com>");
MODULE_DESCRIPTION("HID Keyboard driver for Gigabyte Keyboards.");
MODULE_LICENSE("GPL v2");

//TODO: If put in mainstream kernel, modify this file to include the VID and PID.
//#include "hid-ids.h"

#define HIDRAW_FN_ESC 0x04000084
#define HIDRAW_FN_F2 0x0400007C
#define HIDRAW_FN_F3 0x0400007D
#define HIDRAW_FN_F4 0x0400007E
#define HIDRAW_FN_F6 0x04000080
#define HIDRAW_FN_F10 0x04000081
#define HIDRAW_FN_F11 0x04000082
#define HIDRAW_FN_F12 0x04000083

#define make_u32(a, b, c, d) a << 24 | b << 16 | c << 8 | d

static int gigabyte_kbd_raw_event(struct hid_device *hdev, struct hid_report *report, u8 *rd, int size)
{
	if (report->id == 4 && size == 4)
	{
		u32 hidraw = make_u32(rd[0], rd[1], rd[2], rd[3]);
		// printk("Gigabyte kbd raw event. hidraw code : %x", hidraw);
		switch (hidraw)
		{
		case HIDRAW_FN_F3:
			rd[0] = 0x03;rd[1] = 0x70;rd[2] = 0x00;
			hid_report_raw_event(hdev, HID_INPUT_REPORT, rd, 4, 0);
			rd[0] = 0x03;rd[1] = 0x00;rd[2] = 0x00;
			return 1;
		case HIDRAW_FN_F4:
			rd[0] = 0x03;rd[1] = 0x6f;rd[2] = 0x00;
			hid_report_raw_event(hdev, HID_INPUT_REPORT, rd, 4, 0);
			rd[0] = 0x03;rd[1] = 0x00;rd[2] = 0x00;
			return 1;
		default:
			return 0;
			break;
		}
	}
	return 0;
}

static int gigabyte_kbd_probe(struct hid_device *hdev, const struct hid_device_id *id)
{
	printk("Gigabyte kbd driver loaded.");
	int ret;
	hdev->quirks |= HID_QUIRK_INPUT_PER_APP;

	ret = hid_parse(hdev);
	if (ret)
		return ret;

	return hid_hw_start(hdev, HID_CONNECT_DEFAULT);
}

static const struct hid_device_id gigabyte_kbd_devices[] = {
	{HID_USB_DEVICE(USB_VENDOR_ID_GIGABYTE_AERO15XV8, USB_DEVICE_ID_GIGABYTE_AERO15XV8)},
	{HID_USB_DEVICE(USB_VENDOR_ID_GIGABYTE_AERO15SA, USB_DEVICE_ID_GIGABYTE_AERO15SA)},
	{HID_USB_DEVICE(USB_VENDOR_ID_GIGABYTE_AORUS15P, USB_DEVICE_ID_GIGABYTE_AORUS15P)},
	{HID_USB_DEVICE(USB_VENDOR_ID_GIGABYTE_AORUS15G, USB_DEVICE_ID_GIGABYTE_AORUS15G)},
	{HID_USB_DEVICE(USB_VENDOR_ID_GIGABYTE_AORUS16X, USB_DEVICE_ID_GIGABYTE_AORUS16X)},
	{}
};
MODULE_DEVICE_TABLE(hid, gigabyte_kbd_devices);

static struct hid_driver gigabyte_kbd_driver = {
	.name = "gigabytekbd",
	.id_table = gigabyte_kbd_devices,
	.probe = gigabyte_kbd_probe,
	.raw_event = gigabyte_kbd_raw_event};
module_hid_driver(gigabyte_kbd_driver);
