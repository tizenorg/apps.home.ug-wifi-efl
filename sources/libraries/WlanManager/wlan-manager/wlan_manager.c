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



#include <syspopup_caller.h>
#include "common.h"
#include "wlan_manager.h"
#include "wifi-setting.h"


int ts_temp, ts_first, ts_middle, ts_last; 
struct timeval ex_tv; 

static wlan_manager_object* manager_object = NULL;

static int _power_on(void);


wlan_manager_object* wlan_manager_get_singleton(void)
{
	if (NULL == manager_object) {
		manager_object = (wlan_manager_object *)g_malloc0(sizeof(wlan_manager_object));
		manager_object->message_func = NULL;
		manager_object->refresh_func = NULL;
	}

	return manager_object;
}

int wlan_manager_connect_with_profile(const char *profile_name)
{
	if (profile_name == NULL) {
		return WLAN_MANAGER_ERR_INVALID_PARAM;
	}

	return connman_request_connection_open(profile_name);
}

int wlan_manager_connect_with_password(const char *profile_name, int security_mode, void *authdata)
{
	if (profile_name == NULL) {
		ERROR_LOG(UG_NAME_SCAN, "profile name is NULL");
		return WLAN_MANAGER_ERR_INVALID_PARAM;
	}

	int ret = wlan_manager_request_profile_add(profile_name, security_mode, authdata);
	if (ret == WLAN_MANAGER_ERR_NONE) {
		ret = connman_request_connection_open(profile_name);
		if (ret == WLAN_MANAGER_ERR_NONE) {
			INFO_LOG(UG_NAME_RESP, "====  open_connection() request Success for profile [%s]", profile_name);
			__COMMON_FUNC_EXIT__;
			return WLAN_MANAGER_ERR_NONE;
		} else {
			ERROR_LOG(UG_NAME_RESP, "====  open_connection() request Failed for profile [%s]", profile_name);
			__COMMON_FUNC_EXIT__;
			return WLAN_MANAGER_ERR_UNKNOWN;
		}
	} else {
		__COMMON_FUNC_EXIT__;
		return WLAN_MANAGER_ERR_UNKNOWN;
	}
}

int wlan_manager_disconnect(void* data)
{
	__COMMON_FUNC_ENTER__;

	if (data == NULL) {
		__COMMON_FUNC_EXIT__;
		return FALSE;
	}

	wifi_device_info_t *di_s = (wifi_device_info_t *)data;

	INFO_LOG(UG_NAME_RESP, "Closing connection with profile name [%s]", di_s->profile_name);

	int ret = connman_request_connection_close(di_s->profile_name);
	if (ret != WLAN_MANAGER_ERR_NONE) {
		ERROR_LOG(UG_NAME_RESP, "Error..!!! Closing connection. Error Reason [%d]", ret);
		__COMMON_FUNC_EXIT__;
		return WLAN_MANAGER_ERR_UNKNOWN;
	} else {
		__COMMON_FUNC_EXIT__;
		return WLAN_MANAGER_ERR_NONE;
	}
}

int wlan_manager_forget(const char *profile_name)
{
	if (profile_name == NULL)
		return 0;

	int favourite = 0;
	int ret = connman_profile_manager_check_favourite(profile_name, &favourite);
	if (ret != WLAN_MANAGER_ERR_NONE)
		return 0;

	if (favourite) {
		INFO_LOG(UG_NAME_RESP, "favourite AP\n");
		connman_request_delete_profile(profile_name);
	} else {
		INFO_LOG(UG_NAME_RESP, "No favourite AP\n");
	}

	return 1;
}

void wlan_manager_set_refresh_callback(wlan_manager_ui_refresh_func_t func)
{
	wlan_manager_object* manager_object = wlan_manager_get_singleton();
	manager_object->refresh_func = func;
}

void wlan_manager_set_message_callback(void *func)
{
	wlan_manager_object* manager_object = wlan_manager_get_singleton();
	manager_object->message_func = func;
}

void* wlan_manager_create()
{
	wlan_manager_get_singleton();
	return NULL;
}

int wlan_manager_destroy()
{
	int ret = connman_request_deregister();
	if (ret == WLAN_MANAGER_ERR_NONE) {
		connman_profile_manager_destroy();
	}
	if (manager_object) {
		g_free(manager_object);
		manager_object = NULL;
	}
	return ret;
}

