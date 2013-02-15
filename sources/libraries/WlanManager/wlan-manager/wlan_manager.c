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
#include "wlan_manager.h"
#include "common_utils.h"
#include "wifi-engine-callback.h"
#include "common_invalid_password.h"

typedef enum {
	WLAN_MANAGER_REQ_TYPE_ACTIVATE,
	WLAN_MANAGER_REQ_TYPE_DEACTIVATE,
	WLAN_MANAGER_REQ_TYPE_SCAN,
	WLAN_MANAGER_REQ_TYPE_SPECIFIC_SCAN,
	WLAN_MANAGER_REQ_TYPE_BG_SCAN,
	WLAN_MANAGER_REQ_TYPE_CONNECT,
	WLAN_MANAGER_REQ_TYPE_WPS_CONNECT,
} WLAN_MANAGER_REQUEST_TYPES;

typedef struct {
	WLAN_MANAGER_REQUEST_TYPES req_type;
	wifi_ap_h ap;
	void *user_data;
} wlan_mgr_req_data_t;

typedef struct {
	int state;
	wifi_ap_h *ap;
} ap_state_info_t;

wlan_mgr_req_data_t bg_scan_req_data;

static void wlan_manager_register_cbs(void);
static void wlan_manager_deregister_cbs(void);

static wlan_manager_object* manager_object = NULL;

wlan_manager_object* wlan_manager_get_singleton(void)
{
	if (NULL == manager_object) {
		manager_object = g_new0(wlan_manager_object, 1);
		manager_object->message_func = NULL;
		manager_object->refresh_func = NULL;
	}

	return manager_object;
}

void wlan_manager_set_refresh_callback(wlan_manager_ui_refresh_func_t func)
{
	manager_object->refresh_func = func;
}


void* wlan_manager_create()
{
	wlan_manager_get_singleton();
	return NULL;
}

int wlan_manager_destroy()
{
	int ret = WLAN_MANAGER_ERR_NONE;

	_common_deregister_invalid_password_popup();

	wlan_manager_deregister_cbs();

	wifi_deinitialize();

	if (manager_object != NULL) {
		g_free(manager_object);
		manager_object = NULL;
	}

	return ret;
}

int wlan_manager_start()
{
	__COMMON_FUNC_ENTER__;
	switch (wifi_initialize()) {
	case WIFI_ERROR_NONE:
		/* Register the callbacks */
		wlan_manager_register_cbs();
		break;
	case WIFI_ERROR_INVALID_OPERATION:
		/* Register the callbacks */
		wlan_manager_register_cbs();
		return WLAN_MANAGER_ERR_ALREADY_REGISTERED;
	default:
		return WLAN_MANAGER_ERR_UNKNOWN;
	}
	return WLAN_MANAGER_ERR_NONE;
}

int wlan_manager_forget(wifi_ap_h ap)
{
	wifi_forget_ap(ap);
	return WLAN_MANAGER_ERR_NONE;
}

static void wlan_manager_device_state_changed_cb(wifi_device_state_e state, void *user_data)
{
	__COMMON_FUNC_ENTER__;
	wlan_mgr_event_info_t event_info;
	memset(&event_info, 0, sizeof(event_info));
	switch (state) {
	case WIFI_DEVICE_STATE_ACTIVATED:
		wlan_manager_enable_scan_result_update();
		wlan_manager_register_cbs();
		event_info.event_type = WLAN_MANAGER_RESPONSE_TYPE_POWER_ON_OK;
		break;
	case WIFI_DEVICE_STATE_DEACTIVATED:
		wlan_manager_deregister_cbs();
		event_info.event_type = WLAN_MANAGER_RESPONSE_TYPE_POWER_OFF_OK;
		break;
	default:
		return;
	}

	manager_object->message_func(&event_info, user_data);

	__COMMON_FUNC_EXIT__;
	return;
}

