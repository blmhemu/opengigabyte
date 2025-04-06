#ifndef __HID_GIGABYTE_KBD_H
#define __HID_GIGABYTE_KBD_H

#define USB_VENDOR_ID_GIGABYTE_AERO15XV8 0x1044
#define USB_DEVICE_ID_GIGABYTE_AERO15XV8 0x7A39

#define USB_VENDOR_ID_GIGABYTE_AERO15SA 0x1044
#define USB_DEVICE_ID_GIGABYTE_AERO15SA 0x7A3F

#define USB_VENDOR_ID_GIGABYTE_AORUS15P 0x1044
#define USB_DEVICE_ID_GIGABYTE_AORUS15P 0x7A3B

#define USB_VENDOR_ID_GIGABYTE_AORUS15G 0x1044
#define USB_DEVICE_ID_GIGABYTE_AORUS15G 0x7A3C

#define USB_VENDOR_ID_GIGABYTE_AORUS15XE4 0x1044
#define USB_DEVICE_ID_GIGABYTE_AORUS15XE4 0x7A3B

struct gigabyte_kbd_touchpad_device_identifier
{
        const char *hid;
        const char *bid;
        int instance_no;
};

// touchpad device is in /sys/bus/i2c/devices/i2c-{hid}:{instance_no}
const struct gigabyte_kbd_touchpad_device_identifier gigabyte_kbd_touchpad_device_identifiers[] = {
    {"PNP0C50", "TPD0", 1}, // 15P (and probably more)
    {"ELAN0A02", "TPD0", 1}, // 15XE4 (and probably more)
    {"ELAN0A02", "TPD0", 0} // 17X (and probably more)
};

#endif