int wlan_manager_start()
{
	__COMMON_FUNC_ENTER__;

	int ret = WLAN_MANAGER_ERR_NONE;
	char profile_name[NET_PROFILE_NAME_LEN_MAX+1] = "";

	ret = connman_request_register();
	if (ret != WLAN_MANAGER_ERR_NONE) {
		ERROR_LOG(UG_NAME_ERR, "[Failed] Register");
		return ret;
	}

	int state = wlan_manager_state_get(profile_name);
	switch (state) {
	case WLAN_MANAGER_OFF:
		INFO_LOG(UG_NAME_NORMAL, "WIFI_OFF\n");
		break;
	case WLAN_MANAGER_UNCONNECTED:
		INFO_LOG(UG_NAME_NORMAL, "WIFI_ON\n");

		ret = wlan_manager_request_scan();
		if (ret == WLAN_MANAGER_ERR_NONE) {
			INFO_LOG(UG_NAME_NORMAL, "Scan [OK]");
		} else {
			ERROR_LOG(UG_NAME_NORMAL, "Scan [FAIL]");
		}
		break;
	case WLAN_MANAGER_CONNECTED:
		INFO_LOG(UG_NAME_NORMAL, "WIFI_CONNECTED\n");

		if (!connman_profile_manager_connected_ssid_set((const char *)profile_name))
			ERROR_LOG(UG_NAME_NORMAL, "Fail to get profile_name of connected AP\n");

		ret = wlan_manager_request_scan();
		if (ret == WLAN_MANAGER_ERR_NONE) {
			INFO_LOG(UG_NAME_NORMAL, "Scan [OK]");
		} else {
			ERROR_LOG(UG_NAME_NORMAL, "Scan [FAIL]");
		}
		break;
	case WLAN_MANAGER_CONNECTING:
		INFO_LOG(UG_NAME_NORMAL, "WIFI_CONNECTING\n");

		ret = wlan_manager_request_scan();
		if (ret == WLAN_MANAGER_ERR_NONE) {
			INFO_LOG(UG_NAME_NORMAL, "Scan [OK]");
		} else {
			ERROR_LOG(UG_NAME_NORMAL, "Scan [FAIL]");
		}
		break;
	default:
		INFO_LOG(UG_NAME_NORMAL, "WIFI STATE ERROR\n");
	}
	
	return WLAN_MANAGER_ERR_NONE;
}

int wlan_manager_state_get(char* profile_name)
{
	return connman_request_state_get(profile_name);
}

int wlan_manager_request_power_on(void)
{
	__COMMON_FUNC_ENTER__;
	INFO_LOG(UG_NAME_REQ, "power on\n");
	
	int ret =  _power_on();

	__COMMON_FUNC_EXIT__;
	return ret;
}

int wlan_manager_request_power_off()
{
	return connman_request_power_off();
}

int wlan_manager_request_cancel_wps_connection(const char *profile_name)
{
	int ret = connman_request_connection_close(profile_name);
	return ret;
}

int wlan_manager_request_wps_connection(const char *profile_name)
{
	int ret = connman_request_wps_connection(profile_name);
	return ret;
}

int wlan_manager_request_connection(void* data)
{
	__COMMON_FUNC_ENTER__;

	wifi_device_info_t *device_info = (wifi_device_info_t*)data;

	int favourite;
	int ret = connman_profile_manager_check_favourite(device_info->profile_name, &favourite);
	if (ret == WLAN_MANAGER_ERR_NONE) {
		if (favourite) {
			INFO_LOG(UG_NAME_REQ, "existed the profile");
			return wlan_manager_connect_with_profile(device_info->profile_name);
		} else {
			INFO_LOG(UG_NAME_REQ, "no existed the profile");
			if (device_info->security_mode == WLAN_SEC_MODE_NONE) {
				return wlan_manager_connect_with_profile(device_info->profile_name);
			} else if (device_info->security_mode == WLAN_SEC_MODE_IEEE8021X) {
				return WLAN_MANAGER_ERR_CONNECT_EAP_SEC_TYPE;
			} else {
				return WLAN_MANAGER_ERR_CONNECT_PASSWORD_NEEDED;
			}
		}
	} else {
		return WLAN_MANAGER_ERR_NOSERVICE;
	}
}

int wlan_manager_request_disconnection(void* data)
{
	__COMMON_FUNC_ENTER__;
	INFO_LOG(UG_NAME_REQ, "request disconnection");

	if ((wlan_manager_disconnect(data)) != WLAN_MANAGER_ERR_NONE) {
		return WLAN_MANAGER_ERR_UNKNOWN;
	}

	return WLAN_MANAGER_ERR_NONE;
}

