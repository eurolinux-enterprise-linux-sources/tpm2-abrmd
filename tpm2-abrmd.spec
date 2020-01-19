Name: tpm2-abrmd
Version: 1.1.0
Release: 11%{?dist}
Summary: A system daemon implementing TPM2 Access Broker and Resource Manager

License: BSD
URL:     https://github.com/01org/tpm2-abrmd
Source0: https://github.com/01org/tpm2-abrmd/archive/%{version}/%{name}-%{version}.tar.gz
# upstream commit 418d49669a33f9e6b029787e3869b3a534bb7de8
Patch0: 0001-tcti-tabrmd-Fix-NULL-deref-bug-by-moving-debug-outpu.patch
Patch1: autoconf-fixup.patch

%{?systemd_requires}
BuildRequires: systemd
BuildRequires: libtool
BuildRequires: autoconf-archive
BuildRequires: pkgconfig(cmocka)
BuildRequires: pkgconfig(dbus-1)
BuildRequires: pkgconfig(gio-unix-2.0)
BuildRequires: pkgconfig(sapi)
BuildRequires: pkgconfig(tcti-device)
BuildRequires: pkgconfig(tcti-socket)
# tpm2-abrmd build depends on tpm2-tss-devel for sapi/tcti-device/tcti-socket libs
BuildRequires: tpm2-tss-devel%{?_isa} >= 1.4.0-1%{?dist}

# this package does not support big endian arch so far,
# and has been verified only on Intel platforms.
ExclusiveArch: %{ix86} x86_64

# tpm2-abrmd depends on tpm2-tss for sapi/tcti-device/tcti-socket libs
Requires: tpm2-tss%{?_isa} >= 1.4.0-1%{?dist}

%description
tpm2-abrmd is a system daemon implementing the TPM2 access broker (TAB) and
Resource Manager (RM) spec from the TCG.

%prep
%autosetup -p1 -n %{name}-%{version}
autoreconf -vif

%build
%configure --disable-static --disable-silent-rules \
           --with-systemdsystemunitdir=%{_unitdir} \
           --with-udevrulesdir=%{_udevrulesdir}
%make_build

%install
%make_install
rm -f %{buildroot}/%{_udevrulesdir}/tpm-udev.rules
find %{buildroot}%{_libdir} -type f -name \*.la -delete

%pre
getent group tss >/dev/null || groupadd -g 59 -r tss
getent passwd tss >/dev/null || \
useradd -r -u 59 -g tss -d /dev/null -s /sbin/nologin \
 -c "Account used by tpm2-abrmd package to sandbox the tpm2-abrmd daemon" tss
exit 0

%files
%doc README.md CHANGELOG.md
%license LICENSE
%{_libdir}/libtcti-tabrmd.so.*
%{_sbindir}/tpm2-abrmd
%config(noreplace) %{_sysconfdir}/dbus-1/system.d/tpm2-abrmd.conf
%{_unitdir}/tpm2-abrmd.service
%{_mandir}/man3/tss2_tcti_tabrmd_init.3.gz
%{_mandir}/man3/tss2_tcti_tabrmd_init_full.3.gz
%{_mandir}/man7/tcti-tabrmd.7.gz
%{_mandir}/man8/tpm2-abrmd.8.gz


%package devel
Summary: Headers, static libraries and package config files of tpm2-abrmd 
Requires: %{name}%{_isa} = %{version}-%{release}
# tpm2-abrmd-devel depends on tpm2-tss-devel for sapi/tcti-device/tcti-socket libs
Requires: tpm2-tss-devel%{?_isa} >= 1.4.0-1%{?dist}

%description devel
This package contains headers, static libraries and package config files 
required to build applications that use tpm2-abrmd.

%files devel
%{_includedir}/tcti/tcti-tabrmd.h
%{_libdir}/libtcti-tabrmd.so
%{_libdir}/pkgconfig/tcti-tabrmd.pc

# on package installation
%post
/sbin/ldconfig
%systemd_post tpm2-abrmd.service

%preun
%systemd_preun tpm2-abrmd.service

%postun
/sbin/ldconfig
%systemd_postun tpm2-abrmd.service

%changelog
* Wed Mar 25 2019 Jerry Snitselaar <jsnitsel@redhat.com> - 1.1.0-11
- Fix Requires to be against tpm2-tss instead of tpm2-tss-devel
- Add BuildRequires to specify version tpm2-tss-devel needed
- Add Requires to tpm2-abrmd-devel for tpm2-tss-devel
resolves: rhbz#1627827

* Thu Sep 06 2018 Jerry Snitselaar <jsnitsel@redhat,com> - 1.1.0-10
- update tpm2-tss-devel requirement to 1.4.0-1
resolves: rhbz#1626227

* Mon Jun 18 2018 Jerry Snitselaar <jsnitsel@redhat.com> - 1.1.0-9
- udev rules moved to tpm2-tss package.
resolves: rhbz#1592583

* Thu Dec 14 2017 Jerry Snitselaar <jsnitsel@redhat.com> - 1.1.0-8
- Fix package version used by autoconf
resolves: rhbz#1492466

* Wed Oct 18 2017 Jerry Snitselaar <jsnitsel@redhat.com> - 1.1.0-7
- tcti-abrmd: Fix null deref
resolves: rhbz#1492466

* Wed Oct 11 2017 Jerry Snitselaar <jsnitsel@redhat.com> - 1.1.0-6
- Add scriptlet to add tss user if doesn't exist.
resolves: rhbz#1492466

* Wed Aug 16 2017 Sun Yunying <yunying.sun@intel.com> - 1.1.0-5
- Updated source0 URL to fix rpmlint warnings

* Tue Aug 15 2017 Sun Yunying <yunying.sun@intel.com> - 1.1.0-4
- Rename and relocate udev rules file to _udevrulesdir
- Update scriptlet to add service name after systemd_postrun

* Tue Aug 1 2017 Sun Yunying <yunying.sun@intel.com> - 1.1.0-3
- Use config option with-systemdsystemunitdir to set systemd unit file location

* Mon Jul 31 2017 Sun Yunying <yunying.sun@intel.com> - 1.1.0-2
- Removed BuildRequires for gcc
- Move tpm2-abrmd systemd service to /usr/lib/systemd/system
- Added scriptlet for tpm2-abrmd systemd service
- Use autoreconf instead of bootstrap

* Wed Jul 26 2017 Sun Yunying <yunying.sun@intel.com> - 1.1.0-1
- Initial packaging
