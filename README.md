# OpenGigabyte
Linux drivers for Gigabyte devices.

## What's Done
### Keyboard
- [x] All normal keys (alphanumerics and special character) working beforehand.
- [x] Make the C code into actual driver and ppa support.
- [x] Sleep, Mute, Volume Keys, Backlight [Fn + F1/F7/F8/F9/F11/SPC] working even before this driver.
- [x] Brightness keys [Fn + F3/F4] working with this driver.
- [x] Touchpad on/off [Fn + F10] working with this driver.
- [ ] Make Fn + ESC/F2/F5/F12 work.
- [ ] Look into controlling fan profiles with [Fn + Esc]
- [ ] Look into the possibility of full keyboard RGB backlight support.
- [ ] Add into the main linux kernel ?

## Supported Devices
### Keyboard
| Device                                        |   VID:PID   |
| --------------------------------------------- | ----------- |
| Gigabyte Aero 15X V8                          |  1044:7A39  |
| Gigabyte Aero 15X V9                          |  1044:7A39  |
| Gigabyte Aorus 15 XE4                         |  1044:7A3B  |
| Gigabyte Aero 15 SA                           |  1044:7A3F  |
| Gigabyte Aorus 15P (RTX 30 series)            |  1044:7A3B  |
| Gigabyte Aorus 15G                            |  1044:7A3C  |
| Gigabyte Aorus 17G YC (RTX 30 series)         |  1044:7A3C  |

## Install Instructions

### Ubuntu :
```
sudo add-apt-repository ppa:jqnfxa/opengigabyte
sudo apt update
sudo apt install opengigabyte-driver-dkms
```

### Arch Linux :
Install [opengigabyte-meta]() from the AUR using an [AUR helper](https://wiki.archlinux.org/title/AUR_helpers#Pacman_wrappers). 
For example:
```
yay -S opengigabyte-meta
```

## Notes
* The keyboard driver converts the obscure key codes from the keyboard into standard keycodes. Handling the functionality of the keys is up to the user.
* For example, link the BrightnessUp / BrightnessDown keyboard symbol to [light utility](https://github.com/haikarainen/light) or xbacklight to control brightness using the keys.

## Releases / Changelog
https://github.com/jqfxa/opengigabyte/releases

## Credits
* [OpenRazer](https://openrazer.github.io/)
* [blmhemu](https://github.com/blmhemu)

---
OpenGigabyte is licensed under the GPL and not officially endorsed by [Gigabyte, Ltd](https://www.gigabyte.com)