static void wlan_manager_connection_state_changed_cb(wifi_connection_state_e state, wifi_ap_h ap, void *user_data)
{
	__COMMON_FUNC_ENTER__;

	if (TRUE == manager_object->b_scan_blocked) {
		__COMMON_FUNC_EXIT__;
		return;
	}

	wlan_mgr_event_info_t event_info;
	memset(&event_info, 0, sizeof(event_info));

	event_info.ap = ap;

	switch (state) {
	case WIFI_CONNECTION_STATE_DISCONNECTED:  /**< Disconnected state */
		event_info.event_type = WLAN_MANAGER_RESPONSE_TYPE_DISCONNECTION_OK;
		break;
	case WIFI_CONNECTION_STATE_ASSOCIATION:  /**< Association state */
	case WIFI_CONNECTION_STATE_CONFIGURATION:  /**< Configuration state */
		event_info.event_type = WLAN_MANAGER_RESPONSE_TYPE_CONNECTING;
		break;
	case WIFI_CONNECTION_STATE_CONNECTED:  /**< Connected state */
		event_info.event_type = WLAN_MANAGER_RESPONSE_TYPE_CONNECTION_OK;
		break;
	default:
		return;
	}

	manager_object->message_func(&event_info, user_data);

	__COMMON_FUNC_EXIT__;
}

static void wlan_manager_rssi_level_changed_cb(
		wifi_rssi_level_e rssi_level, void *user_data)
{
	__COMMON_FUNC_ENTER__;

	if (TRUE == manager_object->b_scan_blocked) {
		__COMMON_FUNC_EXIT__;
		return;
	}

	wlan_mgr_event_info_t event_info;

	memset(&event_info, 0, sizeof(event_info));
	event_info.event_type = WLAN_MANAGER_RESPONSE_TYPE_UPDATE_SIG_STR;
	event_info.rssi_level = rssi_level;

	manager_object->message_func(&event_info, user_data);

	__COMMON_FUNC_EXIT__;
}

static void wlan_manager_specific_scan_finished_cb(
		wifi_error_e error_code, GSList *bss_info_list, void *user_data)
{
	__COMMON_FUNC_ENTER__;

	wlan_mgr_event_info_t event_info;
	memset(&event_info, 0, sizeof(event_info));

	if (WIFI_ERROR_NONE == error_code) {
		event_info.event_type = WLAN_MANAGER_RESPONSE_TYPE_SPECIFIC_SCAN_OK;
		event_info.bss_info_list = bss_info_list;
	} else
		event_info.event_type = WLAN_MANAGER_RESPONSE_TYPE_SPECIFIC_SCAN_FAIL;

	manager_object->message_func(&event_info, user_data);

	__COMMON_FUNC_EXIT__;
}

