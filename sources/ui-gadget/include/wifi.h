/*
 * Copyright (c) 2000-2012 Samsung Electronics Co., Ltd All Rights Reserved.
 *
 * This file is part of Wi-Fi UG
 * Written by Sanghoon Cho <sanghoon80.cho@samsung.com>
 *
 * PROPRIETARY/CONFIDENTIAL
 *
 * This software is the confidential and proprietary information of
 * SAMSUNG ELECTRONICS ("Confidential Information").
 * You shall not disclose such Confidential Information and shall
 * use it only in accordance with the terms of the license agreement
 * you entered into with SAMSUNG ELECTRONICS.
 *
 * SAMSUNG make no representations or warranties about the suitability
 * of the software, either express or implied, including but not limited
 * to the implied warranties of merchantability, fitness for a particular
 * purpose, or non-infringement. SAMSUNG shall not be liable for
 * any damages suffered by licensee as a result of using, modifying or
 * distributing this software or its derivatives.
 *
 */



#ifndef __NEW_WIFI_H_
#define __NEW_WIFI_H_

#define TARGET

#ifdef __cplusplus
extern "C"
{
#endif


#include "common.h"
#include "wlan_manager.h"
#include "viewer_manager.h"

#define PACKAGE "ug-wifi-efl-UG"
#define LOCALEDIR "/opt/ug/res/locale"

#define WIFI_APP_IMAGE_DIR	FACTORYFS"/res/images/wifi-efl-UG"
#define WIFI_APP_ICON_PATH_SCAN	WIFI_APP_IMAGE_DIR"/01_controlbar_icon_update.png"
#define WIFI_APP_ICON_PATH_DONE	WIFI_APP_IMAGE_DIR"/01_controlbar_icon_edit.png"
#define WIFI_APP_ICON_PATH_FORGET	WIFI_APP_IMAGE_DIR"/01_controlbar_icon_delete.png"

#define WIFI_UG_FAKE_ICON_PATH FACTORYFS"/res/edje/wifi-efl-UG/wifi_ug_edj_etc.edj"

#define UG_MAIN_MESSAGE_DESTROY 1

typedef enum {
	VIEW_MAIN=0,
	VIEW_PASSWORD,
	VIEW_DETAIL,
	VIEW_HIDDEN_AP,
	VIEW_STATIC_IP,
	VIEW_DHCP_IP
} VIEW_TYPE;


struct wifi_appdata {
	/* ui gadget object */
	void* gadget;
	struct ui_gadget* ug;
	Eina_Bool bundle_back_button_show_force_when_connected;

	//Basic Evas_Objects
	Evas_Object *win_main;
	Evas *evas;

	VIEW_TYPE current_view;
	Eina_Bool bAlive;
};

int wifi_exit();

#ifdef __cplusplus
}
#endif

#endif /* __WIFI_H_ */


