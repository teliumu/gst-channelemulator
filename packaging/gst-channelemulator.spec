Name:       gst-channelemulator
Version:    0.2.0
Summary:    GStreamer Channel Emulator Plugin
Release:    1
Group:      Multimedia/Framework
Url:        http://gstreamer.freedesktop.org/
License:    LGPL-2.0+
Source0:    %{name}-%{version}.tar.gz
BuildRequires:  pkgconfig(gstreamer-1.0)

%description
GStreamer Channel Emulator Plugin is Gstreamer 1.0 plugin for simulating
network environment like packets drop, duplication, delay, etc.

%prep
%setup -q

%build
./autogen.sh

make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install
mkdir -p %{buildroot}/usr/share/license
cp LICENSE %{buildroot}/usr/share/license/%{name}

%files
%manifest gst-channelemulator.manifest
%defattr(-,root,root,-)
/usr/lib/gstreamer-1.0/*
/usr/share/license/%{name}