static void wlan_manager_network_event_cb(
		wifi_error_e error_code, void *user_data)
{
	__COMMON_FUNC_ENTER__;

	wlan_mgr_req_data_t *req_data = (wlan_mgr_req_data_t *)user_data;
	if (req_data == NULL) {
		ERROR_LOG(UG_NAME_ERR, "Request data is NULL !!!");

		__COMMON_FUNC_EXIT__;
		return;
	}

	wlan_mgr_event_info_t event_info;
	memset(&event_info, 0, sizeof(event_info));

	switch (req_data->req_type) {
	case WLAN_MANAGER_REQ_TYPE_ACTIVATE:
		if (WIFI_ERROR_NONE == error_code) {
			wlan_manager_enable_scan_result_update();
			wlan_manager_register_cbs();
			event_info.event_type = WLAN_MANAGER_RESPONSE_TYPE_POWER_ON_OK;
		} else
			goto exit;

		break;
	case WLAN_MANAGER_REQ_TYPE_DEACTIVATE:
		if (WIFI_ERROR_NONE == error_code) {
			wlan_manager_deregister_cbs();
			event_info.event_type = WLAN_MANAGER_RESPONSE_TYPE_POWER_OFF_OK;
		} else
			goto exit;

		break;
	case WLAN_MANAGER_REQ_TYPE_SCAN:
		if (WIFI_ERROR_NONE == error_code)
			event_info.event_type = WLAN_MANAGER_RESPONSE_TYPE_SCAN_OK;
		else
			goto exit;

		break;
	case WLAN_MANAGER_REQ_TYPE_SPECIFIC_SCAN:
		if (WIFI_ERROR_NONE == error_code)
			event_info.event_type = WLAN_MANAGER_RESPONSE_TYPE_SPECIFIC_SCAN_OK;
		else
			event_info.event_type = WLAN_MANAGER_RESPONSE_TYPE_SPECIFIC_SCAN_FAIL;

		break;
	case WLAN_MANAGER_REQ_TYPE_BG_SCAN:
		if (WIFI_ERROR_NONE == error_code) {
			event_info.event_type = WLAN_MANAGER_RESPONSE_TYPE_SCAN_RESULT_IND;
			manager_object->message_func(&event_info, req_data->user_data);
		}

		return;	// The request data is static. So returning here.
	case WLAN_MANAGER_REQ_TYPE_CONNECT:
		event_info.ap = req_data->ap;

		_common_deregister_invalid_password_popup();

		if (WIFI_ERROR_NONE != error_code) {
			if (_common_is_invalid_password() == TRUE)
				event_info.event_type =
						WLAN_MANAGER_RESPONSE_TYPE_CONNECTION_INVALID_KEY;
			else
				event_info.event_type =
						WLAN_MANAGER_RESPONSE_TYPE_CONNECTION_CONNECT_FAILED;
		} else
			goto exit;

		break;
	case WLAN_MANAGER_REQ_TYPE_WPS_CONNECT:
		event_info.ap = req_data->ap;

		if (WIFI_ERROR_NONE != error_code)
			event_info.event_type = WLAN_MANAGER_RESPONSE_TYPE_WPS_ENROLL_FAIL;
		else
			goto exit;

		break;
	default:
		goto exit;
	}

	manager_object->message_func(&event_info, req_data->user_data);

exit:
	if (req_data != NULL) {
		wifi_ap_destroy(req_data->ap);
		g_free(req_data);
	}

	__COMMON_FUNC_EXIT__;
}

static void wlan_manager_register_cbs(void)
{
	__COMMON_FUNC_ENTER__;
	wifi_set_device_state_changed_cb(wlan_manager_device_state_changed_cb, NULL);
	wifi_set_connection_state_changed_cb(wlan_manager_connection_state_changed_cb, NULL);
	wifi_set_rssi_level_changed_cb(wlan_manager_rssi_level_changed_cb, NULL);

	memset(&bg_scan_req_data, 0, sizeof(bg_scan_req_data));
	bg_scan_req_data.req_type = WLAN_MANAGER_REQ_TYPE_BG_SCAN;
	wifi_set_background_scan_cb(wlan_manager_network_event_cb, &bg_scan_req_data);

	connman_request_register();

	__COMMON_FUNC_EXIT__;
}

static void wlan_manager_deregister_cbs(void)
{
	__COMMON_FUNC_ENTER__;
//	wifi_unset_device_state_changed_cb();
	wifi_unset_background_scan_cb();
	wifi_unset_connection_state_changed_cb();
	wifi_unset_rssi_level_changed_cb();
	wifi_unset_specific_scan_cb();
	connman_request_deregister();
	__COMMON_FUNC_EXIT__;
}

void wlan_manager_set_message_callback(wlan_event_handler func)
{
	manager_object->message_func = func;
}

