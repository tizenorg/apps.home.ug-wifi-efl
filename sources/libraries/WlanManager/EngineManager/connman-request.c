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



#include "common.h"
#include "wlan_manager.h"


int connman_request_register(void)
{
	int ret = net_register_client((net_event_cb_t) network_evt_cb, NULL);
	switch (ret) {
		case NET_ERR_NONE:
			return WLAN_MANAGER_ERR_NONE;
		case NET_ERR_APP_ALREADY_REGISTERED:
			return WLAN_MANAGER_ERR_ALREADY_REGISTERED;
		default:
			return WLAN_MANAGER_ERR_UNKNOWN;
	}
}

int connman_request_deregister(void)
{
	int ret = net_deregister_client();
	switch (ret) {
		case NET_ERR_NONE:
			return WLAN_MANAGER_ERR_NONE;
		default:
			return WLAN_MANAGER_ERR_UNKNOWN;
	}
}

int connman_request_power_on(void)
{
	int ret = NET_ERR_NONE;

	ret = net_wifi_power_on();
	INFO_LOG(COMMON_NAME_LIB,"network_wifi_power_on ret: %d", ret);

	switch (ret){
		case NET_ERR_NONE:
			return WLAN_MANAGER_ERR_NONE;
		default:
			return WLAN_MANAGER_ERR_UNKNOWN;
	}
}

int connman_request_power_off(void)
{
	int ret = NET_ERR_NONE;

	ret = net_wifi_power_off();
	INFO_LOG(COMMON_NAME_LIB, "network_wifi_power_off ret: %d", ret);

	switch (ret){
		case NET_ERR_NONE:
			return WLAN_MANAGER_ERR_NONE;
		default:
			return WLAN_MANAGER_ERR_UNKNOWN;
	}
}

int connman_request_connection_open(const char* profile_name)
{
	int ret = NET_ERR_NONE;
	ret = net_open_connection_with_profile(profile_name);	
	INFO_LOG(COMMON_NAME_LIB, "connman_request_connection_open ret: %d", ret);
	
	switch (ret){
		case NET_ERR_NONE:
			return WLAN_MANAGER_ERR_NONE;
		default:
			return WLAN_MANAGER_ERR_UNKNOWN;
	}
}

int connman_request_connection_open_hidden_ap(net_wifi_connection_info_t* conninfo)
	{
	int ret = NET_ERR_NONE;
	ret = net_open_connection_with_wifi_info(conninfo);
	switch (ret){
		case NET_ERR_NONE:
			return WLAN_MANAGER_ERR_NONE;
		default:
			return WLAN_MANAGER_ERR_UNKNOWN;
	}
}

int connman_request_connection_close(const char* profile_name)
{
	int ret = NET_ERR_NONE;
	ret = net_close_connection(profile_name);

	switch (ret) {
	case NET_ERR_NONE:
		return WLAN_MANAGER_ERR_NONE;
	default:
		return WLAN_MANAGER_ERR_UNKNOWN;
	}
}

int connman_request_delete_profile(const char* profile_name)
{
	int ret = NET_ERR_NONE;
	ret = net_delete_profile(profile_name);
	switch (ret){
		case NET_ERR_NONE:
			return WLAN_MANAGER_ERR_NONE;
		default:
			return WLAN_MANAGER_ERR_UNKNOWN;
	}
}

int connman_request_scan()
{
	int ret = NET_ERR_NONE;
	ret = net_scan_wifi();
	switch (ret){
		case NET_ERR_NONE:
			return WLAN_MANAGER_ERR_NONE;
		default:
			return WLAN_MANAGER_ERR_UNKNOWN;
	}
}

int connman_request_scan_mode_set(net_wifi_background_scan_mode_t scan_mode)
{
	int ret = NET_ERR_NONE;

	ret = net_wifi_set_background_scan_mode(scan_mode);
	INFO_LOG(COMMON_NAME_LIB, "net_wifi_set_background_scan_mode ret: %d", ret);

	switch (ret) {
	case NET_ERR_NONE:
		return WLAN_MANAGER_ERR_NONE;
	default:
		return WLAN_MANAGER_ERR_UNKNOWN;
	}
}

int connman_request_wps_connection(const char *profile_name)
{
	__COMMON_FUNC_ENTER__;

	if (profile_name == NULL) {
		ERROR_LOG(COMMON_NAME_ERR, "profile_name is NULL!!!");
		return WLAN_MANAGER_ERR_UNKNOWN;
	}

	net_wifi_wps_info_t wps_info;
	memset(&wps_info, 0, sizeof(net_wifi_wps_info_t));
	wps_info.type = WIFI_WPS_PBC;

	int ret = net_wifi_enroll_wps(profile_name, &wps_info);
	if (ret != NET_ERR_NONE) {
		ERROR_LOG(COMMON_NAME_ERR, "Failed net_wifi_enroll_wps() error - %d", ret);
		return WLAN_MANAGER_ERR_UNKNOWN;
	}

	__COMMON_FUNC_EXIT__;
	return WLAN_MANAGER_ERR_NONE;
}

int connman_request_state_get(const char* profile_name)
{
	__COMMON_FUNC_ENTER__;

	int ret = NET_ERR_NONE;
	net_wifi_state_t state;

	ret = net_get_wifi_state(&state, (net_profile_name_t *)profile_name);
	switch (ret) {
	case NET_ERR_NONE:
		if (WIFI_OFF == state) {
			INFO_LOG(COMMON_NAME_LIB, "STATE : WIFI_OFF");
			__COMMON_FUNC_EXIT__;
			return WLAN_MANAGER_OFF;
		} else if (WIFI_ON == state) {
			INFO_LOG(COMMON_NAME_LIB, "STATE : WIFI_ON");
			__COMMON_FUNC_EXIT__;
			return WLAN_MANAGER_UNCONNECTED;
		} else if (WIFI_CONNECTED == state) {
			INFO_LOG(COMMON_NAME_LIB, "STATE : WIFI_CONNECTED - %s", profile_name);
			__COMMON_FUNC_EXIT__;
			return WLAN_MANAGER_CONNECTED;
		} else if (WIFI_CONNECTING == state) {
			INFO_LOG(COMMON_NAME_LIB, "STATE : WIFI_CONNECTING - %s", profile_name);
			__COMMON_FUNC_EXIT__;
			return WLAN_MANAGER_CONNECTING;
		} else if (WIFI_DISCONNECTING == state) {
			INFO_LOG(COMMON_NAME_LIB, "STATE : WIFI_DISCONNECTING");
			__COMMON_FUNC_EXIT__;
			return WLAN_MANAGER_DISCONNECTING;
		} else {
			ERROR_LOG(COMMON_NAME_ERR, "STATE : UNKNOWN[%d]", state);
			__COMMON_FUNC_EXIT__;
			return WLAN_MANAGER_ERROR;
		}
	default:
		ERROR_LOG(COMMON_NAME_ERR, "Failed to get state [%d]", ret);
		__COMMON_FUNC_EXIT__;
		return WLAN_MANAGER_ERR_UNKNOWN;
	}
}
