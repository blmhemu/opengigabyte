// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * HID driver for Gigabyte Keyboards
 * Copyright (c) 2020 Hemanth Bollamreddi
 *
 * Supports Fn key combinations for:
 *   - Fn+F2: WiFi toggle (KEY_WLAN)
 *   - Fn+F3/F4: Screen brightness
 *   - Fn+F5: Display switch (KEY_SWITCHVIDEOMODE)
 *   - Fn+F6: Backlight toggle
 *   - Fn+F8/F9: Volume down/up
 *   - Fn+F10: Touchpad toggle
 *   - Fn+F11: Airplane mode (KEY_RFKILL)
 *   - Fn+F12: Programmable key (KEY_PROG1)
 *   - Fn+ESC: Fan control placeholder (KEY_PROG2)
 */

#include <linux/hid.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/workqueue.h>
#include <linux/backlight.h>
#include <linux/device.h>
#include <linux/acpi.h>
#include <linux/input.h>
#include "gigabytekbd_driver.h"

MODULE_AUTHOR("Hemanth Bollamreddi <blmhemu@gmail.com>");
MODULE_DESCRIPTION("HID Keyboard driver for Gigabyte Keyboards.");
MODULE_LICENSE("GPL v2");

/* Fn key HID raw event codes */
#define HIDRAW_FN_ESC		0x04000084
#define HIDRAW_FN_F2		0x0400007C
#define HIDRAW_FN_F3		0x0400007D
#define HIDRAW_FN_F4		0x0400007E
#define HIDRAW_FN_F5		0x0400007F
#define HIDRAW_FN_F6		0x04000080
#define HIDRAW_FN_F8_PRESS	0x04000186
#define HIDRAW_FN_F8_RELEASE	0x04000086
#define HIDRAW_FN_F9_PRESS	0x04000187
#define HIDRAW_FN_F9_RELEASE	0x04000087
#define HIDRAW_FN_F10		0x04000081
#define HIDRAW_FN_F11		0x04000082
#define HIDRAW_FN_F12		0x04000083
#define HIDRAW_FN_F12_ALT	0x04000088	/* Aorus 16X */

#define make_u32(a, b, c, d) ((a) << 24 | (b) << 16 | (c) << 8 | (d))

/* Driver private data */
struct gigabyte_kbd_data {
	struct backlight_device *backlight;
	struct device_driver *touchpad_driver;
	struct device *touchpad_device;
};

/* Global state shared across HID interfaces */
static struct gigabyte_kbd_data *gigabyte_kbd_priv;
static struct input_dev *gigabyte_kbd_input_dev;
static struct input_dev *gigabyte_kbd_consumer_dev;
static int gigabyte_kbd_refcount;

static struct backlight_device *gigabyte_kbd_backlight_device;
static struct device_driver *gigabyte_kbd_touchpad_driver;
static struct device *gigabyte_kbd_touchpad_device;

static inline int gigabyte_kbd_is_backlight_off(void)
{
	return gigabyte_kbd_backlight_device->props.power == FB_BLANK_POWERDOWN;
}

static void gigabyte_kbd_backlight_toggle(struct work_struct *s)
{
	if (gigabyte_kbd_is_backlight_off())
		backlight_enable(gigabyte_kbd_backlight_device);
	else
		backlight_disable(gigabyte_kbd_backlight_device);
}

static void gigabyte_kbd_touchpad_toggle_driver(struct work_struct *s)
{
	int err;

	if (gigabyte_kbd_touchpad_device->driver) {
		gigabyte_kbd_touchpad_driver = gigabyte_kbd_touchpad_device->driver;
		device_release_driver(gigabyte_kbd_touchpad_device);
	} else if (gigabyte_kbd_touchpad_driver) {
		err = device_driver_attach(gigabyte_kbd_touchpad_driver,
					   gigabyte_kbd_touchpad_device);
		(void)err;
	}
}

/*
 * Work queues for device operations that cannot be called from
 * the HID event handler context
 */
static DECLARE_WORK(gigabyte_kbd_backlight_toggle_work, gigabyte_kbd_backlight_toggle);
static DECLARE_WORK(gigabyte_kbd_touchpad_toggle_driver_work, gigabyte_kbd_touchpad_toggle_driver);

/* Emit a key press and release event */
static void gigabyte_kbd_emit_key(struct input_dev *input, unsigned int key)
{
	if (!input)
		return;
	input_report_key(input, key, 1);
	input_sync(input);
	input_report_key(input, key, 0);
	input_sync(input);
}

/* Emit volume key to Consumer Control device for proper DE integration */
static void gigabyte_kbd_emit_volume(unsigned int key, int pressed)
{
	struct input_dev *dev = gigabyte_kbd_consumer_dev ?: gigabyte_kbd_input_dev;

	if (!dev)
		return;
	input_report_key(dev, key, pressed);
	input_sync(dev);
}