static bool wifi_found_ap_with_state_cb(wifi_ap_h ap, void* user_data)
{
	wifi_connection_state_e state;
	ap_state_info_t *ap_state_info = (ap_state_info_t *)user_data;
	bool found_match = false;

	if (WIFI_ERROR_NONE != wifi_ap_get_connection_state(ap, &state))
		return true; // continue with next AP

	switch (ap_state_info->state) {
	case WLAN_MANAGER_UNCONNECTED:
		if (WIFI_CONNECTION_STATE_DISCONNECTED == state) {
			/* Found a match, so terminate the loop */
			found_match = true;
		}
		break;
	case WLAN_MANAGER_CONNECTING:
		if (WIFI_CONNECTION_STATE_ASSOCIATION == state ||
			WIFI_CONNECTION_STATE_CONFIGURATION == state) {
			/* Found a match, so terminate the loop */
			found_match = true;
		}
		break;
	case WLAN_MANAGER_CONNECTED:
		if (WIFI_CONNECTION_STATE_CONNECTED == state) {
			/* Found a match, so terminate the loop */
			found_match = true;
		}
		break;
	default:
		ERROR_LOG(COMMON_NAME_ERR, "Unknown state : %d", ap_state_info->state);
		return false;
	}

	if (true == found_match) {
		if (ap_state_info->ap) {
			wifi_ap_clone(ap_state_info->ap, ap);
		}
		INFO_LOG(COMMON_NAME_LIB, "Found an AP[0x%x] in the state %d", ap, ap_state_info->state);
		return false;	// found the match, so terminate the loop.
	}
	return true;
}

wifi_ap_h wlan_manager_get_ap_with_state(int ap_state)
{
	ap_state_info_t ap_state_info;
	wifi_ap_h ap = NULL;

	ap_state_info.state = ap_state;
	ap_state_info.ap = &ap;

	wifi_foreach_found_aps (wifi_found_ap_with_state_cb, &ap_state_info);

	return ap;
}

int wlan_manager_state_get(void)
{
	int ret_val = 0;
	wifi_connection_state_e connection_state;
	bool activated;

	if (WIFI_ERROR_NONE != wifi_is_activated(&activated))
		return WLAN_MANAGER_ERROR;
	else if (false == activated) {
		INFO_LOG(COMMON_NAME_LIB, "STATE: WIFI_OFF");

		return WLAN_MANAGER_OFF;
	}

	ret_val = wifi_get_connection_state(&connection_state);
	if (WIFI_ERROR_NONE != ret_val)
		return WLAN_MANAGER_ERROR;

	switch (connection_state) {
	case WIFI_CONNECTION_STATE_DISCONNECTED:
		INFO_LOG(COMMON_NAME_LIB, "STATE: WIFI_DISCONNECTED");

		ret_val = WLAN_MANAGER_UNCONNECTED;
		break;
	case WIFI_CONNECTION_STATE_ASSOCIATION:
	case WIFI_CONNECTION_STATE_CONFIGURATION:
		INFO_LOG(COMMON_NAME_LIB, "STATE: WIFI_CONNECTING");

		ret_val = WLAN_MANAGER_CONNECTING;
		break;
	case WIFI_CONNECTION_STATE_CONNECTED:
		INFO_LOG(COMMON_NAME_LIB, "STATE: WIFI_CONNECTED");

		ret_val = WLAN_MANAGER_CONNECTED;
		break;
	default:
		ERROR_LOG(COMMON_NAME_ERR, "Unknown state: %d", connection_state);

		ret_val = WLAN_MANAGER_ERROR;
		break;
	}

	return ret_val;
}

