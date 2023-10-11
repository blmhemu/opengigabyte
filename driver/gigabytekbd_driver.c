// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * HID driver for Gigabyte Keyboards
 * Copyright (c) 2020 Hemanth Bollamreddi
*/

#include <linux/hid.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/workqueue.h>
#include <linux/backlight.h>
#include <linux/device.h>
#include <linux/acpi.h>
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

struct backlight_device* gigabyte_kbd_backlight_device;
struct device_driver* gigabyte_kbd_touchpad_driver;
struct device* gigabyte_kbd_touchpad_device;

static inline int gigabyte_kbd_is_backlight_off(void)
{
	return gigabyte_kbd_backlight_device->props.power == FB_BLANK_POWERDOWN;
}

static void gigabyte_kbd_backlight_toggle(struct work_struct* s)
{
	if (gigabyte_kbd_is_backlight_off())
	{
		backlight_enable(gigabyte_kbd_backlight_device);
	}
	else
	{
		backlight_disable(gigabyte_kbd_backlight_device);
	}
}

static void gigabyte_kbd_touchpad_toggle_driver(struct work_struct* s)
{
	int err;
	if (gigabyte_kbd_touchpad_device->driver)
	{
		// Toggle off
		gigabyte_kbd_touchpad_driver = gigabyte_kbd_touchpad_device->driver;
		device_release_driver(gigabyte_kbd_touchpad_device);
	}
	else if (gigabyte_kbd_touchpad_driver)
	{
		// Toggle on
		err = device_driver_attach(gigabyte_kbd_touchpad_driver, gigabyte_kbd_touchpad_device);
		(void)err; // Avoid compiler warning
	}
}

// We have to call device functions outside of the event thread (othewise the system crashes),
// thus we use linux's work queue system
DECLARE_WORK(gigabyte_kbd_backlight_toggle_work, gigabyte_kbd_backlight_toggle);
DECLARE_WORK(gigabyte_kbd_touchpad_toggle_driver_work, gigabyte_kbd_touchpad_toggle_driver);

static int gigabyte_kbd_raw_event(struct hid_device *hdev, struct hid_report *report, u8 *rd, int size)
{
	if (report->id == 4 && size == 4)
	{
		u32 hidraw = make_u32(rd[0], rd[1], rd[2], rd[3]);
		// printk("Gigabyte kbd raw event. hidraw code : %x", hidraw);
		switch (hidraw)
		{
		case HIDRAW_FN_F3:
			if (gigabyte_kbd_is_backlight_off())
				return 0;

			rd[0] = 0x03;rd[1] = 0x70;rd[2] = 0x00;
			hid_report_raw_event(hdev, HID_INPUT_REPORT, rd, 4, 0);
			rd[0] = 0x03;rd[1] = 0x00;rd[2] = 0x00;
			return 1;
		case HIDRAW_FN_F4:
			if (gigabyte_kbd_is_backlight_off())
				return 0;

			rd[0] = 0x03;rd[1] = 0x6f;rd[2] = 0x00;
			hid_report_raw_event(hdev, HID_INPUT_REPORT, rd, 4, 0);
			rd[0] = 0x03;rd[1] = 0x00;rd[2] = 0x00;
			return 1;
		case HIDRAW_FN_F6:
			if (gigabyte_kbd_backlight_device)
			{
				schedule_work(&gigabyte_kbd_backlight_toggle_work);
			}
			return 0;
		case HIDRAW_FN_F10:
			if (gigabyte_kbd_touchpad_device)
			{
				schedule_work(&gigabyte_kbd_touchpad_toggle_driver_work);
			}
			return 0;
		default:
			return 0;
			break;
		}
	}
	return 0;
}

static int gigabyte_kbd_match_touchpad_device(struct device *dev, const void *adev)
{
	struct acpi_device* acpi;
	const char* hid;
	char* bid;
	int instance_no, i;

	acpi = ACPI_COMPANION(dev); // cast the device as an acpi device
	if (acpi)
	{
		hid = acpi_device_hid(acpi);
		bid = acpi_device_bid(acpi);
		instance_no = acpi->pnp.instance_no;

		// printk("hid: %s, bid: %s, instance_no=%d", hid, bid, instance_no);
		for (i = 0; i < sizeof(gigabyte_kbd_touchpad_device_identifiers) / sizeof(struct gigabyte_kbd_touchpad_device_identifier); i++)
		{
			if (!strcmp(gigabyte_kbd_touchpad_device_identifiers[i].hid, hid)
				&& !strcmp(gigabyte_kbd_touchpad_device_identifiers[i].bid, bid)
				&& gigabyte_kbd_touchpad_device_identifiers[i].instance_no == instance_no)
			{
				return 1;
			}
		}
	}
	return 0;
}

static int gigabyte_kbd_probe(struct hid_device *hdev, const struct hid_device_id *id)
{
	// printk("Gigabyte kbd driver loaded.");

	int ret;
	hdev->quirks |= HID_QUIRK_INPUT_PER_APP;

	ret = hid_parse(hdev);
	if (ret)
		return ret;

 	gigabyte_kbd_backlight_device = backlight_device_get_by_name(GIGABYTE_KBD_BACKLIGHT_DEVICE_NAME);
	gigabyte_kbd_touchpad_device = bus_find_device(&i2c_bus_type, NULL, NULL, gigabyte_kbd_match_touchpad_device);
	
	if (gigabyte_kbd_touchpad_device)
	{
		gigabyte_kbd_touchpad_driver = gigabyte_kbd_touchpad_device->driver;
	}
	else
	{
		printk(KERN_ERR "Touchpad acpi device not found");
	}

	return hid_hw_start(hdev, HID_CONNECT_DEFAULT);
}

static const struct hid_device_id gigabyte_kbd_devices[] = {
	{HID_USB_DEVICE(USB_VENDOR_ID_GIGABYTE_AERO15XV8, USB_DEVICE_ID_GIGABYTE_AERO15XV8)},
	{HID_USB_DEVICE(USB_VENDOR_ID_GIGABYTE_AERO15SA, USB_DEVICE_ID_GIGABYTE_AERO15SA)},
	{HID_USB_DEVICE(USB_VENDOR_ID_GIGABYTE_AORUS15P, USB_DEVICE_ID_GIGABYTE_AORUS15P)},
	{HID_USB_DEVICE(USB_VENDOR_ID_GIGABYTE_AORUS15G, USB_DEVICE_ID_GIGABYTE_AORUS15G)},
	{}
};
MODULE_DEVICE_TABLE(hid, gigabyte_kbd_devices);

static struct hid_driver gigabyte_kbd_driver = {
	.name = "gigabytekbd",
	.id_table = gigabyte_kbd_devices,
	.probe = gigabyte_kbd_probe,
	.raw_event = gigabyte_kbd_raw_event};
module_hid_driver(gigabyte_kbd_driver);