int wlan_manager_request_cancel_connecting(const char *profile_name)
{
	return connman_request_connection_close(profile_name);
}

int wlan_manager_request_scan(void)
{
	if (WLAN_MANAGER_ERR_NONE == connman_request_scan()) {
		/* Since the scan request was success, lets reset the ui refresh and scan update blocked flags. */
		manager_object->b_scan_blocked = FALSE;
		manager_object->b_ui_refresh = FALSE;
		return WLAN_MANAGER_ERR_NONE;
	} else {
		return WLAN_MANAGER_ERR_UNKNOWN;
	}
}

int wlan_manager_request_profile_add(const char *profile_name, int security_mode, void *authdata)
{
	wlan_manager_password_data* auth = (wlan_manager_password_data*) authdata;
	if (auth != NULL) {
		INFO_LOG(UG_NAME_SCAN, "name [%s]", auth->username);
		INFO_LOG(UG_NAME_SCAN, "name [%s]", auth->userpassword);
	}

	if (security_mode != WLAN_SEC_MODE_NONE) {
		if (connman_profile_manager_profile_modify_auth(profile_name, authdata, security_mode)) {
			DEBUG_LOG(COMMON_NAME_LIB, "Success - profile modify");
			return WLAN_MANAGER_ERR_NONE;
		}
		else {
			DEBUG_LOG(COMMON_NAME_LIB, "Failed - profile modify");
			return WLAN_MANAGER_ERR_UNKNOWN;
		}
	}

	return WLAN_MANAGER_ERR_UNKNOWN;
}

void* wlan_manager_profile_table_get()
{
	return connman_profile_manager_profile_table_get();
}

int wlan_manager_profile_modify_by_device_info(net_profile_info_t *profile)
{
	if (connman_profile_manager_profile_modify(profile)) {
		DEBUG_LOG(COMMON_NAME_LIB, "Success - profile modify");
		return WLAN_MANAGER_ERR_NONE;
	}
	else {
		DEBUG_LOG(COMMON_NAME_LIB, "Failed - profile modify");
		return WLAN_MANAGER_ERR_UNKNOWN;
	}
}

int wlan_manager_scanned_profile_refresh_with_count(int count)
{
	__COMMON_FUNC_ENTER__;

	int ret = connman_profile_manager_profile_cache(count);
	if (ret > 0) {
		DEBUG_LOG(COMMON_NAME_LIB, "success %d profiles update", count);
		manager_object->refresh_func();
	}

	__COMMON_FUNC_EXIT__;

	/* the return value should be FALSE - ecore_idler_callback function */
	return FALSE;
}

void wlan_manager_scanned_profile_refresh(void)
{
	__COMMON_FUNC_ENTER__;

	if (FALSE == manager_object->b_scan_blocked) {
		connman_profile_manager_profile_cache(0);
		manager_object->refresh_func();
		DEBUG_LOG(COMMON_NAME_LIB, "success profiles update");
	} else {
		manager_object->b_ui_refresh = TRUE;
		DEBUG_LOG(COMMON_NAME_LIB, "Scan update is blocked");
	}
	__COMMON_FUNC_EXIT__;
}

STRENGTH_TYPES wlan_manager_get_signal_strength(int rssi)
{
	/* Wi-Fi Signal Strength Display (dB / ConnMan normalized value)
	 *
	 * Excellent :	-63 ~		/ 57 ~
	 * Good:		-74 ~ -64	/ 46 ~ 56
	 * Weak:		-82 ~ -75	/ 38 ~ 45
	 * Very weak:		~ -83	/    ~ 37
	 */
	if (rssi >= 57)
		return SIGNAL_STRENGTH_TYPE_EXCELLENT;
	else if (rssi >= 46)
		return SIGNAL_STRENGTH_TYPE_GOOD;
	else if (rssi >= 38)
		return SIGNAL_STRENGTH_TYPE_WEAK;
	else
		return SIGNAL_STRENGTH_TYPE_VERY_WEAK;
}

