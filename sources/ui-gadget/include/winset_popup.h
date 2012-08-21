/*
*  Wi-Fi UG
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



#ifndef __DEF_POPUP_H_
#define __DEF_POPUP_H_

typedef enum {
	POPUP_OPTION_NONE = 0X01,
	POPUP_OPTION_REGISTER_FAILED_COMMUNICATION_FAILED,
	POPUP_OPTION_REGISTER_FAILED_UNKNOWN,
	POPUP_OPTION_POWER_ON_FAILED_MOBILE_HOTSPOT,
	POPUP_OPTION_ETC_WLAN_STATE_GET_ERROR,
	POPUP_OPTION_CONNECTING_FAILED,
	POPUP_OPTION_HIDDEN_AP_SSID_LEN_ERROR,
	POPUP_OPTION_UNREG_WLAN_EVENT_ERROR,
	POPUP_OPTION_WEP_PSWD_LEN_ERROR,
	POPUP_OPTION_WPA_PSWD_LEN_ERROR,
	POPUP_OPTION_MAX
} POPUP_MODE_OPTIONS;

typedef struct popup_manager_object popup_manager_object_t;

popup_manager_object_t *winset_popup_manager_create(Evas_Object* win, const char *str_pkg_name);
void winset_popup_mode_set(popup_manager_object_t *manager_object, POPUP_MODE_OPTIONS option, void *input_data);
boolean winset_popup_manager_destroy(popup_manager_object_t *manager_object);
boolean winset_popup_hide_popup(popup_manager_object_t *manager_object);

#endif