int wlan_manager_request_power_on(void)
{
	__COMMON_FUNC_ENTER__;

	INFO_LOG(UG_NAME_REQ, "power on");

	int ret = 0;
	int hot_spot_mode =
			common_util_get_system_registry("memory/mobile_hotspot/mode");
	if (hot_spot_mode < 0) {
		INFO_LOG(COMMON_NAME_LIB, "FAILED to get the mobile hotspot mode");
		__COMMON_FUNC_EXIT__;
		return WLAN_MANAGER_ERR_UNKNOWN;
	} else if (VCONFKEY_MOBILE_HOTSPOT_MODE_WIFI & hot_spot_mode) {
		INFO_LOG(COMMON_NAME_LIB, "mobile_hotspot wifi ON");

		__COMMON_FUNC_EXIT__;
		return WLAN_MANAGER_ERR_MOBILE_HOTSPOT_OCCUPIED;
	}

	INFO_LOG(UG_NAME_REQ, "Mobile hotspot mode = %d\n", hot_spot_mode);

	wlan_mgr_req_data_t *req_data = g_new0(wlan_mgr_req_data_t, 1);
	req_data->req_type = WLAN_MANAGER_REQ_TYPE_ACTIVATE;
	ret = wifi_activate(wlan_manager_network_event_cb, req_data);
	if (WIFI_ERROR_NONE != ret) {
		ERROR_LOG(UG_NAME_REQ, "Power on request. Error Reason [%d]", ret);
		g_free(req_data);
		__COMMON_FUNC_EXIT__;
		return WLAN_MANAGER_ERR_UNKNOWN;
	}

	__COMMON_FUNC_EXIT__;
	return WLAN_MANAGER_ERR_NONE;
}

int wlan_manager_request_power_off(void)
{
	__COMMON_FUNC_ENTER__;
	wlan_mgr_req_data_t *req_data = g_new0(wlan_mgr_req_data_t, 1);
	req_data->req_type = WLAN_MANAGER_REQ_TYPE_DEACTIVATE;
	int ret = wifi_deactivate(wlan_manager_network_event_cb, req_data);
	if (WIFI_ERROR_NONE != ret) {
		ERROR_LOG(UG_NAME_REQ, "Power off request. Error Reason [%d]", ret);
		g_free(req_data);
		__COMMON_FUNC_EXIT__;
		return WLAN_MANAGER_ERR_UNKNOWN;
	}

	__COMMON_FUNC_EXIT__;
	return WLAN_MANAGER_ERR_NONE;
}

int wlan_manager_request_wps_connection(wifi_ap_h ap)
{
	__COMMON_FUNC_ENTER__;
	wlan_mgr_req_data_t *req_data = g_new0(wlan_mgr_req_data_t, 1);
	req_data->req_type = WLAN_MANAGER_REQ_TYPE_WPS_CONNECT;
	wifi_ap_clone(&(req_data->ap), ap);

	int ret = wifi_connect_by_wps_pbc(req_data->ap, wlan_manager_network_event_cb, req_data);
	if (WIFI_ERROR_NONE != ret) {
		ERROR_LOG(UG_NAME_REQ, "WPS Connect failed. Error Reason [%d]", ret);
		wifi_ap_destroy(req_data->ap);
		g_free(req_data);
		__COMMON_FUNC_EXIT__;
		return WLAN_MANAGER_ERR_UNKNOWN;
	}

	__COMMON_FUNC_EXIT__;
	return WLAN_MANAGER_ERR_NONE;
}

