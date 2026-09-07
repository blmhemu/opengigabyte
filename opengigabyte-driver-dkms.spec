%global dkms_name    opengigabyte-driver
%global dkms_version 0.0.2

Name:           opengigabyte-driver-dkms
Version:        %{dkms_version}
Release:        1%{?dist}
Summary:        OpenGigabyte HID keyboard driver for Gigabyte/AORUS laptops (DKMS)

License:        GPL-2.0-or-later
URL:            https://github.com/blmhemu/opengigabyte
Source0:        opengigabyte-%{version}.tar.gz

BuildArch:      noarch

Requires:       dkms
Requires:       gcc
Requires:       make
Requires:       systemd-udev
Recommends:     kernel-devel

Provides:       opengigabyte-kernel-modules-dkms = %{version}-%{release}
Conflicts:      opengigabyte-kernel-modules-dkms

%description
OpenGigabyte is a collection of GNU/Linux drivers for Gigabyte devices.

This package ships the source for the gigabytekbd kernel module and registers
it with DKMS, so it is rebuilt automatically for every installed kernel. The
module translates the vendor specific HID codes emitted by Gigabyte and AORUS
laptop keyboards into standard Linux keycodes.

Kernel headers (kernel-devel) are required to compile the module.

%prep
%autosetup -n opengigabyte-%{version}

%build
# Nothing to build here - DKMS compiles the module at install time, against
# whichever kernel is running.

%install
make setup_dkms udev_install appstream_install DESTDIR=%{buildroot}

%post
# A previous manual `dkms install` of the same version would collide with the
# tree we just laid down, so clear any existing registration first.
if dkms status -m %{dkms_name} -v %{dkms_version} 2>/dev/null | grep -q .; then
    dkms remove -m %{dkms_name} -v %{dkms_version} --all --rpm_safe_upgrade >/dev/null 2>&1 || :
fi
dkms add -m %{dkms_name} -v %{dkms_version} --rpm_safe_upgrade || :
if dkms build -m %{dkms_name} -v %{dkms_version}; then
    dkms install -m %{dkms_name} -v %{dkms_version} --force || :
else
    echo "opengigabyte: DKMS build failed - install kernel-devel matching your"
    echo "running kernel, then run: dkms install -m %{dkms_name} -v %{dkms_version}"
fi

%preun
# $1 is 0 on erase, 1 on upgrade. Only tear down on a real removal.
if [ $1 -eq 0 ]; then
    dkms remove -m %{dkms_name} -v %{dkms_version} --all --rpm_safe_upgrade || :
fi

%files
%license LICENSE
%doc README.md
%{_usrsrc}/%{dkms_name}-%{dkms_version}/
%{_udevrulesdir}/99-gigabyte.rules
%{_prefix}/lib/udev/gigabyte_mount
%{_datadir}/metainfo/io.github.blmhemu.opengigabyte.metainfo.xml

%changelog
* Tue Sep 08 2026 Ibrahim Gamal <ib.gamal.a@gmail.com> - 0.0.2-1
- Initial RPM packaging for Fedora
