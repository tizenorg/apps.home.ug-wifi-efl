/*
 * Wi-Fi
 *
 * Copyright 2012 Samsung Electronics Co., Ltd
 *
 * Licensed under the Flora License, Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.tizenopensource.org/license
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <vconf-keys.h>
#include <syspopup_caller.h>

#include "common.h"
#include "view-main.h"
#include "view-alerts.h"
#include "common_utils.h"
#include "wlan_connection.h"
#include "wifi-syspopup-engine-callback.h"

extern wifi_object* syspopup_app_state;

void wlan_engine_callback(wlan_mgr_event_info_t *event_info, void *user_data)
{
	__COMMON_FUNC_ENTER__;

	char *ssid = NULL;

	if (event_info == NULL) {
		__COMMON_FUNC_EXIT__;
		return;
	}

	INFO_LOG(SP_NAME_NORMAL, "event type [%d]", event_info->event_type);

	switch (event_info->event_type) {
	case WLAN_MANAGER_RESPONSE_TYPE_NONE:
		break;

	case WLAN_MANAGER_RESPONSE_TYPE_CONNECTION_OK:
	case WLAN_MANAGER_RESPONSE_TYPE_WPS_ENROLL_OK:
		ssid = wlan_manager_get_connected_ssid();

		common_utils_send_message_to_net_popup(
				"Network connection popup", "wifi connected",
				"notification", ssid);
		syspopup_app_state->connection_result = VCONFKEY_WIFI_QS_WIFI_CONNECTED;
		wifi_syspopup_destroy();
		g_free(ssid);

		break;

	case WLAN_MANAGER_RESPONSE_TYPE_CONNECTION_CONNECT_FAILED:
	case WLAN_MANAGER_RESPONSE_TYPE_WPS_ENROLL_FAIL:
		if (syspopup_app_state->passpopup) {
			passwd_popup_free(syspopup_app_state->passpopup);
			syspopup_app_state->passpopup = NULL;
		}

		view_main_item_state_set(event_info->ap, ITEM_CONNECTION_MODE_OFF);
		break;

	case WLAN_MANAGER_RESPONSE_TYPE_DISCONNECTION_OK:
		view_main_item_state_set(event_info->ap, ITEM_CONNECTION_MODE_OFF);
		break;

	case WLAN_MANAGER_RESPONSE_TYPE_POWER_ON_OK:
		if (syspopup_app_state->syspopup_type == WIFI_SYSPOPUP_WITHOUT_AP_LIST)
			wifi_syspopup_destroy();

		break;

	case WLAN_MANAGER_RESPONSE_TYPE_POWER_ON_NOT_SUPPORTED:
		if (syspopup_app_state->alertpopup) {
			evas_object_del(syspopup_app_state->alertpopup);
			syspopup_app_state->alertpopup = NULL;
		}

		common_utils_send_message_to_net_popup("Network connection popup",
				"not support", "notification", NULL);
		wifi_syspopup_destroy();

		break;

	case WLAN_MANAGER_RESPONSE_TYPE_POWER_ON_RESTRICTED:
		if (syspopup_app_state->alertpopup) {
			evas_object_del(syspopup_app_state->alertpopup);
			syspopup_app_state->alertpopup = NULL;
		}

		common_utils_send_message_to_net_popup("Network connection popup",
				"wifi restricted", "popup", NULL);
		wifi_syspopup_destroy();

		break;

	case WLAN_MANAGER_RESPONSE_TYPE_POWER_OFF_OK:
		wifi_syspopup_destroy();
		break;

	case WLAN_MANAGER_RESPONSE_TYPE_SCAN_OK:
		wlan_manager_scanned_profile_refresh();
		break;

	case WLAN_MANAGER_RESPONSE_TYPE_CONNECTING:
		view_main_item_state_set(event_info->ap,
				ITEM_CONNECTION_MODE_CONNECTING);
		break;

	case WLAN_MANAGER_RESPONSE_TYPE_SCAN_RESULT_IND:
		wlan_manager_scanned_profile_refresh();
		break;

	case WLAN_MANAGER_RESPONSE_TYPE_CONNECTION_INVALID_KEY:
		view_alerts_popup_ok_show(INVALID_PASSWORD);
		break;

	default:
		break;
	}

	wlan_validate_alt_connection();
	__COMMON_FUNC_EXIT__;
}

void wlan_engine_refresh_callback(void)
{
	__COMMON_FUNC_ENTER__;

	if (NULL == syspopup_app_state) {
		INFO_LOG(SP_NAME_ERR, "syspopup_app_state is NULL!! Is it test mode?");

		__COMMON_FUNC_EXIT__;
		return;
	}

	/* Make System popup filled, if it was first launched */
	if (NULL != syspopup_app_state->alertpopup) {
		/* deallocate alert popup if it has allocated */
		evas_object_del(syspopup_app_state->alertpopup);
		syspopup_app_state->alertpopup = NULL;
	}

	INFO_LOG(SP_NAME_NORMAL, "Wi-Fi QS launch");

	common_util_managed_idle_add(view_main_show, NULL);

	__COMMON_FUNC_EXIT__;
	return;
}