static int gigabyte_kbd_raw_event(struct hid_device *hdev,
				  struct hid_report *report, u8 *rd, int size)
{
	u32 hidraw;

	if (report->id != 4 || size != 4)
		return 0;

	hidraw = make_u32(rd[0], rd[1], rd[2], rd[3]);

	switch (hidraw) {
	case HIDRAW_FN_ESC:
		/* Fan control - emit KEY_PROG2 for user-space handling */
		gigabyte_kbd_emit_key(gigabyte_kbd_input_dev, KEY_PROG2);
		return 1;

	case HIDRAW_FN_F2:
		gigabyte_kbd_emit_key(gigabyte_kbd_input_dev, KEY_WLAN);
		return 1;

	case HIDRAW_FN_F3:
		if (gigabyte_kbd_is_backlight_off())
			return 0;
		rd[0] = 0x03; rd[1] = 0x70; rd[2] = 0x00;
		hid_report_raw_event(hdev, HID_INPUT_REPORT, rd, 4, 0);
		rd[0] = 0x03; rd[1] = 0x00; rd[2] = 0x00;
		return 1;

	case HIDRAW_FN_F4:
		if (gigabyte_kbd_is_backlight_off())
			return 0;
		rd[0] = 0x03; rd[1] = 0x6f; rd[2] = 0x00;
		hid_report_raw_event(hdev, HID_INPUT_REPORT, rd, 4, 0);
		rd[0] = 0x03; rd[1] = 0x00; rd[2] = 0x00;
		return 1;

	case HIDRAW_FN_F5:
		gigabyte_kbd_emit_key(gigabyte_kbd_input_dev, KEY_SWITCHVIDEOMODE);
		return 1;

	case HIDRAW_FN_F6:
		if (gigabyte_kbd_backlight_device)
			schedule_work(&gigabyte_kbd_backlight_toggle_work);
		return 0;	/* Pass through for other handlers */

	case HIDRAW_FN_F8_PRESS:
		gigabyte_kbd_emit_volume(KEY_VOLUMEDOWN, 1);
		return 1;

	case HIDRAW_FN_F8_RELEASE:
		gigabyte_kbd_emit_volume(KEY_VOLUMEDOWN, 0);
		return 1;

	case HIDRAW_FN_F9_PRESS:
		gigabyte_kbd_emit_volume(KEY_VOLUMEUP, 1);
		return 1;

	case HIDRAW_FN_F9_RELEASE:
		gigabyte_kbd_emit_volume(KEY_VOLUMEUP, 0);
		return 1;

	case HIDRAW_FN_F10:
		if (gigabyte_kbd_touchpad_device)
			schedule_work(&gigabyte_kbd_touchpad_toggle_driver_work);
		return 0;

	case HIDRAW_FN_F11:
		gigabyte_kbd_emit_key(gigabyte_kbd_input_dev, KEY_RFKILL);
		return 1;

	case HIDRAW_FN_F12:
	case HIDRAW_FN_F12_ALT:
		gigabyte_kbd_emit_key(gigabyte_kbd_input_dev, KEY_PROG1);
		return 1;

	default:
		return 0;
	}
}

static int gigabyte_kbd_match_touchpad_device(struct device *dev, const void *data)
{
	struct acpi_device *acpi;
	const char *hid;
	char *bid;
	int instance_no, i;

	acpi = ACPI_COMPANION(dev);
	if (!acpi)
		return 0;

	hid = acpi_device_hid(acpi);
	bid = acpi_device_bid(acpi);
	instance_no = acpi->pnp.instance_no;

	for (i = 0; i < ARRAY_SIZE(gigabyte_kbd_touchpad_device_identifiers); i++) {
		if (!strcmp(gigabyte_kbd_touchpad_device_identifiers[i].hid, hid) &&
		    !strcmp(gigabyte_kbd_touchpad_device_identifiers[i].bid, bid) &&
		    gigabyte_kbd_touchpad_device_identifiers[i].instance_no == instance_no)
			return 1;
	}
	return 0;
}

static int gigabyte_kbd_setup_input_dev(struct hid_device *hdev)
{
	struct input_dev *input;
	int ret;

	if (gigabyte_kbd_input_dev) {
		gigabyte_kbd_refcount++;
		return 0;
	}

	input = input_allocate_device();
	if (!input)
		return -ENOMEM;

	input->name = "Gigabyte Fn Keys";
	input->phys = "gigabytekbd/input0";
	input->id.bustype = BUS_USB;
	input->id.vendor = hdev->vendor;
	input->id.product = hdev->product;
	input->id.version = hdev->version;
	input->dev.parent = &hdev->dev;

	set_bit(EV_KEY, input->evbit);
	set_bit(KEY_WLAN, input->keybit);
	set_bit(KEY_SWITCHVIDEOMODE, input->keybit);
	set_bit(KEY_VOLUMEDOWN, input->keybit);
	set_bit(KEY_VOLUMEUP, input->keybit);
	set_bit(KEY_RFKILL, input->keybit);
	set_bit(KEY_PROG1, input->keybit);
	set_bit(KEY_PROG2, input->keybit);

	ret = input_register_device(input);
	if (ret) {
		input_free_device(input);
		return ret;
	}

	gigabyte_kbd_input_dev = input;
	gigabyte_kbd_refcount = 1;
	return 0;
}

