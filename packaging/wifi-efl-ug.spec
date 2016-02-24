%define _unpackaged_files_terminate_build 0
Name:		wifi-efl-ug
Summary:	Wi-Fi UI Gadget for TIZEN
Version:	1.0.156
Release:	1
Group:		App/Network
License:	Flora-1.1
Source0:	%{name}-%{version}.tar.gz

%if "%{?tizen_profile_name}" == "wearable"
ExcludeArch: %{arm} %ix86 x86_64
%endif

BuildRequires:	pkgconfig(ecore)
BuildRequires:	pkgconfig(ecore-imf)
BuildRequires:	pkgconfig(ecore-input)
BuildRequires:	pkgconfig(appcore-efl)
BuildRequires:	pkgconfig(elementary)
BuildRequires:	pkgconfig(glib-2.0)
BuildRequires:	pkgconfig(openssl)
BuildRequires:	pkgconfig(cert-svc-vcore)
BuildRequires:	pkgconfig(utilX)
BuildRequires:	pkgconfig(ui-gadget-1)
BuildRequires:	pkgconfig(x11)
BuildRequires:	pkgconfig(sensor)
BuildRequires:	pkgconfig(capi-network-wifi)
BuildRequires:	pkgconfig(capi-network-connection)
BuildRequires:	pkgconfig(capi-network-tethering)
BuildRequires:	pkgconfig(capi-ui-efl-util)
BuildRequires:	pkgconfig(network)
BuildRequires:	pkgconfig(feedback)
BuildRequires:	pkgconfig(efl-extension)
#BuildRequires:  pkgconfig(setting-common-internal)
#BuildRequires:  pkgconfig(setting-lite-common-internal)
BuildRequires:	cmake
BuildRequires:	gettext-tools
BuildRequires:	edje-tools
BuildRequires:	model-build-features
Requires(post):		/sbin/ldconfig
Requires(post):		/usr/bin/vconftool
requires(postun):	/sbin/ldconfig

%description
Wi-Fi UI Gadget

%package -n net.wifi-qs
Summary:    Wi-Fi System popup
Requires:   %{name} = %{version}
Requires(post):   /usr/bin/vconftool

%description -n net.wifi-qs
Wi-Fi System popup for TIZEN

%prep
%setup -q

%define PREFIX /usr/


%build
#LDFLAGS+="-Wl,--rpath=%{PREFIX}/lib -Wl,--as-needed"
cmake -DCMAKE_INSTALL_PREFIX=%{PREFIX} \
%if ! 0%{?model_build_feature_network_tethering_disable}
	-DTIZEN_TETHERING_ENABLE=1 \
%endif
	-DMODEL_BUILD_FEATURE_WLAN_CONCURRENT_MODE=1 \
	.

make %{?_smp_mflags}


%install
%make_install

mkdir -p %{buildroot}%{_sysconfdir}/smack/accesses.d/
cp -v net.wifi-qs.efl %{buildroot}%{_sysconfdir}/smack/accesses.d/

#License
mkdir -p %{buildroot}%{_datadir}/license
cp LICENSE %{buildroot}%{_datadir}/license/wifi-efl-ug
cp LICENSE %{buildroot}%{_datadir}/license/net.wifi-qs

%post
/sbin/ldconfig

mkdir -p %{PREFIX}/bin/

vconftool set -t int memory/wifi/ug_run_state 3 -i -g 6519 -s tizen::vconf::platform::rw

vconftool set -t int memory/wifi/wifi_qs_exit 0 -g 6519 -i -s tizen::vconf::platform::rw
vconftool set -t int db/wifi/enable_quick_start 1 -g 6519 -i -s tizen::vconf::setting::admin

vconftool set -t int file/private/wifi/network_bonding 0 -g 6519 -s tizen::vconf::setting::admin
vconftool set -t int file/private/wifi/sort_by 1 -g 6519 -s tizen::vconf::setting::admin

%postun -p /sbin/ldconfig


%files
%manifest wifi-efl-ug.manifest
%{PREFIX}/apps/wifi-efl-ug/lib/ug/*
%attr(644,-,-) %{PREFIX}/apps/wifi-efl-ug/lib/*
%attr(755,-,-) %{PREFIX}/apps/wifi-efl-ug/lib/ug
%{PREFIX}/apps/wifi-efl-ug/res/edje/wifi-efl-UG/*.edj
%{_datadir}/locale/*/LC_MESSAGES/*.mo
%{_datadir}/license/wifi-efl-ug
%{_datadir}/packages/wifi-efl-ug.xml
/usr/apps/wifi-efl-ug/shared/res/tables/ug-wifi-efl_ChangeableColorTable.xml
/usr/apps/wifi-efl-ug/shared/res/tables/ug-wifi-efl_FontInfoTable.xml

%files -n net.wifi-qs
%manifest net.wifi-qs.manifest
%{_bindir}/wifi-qs
%{_datadir}/packages/net.wifi-qs.xml
%{_datadir}/icons/*.png
%{PREFIX}/apps/wifi-efl-ug/res/edje/wifi-qs/*.edj
%{_datadir}/license/net.wifi-qs
%{_sysconfdir}/smack/accesses.d/net.wifi-qs.efl
