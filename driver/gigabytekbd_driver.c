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
#include <linux/version.h>
#include <linux/i2c.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(7, 0, 0)
#include <linux/fb.h>
#endif

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

// HID keyboard page usage for KeyboardF20, reported by the Fn key on some models.
#define HID_USAGE_KEYBOARD_F20 0x6f

#define make_u32(a, b, c, d) a << 24 | b << 16 | c << 8 | d

struct backlight_device* gigabyte_kbd_backlight_device;
struct device_driver* gigabyte_kbd_touchpad_driver;
struct device* gigabyte_kbd_touchpad_device;
struct input_dev* gigabyte_kbd_input_device;

static inline int gigabyte_kbd_is_backlight_off(void)
{
	if (!gigabyte_kbd_backlight_device)
		return 1;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(7, 0, 0)
	return backlight_is_blank(gigabyte_kbd_backlight_device);
#else
	return gigabyte_kbd_backlight_device->props.power == FB_BLANK_POWERDOWN;
#endif
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
#if LINUX_VERSION_CODE >= KERNEL_VERSION(7, 0, 0)
			hid_report_raw_event(hdev, HID_INPUT_REPORT, rd, 4, 4, 0);
#else
			hid_report_raw_event(hdev, HID_INPUT_REPORT, rd, 4, 0);
#endif
			rd[0] = 0x03;rd[1] = 0x00;rd[2] = 0x00;
			return 1;
		case HIDRAW_FN_F4:
			if (gigabyte_kbd_is_backlight_off())
				return 0;

			rd[0] = 0x03;rd[1] = 0x6f;rd[2] = 0x00;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(7, 0, 0)
			hid_report_raw_event(hdev, HID_INPUT_REPORT, rd, 4, 4, 0);
#else
			hid_report_raw_event(hdev, HID_INPUT_REPORT, rd, 4, 0);
#endif
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
		case HIDRAW_FN_F11:
			if (gigabyte_kbd_input_device)
			{
				input_report_key(gigabyte_kbd_input_device, KEY_RFKILL, 1);
				input_sync(gigabyte_kbd_input_device);
				input_report_key(gigabyte_kbd_input_device, KEY_RFKILL, 0);
				input_sync(gigabyte_kbd_input_device);
			}
			return 0;
		default:
			return 0;
			break;
		}
	}
	return 0;
}

static int gigabyte_kbd_input_mapping(struct hid_device *hdev, struct hid_input *hi,
	struct hid_field *field, struct hid_usage *usage, unsigned long **bit, int *max)
{
	// On the Aorus 16X the Fn key reports KeyboardF20 as an ordinary key press on
	// top of acting as a hardware modifier. Userspace reads KEY_F20 as the mic mute
	// key, so every Fn press (i.e. every Fn combo) toggles the microphone. Report it
	// as KEY_FN instead, which desktop environments ignore.
	// Only done for models known to do this - other Gigabyte laptops have macro keys
	// that legitimately emit the F13-F24 range.
	if (hdev->product == USB_DEVICE_ID_GIGABYTE_AORUS16X
		&& (usage->hid & HID_USAGE_PAGE) == HID_UP_KEYBOARD
		&& (usage->hid & HID_USAGE) == HID_USAGE_KEYBOARD_F20)
	{
		hid_map_usage_clear(hi, usage, bit, max, EV_KEY, KEY_FN);
		return 1;
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
	printk("Gigabyte kbd driver loaded.");
	int ret;
	struct hid_input *hidinput;
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

	ret = hid_hw_start(hdev, HID_CONNECT_DEFAULT);
	if (ret)
		return ret;

	// Advertise KEY_RFKILL so we can report the Fn+F11 airplane-mode
	// press to userspace, which owns the actual soft/hard-block policy
	// via rfkill (there is no in-kernel "toggle all radios" API anymore).
	list_for_each_entry(hidinput, &hdev->inputs, list)
	{
		input_set_capability(hidinput->input, EV_KEY, KEY_RFKILL);
		if (!gigabyte_kbd_input_device)
			gigabyte_kbd_input_device = hidinput->input;
	}

	return 0;
}

static const struct hid_device_id gigabyte_kbd_devices[] = {
	{HID_USB_DEVICE(USB_VENDOR_ID_GIGABYTE_AERO15XV8, USB_DEVICE_ID_GIGABYTE_AERO15XV8)},
	{HID_USB_DEVICE(USB_VENDOR_ID_GIGABYTE_AERO15SA, USB_DEVICE_ID_GIGABYTE_AERO15SA)},
	{HID_USB_DEVICE(USB_VENDOR_ID_GIGABYTE_AORUS15P, USB_DEVICE_ID_GIGABYTE_AORUS15P)},
	{HID_USB_DEVICE(USB_VENDOR_ID_GIGABYTE_AORUS15G, USB_DEVICE_ID_GIGABYTE_AORUS15G)},
	{HID_USB_DEVICE(USB_VENDOR_ID_GIGABYTE_AORUS16X, USB_DEVICE_ID_GIGABYTE_AORUS16X)},
    {HID_USB_DEVICE(USB_VENDOR_ID_GIGABYTE_AORUS15_9KF_1, USB_DEVICE_ID_GIGABYTE_AORUS15_9KF_1)},
	{HID_USB_DEVICE(USB_VENDOR_ID_GIGABYTE_AORUS15_9KF_2, USB_DEVICE_ID_GIGABYTE_AORUS15_9KF_2)},
	{HID_USB_DEVICE(USB_VENDOR_ID_GIGABYTE_AORUS15_BKG, USB_DEVICE_ID_GIGABYTE_AORUS15_BKG)},
	{}
};
MODULE_DEVICE_TABLE(hid, gigabyte_kbd_devices);

static struct hid_driver gigabyte_kbd_driver = {
	.name = "gigabytekbd",
	.id_table = gigabyte_kbd_devices,
	.probe = gigabyte_kbd_probe,
	.input_mapping = gigabyte_kbd_input_mapping,
	.raw_event = gigabyte_kbd_raw_event};
module_hid_driver(gigabyte_kbd_driver);
