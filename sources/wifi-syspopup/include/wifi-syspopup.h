/*
*  Wi-Fi syspopup
*
* Copyright 2012  Samsung Electronics Co., Ltd

* Licensed under the Flora License, Version 1.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at

* http://www.tizenopensource.org/license

* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
*/



#ifndef __WIFI_SYSPOPUP_H__
#define __WIFI_SYSPOPUP_H__


#include "common.h"
#include "common_pswd_popup.h"
#include "wlan_manager.h"

#define PACKAGE "wifi-qs"
#define LOCALEDIR "/usr/share/locale"
#define WIFI_SP_ICON_PATH "/usr/share/icon"

typedef enum {
	WIFI_SYSPOPUP_SUPPORT_NONE =0,
	WIFI_SYSPOPUP_SUPPORT_QUICKPANEL=1,
	WIFI_SYSPOPUP_SUPPORT_MAX
} WIFI_SYSPOPUP_SUPPORTS;

typedef enum {
	WIFI_SYSPOPUP_WITH_AP_LIST = 0,
	WIFI_SYSPOPUP_WITHOUT_AP_LIST
} WIFI_SYSPOPUP_TYPE;

#define MAX_PROFILE_NUM NETPM_PROFILES_PERSISTENT_MAX

typedef struct wifi_object {
	/* wifi object attributes */
	WIFI_SYSPOPUP_SUPPORTS wifi_syspopup_support;

	/* connection_result */
	int connection_result;

	Eina_Bool update_enabled;

	/* caller type */
	WIFI_SYSPOPUP_TYPE syspopup_type;

	/* window */
	Evas_Object* win_main;
	Evas* evas;
	bundle* b;

	/* popups */
	Evas_Object* syspopup;
	pswd_popup_t *passpopup;
	Evas_Object* alertpopup; 

} wifi_object;

typedef enum {
	ITEM_CONNECTION_MODE_NULL,
	ITEM_CONNECTION_MODE_OFF,
	ITEM_CONNECTION_MODE_CONNECTING,
	ITEM_CONNECTION_MODE_MAX
} ITEM_CONNECTION_MODES;

typedef struct {
	Elm_Object_Item *it;
	ITEM_CONNECTION_MODES connection_mode;
	wifi_device_info_t *dev_info;
} syspopup_genlist_data_t;

int wifi_syspopup_create(void);
int wifi_syspopup_destroy(void);

#endif
