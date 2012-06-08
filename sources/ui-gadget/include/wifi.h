/*
  * Copyright 2012  Samsung Electronics Co., Ltd
  *
  * Licensed under the Flora License, Version 1.0 (the "License");
  * you may not use this file except in compliance with the License.
  * You may obtain a copy of the License at
  *
  *    http://www.tizenopensource.org/license
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
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


