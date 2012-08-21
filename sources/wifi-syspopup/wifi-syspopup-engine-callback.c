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



#include <vconf-keys.h>
#include <syspopup_caller.h>
#include "common.h"
#include "common_datamodel.h"
#include "wifi-syspopup-engine-callback.h"
#include "wlan_manager.h"
#include "view-main.h"
#include "view-alerts.h"


extern wifi_object* syspopup_app_state;

int wlan_show_network_syspopup_message(const char *title, const char *content, const char *type, const char *ssid)
{
	int ret = 0;
	bundle *b = bundle_create();

	bundle_add(b, "_SYSPOPUP_TITLE_", title);
	bundle_add(b, "_SYSPOPUP_CONTENT_", content);
	bundle_add(b, "_SYSPOPUP_TYPE_", type);
	bundle_add(b, "_AP_NAME_", ssid);

	ret = syspopup_launch("net-popup", b);
	bundle_free(b);

	return ret;
}

/* wlan_manager handler */
void wlan_engine_callback(void *user_data, void *wlan_data)
{
	__COMMON_FUNC_ENTER__;

	assertm_if(NULL == wlan_data, "wlan data is NULL!!");

	callback_data* det = (callback_data*)wlan_data;

	INFO_LOG(SP_NAME_NORMAL, "callback data response type [%d]", det->type);

	switch (det->type) {
	case WLAN_MANAGER_RESPONSE_TYPE_NONE:
		ERROR_LOG(SP_NAME_ERR, "case NONE:");
		break;

	case WLAN_MANAGER_RESPONSE_TYPE_WPS_ENROLL_OK:
		INFO_LOG(SP_NAME_NORMAL, "case WPS ENROLL OK:");
		syspopup_app_state->connection_result = VCONFKEY_WIFI_QS_WIFI_CONNECTED;
		wifi_syspopup_destroy();
		break;

	case WLAN_MANAGER_RESPONSE_TYPE_CONNECTION_OK:
		INFO_LOG(SP_NAME_NORMAL, "case CONNECTION_OK:");
		syspopup_app_state->connection_result = VCONFKEY_WIFI_QS_WIFI_CONNECTED;
		wifi_syspopup_destroy();
		break;

	case WLAN_MANAGER_RESPONSE_TYPE_CONNECTION_FAIL_IN_PROGRESS:
	case WLAN_MANAGER_RESPONSE_TYPE_CONNECTION_FAIL_ALREADY_EXIST:
	case WLAN_MANAGER_RESPONSE_TYPE_CONNECTION_FAIL_ABORTED:
	case WLAN_MANAGER_RESPONSE_TYPE_CONNECTION_FAIL_UNKNOWN:
		ERROR_LOG(SP_NAME_NORMAL, "Connection failed.");

		if (syspopup_app_state->passpopup) {
			common_pswd_popup_destroy(syspopup_app_state->passpopup);
			syspopup_app_state->passpopup = NULL;
		}

		view_main_refresh();
		break;

	case WLAN_MANAGER_RESPONSE_TYPE_CONNECTION_INVALID_KEY:
	case WLAN_MANAGER_RESPONSE_TYPE_CONNECTION_CONNECT_FAILED:
	case WLAN_MANAGER_RESPONSE_TYPE_CONNECTION_FAIL_UNKNOWN_METHOD:
	case WLAN_MANAGER_RESPONSE_TYPE_CONNECTION_FAIL_TIMEOUT:
	case WLAN_MANAGER_RESPONSE_TYPE_WPS_ENROLL_FAIL:
		INFO_LOG(SP_NAME_NORMAL, "Connection failed.");

		if (syspopup_app_state->passpopup) {
			common_pswd_popup_destroy(syspopup_app_state->passpopup);
			syspopup_app_state->passpopup = NULL;
		}

		view_main_refresh();
		break;

	case WLAN_MANAGER_RESPONSE_TYPE_DISCONNECTION_OK:
		ERROR_LOG(SP_NAME_NORMAL, "case DISCONNECTION OK:");
		break;

	case WLAN_MANAGER_RESPONSE_TYPE_POWER_ON_OK:
		INFO_LOG(SP_NAME_NORMAL, "case POWER ON OK:");
		wlan_manager_request_scan(); /* First scan request after power on */
		if (syspopup_app_state->syspopup_type == WIFI_SYSPOPUP_WITHOUT_AP_LIST) {
			wifi_syspopup_destroy();
		}
		break;

	case WLAN_MANAGER_RESPONSE_TYPE_POWER_ON_NOT_SUPPORTED:
		INFO_LOG(SP_NAME_NORMAL, "case POWER ON NOT SUPPORTED:");
		if (syspopup_app_state->alertpopup) {
			evas_object_del(syspopup_app_state->alertpopup);
			syspopup_app_state->alertpopup = NULL;
		}
		wlan_show_network_syspopup_message("Network connection popup", "not support", "notification", NULL);
		wifi_syspopup_destroy();
		break;

	case WLAN_MANAGER_RESPONSE_TYPE_POWER_ON_RESTRICTED:
		INFO_LOG(SP_NAME_NORMAL, "case POWER ON RESTRICTED:");
		if (syspopup_app_state->alertpopup) {
			evas_object_del(syspopup_app_state->alertpopup);
			syspopup_app_state->alertpopup = NULL;
		}
		wlan_show_network_syspopup_message("Network connection popup", "wifi restricted", "popup", NULL);
		wifi_syspopup_destroy();
		break;

	case WLAN_MANAGER_RESPONSE_TYPE_POWER_OFF_OK:
		INFO_LOG(SP_NAME_NORMAL, "case POWER OFF OK:");
		wifi_syspopup_destroy();
		break;

	case WLAN_MANAGER_RESPONSE_TYPE_SCAN_OK:
		INFO_LOG(SP_NAME_NORMAL, "case SCAN OK:");
		wlan_manager_scanned_profile_refresh();
		break;

	case WLAN_MANAGER_RESPONSE_TYPE_CANCEL_WPS_ENROLL_OK:
		INFO_LOG(SP_NAME_NORMAL, "case CANCEL WPS ENROLL OK:");
		break;

	case WLAN_MANAGER_RESPONSE_TYPE_CANCEL_WPS_ENROLL_FAIL:
		INFO_LOG(SP_NAME_NORMAL, "case CANCEL WPS ENROLL FAIL:");
		break;

	case WLAN_MANAGER_RESPONSE_TYPE_CONNECTION_IND:
		INFO_LOG(SP_NAME_NORMAL, "case CONNECTION IND:");
		const char *ssid = wlan_manager_get_connected_ssid();
		wlan_show_network_syspopup_message("Network connection popup", "wifi connected", "notification", ssid);
		syspopup_app_state->connection_result = VCONFKEY_WIFI_QS_WIFI_CONNECTED;
		wifi_syspopup_destroy();
		break;

	case WLAN_MANAGER_RESPONSE_TYPE_CONNECTING:
		/* TODO: We need to show the connecting progress indi. Check this later. */
		break;

	case WLAN_MANAGER_RESPONSE_TYPE_DISCONNECTION_IND:
		INFO_LOG(SP_NAME_NORMAL, "case DISCONNECTION IND:");
		break;

	case WLAN_MANAGER_RESPONSE_TYPE_SCAN_RESULT_IND:
		INFO_LOG(SP_NAME_NORMAL, "case SCAN RESULT IND:");
		wlan_manager_scanned_profile_refresh();
		break;

	default:
		ERROR_LOG(SP_NAME_ERR, "case Err [%d]", det->type);
		break;
	}

	__COMMON_FUNC_EXIT__;
	return;
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

	ecore_idler_add(view_main_show, NULL);

	__COMMON_FUNC_EXIT__;
	return;
}
