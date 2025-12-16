/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef __HID_GIGABYTE_KBD_H
#define __HID_GIGABYTE_KBD_H

/* Gigabyte laptop USB VID/PID pairs */
#define USB_VENDOR_ID_GIGABYTE_AERO15XV8	0x1044
#define USB_DEVICE_ID_GIGABYTE_AERO15XV8	0x7A39

#define USB_VENDOR_ID_GIGABYTE_AERO15SA		0x1044
#define USB_DEVICE_ID_GIGABYTE_AERO15SA		0x7A3F

#define USB_VENDOR_ID_GIGABYTE_AORUS15P		0x1044
#define USB_DEVICE_ID_GIGABYTE_AORUS15P		0x7A3B

#define USB_VENDOR_ID_GIGABYTE_AORUS15G		0x1044
#define USB_DEVICE_ID_GIGABYTE_AORUS15G		0x7A3C

#define USB_VENDOR_ID_GIGABYTE_AORUS16X		0x0414
#define USB_DEVICE_ID_GIGABYTE_AORUS16X		0x8005

#define USB_VENDOR_ID_GIGABYTE_AORUS15_9KF_1	0x0414
#define USB_DEVICE_ID_GIGABYTE_AORUS15_9KF_1	0x7a43

#define USB_VENDOR_ID_GIGABYTE_AORUS15_9KF_2	0x0414
#define USB_DEVICE_ID_GIGABYTE_AORUS15_9KF_2	0x7a44

/* Backlight device name in /sys/class/backlight/ */
#define GIGABYTE_KBD_BACKLIGHT_DEVICE_NAME	"intel_backlight"

/* Touchpad device identifiers for I2C bus matching */
struct gigabyte_kbd_touchpad_device_identifier {
	const char *hid;
	const char *bid;
	int instance_no;
};

static const struct gigabyte_kbd_touchpad_device_identifier
gigabyte_kbd_touchpad_device_identifiers[] = {
	{ "PNP0C50",  "TPD0", 1 },	/* Aero 15P and similar */
	{ "ELAN0A02", "TPD0", 0 },	/* Aorus 17X and similar */
	{ "ELAN0A03", "TPD0", 1 },	/* Aorus 15 9KF */
	{ "ELAN0A04", "TPD0", 0 },	/* Aorus 16X and similar */
};

#endif /* __HID_GIGABYTE_KBD_H */
