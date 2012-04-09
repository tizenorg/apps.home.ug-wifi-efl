CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(wifi-qs C)

# defines 
SET(PREFIX ${CMAKE_INSTALL_PREFIX})
SET(EXEC_PREFIX "\${prefix}")
SET(LIBDIR "\${prefix}/lib")
SET(INCLUDEDIR "\${prefix}/include")
SET(VERSION 0.1.0)

# source files
SET(SRCS 
	../libraries/settingWrapper/wifi-setting.c
	../libraries/i18nManager/i18nmanager.c
	../libraries/appcoreWrapper/appcoreWrapper.c
	../libraries/WlanManager/wlan-manager/wlan_manager.c
	${ENGINE_SOURCES}
	viewer-popups/view-main.c
	viewer-popups/view-password.c
	viewer-popups/view-alerts.c
	wifi-syspopup.c 
	wifi-syspopup-engine-callback.c)

FOREACH(flag ${pkgs_CFLAGS})
	SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS}")

ADD_DEFINITIONS("-DPREFIX=\"${CMAKE_INSTALL_PREFIX}\"")
ADD_DEFINITIONS("-DFACTORYFS=\"$ENV{FACTORYFS}\"")

# add executable
ADD_EXECUTABLE(${PROJECT_NAME} ${SRCS})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${pkgs_LDFLAGS})

INSTALL(TARGETS ${PROJECT_NAME} DESTINATION /usr/bin)

# install image files
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../resources/images/37_wifi_icon_00.png DESTINATION /usr/share/icon)
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../resources/images/37_wifi_icon_01.png DESTINATION /usr/share/icon)
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../resources/images/37_wifi_icon_02.png DESTINATION /usr/share/icon)
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../resources/images/37_wifi_icon_03.png DESTINATION /usr/share/icon)
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../resources/images/37_wifi_icon_lock_00.png DESTINATION /usr/share/icon)
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../resources/images/37_wifi_icon_lock_01.png DESTINATION /usr/share/icon)
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../resources/images/37_wifi_icon_lock_02.png DESTINATION /usr/share/icon)
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../resources/images/37_wifi_icon_lock_03.png DESTINATION /usr/share/icon)

# install desktop file & icon
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/net.${PROJECT_NAME}.desktop.in ${CMAKE_CURRENT_SOURCE_DIR}/net.${PROJECT_NAME}.desktop)
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/net.${PROJECT_NAME}.desktop DESTINATION /opt/share/applications/)

# install ini file
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}.ini DESTINATION /usr/share/process-info)

ADD_SUBDIRECTORY(po)
