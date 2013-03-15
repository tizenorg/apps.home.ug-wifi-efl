CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

ADD_DEFINITIONS("-DPREFIX=\"${CMAKE_INSTALL_PREFIX}\"")
SET(ENGINE_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/sources/libraries/WlanManager/EngineManager/connman-request.c
	${CMAKE_CURRENT_SOURCE_DIR}/sources/libraries/WlanManager/EngineManager/connman-response.c
)

SET(ENGINE_INCLUDES
	${CMAKE_CURRENT_SOURCE_DIR}/sources/libraries/WlanManager/EngineManager/include)

SET(ENGINE_PKGCONFIGS
	capi-network-wifi
	network)

INCLUDE(FindPkgConfig)
pkg_check_modules(pkgs REQUIRED
	ecore
	ecore-imf
	ecore-input
	appcore-efl
	elementary
	glib-2.0
	utilX
	ui-gadget-1
	x11
	sensor
	syspopup
	syspopup-caller
#engine pkgconfigs
	${ENGINE_PKGCONFIGS})

INCLUDE_DIRECTORIES(
	${CMAKE_CURRENT_SOURCE_DIR}/sources/ui-gadget/include
	${CMAKE_CURRENT_SOURCE_DIR}/sources/ui-gadget/viewers-layout/include
	${CMAKE_CURRENT_SOURCE_DIR}/sources/wifi-syspopup/include
	${CMAKE_CURRENT_SOURCE_DIR}/sources/wifi-syspopup/viewer-popups/include
	${CMAKE_CURRENT_SOURCE_DIR}/sources/libraries/Common/include
	${CMAKE_CURRENT_SOURCE_DIR}/sources/libraries/WlanManager/WlanManager/include
	${CMAKE_CURRENT_SOURCE_DIR}/sources/libraries/WlanManager/EngineManager/include
	${CMAKE_CURRENT_SOURCE_DIR}/sources/libraries/appcoreWrapper/include
	${CMAKE_CURRENT_SOURCE_DIR}/sources/libraries/i18nManager/include
#engine includes
	${ENGINE_INCLUDES})

ADD_SUBDIRECTORY(po)
ADD_SUBDIRECTORY(sources/ui-gadget)
ADD_SUBDIRECTORY(sources/wifi-syspopup)
