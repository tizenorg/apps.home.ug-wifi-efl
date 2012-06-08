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

