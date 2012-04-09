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



#include "wifi.h"
#include "wifi-ui-list-callbacks.h"
#include "wlan_manager.h"
#include "viewer_manager.h"
#include "popup.h"
#include "wifi-engine-callback.h"
#include "view_ime_password.h"
#include "viewer_list.h"


void wlan_engine_callback(void *user_data, void *wlan_data)
{
	__COMMON_FUNC_ENTER__;

	if (wlan_data == NULL) {
		ERROR_LOG(UG_NAME_RESP, "[Error] Invalid user data");
		__COMMON_FUNC_EXIT__;
		return;
	}

	Elm_Object_Item* target_item = NULL;
	callback_data* det = (callback_data*) wlan_data;

	int header_state = -1;
	header_state = viewer_manager_header_mode_get();

	INFO_LOG(UG_NAME_NORMAL, "header state [%d]", header_state);
	INFO_LOG(UG_NAME_NORMAL, "daemon message [%d]", det->type);

	switch (det->type) {
	case WLAN_MANAGER_RESPONSE_TYPE_POWER_ON_OK:
		INFO_LOG(UG_NAME_RESP,"Power ok..\n");

		if (connman_request_scan_mode_set(WIFI_BGSCAN_MODE_PERIODIC) == WLAN_MANAGER_ERR_NONE) {
			DEBUG_LOG(UG_NAME_REQ, "Set BG scan mode - PERIODIC");
		}

		viewer_manager_header_mode_set(HEADER_MODE_SEARCHING);
		viewer_manager_hidden_disable_set(TRUE);
		viewer_manager_show(VIEWER_WINSET_SEARCHING);
		viewer_manager_show(VIEWER_WINSET_SUB_CONTENTS);
		break;
	case WLAN_MANAGER_RESPONSE_TYPE_POWER_OFF_OK:
		viewer_list_item_clear();
		wlan_manager_reset_connected_AP();
		viewer_manager_header_mode_set(HEADER_MODE_OFF);

		viewer_manager_hide(VIEWER_WINSET_SEARCHING);
		viewer_manager_hide(VIEWER_WINSET_SUB_CONTENTS);
		break;
	case WLAN_MANAGER_RESPONSE_TYPE_POWER_ON_NOT_SUPPORTED:
		viewer_manager_header_mode_set(HEADER_MODE_OFF);
		wlan_manager_network_syspopup_message("Network connection popup", "not support");
		break;
	case WLAN_MANAGER_RESPONSE_TYPE_POWER_ON_RESTRICTED:
		viewer_manager_header_mode_set(HEADER_MODE_OFF);
		wlan_manager_network_syspopup_message("Network connection popup", "wifi restricted");
		break;
	case WLAN_MANAGER_RESPONSE_TYPE_SCAN_OK:
		INFO_LOG(UG_NAME_RESP,"Scan ok..\n");
		wlan_manager_scanned_profile_refresh(TRUE);

		if (HEADER_MODE_CONNECTED == header_state) {
			viewer_manager_hidden_disable_set(FALSE);
		} else if (HEADER_MODE_ON == header_state) {
			viewer_manager_hidden_disable_set(FALSE);
		}
		break;
	case WLAN_MANAGER_RESPONSE_TYPE_SCAN_RESULT_IND:
		INFO_LOG(UG_NAME_RESP,"Scan result ind");
		switch (header_state) {
		case HEADER_MODE_CONNECTING:
		case HEADER_MODE_CONNECTED:
		case HEADER_MODE_DISCONNECTING:
		case HEADER_MODE_ON:
		case HEADER_MODE_SEARCHING:
			wlan_manager_scanned_profile_refresh(TRUE);
			break;
		case HEADER_MODE_DEACTIVATING:
		case HEADER_MODE_OFF:
		case HEADER_MODE_ACTIVATING:
		default:
			break;
		}
		break;
	case WLAN_MANAGER_RESPONSE_TYPE_CONNECTION_IND:
		target_item = item_get_for_profile_name(det->profile_name);

		switch (header_state) {
		case HEADER_MODE_CONNECTING:
		case HEADER_MODE_CONNECTED:
		case HEADER_MODE_DISCONNECTING:
		case HEADER_MODE_ON:
			if (target_item == NULL) {
				ERROR_LOG(UG_NAME_RESP,"Error!!! Target item is NULL");
			} else {
				viewer_manager_item_radio_mode_set(NULL, target_item , VIEWER_ITEM_RADIO_MODE_CONNECTED);
			}

			viewer_manager_set_enabled_list_update(EINA_TRUE);
			viewer_manager_refresh(TRUE);
			viewer_manager_header_mode_set(HEADER_MODE_CONNECTED);
			viewer_manager_scroll_to_top();
			break;
		case HEADER_MODE_DEACTIVATING:
		case HEADER_MODE_OFF:
		case HEADER_MODE_ACTIVATING:
		default:
			break;
		}
		break;
	case WLAN_MANAGER_RESPONSE_TYPE_CONNECTING:
		switch (header_state) {
		case HEADER_MODE_CONNECTING:
		case HEADER_MODE_DISCONNECTING:
		case HEADER_MODE_DEACTIVATING:
		case HEADER_MODE_CONNECTED:
			break;
		default:
			target_item = item_get_for_profile_name(det->profile_name);
			viewer_manager_header_mode_set(HEADER_MODE_CONNECTING);
			viewer_manager_item_radio_mode_set(NULL, target_item, VIEWER_ITEM_RADIO_MODE_CONNECTING);
			break;
		}
		break;
	case WLAN_MANAGER_RESPONSE_TYPE_CONNECTION_OK:
		target_item = item_get_for_profile_name(det->profile_name);
		switch (header_state) {
		case HEADER_MODE_CONNECTING:
		case HEADER_MODE_CONNECTED:
		case HEADER_MODE_DISCONNECTING:
		case HEADER_MODE_ON:
			if (target_item == NULL) {
				ERROR_LOG(UG_NAME_RESP,"Error!!! Target item is NULL");
			} else {
				viewer_manager_item_radio_mode_set(NULL, target_item , VIEWER_ITEM_RADIO_MODE_CONNECTED);
			}

			viewer_manager_set_enabled_list_update(EINA_TRUE);
			viewer_manager_refresh(TRUE);
			viewer_manager_header_mode_set(HEADER_MODE_CONNECTED);
			viewer_manager_scroll_to_top();
			break;
		case HEADER_MODE_DEACTIVATING:
		case HEADER_MODE_OFF:
		case HEADER_MODE_ACTIVATING:
		default:
			break;
		}
		break;
	case WLAN_MANAGER_RESPONSE_TYPE_CONNECTION_FAIL_IN_PROGRESS:
		target_item = item_get_for_profile_name(det->profile_name);
		viewer_manager_header_mode_set(HEADER_MODE_ON);

		if (target_item == NULL) {
			ERROR_LOG(UG_NAME_RESP,"Error!!! Target item is NULL");
		} else {
			viewer_manager_item_radio_mode_set(NULL, target_item , VIEWER_ITEM_RADIO_MODE_OFF);
		}

		viewer_manager_set_enabled_list_update(EINA_TRUE);
		viewer_manager_refresh(TRUE);
		break;
	case WLAN_MANAGER_RESPONSE_TYPE_CONNECTION_FAIL_UNKNOWN:
		target_item = item_get_for_profile_name(det->profile_name);
		viewer_manager_header_mode_set(HEADER_MODE_ON);

		if (target_item == NULL) {
			ERROR_LOG(UG_NAME_RESP,"Error!!! Target item is NULL");
		} else {
			viewer_manager_item_radio_mode_set(NULL, target_item , VIEWER_ITEM_RADIO_MODE_OFF);
		}

		viewer_manager_set_enabled_list_update(EINA_TRUE);
		viewer_manager_refresh(TRUE);

		wlan_manager_network_syspopup_message("Network connection popup", "unable to connect");
		break;
	case WLAN_MANAGER_RESPONSE_TYPE_CONNECTION_FAIL_TIMEOUT:
		target_item = item_get_for_profile_name(det->profile_name);
		viewer_manager_header_mode_set(HEADER_MODE_ON);

		if (target_item == NULL) {
			ERROR_LOG(UG_NAME_RESP,"Error!!! Target item is NULL");
		} else {
			viewer_manager_item_radio_mode_set(NULL, target_item, VIEWER_ITEM_RADIO_MODE_OFF);
		}

		viewer_manager_set_enabled_list_update(EINA_TRUE);
		viewer_manager_refresh(TRUE);
		winset_popup_mode_set(NULL, POPUP_MODE_CONNECTING_FAILED, POPUP_OPTION_CONNECTING_FAILED_TIMEOUT);
		break;
	case WLAN_MANAGER_RESPONSE_TYPE_CONNECTION_FAIL_ALREADY_EXIST:
		target_item = item_get_for_profile_name(det->profile_name);
		viewer_manager_header_mode_set(HEADER_MODE_ON);

		if (target_item == NULL) {
			ERROR_LOG(UG_NAME_RESP,"Error!!! Target item is NULL");
		} else {
			viewer_manager_item_radio_mode_set(NULL, target_item , VIEWER_ITEM_RADIO_MODE_OFF);
		}

		viewer_manager_set_enabled_list_update(EINA_TRUE);
		wlan_manager_scanned_profile_refresh(TRUE);
		break;
	case WLAN_MANAGER_RESPONSE_TYPE_CONNECTION_FAIL_UNKNOWN_METHOD:
		target_item = item_get_for_profile_name(det->profile_name);
		viewer_manager_header_mode_set(HEADER_MODE_ON);

		if (target_item == NULL) {
			ERROR_LOG(UG_NAME_RESP, "Error!!! Target item is NULL");
		} else {
			viewer_manager_item_radio_mode_set(NULL, target_item , VIEWER_ITEM_RADIO_MODE_OFF);
		}

		viewer_manager_set_enabled_list_update(EINA_TRUE);
		wlan_manager_scanned_profile_refresh(TRUE);
		break;
	case WLAN_MANAGER_RESPONSE_TYPE_CONNECTION_FAIL_ABORTED:
		target_item = item_get_for_profile_name(det->profile_name);
		viewer_manager_header_mode_set(HEADER_MODE_ON);

		if (target_item == NULL) {
			ERROR_LOG(UG_NAME_RESP, "Error!!! Target item is NULL");
		} else {
			viewer_manager_item_radio_mode_set(NULL, target_item , VIEWER_ITEM_RADIO_MODE_OFF);
		}

		viewer_manager_set_enabled_list_update(EINA_TRUE);
		wlan_manager_scanned_profile_refresh(TRUE);
		break;
	case WLAN_MANAGER_RESPONSE_TYPE_DISCONNECTION_IND:
		INFO_LOG(UG_NAME_RESP, "DISCONNECTION IND");

		target_item = item_get_for_profile_name(det->profile_name);
		switch (header_state) {
		case HEADER_MODE_CONNECTING:
			if (target_item == NULL) {
				ERROR_LOG(UG_NAME_RESP,"Error!!! Target item is NULL");
			} else {
				genlist_data* gdata = (genlist_data*) viewer_list_item_data_get(target_item, "data");
				if (gdata->radio_mode == VIEWER_ITEM_RADIO_MODE_CONNECTING)
					viewer_manager_header_mode_set(HEADER_MODE_ON);

				viewer_manager_item_radio_mode_set(NULL, target_item, VIEWER_ITEM_RADIO_MODE_OFF);
			}
			break;
		case HEADER_MODE_CONNECTED:
		case HEADER_MODE_DISCONNECTING:
		case HEADER_MODE_ON:
			if (target_item == NULL) {
				ERROR_LOG(UG_NAME_RESP,"Error!!! Target item is NULL");
			} else {
				viewer_manager_item_radio_mode_set(NULL, target_item, VIEWER_ITEM_RADIO_MODE_OFF);
			}

			viewer_manager_header_mode_set(HEADER_MODE_ON);
			viewer_manager_set_enabled_list_update(EINA_TRUE);
			viewer_manager_refresh(TRUE);
			break;
		case HEADER_MODE_CANCEL_CONNECTING:
			viewer_manager_header_mode_set(HEADER_MODE_ON);
			viewer_manager_set_enabled_list_update(EINA_TRUE);
			viewer_manager_refresh(TRUE);
			break;
		case HEADER_MODE_DEACTIVATING:
		case HEADER_MODE_OFF:
		case HEADER_MODE_ACTIVATING:
		default:
			INFO_LOG(UG_NAME_RESP,"DISCONNECTION IND will not applied at header mode [%d]", header_state);
					break;
			}

		break;
	case WLAN_MANAGER_RESPONSE_TYPE_DISCONNECTION_OK:
		INFO_LOG(UG_NAME_RESP,"DISCONNECTION_OK");

		target_item = item_get_for_profile_name(det->profile_name);
		if (target_item == NULL) {
			ERROR_LOG(UG_NAME_RESP,"Error!!! Target item is NULL");
		} else {
			genlist_data* gdata = (genlist_data*) viewer_list_item_data_get(target_item, "data");
			if (gdata->radio_mode == VIEWER_ITEM_RADIO_MODE_WPS_CONNECTING) {
				viewer_manager_header_mode_set(HEADER_MODE_ON);
				viewer_manager_item_radio_mode_set(NULL, target_item, VIEWER_ITEM_RADIO_MODE_OFF);
				return;
			}

			viewer_manager_item_radio_mode_set(NULL, target_item, VIEWER_ITEM_RADIO_MODE_OFF);
		}

		switch (header_state) {
		case HEADER_MODE_CONNECTING:
		case HEADER_MODE_CANCEL_CONNECTING:
		case HEADER_MODE_CONNECTED:
		case HEADER_MODE_DISCONNECTING:
			viewer_manager_header_mode_set(HEADER_MODE_ON);
			break;
		case HEADER_MODE_DEACTIVATING:
			viewer_manager_header_mode_set(HEADER_MODE_OFF);
			break;
		case HEADER_MODE_OFF:
			viewer_manager_header_mode_set(HEADER_MODE_OFF);
			break;
		case HEADER_MODE_ON:
			viewer_manager_header_mode_set(HEADER_MODE_ON);
			break;
		case HEADER_MODE_ACTIVATING:
		default:
			if (det) {
				if (det->profile_name)
					g_free(det->profile_name);
				g_free(det);
			}
			return;
		}

		viewer_manager_set_enabled_list_update(EINA_TRUE);
		viewer_manager_current_selected_item_set(NULL);

		break;
	case WLAN_MANAGER_RESPONSE_TYPE_WPS_ENROLL_OK:
		target_item = item_get_for_profile_name(det->profile_name);
		switch(header_state) {
		case HEADER_MODE_CONNECTING:
		case HEADER_MODE_CONNECTED:
		case HEADER_MODE_DISCONNECTING:
		case HEADER_MODE_ON:
			if(target_item == NULL) {
				ERROR_LOG(UG_NAME_RESP,"Error!!! Target item is NULL");
			} else {
				winset_popup_mode_set(NULL, POPUP_MODE_OFF, POPUP_OPTION_NONE);
				view_ime_password_destroy();
				viewer_manager_item_radio_mode_set(NULL, target_item , VIEWER_ITEM_RADIO_MODE_CONNECTED);
			}

			viewer_manager_set_enabled_list_update(EINA_TRUE);
			viewer_manager_refresh(TRUE);

			viewer_manager_header_mode_set(HEADER_MODE_CONNECTED);
			break;
		case HEADER_MODE_DEACTIVATING:
		case HEADER_MODE_OFF:
		case HEADER_MODE_ACTIVATING:
		default:
			break;
		}
		break;
	case WLAN_MANAGER_RESPONSE_TYPE_WPS_ENROLL_FAIL:
		target_item = item_get_for_profile_name(det->profile_name);
		if (target_item == NULL) {
			ERROR_LOG(UG_NAME_RESP,"Error!!! Target item is NULL");
		} else {
			viewer_manager_item_radio_mode_set(NULL, target_item, VIEWER_ITEM_RADIO_MODE_OFF);
		}

		switch (header_state) {
		case HEADER_MODE_CONNECTING:
		case HEADER_MODE_CANCEL_CONNECTING:
		case HEADER_MODE_CONNECTED:
		case HEADER_MODE_DISCONNECTING:
			viewer_manager_header_mode_set(HEADER_MODE_ON);
			break;
		case HEADER_MODE_DEACTIVATING:
			viewer_manager_header_mode_set(HEADER_MODE_OFF);
			break;
		case HEADER_MODE_OFF:
			viewer_manager_header_mode_set(HEADER_MODE_OFF);
			break;
		case HEADER_MODE_ON:
			viewer_manager_header_mode_set(HEADER_MODE_ON);
			break;
		case HEADER_MODE_ACTIVATING:
		default:
			if (det) {
				if (det->profile_name)
					g_free(det->profile_name);
				g_free(det);
			}
			return;
		}

		winset_popup_mode_set(NULL, POPUP_MODE_OFF, POPUP_OPTION_NONE);
		wlan_manager_network_syspopup_message("Network connection popup", "unable to connect");
		break;
	case WLAN_MANAGER_RESPONSE_TYPE_CANCEL_WPS_ENROLL_OK:
		target_item = item_get_for_profile_name(det->profile_name);
		if (target_item == NULL) {
			ERROR_LOG(UG_NAME_RESP,"Error!!! Target item is NULL");
		} else {
			viewer_manager_item_radio_mode_set(NULL, target_item, VIEWER_ITEM_RADIO_MODE_OFF);
		}

		switch(header_state){
		case HEADER_MODE_CONNECTING:
		case HEADER_MODE_CANCEL_CONNECTING:
		case HEADER_MODE_CONNECTED:
		case HEADER_MODE_DISCONNECTING:
			viewer_manager_header_mode_set(HEADER_MODE_ON);
			break;
		case HEADER_MODE_DEACTIVATING:
			viewer_manager_header_mode_set(HEADER_MODE_OFF);
			break;
		case HEADER_MODE_OFF:
			viewer_manager_header_mode_set(HEADER_MODE_OFF);
			break;
		case HEADER_MODE_ON:
			viewer_manager_header_mode_set(HEADER_MODE_ON);
			break;
		case HEADER_MODE_ACTIVATING:
		default:
			if (det) {
				if (det->profile_name)
					g_free(det->profile_name);
				g_free(det);
			}
			return;
		}
		break;
	case WLAN_MANAGER_RESPONSE_TYPE_CANCEL_WPS_ENROLL_FAIL:
		break;
	case WLAN_MANAGER_RESPONSE_TYPE_MAC_ID_IND:
		INFO_LOG(UG_NAME_RESP, "MAC_ID_IND");
		break;
	case WLAN_MANAGER_RESPONSE_TYPE_NONE:
	default:
		winset_popup_simple_set("[ERROR] Unregistered WLAN_EVENT number");
		break;
	}

	__COMMON_FUNC_EXIT__;

	if (det) {
		if (det->profile_name)
			g_free(det->profile_name);
		g_free(det);
	}

	return;
}

void wlan_engine_refresh_callback(int is_scan)
{
	__COMMON_FUNC_ENTER__;
	viewer_manager_refresh(is_scan);
	__COMMON_FUNC_EXIT__;
}
