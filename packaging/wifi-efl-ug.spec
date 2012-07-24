#sbs-git:slp/pkgs/s/sg-wifi-efl wifi-efl-ug

Name:       wifi-efl-ug
Summary:    Wi-Fi UI Gadget
Version:    0.4.25
Release:    1
Group:      App/Network
License:    Flora Software License
Source0:    %{name}-%{version}.tar.gz
Source1001: packaging/wifi-efl-ug.manifest 

BuildRequires: cmake
BuildRequires: gettext-tools
BuildRequires: edje-tools
BuildRequires: pkgconfig(ecore)
BuildRequires: pkgconfig(ecore-imf)
BuildRequires: pkgconfig(ecore-input)
BuildRequires: pkgconfig(appcore-efl)
BuildRequires: pkgconfig(elementary)
BuildRequires: pkgconfig(glib-2.0)
BuildRequires: pkgconfig(utilX)
BuildRequires: pkgconfig(UI-idlecapture)
BuildRequires: pkgconfig(ui-gadget)
BuildRequires: pkgconfig(x11)
BuildRequires: pkgconfig(sensor)
BuildRequires: pkgconfig(syspopup)
BuildRequires: pkgconfig(syspopup-caller)
BuildRequires: pkgconfig(network)

Requires(post):   /sbin/ldconfig
Requires(post):   /usr/bin/vconftool
requires(postun): /sbin/ldconfig

%description
Wi-Fi UI Gadget


%package -n net.wifi-qs
Summary:    Wi-Fi System popup
Requires(post):   /usr/bin/vconftool
Requires:   %{name} = %{version}-%{release}

%description -n net.wifi-qs
Wi-Fi System popup

%prep
%setup -q

%define PREFIX /opt/ug

%build
cp %{SOURCE1001} .
export LDFLAGS+="-Wl,--rpath=$PREFIX/lib -Wl,--as-needed"
LDFLAGS="$LDFLAGS"
cmake . -DCMAKE_INSTALL_PREFIX=$PREFIX
make %{?jobs:-j%jobs}

%install
%make_install

%post
/sbin/ldconfig

vconftool set -t int memory/wifi/ug_run_state 3 -i -g 6519

%post -n net.wifi-qs

vconftool set -t int memory/wifi/wifi_qs_exit 0 -g 6519 -i
vconftool set -t int db/wifi/enable_quick_start 1 -g 6519 -i


%postun -p /sbin/ldconfig

%files
%manifest wifi-efl-ug.manifest
%defattr(-,root,root,-)
/opt/ug/lib/libug-wifi-efl-UG.so
/opt/ug/lib/libug-wifi-efl-UG.so.0.1.0
/opt/ug/res/edje/wifi-efl-UG/*.edj
/opt/ug/res/images/wifi-efl-UG/*.png
/opt/ug/res/locale/*/LC_MESSAGES/*.mo

%files -n net.wifi-qs
%manifest wifi-efl-ug.manifest
%defattr(-,root,root,-)
%{_prefix}/bin/wifi-qs
/opt/share/applications/net.wifi-qs.desktop
%{_prefix}/share/process-info/wifi-qs.ini
%{_prefix}/share/icon/*.png
%{_prefix}/share/locale/*/LC_MESSAGES/wifi-qs.mo
