---
name: New Device Support ⌨️
about: Provide information to add support for a new device.
title: Support for [Name of Device]
labels: new device support
assignees: blmhemu

---

**Prechecks**
Is your device in the list of supported devices ? - [Yes/No]

**Provide**
* Device Name : The full device name (Ex: Gigabyte Aero 15X V8)
* lsusb : Output of `lsusb` (Ex: Bus 001 Device 005: ID 1044:7a39 Chu Yuen Enterprise Co., Ltd USB-HID Keyboard)

**Next Steps**
* Clone and make https://github.com/bentiss/hid-replay
* Run `sudo ./src/hid-recorder` in the repo.
* Select various hidraw interfaces and check if there is any output when you press Fn + ESC/F1/F2/...
* HID raw keycodes : Post the output for each key press in the above step. (There can be multiple outputs for each keypress) (Ex : Fn + F9 - E: 0.703934 4 04 00 01 87 and E: 0.710861 3 03 e9 00 )

**Additional context**
Add any other context or screenshots for the request here.
