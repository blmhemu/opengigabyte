// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * HID driver for Gigabyte Keyboards
 * Copyright (c) 2020 Hemanth Bollamreddi
 */

#include <linux/hid.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/workqueue.h>
#include <linux/device.h>
#include <linux/acpi.h>
#include <linux/i2c.h>
#include <linux/slab.h>

#include "gigabytekbd_driver.h"

MODULE_AUTHOR("Hemanth Bollamreddi <blmhemu@gmail.com>");
MODULE_DESCRIPTION("HID Keyboard driver for Gigabyte Keyboards.");
MODULE_LICENSE("GPL v2");

#define HIDRAW_FN_ESC 0x04000084
#define HIDRAW_FN_F2 0x0400007C
#define HIDRAW_FN_F3 0x0400007D
#define HIDRAW_FN_F4 0x0400007E
#define HIDRAW_FN_F6 0x04000080
#define HIDRAW_FN_F10 0x04000081
#define HIDRAW_FN_F11 0x04000082
#define HIDRAW_FN_F12 0x04000083

#define make_u32(a, b, c, d) (((u32)(a) << 24) | ((u32)(b) << 16) | ((u32)(c) << 8) | (u32)(d))

/* Touchpad */
struct device_driver *gigabyte_kbd_touchpad_driver;
struct device *gigabyte_kbd_touchpad_device;

static void gigabyte_kbd_touchpad_toggle_driver(struct work_struct *s)
{
	int err;
	if (!gigabyte_kbd_touchpad_device)
	{
		pr_warn("[!] warning (gigabytekbd): touchpad device not found, cannot toggle.\n");
		return;
	}

	if (gigabyte_kbd_touchpad_device->driver)
	{
		if (!gigabyte_kbd_touchpad_driver)
		{
			gigabyte_kbd_touchpad_driver = gigabyte_kbd_touchpad_device->driver;
		}
		device_release_driver(gigabyte_kbd_touchpad_device);
	}
	else if (gigabyte_kbd_touchpad_driver)
	{
		err = device_driver_attach(
		    gigabyte_kbd_touchpad_driver,
		    gigabyte_kbd_touchpad_device);
		if (err < 0)
		{
			pr_err("[-] error (gigabytekbd): Failed to reattach touchpad driver: %d\n", err);
		}
	}
	else
	{
		pr_warn("[-] error (gigabytekbd): Touchpad driver pointer is null, cannot re-enable touchpad.\n");
	}
}

/* Work item for touchpad */
DECLARE_WORK(gigabyte_kbd_touchpad_toggle_driver_work, gigabyte_kbd_touchpad_toggle_driver);

static int gigabyte_kbd_raw_event(struct hid_device *hdev, struct hid_report *report, u8 *rd, int size)
{
	if (report->id == 4 && size == 4)
	{
		u32 hidraw = make_u32(rd[0], rd[1], rd[2], rd[3]);
		switch (hidraw)
		{
		case HIDRAW_FN_F3:
			rd[0] = 0x03;
			rd[1] = 0x70;
			rd[2] = 0x00;
			hid_report_raw_event(hdev, HID_INPUT_REPORT, rd, 4, 0);
			rd[0] = 0x03;
			rd[1] = 0x00;
			rd[2] = 0x00;
			return 1;
		case HIDRAW_FN_F4:
			rd[0] = 0x03;
			rd[1] = 0x6f;
			rd[2] = 0x00;
			hid_report_raw_event(hdev, HID_INPUT_REPORT, rd, 4, 0);
			rd[0] = 0x03;
			rd[1] = 0x00;
			rd[2] = 0x00;
			return 1;
		case HIDRAW_FN_F10:
			if (gigabyte_kbd_touchpad_device)
			{
				schedule_work(&gigabyte_kbd_touchpad_toggle_driver_work);
			}
			else
			{
				pr_warn("[!] warning (gigabytekbd): Touchpad device not initialized, Fn+F10 ignored.\n");
			}
			return 1;
		default:
			return 0;
			break;
		}
	}
	return 0;
}

static int gigabyte_kbd_match_touchpad_device(struct device *dev, const void *data)
{
	struct acpi_device *acpi_dev = ACPI_COMPANION(dev);
	const char *hid;
	const char *bid;
	int instance_no;
	int i;

	/* Check if the device has ACPI companion data */
	if (!acpi_dev)
	{
		return 0;
	}

	hid = acpi_device_hid(acpi_dev);
	bid = acpi_device_bid(acpi_dev);
	instance_no = acpi_dev->pnp.instance_no;

	if (!hid || !bid)
	{
		return 0;
	}

	/* Iterate through our list of known touchpad identifiers from the header file */
	for (i = 0; i < ARRAY_SIZE(gigabyte_kbd_touchpad_device_identifiers); ++i)
	{
		if (strcmp(gigabyte_kbd_touchpad_device_identifiers[i].hid, hid) == 0 && strcmp(gigabyte_kbd_touchpad_device_identifiers[i].bid, bid) == 0 && gigabyte_kbd_touchpad_device_identifiers[i].instance_no == instance_no)
		{
			return 1;
		}
	}

	/* No match found in our list */
	return 0;
}

static int gigabyte_kbd_probe(struct hid_device *hdev, const struct hid_device_id *id)
{
	printk("[+] gigabyte kbd driver loaded");
	int ret;
	hdev->quirks |= HID_QUIRK_INPUT_PER_APP;

	ret = hid_parse(hdev);
	if (ret)
	{
		return ret;
	}
	gigabyte_kbd_touchpad_device = bus_find_device(&i2c_bus_type, NULL, NULL, gigabyte_kbd_match_touchpad_device);

	if (gigabyte_kbd_touchpad_device)
	{
		gigabyte_kbd_touchpad_driver = gigabyte_kbd_touchpad_device->driver;
	}
	else
	{
		printk(KERN_ERR "[-] error: Touchpad acpi device not found");
	}

	return hid_hw_start(hdev, HID_CONNECT_DEFAULT);
}

static const struct hid_device_id gigabyte_kbd_devices[] = {
    {HID_USB_DEVICE(USB_VENDOR_ID_GIGABYTE_AERO15XV8, USB_DEVICE_ID_GIGABYTE_AERO15XV8)},
    {HID_USB_DEVICE(USB_VENDOR_ID_GIGABYTE_AERO15SA, USB_DEVICE_ID_GIGABYTE_AERO15SA)},
    {HID_USB_DEVICE(USB_VENDOR_ID_GIGABYTE_AORUS15P, USB_DEVICE_ID_GIGABYTE_AORUS15P)},
    {HID_USB_DEVICE(USB_VENDOR_ID_GIGABYTE_AORUS15G, USB_DEVICE_ID_GIGABYTE_AORUS15G)},
    {HID_USB_DEVICE(USB_VENDOR_ID_GIGABYTE_AORUS15XE4, USB_DEVICE_ID_GIGABYTE_AORUS15XE4)},
    {}};
MODULE_DEVICE_TABLE(hid, gigabyte_kbd_devices);

static struct hid_driver gigabyte_kbd_driver = {
    .name = "gigabytekbd",
    .id_table = gigabyte_kbd_devices,
    .probe = gigabyte_kbd_probe,
    .raw_event = gigabyte_kbd_raw_event};
module_hid_driver(gigabyte_kbd_driver);
