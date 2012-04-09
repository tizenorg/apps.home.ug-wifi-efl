/*
 * Copyright (c) 2000-2012 Samsung Electronics Co., Ltd All Rights Reserved.
 *
 * This file is part of Wi-Fi syspopup
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



#ifndef __WIFI_SYSPOPUP_H__
#define __WIFI_SYSPOPUP_H__


#include "common.h"
#include "wlan_manager.h"

#define PACKAGE "wifi-qs"
#define LOCALEDIR "/usr/share/locale"
#define WIFI_SP_ICON_PATH "/usr/share/icon"

typedef enum {
	WIFI_SYSPOPUP_SUPPORT_NONE =0,
	WIFI_SYSPOPUP_SUPPORT_QUICKPANEL=1,
	WIFI_SYSPOPUP_SUPPORT_MAX
} WIFI_SYSPOPUP_SUPPORTS;

#define MAX_PROFILE_NUM NETPM_PROFILES_PERSISTENT_MAX

typedef struct wifi_object {
	/* wifi object attributes */
	WIFI_SYSPOPUP_SUPPORTS wifi_syspopup_support;

	/* connection_result */
	int connection_result;

	/* window */
	Evas_Object* win_main;
	Evas* evas;
	bundle* b;
	Evas_Object* layout_main;

	/* popups */
	Evas_Object* syspopup;
	Evas_Object* passpopup;
	Evas_Object* alertpopup; 

} wifi_object;

typedef enum {
	ITEM_CONNECTION_MODE_NULL,
	ITEM_CONNECTION_MODE_OFF,
	ITEM_CONNECTION_MODE_CONNECTING,
	ITEM_CONNECTION_MODE_MAX
} ITEM_CONNECTION_MODES;

typedef struct _genlist_data {
	Elm_Object_Item *it;
	Evas_Object *progressbar;
	ITEM_CONNECTION_MODES connection_mode;
	wifi_device_info_t *dev_info;
} genlist_data;

int wifi_syspopup_create(void);
int wifi_syspopup_destroy(void);

#endif
