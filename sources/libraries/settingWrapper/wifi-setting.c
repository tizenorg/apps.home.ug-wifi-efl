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



#include <vconf.h>
#include "wifi-setting.h"
#include "common.h"

int wifi_setting_value_set(const char *key, int value) 
{
	__COMMON_FUNC_ENTER__;

	INFO_LOG(UG_NAME_NORMAL, "setting value : %d\n", value);
	
	if (vconf_set_int(key, value) < 0) {
		ERROR_LOG(UG_NAME_NORMAL, "Failed to set vconf\n");
		__COMMON_FUNC_EXIT__;
		return -1;
	}

	__COMMON_FUNC_EXIT__;
	return 0;
}

int wifi_setting_value_get(const char *key) 
{
	__COMMON_FUNC_ENTER__;
	
	int value = 0;
	if (vconf_get_int(key, &value) < 0) {
		ERROR_LOG(UG_NAME_NORMAL, "Failed to get vconf\n");
		__COMMON_FUNC_EXIT__;
		return -1;
	}

	__COMMON_FUNC_EXIT__;
	return value;
}

static void wifi_flight_mode_changed(keynode_t* node, void* user_data)
{
	INFO_LOG(UG_NAME_NORMAL, "Airplane mode [%s] \n", node);
	return;
}

static void mobile_hotspot_status(keynode_t* node, void* user_data)
{
	INFO_LOG(UG_NAME_NORMAL, "MobileAP mode [%s] \n", node);
}

int wifi_setting_key_notify_set() 
{
	vconf_notify_key_changed(VCONFKEY_SETAPPL_FLIGHT_MODE_BOOL, (vconf_callback_fn) wifi_flight_mode_changed, NULL);
	vconf_notify_key_changed(VCONFKEY_MOBILE_HOTSPOT_MODE, (vconf_callback_fn) mobile_hotspot_status, NULL);

	return TRUE;
}