static int gigabyte_kbd_probe(struct hid_device *hdev,
			      const struct hid_device_id *id)
{
	struct gigabyte_kbd_data *priv;
	struct hid_input *hi;
	int ret;

	priv = devm_kzalloc(&hdev->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	hid_set_drvdata(hdev, priv);
	gigabyte_kbd_priv = priv;

	hdev->quirks |= HID_QUIRK_INPUT_PER_APP;

	ret = hid_parse(hdev);
	if (ret)
		return ret;

	ret = hid_hw_start(hdev, HID_CONNECT_DEFAULT);
	if (ret)
		return ret;

	/* Find Consumer Control device for volume key injection */
	if (!gigabyte_kbd_consumer_dev) {
		list_for_each_entry(hi, &hdev->inputs, list) {
			if (hi->input->name &&
			    strstr(hi->input->name, "Consumer Control")) {
				gigabyte_kbd_consumer_dev = hi->input;
				set_bit(KEY_VOLUMEDOWN, hi->input->keybit);
				set_bit(KEY_VOLUMEUP, hi->input->keybit);
				break;
			}
		}
	}

	/* Create input device for Fn key events */
	ret = gigabyte_kbd_setup_input_dev(hdev);
	if (ret)
		hid_warn(hdev, "Failed to create Fn Keys input device\n");

	/* Find backlight device */
	gigabyte_kbd_backlight_device =
		backlight_device_get_by_name(GIGABYTE_KBD_BACKLIGHT_DEVICE_NAME);
	priv->backlight = gigabyte_kbd_backlight_device;

	/* Find touchpad device */
	gigabyte_kbd_touchpad_device =
		bus_find_device(&i2c_bus_type, NULL, NULL,
				gigabyte_kbd_match_touchpad_device);
	priv->touchpad_device = gigabyte_kbd_touchpad_device;

	if (gigabyte_kbd_touchpad_device) {
		gigabyte_kbd_touchpad_driver = gigabyte_kbd_touchpad_device->driver;
		priv->touchpad_driver = gigabyte_kbd_touchpad_driver;
	}

	return 0;
}

static void gigabyte_kbd_remove(struct hid_device *hdev)
{
	hid_hw_stop(hdev);

	if (gigabyte_kbd_refcount > 0) {
		gigabyte_kbd_refcount--;
		if (gigabyte_kbd_refcount == 0 && gigabyte_kbd_input_dev) {
			input_unregister_device(gigabyte_kbd_input_dev);
			gigabyte_kbd_input_dev = NULL;
		}
	}
}

static const struct hid_device_id gigabyte_kbd_devices[] = {
	{ HID_USB_DEVICE(USB_VENDOR_ID_GIGABYTE_AERO15XV8,
			 USB_DEVICE_ID_GIGABYTE_AERO15XV8) },
	{ HID_USB_DEVICE(USB_VENDOR_ID_GIGABYTE_AERO15SA,
			 USB_DEVICE_ID_GIGABYTE_AERO15SA) },
	{ HID_USB_DEVICE(USB_VENDOR_ID_GIGABYTE_AORUS15P,
			 USB_DEVICE_ID_GIGABYTE_AORUS15P) },
	{ HID_USB_DEVICE(USB_VENDOR_ID_GIGABYTE_AORUS15G,
			 USB_DEVICE_ID_GIGABYTE_AORUS15G) },
	{ HID_USB_DEVICE(USB_VENDOR_ID_GIGABYTE_AORUS16X,
			 USB_DEVICE_ID_GIGABYTE_AORUS16X) },
	{ HID_USB_DEVICE(USB_VENDOR_ID_GIGABYTE_AORUS15_9KF_1,
			 USB_DEVICE_ID_GIGABYTE_AORUS15_9KF_1) },
	{ HID_USB_DEVICE(USB_VENDOR_ID_GIGABYTE_AORUS15_9KF_2,
			 USB_DEVICE_ID_GIGABYTE_AORUS15_9KF_2) },
	{ }
};
MODULE_DEVICE_TABLE(hid, gigabyte_kbd_devices);

static struct hid_driver gigabyte_kbd_driver = {
	.name = "gigabytekbd",
	.id_table = gigabyte_kbd_devices,
	.probe = gigabyte_kbd_probe,
	.remove = gigabyte_kbd_remove,
	.raw_event = gigabyte_kbd_raw_event,
};
module_hid_driver(gigabyte_kbd_driver);
