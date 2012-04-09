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



#include <vconf-keys.h>
#include <syspopup_caller.h>
#include "common.h"
#include "wifi-syspopup-engine-callback.h"
#include "wlan_manager.h"
#include "view-main.h"
#include "view-alerts.h"


extern wifi_object* app_state;

int wlan_show_network_syspopup_message(const char *title, const char *content, const char *ssid)
{
	int ret = 0;
	bundle *b = bundle_create();

	bundle_add(b, "_SYSPOPUP_TITLE_", title);
	bundle_add(b, "_SYSPOPUP_CONTENT_", content);
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
	case WLAN_MANAGER_RESPONSE_TYPE_CONNECTION_OK:
		INFO_LOG(SP_NAME_NORMAL, "case CONNECTION_OK:");

		app_state->connection_result = VCONFKEY_WIFI_QS_WIFI_CONNECTED;
		wifi_syspopup_destroy();

		break;
	case WLAN_MANAGER_RESPONSE_TYPE_CONNECTION_FAIL_IN_PROGRESS:
		INFO_LOG(SP_NAME_NORMAL, "case CONNECTION FAIL:");

		view_main_refresh();

		break;
	case WLAN_MANAGER_RESPONSE_TYPE_CONNECTION_FAIL_UNKNOWN:
		INFO_LOG(SP_NAME_NORMAL, "case CONNECTION FAIL UNKNOWN:");

		wlan_show_network_syspopup_message("Network connection popup", "unable to connect", NULL);
		view_main_refresh();

		break;
	case WLAN_MANAGER_RESPONSE_TYPE_CONNECTION_FAIL_TIMEOUT:
		INFO_LOG(SP_NAME_NORMAL, "case CONNECTION FAIL TIMEOUT:");

		view_alerts_connection_fail_timeout_show();
		view_main_refresh();

		break;
	case WLAN_MANAGER_RESPONSE_TYPE_DISCONNECTION_OK:
		ERROR_LOG(SP_NAME_NORMAL, "case DISCONNECTION OK:");
		break;
	case WLAN_MANAGER_RESPONSE_TYPE_POWER_ON_OK:
		INFO_LOG(SP_NAME_NORMAL, "case POWER ON OK:");
		break;
	case WLAN_MANAGER_RESPONSE_TYPE_POWER_ON_NOT_SUPPORTED:
		INFO_LOG(SP_NAME_NORMAL, "case POWER ON NOT SUPPORTED:");
		wlan_show_network_syspopup_message("Network connection popup", "not support", NULL);
		break;
	case WLAN_MANAGER_RESPONSE_TYPE_POWER_ON_RESTRICTED:
		INFO_LOG(SP_NAME_NORMAL, "case POWER ON RESTRICTED:");
		wlan_show_network_syspopup_message("Network connection popup", "wifi restricted", NULL);
		break;
	case WLAN_MANAGER_RESPONSE_TYPE_POWER_OFF_OK:
		INFO_LOG(SP_NAME_NORMAL, "case POWER OFF OK:");
		break;
	case WLAN_MANAGER_RESPONSE_TYPE_SCAN_OK:
		INFO_LOG(SP_NAME_NORMAL, "case SCAN OK:");
		wlan_manager_scanned_profile_refresh(TRUE);
		break;
	case WLAN_MANAGER_RESPONSE_TYPE_WPS_ENROLL_OK:
		INFO_LOG(SP_NAME_NORMAL, "case WPS ENROLL OK:");
		break;
	case WLAN_MANAGER_RESPONSE_TYPE_WPS_ENROLL_FAIL:
		INFO_LOG(SP_NAME_NORMAL, "case WPS ENROLL FAIL:");
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
		wlan_show_network_syspopup_message("Network connection popup", "wifi connected", ssid);
		app_state->connection_result = VCONFKEY_WIFI_QS_WIFI_CONNECTED;
		wifi_syspopup_destroy();

		break;
	case WLAN_MANAGER_RESPONSE_TYPE_DISCONNECTION_IND:
		INFO_LOG(SP_NAME_NORMAL, "case DISCONNECTION IND:");
		break;
	case WLAN_MANAGER_RESPONSE_TYPE_SCAN_RESULT_IND:
		INFO_LOG(SP_NAME_NORMAL, "case SCAN RESULT IND:");
		wlan_manager_scanned_profile_refresh(TRUE);
		break;
	default:
		ERROR_LOG(SP_NAME_ERR, "case Err [%d]", det->type);
		break;
	}

	if (det) {
		if (det->profile_name)
			g_free(det->profile_name);
		g_free(det);
	}

	__COMMON_FUNC_EXIT__;
	return;
}

void wlan_engine_refresh_callback(void)
{
	__COMMON_FUNC_ENTER__;

	if (NULL == app_state) {
		INFO_LOG(SP_NAME_ERR, "app_state is NULL!! Is it test mode?");

		__COMMON_FUNC_EXIT__;
		return;
	}

	if (NULL == app_state->layout_main) {
		/* Make System popup filled, if it was first launched */
		if (NULL != app_state->alertpopup) {
			/* deallocate alert popup if it has allocated */
			evas_object_del(app_state->alertpopup);
			app_state->alertpopup = NULL;
		}

		INFO_LOG(SP_NAME_NORMAL, "Wi-Fi QS launch");
		wifi_syspopup_create();
	}

	if (view_main_show() == FALSE) {
		ERROR_LOG(SP_NAME_ERR, "view_main_show == FALSE");
	}

	__COMMON_FUNC_EXIT__;
	return;
}
