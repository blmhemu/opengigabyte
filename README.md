# OpenGigabyte
A collection of Linux drivers of various components for Gigabyte devices.

## What's Done
### Keyboard
- [x] All normal keys (alphanumerics and special character) working beforehand.
- [x] Sleep, Mute, Volume Keys, Backlight [Fn + F1/F7/F8/F9/SPC] working even before this driver.
- [x] Brightness keys [Fn + F3/F4] working with this driver.
- [ ] Make Fn + ESC/F2/F5/F10/F11/F12 work.
- [ ] Make the C code into actual driver and ppa support.
- [ ] Look into the possibility of full keyboard RGB backlight support.
- [ ] Look into controlling fan profiles with [Fn + Esc] 
- [ ] Add into the main linux kernel ??

## Supported Devices
### Keyboard
| Device                                        |   VID:PID   |
| --------------------------------------------- | ----------- |
| Gigabyte Aero 15X V8                          |  1044:7A39  |
| Gigabyte Aero 15X V9                          |  1044:7A39  |
| Gigabyte Aero 15 SA                           |  1044:7A3F  |

## Install Instructions
**Arch :** 
```
pacman -S linux-headers
pacman -S openaero-meta
```

## Notes
* The keyboard driver converts the obscure key codes from the keyboard into standard keycodes. Handling the functionality of the keys is up to the user.
  * For example, link the BrightnessUp / BrightnessDown keyboard symbol to [light utility](https://github.com/haikarainen/light) or xbacklight to control brightness using the keys.

## Credits
* [OpenRazer](https://openrazer.github.io/)

---
The project is licensed under the GPL and is not officially endorsed by [Gigabyte, Ltd](https://www.gigabyte.com//).