int wlan_manager_request_connection(wifi_ap_h ap)
{
	__COMMON_FUNC_ENTER__;

	bool favourite;
	int ret = wifi_ap_is_favorite(ap, &favourite);
	wifi_security_type_e sec_type;
	if (WIFI_ERROR_NONE != ret) {
		__COMMON_FUNC_EXIT__;
		return WLAN_MANAGER_ERR_NOSERVICE;
	}

	if (false == favourite) {
		if (WIFI_ERROR_NONE != wifi_ap_get_security_type(ap, &sec_type)) {
			__COMMON_FUNC_EXIT__;
			return WLAN_MANAGER_ERR_NOSERVICE;
		}

		if (WIFI_SECURITY_TYPE_EAP == sec_type) {
			__COMMON_FUNC_EXIT__;
			return WLAN_MANAGER_ERR_CONNECT_EAP_SEC_TYPE;
		} else if (WIFI_SECURITY_TYPE_NONE == sec_type) {
			/* Allow Open APs to connect */
		} else {
			__COMMON_FUNC_EXIT__;
			return WLAN_MANAGER_ERR_CONNECT_PASSWORD_NEEDED;
		}
	}

	INFO_LOG(UG_NAME_REQ, "All OK to connect to ap[0x%x]", ap);

	wlan_mgr_req_data_t *req_data = g_new0(wlan_mgr_req_data_t, 1);
	req_data->req_type = WLAN_MANAGER_REQ_TYPE_CONNECT;
	wifi_ap_clone(&(req_data->ap), ap);

	_common_register_invalid_password_popup();
	ret = wifi_connect(req_data->ap, wlan_manager_network_event_cb, req_data);
	if (WIFI_ERROR_NONE != ret) {
		_common_deregister_invalid_password_popup();

		ERROR_LOG(UG_NAME_REQ, "Connect failed. Error Reason [%d]", ret);

		wifi_ap_destroy(req_data->ap);
		g_free(req_data);

		__COMMON_FUNC_EXIT__;
		return WLAN_MANAGER_ERR_UNKNOWN;
	}

	__COMMON_FUNC_EXIT__;
	return WLAN_MANAGER_ERR_NONE;
}

int wlan_manager_connect_with_password(wifi_ap_h ap, const char *pass_phrase)
{
	__COMMON_FUNC_ENTER__;
	if (!ap) {
		ERROR_LOG(UG_NAME_SCAN, "AP handler is NULL");
		return WLAN_MANAGER_ERR_INVALID_PARAM;
	}

	int ret = wifi_ap_set_passphrase(ap, pass_phrase);
	if (WIFI_ERROR_NONE != ret) {
		__COMMON_FUNC_EXIT__;
		return WLAN_MANAGER_ERR_UNKNOWN;
	}

	wlan_mgr_req_data_t *req_data = g_new0(wlan_mgr_req_data_t, 1);
	req_data->req_type = WLAN_MANAGER_REQ_TYPE_CONNECT;
	wifi_ap_clone(&(req_data->ap), ap);

	_common_register_invalid_password_popup();
	ret = wifi_connect(req_data->ap, wlan_manager_network_event_cb, req_data);
	if (WIFI_ERROR_NONE != ret) {
		_common_deregister_invalid_password_popup();

		ERROR_LOG(UG_NAME_REQ, "Connect failed. Error Reason [%d]", ret);

		wifi_ap_destroy(req_data->ap);
		g_free(req_data);

		__COMMON_FUNC_EXIT__;
		return WLAN_MANAGER_ERR_UNKNOWN;
	}

	__COMMON_FUNC_EXIT__;
	return WLAN_MANAGER_ERR_NONE;
}

int wlan_manager_connect_with_wifi_info(wifi_ap_h ap)
{
	__COMMON_FUNC_ENTER__;

	int ret;

	INFO_LOG(UG_NAME_REQ, "All OK to connect to ap[0x%x]", ap);

	wlan_mgr_req_data_t *req_data = g_new0(wlan_mgr_req_data_t, 1);
	req_data->req_type = WLAN_MANAGER_REQ_TYPE_CONNECT;
	wifi_ap_clone(&(req_data->ap), ap);

	_common_register_invalid_password_popup();
	ret = wifi_connect(req_data->ap, wlan_manager_network_event_cb, req_data);
	if (WIFI_ERROR_NONE != ret) {
		_common_deregister_invalid_password_popup();

		ERROR_LOG(UG_NAME_REQ, "Connect failed. Error Reason [%d]", ret);

		wifi_ap_destroy(req_data->ap);
		g_free(req_data);

		__COMMON_FUNC_EXIT__;
		return WLAN_MANAGER_ERR_UNKNOWN;
	}

	__COMMON_FUNC_EXIT__;
	return WLAN_MANAGER_ERR_NONE;
}

