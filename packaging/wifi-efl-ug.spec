Name:       wifi-efl-ug
Summary:    Wi-Fi UI Gadget
Version:    0.5.2_19
Release:    1
Group:      App/Network
License:    Flora License
Source0:    %{name}-%{version}.tar.gz
BuildRequires: cmake
BuildRequires: pkgconfig(ecore)
BuildRequires: pkgconfig(ecore-imf)
BuildRequires: pkgconfig(ecore-input)
BuildRequires: pkgconfig(appcore-efl)
BuildRequires: pkgconfig(elementary)
BuildRequires: pkgconfig(glib-2.0)
BuildRequires: pkgconfig(utilX)
BuildRequires: pkgconfig(ui-gadget-1)
BuildRequires: pkgconfig(x11)
BuildRequires: pkgconfig(sensor)
BuildRequires: pkgconfig(syspopup)
BuildRequires: pkgconfig(syspopup-caller)
BuildRequires: pkgconfig(capi-network-wifi)
BuildRequires: pkgconfig(network)
BuildRequires: gettext-tools
BuildRequires: edje-tools
Requires(post):   /sbin/ldconfig
Requires(post):   /usr/bin/vconftool
requires(postun): /sbin/ldconfig

%description
Wi-Fi UI Gadget

%package -n net.wifi-qs
Summary:    Wi-Fi System popup
Requires:   %{name} = %{version}
Requires(post):   /usr/bin/vconftool

%description -n net.wifi-qs
Wi-Fi System popup

%prep
%setup -q

%define PREFIX /usr/ug


%build
#LDFLAGS+="-Wl,--rpath=%{PREFIX}/lib -Wl,--as-needed"
cmake . -DCMAKE_INSTALL_PREFIX=%{PREFIX}

make %{?_smp_mflags}


%install
%make_install

mkdir -p %{buildroot}%{_sysconfdir}/smack/accesses.d/
cp -v net.wifi-qs.rule %{buildroot}%{_sysconfdir}/smack/accesses.d/

#License
mkdir -p %{buildroot}%{_datadir}/license
cp LICENSE.Flora %{buildroot}%{_datadir}/license/wifi-efl-ug
cp LICENSE.Flora %{buildroot}%{_datadir}/license/net.wifi-qs

%post
/sbin/ldconfig
mkdir -p %{PREFIX}/bin/
ln -sf /usr/bin/ug-client %{PREFIX}/bin/wifi-efl-UG

vconftool set -t int memory/wifi/ug_run_state 3 -i -g 6519 -f

%post -n net.wifi-qs
vconftool set -t int memory/wifi/wifi_qs_exit 0 -g 6519 -i -f
vconftool set -t int db/wifi/enable_quick_start 1 -g 6519 -i -f

%postun -p /sbin/ldconfig


%files
%manifest wifi-efl-ug.manifest
%{PREFIX}/lib/libug-wifi-efl-UG.so
%attr(644,-,-) %{PREFIX}/lib/libug-wifi-efl-UG.so.0.1.0
%{PREFIX}/res/edje/wifi-efl-UG/*.edj
%{PREFIX}/res/images/wifi-efl-UG/*.png
%{PREFIX}/res/locale/*/LC_MESSAGES/*.mo
%{_datadir}/license/wifi-efl-ug
/usr/share/packages/wifi-efl-ug.xml

%files -n net.wifi-qs
%manifest net.wifi-qs.manifest
%{_bindir}/wifi-qs
%{_datadir}/packages/net.wifi-qs.xml
%{_datadir}/process-info/wifi-qs.ini
%{_datadir}/icon/*.png
%{_datadir}/locale/*/LC_MESSAGES/*.mo
%{_datadir}/license/net.wifi-qs
%{_sysconfdir}/smack/accesses.d/net.wifi-qs.rule
