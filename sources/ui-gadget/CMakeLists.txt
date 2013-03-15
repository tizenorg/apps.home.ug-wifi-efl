CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(ug-wifi-efl-UG C)

# defines
SET(PREFIX ${CMAKE_INSTALL_PREFIX})
SET(EXEC_PREFIX "\${prefix}")
SET(LIBDIR "\${prefix}/lib")
SET(INCLUDEDIR "\${prefix}/include")
SET(VERSION 0.1.0)

# source files
SET(SRCS
	../libraries/i18nManager/i18nmanager.c
	../libraries/appcoreWrapper/appcoreWrapper.c
	../libraries/WlanManager/WlanManager/wlan_manager.c
	../libraries/WlanManager/WlanManager/wlan_connection.c
	../libraries/Common/common_utils.c
	../libraries/Common/common_ip_info.c
	../libraries/Common/common_pswd_popup.c
	../libraries/Common/common_eap_connect.c
	${ENGINE_SOURCES}
	wifi-efl-UG.c
	viewers-layout/viewer_manager.c
	viewers-layout/wifi_viewer_list.c
	viewers-layout/view_detail.c
	viewers-layout/view_ime_hidden.c
	wifi-wlan-callbacks.c
	motion_control.c
	winset_popup.c)

FOREACH(flag ${pkgs_CFLAGS})
	SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS}")

# add library
ADD_LIBRARY(${PROJECT_NAME} SHARED ${SRCS})
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES SOVERSION 0.1.0)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${pkgs_LDFLAGS})

ADD_CUSTOM_TARGET(custom_editfield.edj
	COMMAND edje_cc -id ${CMAKE_CURRENT_SOURCE_DIR}/../../resources/images
		${CMAKE_CURRENT_SOURCE_DIR}/edcs/custom_editfield.edc
		${CMAKE_CURRENT_SOURCE_DIR}/edcs/custom_editfield.edj
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/edcs/custom_editfield.edc
)

ADD_DEPENDENCIES(${PROJECT_NAME} custom_editfield.edj)

INSTALL(TARGETS ${PROJECT_NAME} DESTINATION /usr/ug/lib)

# install image files
INSTALL(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../resources/images/ DESTINATION /usr/ug/res/images/wifi-efl-UG FILES_MATCHING PATTERN "*.png")
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/edcs/custom_editfield.edj DESTINATION /usr/ug/res/edje/wifi-efl-UG)