int wlan_manager_request_disconnection(wifi_ap_h ap)
{
	__COMMON_FUNC_ENTER__;
	int ret;

	INFO_LOG(UG_NAME_REQ, "Request disconnection for ap [0x%x]", ap);

	ret = wifi_disconnect(ap, NULL, NULL);
	if (WIFI_ERROR_NONE != ret) {
		ERROR_LOG(UG_NAME_REQ, "Disconnect failed. Error Reason [%d]", ret);
		__COMMON_FUNC_EXIT__;
		return WLAN_MANAGER_ERR_UNKNOWN;
	}

	__COMMON_FUNC_EXIT__;
	return WLAN_MANAGER_ERR_NONE;
}

int wlan_manager_request_scan(void)
{
	__COMMON_FUNC_ENTER__;

	wlan_mgr_req_data_t *req_data = g_new0(wlan_mgr_req_data_t, 1);
	req_data->req_type = WLAN_MANAGER_REQ_TYPE_SCAN;

	int ret = wifi_scan(wlan_manager_network_event_cb, req_data);
	if (ret != WLAN_MANAGER_ERR_NONE) {
		ERROR_LOG(UG_NAME_REQ, "Scan failed. Ret: %d", ret);

		g_free(req_data);

		__COMMON_FUNC_EXIT__;
		return ret;
	}

	/* Since the scan request was success,
	 * lets reset the ui refresh and scan update blocked flags.
	 */
	manager_object->b_scan_blocked = FALSE;
	manager_object->b_ui_refresh = FALSE;

	__COMMON_FUNC_EXIT__;
	return ret;
}

int wlan_manager_request_specific_scan(const char *ssid, void *user_data)
{
	__COMMON_FUNC_ENTER__;

	wifi_set_specific_scan_cb(wlan_manager_specific_scan_finished_cb, user_data);

	int ret = connman_request_specific_scan(ssid);
	if (ret != WLAN_MANAGER_ERR_NONE) {
		ERROR_LOG(UG_NAME_REQ, "Specific Scan failed. Ret: %d", ret);

		__COMMON_FUNC_EXIT__;
		return ret;
	}

	/* Since the scan request was success,
	 * lets reset the ui refresh and scan update blocked flags.
	 */
	manager_object->b_scan_blocked = FALSE;
	manager_object->b_ui_refresh = FALSE;

	__COMMON_FUNC_EXIT__;
	return ret;
}

int wlan_manager_scanned_profile_refresh_with_count(int count)
{
	__COMMON_FUNC_ENTER__;

	manager_object->refresh_func();

	__COMMON_FUNC_EXIT__;
	return 0;
}

void wlan_manager_scanned_profile_refresh(void)
{
	__COMMON_FUNC_ENTER__;

	if (FALSE == manager_object->b_scan_blocked) {
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
	if (rssi >= 57) {
		return SIGNAL_STRENGTH_TYPE_EXCELLENT;
	} else if (rssi >= 46) {
		return SIGNAL_STRENGTH_TYPE_GOOD;
	} else if (rssi >= 38) {
		return SIGNAL_STRENGTH_TYPE_WEAK;
	} else {
		return SIGNAL_STRENGTH_TYPE_VERY_WEAK;
	}
}

wifi_device_info_t* wlan_manager_profile_device_info_blank_create()
{
	__COMMON_FUNC_ENTER__;
	wifi_device_info_t *di_s0 = g_new0(wifi_device_info_t, 1);

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
	return di_s0;
}

static Eina_Bool _refresh_ui(void *data)
{
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

char *wlan_manager_get_connected_ssid(void)
{
	wifi_ap_h ap;
	char *essid = NULL;

	wifi_get_connected_ap(&ap);
	if (ap) {
		wifi_ap_get_essid(ap, &essid);
	}

	wifi_ap_destroy(ap);

	return essid;
}
