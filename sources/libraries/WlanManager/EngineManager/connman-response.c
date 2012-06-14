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


void network_evt_cb(net_event_info_t* net_event, void* user_data)
{
	__COMMON_FUNC_ENTER__;

	wlan_manager_object* manager_object = wlan_manager_get_singleton();

	callback_data *ret = (callback_data *) malloc(sizeof(callback_data));
	ret->profile_name = strdup(net_event->ProfileName);
	ret->type = WLAN_MANAGER_RESPONSE_TYPE_NONE;

	switch (net_event->Event) {
		/** Wi-Fi interface Power On/Off Response Event */ 
		case NET_EVENT_WIFI_POWER_RSP:
		/** Wi-Fi interface Power On/Off Indication Event */ 
		case NET_EVENT_WIFI_POWER_IND:
			INFO_LOG(COMMON_NAME_LIB, "Callback - NET_EVENT_WIFI_POWER_IND : %d", net_event->Error);
			net_wifi_state_t* wifi_state = (net_wifi_state_t*)net_event->Data;
			if (net_event->Error == NET_ERR_NONE && net_event->Datalength == sizeof(net_wifi_state_t)) {
				if (*wifi_state == WIFI_ON) {
					ret->type = WLAN_MANAGER_RESPONSE_TYPE_POWER_ON_OK;
					wlan_manager_request_scan();
				}
				else if (*wifi_state == WIFI_OFF) {
					ret->type = WLAN_MANAGER_RESPONSE_TYPE_POWER_OFF_OK;
				}
				else {
					INFO_LOG(COMMON_NAME_LIB, "State!! - %d", *wifi_state);
				}
			} else if (net_event->Error == NET_ERR_WIFI_DRIVER_FAILURE) {
				ret->type = WLAN_MANAGER_RESPONSE_TYPE_POWER_ON_NOT_SUPPORTED;
			} else if (net_event->Error == NET_ERR_SECURITY_RESTRICTED) {
				ret->type = WLAN_MANAGER_RESPONSE_TYPE_POWER_ON_RESTRICTED;
			} else {
				return;
			}
			break;
		/** Open Connection Response Event */
		case NET_EVENT_OPEN_RSP:
			INFO_LOG(COMMON_NAME_LIB, "Callback - NET_EVENT_OPEN_RSP : %s", ret->profile_name);

			if (net_event->Error == NET_ERR_NONE) {
				if (strstr(ret->profile_name, "wifi_") == NULL) {
					INFO_LOG(COMMON_NAME_LIB, "RESULT: NOT WIFI");
					return;
				}
				INFO_LOG(COMMON_NAME_LIB, "RESULT: CONNECTION OK");
				ret->type = WLAN_MANAGER_RESPONSE_TYPE_CONNECTION_OK;		
				connman_profile_manager_connected_ssid_set((const char *)ret->profile_name);
			} else if (net_event->Error == NET_ERR_TIME_OUT) {
				ERROR_LOG(COMMON_NAME_LIB, "Open response: NET_ERR_TIME_OUT");
				ret->type = WLAN_MANAGER_RESPONSE_TYPE_CONNECTION_FAIL_TIMEOUT;
				wlan_manager_reset_connected_AP();
				connman_request_connection_close(ret->profile_name);
			} else if (net_event->Error == NET_ERR_IN_PROGRESS) {
				ERROR_LOG(COMMON_NAME_LIB, "Open response: NET_ERR_IN_PROGRESS");
				ret->type = WLAN_MANAGER_RESPONSE_TYPE_CONNECTION_FAIL_IN_PROGRESS;
			} else if (net_event->Error == NET_ERR_ACTIVE_CONNECTION_EXISTS) {
				ERROR_LOG(COMMON_NAME_LIB, "Open response: NET_ERR_ACTIVE_CONNECTION_EXISTS");
				ret->type = WLAN_MANAGER_RESPONSE_TYPE_CONNECTION_FAIL_ALREADY_EXIST;
			} else if (net_event->Error == NET_ERR_UNKNOWN_METHOD) {
				ERROR_LOG(COMMON_NAME_LIB, "Open response: NET_ERR_UNKNOWN_METHOD");
				ret->type = WLAN_MANAGER_RESPONSE_TYPE_CONNECTION_FAIL_UNKNOWN_METHOD;
				wlan_manager_reset_connected_AP();
			} else if (net_event->Error == NET_ERR_OPERATION_ABORTED) {
				ERROR_LOG(COMMON_NAME_LIB, "Open response: NET_ERR_OPERATION_ABORTED");
				ret->type = WLAN_MANAGER_RESPONSE_TYPE_CONNECTION_FAIL_ABORTED;
				wlan_manager_reset_connected_AP();
			} else {
				ERROR_LOG(COMMON_NAME_LIB, "Open response: %d", net_event->Error);
				ret->type = WLAN_MANAGER_RESPONSE_TYPE_CONNECTION_FAIL_UNKNOWN;
				wlan_manager_reset_connected_AP();
			}
			break;
		/** Open connection Indication (auto join) */
		case NET_EVENT_OPEN_IND:
			INFO_LOG(COMMON_NAME_LIB, "Callback - NET_EVENT_OPEN_IND : %s", ret->profile_name);
			if (strstr(ret->profile_name, "wifi_") == NULL) {
				return;
			}
			
			ret->type = WLAN_MANAGER_RESPONSE_TYPE_CONNECTION_IND;
			connman_profile_manager_connected_ssid_set((const char *)ret->profile_name);
			break;
		/** Close Connection Response Event */ 
		case NET_EVENT_CLOSE_RSP:
			INFO_LOG(COMMON_NAME_LIB, "Callback - NET_EVENT_CLOSE_RSP : %s", ret->profile_name);
			if (strstr(ret->profile_name, "wifi_") == NULL) {
				return;
			}

			ret->type = WLAN_MANAGER_RESPONSE_TYPE_DISCONNECTION_OK;
			connman_profile_manager_disconnected_ssid_set((const char *)ret->profile_name);
			break;
		/** Connection Close Indication Event */
		case NET_EVENT_CLOSE_IND:
			INFO_LOG(COMMON_NAME_LIB, "Callback - NET_EVENT_CLOSE_IND : %s", ret->profile_name);
			if (strstr(ret->profile_name, "wifi_") == NULL) {
				return;
			}

			ret->type = WLAN_MANAGER_RESPONSE_TYPE_DISCONNECTION_IND;
			connman_profile_manager_disconnected_ssid_set((const char *)ret->profile_name);
			break;
		/** Network PS state change Indication Event */
		case NET_EVENT_NET_STATE_IND:
			INFO_LOG(COMMON_NAME_LIB, "Callback - NET_EVENT_NET_STATE_IND");

			if (net_event->Error == NET_ERR_NONE) {
				if (strstr(ret->profile_name, "wifi_") == NULL) {
					return;
				} else {
					net_state_type_t* state_type = (net_state_type_t*)net_event->Data;
					if (*state_type == NET_STATE_TYPE_ASSOCIATION) {
						INFO_LOG(COMMON_NAME_LIB, "STATE_IND : ASSOCIATION");
						ret->type = WLAN_MANAGER_RESPONSE_TYPE_CONNECTING;
					} else if (*state_type == NET_STATE_TYPE_DISCONNECT) {
						INFO_LOG(COMMON_NAME_LIB, "STATE_IND : DISCONNECT");
						return;
					} else if (*state_type == NET_STATE_TYPE_FAILURE) {
						INFO_LOG(COMMON_NAME_LIB, "STATE_IND : FAILURE");
						ret->type = WLAN_MANAGER_RESPONSE_TYPE_DISCONNECTION_OK;
					} else if (*state_type == NET_STATE_TYPE_IDLE) {
						INFO_LOG(COMMON_NAME_LIB, "STATE_IND : IDLE");
						return;
					} else if (*state_type == NET_STATE_TYPE_CONFIGURATION) {
						INFO_LOG(COMMON_NAME_LIB, "STATE_IND : CONFIGURATION");
						return;
					} else if (*state_type == NET_STATE_TYPE_READY) {
						INFO_LOG(COMMON_NAME_LIB, "STATE_IND : READY");
						return;
					} else if (*state_type == NET_STATE_TYPE_ONLINE) {
						INFO_LOG(COMMON_NAME_LIB, "STATE_IND : ONLINE");
						return;
					} else {
						INFO_LOG(COMMON_NAME_LIB, "STATE_IND : ?????????");
						return;		
					}
				}
			} else {
				return;		
			}
			break;
		/** Profile modify indication Event */
		case NET_EVENT_PROFILE_MODIFY_IND:
			INFO_LOG(COMMON_NAME_LIB, "Callback - NET_EVENT_PROFILE_MODIFY_IND");
			break;
		/** Wi-Fi interface Mac Address */
		case NET_EVENT_WIFI_MAC_ID_IND:
			INFO_LOG(COMMON_NAME_LIB, "Callback - NET_EVENT_WIFI_MAC_ID_IND");
			ret->type = WLAN_MANAGER_RESPONSE_TYPE_MAC_ID_IND;
			break;
		/** Scan Indication Event(BG scan) */ 
		case NET_EVENT_WIFI_SCAN_IND:
			INFO_LOG(COMMON_NAME_LIB, "Callback - NET_EVENT_WIFI_SCAN_IND");
			ret->type = WLAN_MANAGER_RESPONSE_TYPE_SCAN_RESULT_IND;
			break;
		/** Scan Response Event */ 
		case NET_EVENT_WIFI_SCAN_RSP:
			INFO_LOG(COMMON_NAME_LIB, "Callback - NET_EVENT_WIFI_SCAN_RSP");
			ret->type = WLAN_MANAGER_RESPONSE_TYPE_SCAN_OK;
			break;
		/** Network Configure/Reconfigure Response Event */ 
		case NET_EVENT_NET_CONFIGURE_RSP:
			INFO_LOG(COMMON_NAME_LIB, "Callback - NET_EVENT_NET_CONFIGURE_RSP");
			break;
		case NET_EVENT_WIFI_WPS_RSP:
			INFO_LOG(COMMON_NAME_LIB, "Callback - NET_EVENT_WIFI_WPS_RSP");

			if (net_event->Error == NET_ERR_NONE) {
				if (strstr(ret->profile_name, "wifi_") == NULL) {
					INFO_LOG(COMMON_NAME_LIB, "RESULT : NOT WIFI");
					return;
				}
				INFO_LOG(COMMON_NAME_LIB, "RESULT : CONNECTION OK");
				ret->type = WLAN_MANAGER_RESPONSE_TYPE_WPS_ENROLL_OK;
				connman_profile_manager_connected_ssid_set(ret->profile_name);
			} else if (net_event->Error == NET_ERR_OPERATION_ABORTED) {
				INFO_LOG(COMMON_NAME_LIB, "RESULT : ERR_ABORTED");
				ret->type = WLAN_MANAGER_RESPONSE_TYPE_CANCEL_WPS_ENROLL_FAIL;
				wlan_manager_reset_connected_AP();
			} else {
				INFO_LOG(COMMON_NAME_LIB, "RESULT : CONNECTION FAIL");
				ret->type = WLAN_MANAGER_RESPONSE_TYPE_WPS_ENROLL_FAIL;
			}
			break;
		default:
			INFO_LOG(COMMON_NAME_LIB, "Callback - %d", net_event->Event);
			break;
	}

	if (manager_object->message_func)
		manager_object->message_func((void*)net_event->Data, (void*)ret);
}
