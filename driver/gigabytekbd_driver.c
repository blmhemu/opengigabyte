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
#define HIDRAW_FN_F12_AERO17XE5 0x04000088

#define HIDRAW_FN_SPACE_KDB_LIGHT_OFF 0x04010000
#define HIDRAW_FN_SPACE_KDB_LIGHT_HALF 0x04011900
#define HIDRAW_FN_SPACE_KDB_LIGHT_FULL 0x04013200

#define make_u32(a, b, c, d) a << 24 | b << 16 | c << 8 | d

#define MOD_CODE_LEFT_CTRL 0x01
#define MOD_CODE_LEFT_SUPER 0x08

#define KEY_CODE_XF86_TOUCHPAD_OFF 0x72
#define KEY_CODE_MYSTERIOUS 0x73

struct gigabyte_kbd_aero17xe5_drvdata {
	u8 fn_f10_seq;
	bool fn_f10_ctrl_down;
};

static void gigabyte_kbd_util_remove_first_byte(u8 *rd, int size)
{
	for (int i = 1; i < size; i++) {
		rd[i - 1] = rd[i];
	}
	rd[size - 1] = 0x0;
}

static void gigabyte_kbd_util_remove_byte_occurrences(u8 *rd, u8 byte, int size)
{
	for (int i = 0; i < size; i++) {
		while (rd[i] == byte) {
			for (int j = i + 1; j < size; j++) {
				rd[j - 1] = rd[j];
			}
			rd[size - 1] = 0x00;
		}
	}
}

static int gigabyte_kbd_raw_event_aero17xe5(struct hid_device *hdev, struct hid_report *report, u8 *rd, int size)
{
	if (report->id == 0 && report->type == HID_INPUT_REPORT && size == 36) {
		struct gigabyte_kbd_aero17xe5_drvdata *data = hid_get_drvdata(hdev);
		if (!(rd[0] & (MOD_CODE_LEFT_CTRL | MOD_CODE_LEFT_SUPER)) && rd[2] != KEY_CODE_XF86_TOUCHPAD_OFF) {
			data->fn_f10_seq = 0;
			data->fn_f10_ctrl_down = false;
		}
		// Pressing Fn.
		else if (data->fn_f10_seq == 0 && rd[2] == KEY_CODE_XF86_TOUCHPAD_OFF) {
			gigabyte_kbd_util_remove_first_byte(&rd[2], 34);
			data->fn_f10_seq = 1;
		}
		// Holding Fn.
		else if (data->fn_f10_seq == 1 && !(rd[0] & MOD_CODE_LEFT_SUPER) && rd[2] == KEY_CODE_XF86_TOUCHPAD_OFF) {
			gigabyte_kbd_util_remove_first_byte(&rd[2], 34);
		}
		// Holding Fn and pressing F10 (first event) or pressing Super.
		else if (data->fn_f10_seq == 1 && rd[0] & MOD_CODE_LEFT_SUPER && rd[2] == KEY_CODE_XF86_TOUCHPAD_OFF) {
			rd[0] &= ~MOD_CODE_LEFT_SUPER;
			gigabyte_kbd_util_remove_first_byte(&rd[2], 34);
			data->fn_f10_seq = 2;
		}
		// Holding Fn and releasing Super.
		else if (data->fn_f10_seq == 2 && !(rd[0] & MOD_CODE_LEFT_SUPER) && rd[2] == KEY_CODE_XF86_TOUCHPAD_OFF) {
			gigabyte_kbd_util_remove_first_byte(&rd[2], 34);
			data->fn_f10_seq = 1;
		}
		// Holding Fn+Super.
		else if (data->fn_f10_seq == 2 && rd[0] & MOD_CODE_LEFT_SUPER && rd[2] == KEY_CODE_XF86_TOUCHPAD_OFF && !memchr(&rd[3], KEY_CODE_MYSTERIOUS, 33)) {
			rd[0] &= ~MOD_CODE_LEFT_SUPER;
			if (data->fn_f10_ctrl_down) {
				rd[0] &= ~MOD_CODE_LEFT_CTRL;
			}
			gigabyte_kbd_util_remove_first_byte(&rd[2], 34);
		}
		// Holding Fn and pressing F10 (second event).
		else if (data->fn_f10_seq == 2 && rd[0] & (MOD_CODE_LEFT_SUPER | MOD_CODE_LEFT_CTRL) && rd[2] == KEY_CODE_XF86_TOUCHPAD_OFF && memchr(&rd[3], KEY_CODE_MYSTERIOUS, 33)) {
			rd[0] &= ~MOD_CODE_LEFT_SUPER;
			rd[0] &= ~MOD_CODE_LEFT_CTRL;
			gigabyte_kbd_util_remove_first_byte(&rd[2], 34);
			gigabyte_kbd_util_remove_byte_occurrences(&rd[2], KEY_CODE_MYSTERIOUS, 34);
			data->fn_f10_seq = 3;
			data->fn_f10_ctrl_down = true;
		}
		// Holding Fn+F10.
		else if (data->fn_f10_seq == 3 && rd[0] & (MOD_CODE_LEFT_SUPER | MOD_CODE_LEFT_CTRL) && (rd[2] == KEY_CODE_XF86_TOUCHPAD_OFF && memchr(&rd[3], KEY_CODE_MYSTERIOUS, 33))) {
			rd[0] &= ~MOD_CODE_LEFT_SUPER;
			rd[0] &= ~MOD_CODE_LEFT_CTRL;
			gigabyte_kbd_util_remove_first_byte(&rd[2], 34);
			gigabyte_kbd_util_remove_byte_occurrences(&rd[2], KEY_CODE_MYSTERIOUS, 34);
		}
		// Releasing F10 or Fn.
		else if (data->fn_f10_seq == 3 && rd[0] & (MOD_CODE_LEFT_SUPER | MOD_CODE_LEFT_CTRL) && ((rd[2] == KEY_CODE_XF86_TOUCHPAD_OFF && !memchr(&rd[3], KEY_CODE_MYSTERIOUS, 33)) || memchr(&rd[2], KEY_CODE_MYSTERIOUS, 34))) {
			rd[0] &= ~MOD_CODE_LEFT_SUPER;
			rd[0] &= ~MOD_CODE_LEFT_CTRL;
			if (rd[2] == KEY_CODE_XF86_TOUCHPAD_OFF) {
				gigabyte_kbd_util_remove_first_byte(&rd[2], 34);
			}
			else {
				gigabyte_kbd_util_remove_byte_occurrences(&rd[2], KEY_CODE_MYSTERIOUS, 34);
			}
			data->fn_f10_seq = 4;
		}
		// Holding Fn and pressing F10 (again).
		else if (data->fn_f10_seq == 4 && rd[0] & (MOD_CODE_LEFT_SUPER | MOD_CODE_LEFT_CTRL) && rd[2] == KEY_CODE_XF86_TOUCHPAD_OFF) {
			rd[0] &= ~MOD_CODE_LEFT_SUPER;
			rd[0] &= ~MOD_CODE_LEFT_CTRL;
			gigabyte_kbd_util_remove_first_byte(&rd[2], 34);
			data->fn_f10_seq = 2;
		}
		// Releasing Fn.
		else if (data->fn_f10_seq == 4 && rd[0] & (MOD_CODE_LEFT_SUPER | MOD_CODE_LEFT_CTRL) && rd[2] != KEY_CODE_XF86_TOUCHPAD_OFF && !memchr(&rd[2], KEY_CODE_MYSTERIOUS, 34)) {
			rd[0] &= ~MOD_CODE_LEFT_SUPER;
			rd[0] &= ~MOD_CODE_LEFT_CTRL;
			data->fn_f10_seq = 5;
		}
		// This will remove Xf86TouchpadOff then Fn has been pressed after another button.
		else {
			gigabyte_kbd_util_remove_byte_occurrences(&rd[3], KEY_CODE_XF86_TOUCHPAD_OFF, 33);
		}
	}

	return 0;
}

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
	if (hdev->product == USB_DEVICE_ID_GIGABYTE_AERO17XE5) {
		return gigabyte_kbd_raw_event_aero17xe5(hdev, report, rd, size);
	}
	return 0;
}

