# OpenGigabyte
A collection of Linux drivers of various components for Gigabyte devices.

## What's Done
### Keyboard
- [x] All normal keys (alphanumerics and special character) working beforehand.
- [x] Make the C code into actual driver and ppa support.
- [x] Sleep, Mute, Volume Keys, Backlight [Fn + F1/F7/F8/F9/SPC] working even before this driver.
- [x] Brightness keys [Fn + F3/F4] working with this driver.
- [ ] Make Fn + ESC/F2/F5/F10/F11/F12 work.
- [ ] Look into controlling fan profiles with [Fn + Esc]
- [ ] Look into the possibility of full keyboard RGB backlight support.
- [ ] Add into the main linux kernel ?

## Supported Devices
### Keyboard
| Device                                        |   VID:PID   |
| --------------------------------------------- | ----------- |
| Gigabyte Aero 15X V8                          |  1044:7A39  |
| Gigabyte Aero 15X V9                          |  1044:7A39  |
| Gigabyte Aero 15 SA                           |  1044:7A3F  |
| Gigabyte Aero 17 XD                           |  1044:7A3F  |
| Gigabyte Aorus 15P (RTX 30 series)            |  1044:7A3B  |
| Gigabyte Aorus 15G                            |  1044:7A3C  |
| Gigabyte Aorus 17G YC (RTX 30 series)         |  1044:7A3C  |
| Gigabyte Aorus 16X ASG                        |  0414:8005  |
| Gigabyte Aorus 15 9KF              |  0414:7a43, 0414:7a44  |




## Install Instructions

### Ubuntu :
```
sudo add-apt-repository ppa:blmhemu/opengigabyte
sudo apt update
sudo apt install opengigabyte-driver-dkms
```

Or to manully build and install the package:
```bash
sudo apt install dkms build-essential dh-make dh-dkms
./scripts/build_debs.sh
sudo dpkg -i ./dist/opengigabyte-driver-dkms_0.0.2-0_all.deb
```
### Rocky Linux / RHEL based distros :

Install required build dependencies and DKMS:
```
sudo dnf install epel-release
sudo dnf install dkms gcc make kernel-devel-$(uname -r) kernel-headers
```

Clone the repo:
```
git clone https://github.com/blmhemu/opengigabyte.git
cd opengigabyte
```

Install the driver source into the DKMS source tree:
```
sudo make setup_dkms
```

Register, build, and install the driver module:
```
sudo dkms add -m opengigabyte-driver -v 0.0.2
sudo dkms build -m opengigabyte-driver -v 0.0.2
sudo dkms install -m opengigabyte-driver -v 0.0.2
```

Install and reload the udev rules:
```
sudo make udev_install
sudo udevadm control --reload-rules
sudo udevadm trigger
```

Load the driver:
```
sudo modprobe gigabytekbd
```

Verify the installation has succeeded:
```
dkms status
lsmod | grep gigabyte
```

**Tested on Rocky Linux 10.2 with kernel 6.12.0-211.58.1.el10_2.x86_64.**
### Arch Linux :
Install [opengigabyte-meta]() from the AUR using an [AUR helper](https://wiki.archlinux.org/title/AUR_helpers#Pacman_wrappers). For example:
```
yay -S opengigabyte
```
## Uninstall Instructions

### Rocky Linux / RHEL based distros :

Unload the kernel module:
```
sudo modprobe -r gigabytekbd
```

Remove the driver from DKMS:
```
sudo dkms remove -m opengigabyte-driver -v 0.0.2 --all
```

Remove the udev rules:
```
sudo make udev_uninstall
sudo udevadm control --reload-rules
sudo udevadm trigger
```

Remove the DKMS source files:
```
sudo make remove_dkms
```

Verify the driver has been uninstalled successfully:
```
dkms status
lsmod | grep gigabyte
```

## Notes
* The keyboard driver converts the obscure key codes from the keyboard into standard keycodes. Handling the functionality of the keys is up to the user.
  * For example, link the BrightnessUp / BrightnessDown keyboard symbol to [light utility](https://github.com/haikarainen/light) or xbacklight to control brightness using the keys.

## Releases / Changelog
https://github.com/blmhemu/opengigabyte/releases

## Credits
* [OpenRazer](https://openrazer.github.io/)

---
OpenGigabyte is licensed under the GPL and not officially endorsed by [Gigabyte, Ltd](https://www.gigabyte.com)