void* wlan_manager_profile_device_info_blank_create()
{
	__COMMON_FUNC_ENTER__;
	wifi_device_info_t *di_s0 = NULL;
	di_s0 = g_malloc0(sizeof(wifi_device_info_t));

	if (di_s0 == NULL) {
		ERROR_LOG(UG_NAME_NORMAL, "Error!!! Failed to allocate memory\n");
		__COMMON_FUNC_EXIT__;
		return NULL;
	}

	char No_AP_found[] = "No AP found";

	di_s0->ssid = g_strdup(No_AP_found);
	if (NULL == di_s0->ssid) {
		g_free(di_s0);
		di_s0 = NULL;
		ERROR_LOG(UG_NAME_NORMAL, "Error!!! Failed to allocate memory\n");
	}

	__COMMON_FUNC_EXIT__;
	return (void*) di_s0;
}

int wlan_manager_profile_scanned_length_get()
{
	return connman_profile_manager_scanned_profile_table_size_get();
}

static int _power_on(void)
{
	int mobile_hotspot = 0;
	int ret = 0;
	
	mobile_hotspot = wifi_setting_value_get("memory/mobile_hotspot/mode");
	switch(mobile_hotspot){
		case VCONFKEY_MOBILE_HOTSPOT_MODE_NONE:
		case VCONFKEY_MOBILE_HOTSPOT_MODE_USB:
		case VCONFKEY_MOBILE_HOTSPOT_MODE_BT:
			INFO_LOG(COMMON_NAME_LIB, "mobile_hotspot OFF - wifi power on");
			ret = connman_request_power_on();
			return ret;
		case VCONFKEY_MOBILE_HOTSPOT_MODE_WIFI:
			INFO_LOG(COMMON_NAME_LIB, "mobile_hotspot wifi ON");
			return WLAN_MANAGER_ERR_MOBILE_HOTSPOT_OCCUPIED;
		default:
			INFO_LOG(COMMON_NAME_LIB, "mobile_hotspot status unknown");
			ret = connman_request_power_on();
			return ret;
	}
}

void wlan_manager_set_connected_AP(const net_profile_info_t *profile)
{
	wlan_manager_object* obj = wlan_manager_get_singleton();

	g_free(obj->connected_AP.profile_name);
	obj->connected_AP.profile_name = strdup(profile->ProfileName);
	g_free(obj->connected_AP.ssid);
	obj->connected_AP.ssid = strdup(profile->ProfileInfo.Wlan.essid);

	INFO_LOG(UG_NAME_REQ, "Profile ESSID = [%s]\n", obj->connected_AP.ssid);
}

void wlan_manager_reset_connected_AP(void)
{
	wlan_manager_object* obj = wlan_manager_get_singleton();

	g_free(obj->connected_AP.profile_name);
	obj->connected_AP.profile_name = NULL;
	g_free(obj->connected_AP.ssid);
	obj->connected_AP.ssid = NULL;

	INFO_LOG(UG_NAME_REQ, "clear connected AP information\n");
}

static Eina_Bool _refresh_ui(void *data)
{
	connman_profile_manager_profile_cache(0);
	manager_object->refresh_func();
	manager_object->b_scan_blocked = FALSE;
	manager_object->b_ui_refresh = FALSE;
	return ECORE_CALLBACK_CANCEL;
}

void wlan_manager_enable_scan_result_update(void)
{
	if (TRUE == manager_object->b_scan_blocked) {
		if (TRUE == manager_object->b_ui_refresh) {
			DEBUG_LOG(COMMON_NAME_LIB, "Refresh the UI with last scan update");

			/* We delay the rendering inorder to get smooth effect of popup close */
			ecore_idler_add(_refresh_ui, NULL);
		} else {
			manager_object->b_scan_blocked = FALSE;
		}
	}
}

void wlan_manager_disable_scan_result_update(void)
{
	manager_object->b_scan_blocked = TRUE;
}

const char *wlan_manager_get_connected_profile(void)
{
	wlan_manager_object* obj = wlan_manager_get_singleton();

	return (const char *)obj->connected_AP.profile_name;
}

const char *wlan_manager_get_connected_ssid(void)
{
	wlan_manager_object* obj = wlan_manager_get_singleton();

	return (const char *)obj->connected_AP.ssid;
}

int wlan_manager_network_syspopup_message(const char *title, const char *content, const char *type)
{
	int ret = 0;
	bundle *b = bundle_create();

	bundle_add(b, "_SYSPOPUP_TITLE_", title);
	bundle_add(b, "_SYSPOPUP_CONTENT_", content);
	bundle_add(b, "_SYSPOPUP_TYPE_", type);

	ret = syspopup_launch("net-popup", b);
	bundle_free(b);

	return ret;
}