static int gigabyte_kbd_probe_aero17xe5(struct hid_device *hdev, const struct hid_device_id *id)
{
	int ret = 0;
	struct gigabyte_kbd_aero17xe5_drvdata *data;

	data = devm_kzalloc(&hdev->dev, sizeof(struct gigabyte_kbd_aero17xe5_drvdata), GFP_KERNEL);
	if (data == NULL) {
		hid_err(hdev, "Could not allocate memory for driver data\n");
		ret = -ENOMEM;
		return ret;
	}
	data->fn_f10_seq = 0;
	data->fn_f10_ctrl_down = false;
	hid_set_drvdata(hdev, data);

	return ret;
}

static int gigabyte_kbd_probe(struct hid_device *hdev, const struct hid_device_id *id)
{
	// printk("Gigabyte kbd driver loaded.");
	int ret;
	hdev->quirks |= HID_QUIRK_INPUT_PER_APP;

	ret = hid_parse(hdev);
	if (ret) {
		hid_err(hdev, "hid_parse failed\n");
		return ret;
	}

	ret = hid_hw_start(hdev, HID_CONNECT_DEFAULT);
	if (ret) {
		hid_err(hdev, "hid_hw_start failed\n");
		return ret;
	}

	if (hdev->product == USB_DEVICE_ID_GIGABYTE_AERO17XE5) {
		ret = gigabyte_kbd_probe_aero17xe5(hdev, id);
		if (ret)
			return ret;
	}

	return ret;
}

static const struct hid_device_id gigabyte_kbd_devices[] = {
	{HID_USB_DEVICE(USB_VENDOR_ID_GIGABYTE_AERO15XV8, USB_DEVICE_ID_GIGABYTE_AERO15XV8)},
	{HID_USB_DEVICE(USB_VENDOR_ID_GIGABYTE_AERO15SA, USB_DEVICE_ID_GIGABYTE_AERO15SA)},
	{HID_USB_DEVICE(USB_VENDOR_ID_GIGABYTE_AORUS15P, USB_DEVICE_ID_GIGABYTE_AORUS15P)},
	{HID_USB_DEVICE(USB_VENDOR_ID_GIGABYTE_AORUS15G, USB_DEVICE_ID_GIGABYTE_AORUS15G)},
	{HID_USB_DEVICE(USB_VENDOR_ID_GIGABYTE_AERO17XE5, USB_DEVICE_ID_GIGABYTE_AERO17XE5)},
	{}
};
MODULE_DEVICE_TABLE(hid, gigabyte_kbd_devices);

static struct hid_driver gigabyte_kbd_driver = {
	.name = "gigabytekbd",
	.id_table = gigabyte_kbd_devices,
	.probe = gigabyte_kbd_probe,
	.raw_event = gigabyte_kbd_raw_event};
module_hid_driver(gigabyte_kbd_driver);
