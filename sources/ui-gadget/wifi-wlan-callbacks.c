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



#include "wifi.h"
#include "wifi-ui-list-callbacks.h"
#include "wlan_manager.h"
#include "viewer_manager.h"
#include "winset_popup.h"
#include "wifi-engine-callback.h"
#include "viewer_list.h"
#include "motion_control.h"

extern wifi_appdata *ug_app_state;

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
	ug_genlist_data_t* gdata = NULL;

	int header_state = -1;
	header_state = viewer_manager_header_mode_get();

	INFO_LOG(UG_NAME_NORMAL, "header state [%d]", header_state);
	INFO_LOG(UG_NAME_NORMAL, "daemon message [%d]", det->type);
	INFO_LOG(UG_NAME_NORMAL, "profile_name [%s]", det->profile_name);

	if (det->profile_name[0]) {	/* Is it a response with a profile name? */

		/* All responses with profile names should have an associated genlist item
		 * Verify if the genlist item exists and associated genlist item data exists
		 */
		target_item = item_get_for_profile_name(det->profile_name);
		if (!target_item ||
			!(gdata = (ug_genlist_data_t *)elm_object_item_data_get(target_item))) {
			/* This is a case where the profile name exists but no
			 * associated genlist item OR genlist item data exists.
			 * This condition can come when an AP action(Example connect)
			 * is triggered and by the time the response came the genlist is cleared.
			 */
			ERROR_LOG(UG_NAME_RESP,"Error!!! Target item[0x%x] is NULL OR item data[0x%x] is NULL", target_item, gdata);
			if (det->type == WLAN_MANAGER_RESPONSE_TYPE_CONNECTING ||
				det->type == WLAN_MANAGER_RESPONSE_TYPE_CONNECTION_OK ||
				det->type == WLAN_MANAGER_RESPONSE_TYPE_CONNECTION_IND) {
					/* This situation comes during hidden AP connecting/connected event.
					 * Anyways its always better to add the connecting/connected AP */
					target_item = viewer_manager_add_new_item(det->profile_name);
					if (!target_item ||
						!(gdata = (ug_genlist_data_t *)elm_object_item_data_get(target_item))) {
						ERROR_LOG(UG_NAME_RESP, "Error!!! Fatal: Unable to add a connecting/connected item.");
						__COMMON_FUNC_EXIT__;
						return;
					}
			} else {
				ERROR_LOG(UG_NAME_RESP, "Fatal: target_item or gdata is NULL");
				__COMMON_FUNC_EXIT__;
				return;
			}

		}
	}

	/* All OK to process the response */

	switch (det->type) {
	case WLAN_MANAGER_RESPONSE_TYPE_POWER_ON_OK:
		if (WLAN_MANAGER_ERR_NONE == wlan_manager_request_scan()) { /* First scan request after power on */
			INFO_LOG(UG_NAME_RESP,"Scan request is success\n");
			connman_request_scan_mode_set(WIFI_BGSCAN_MODE_PERIODIC);
			viewer_manager_header_mode_set(HEADER_MODE_SEARCHING);
			viewer_manager_hidden_disable_set(TRUE);
			viewer_manager_show(VIEWER_WINSET_SEARCHING);
			viewer_manager_show(VIEWER_WINSET_SUB_CONTENTS);
		} else {	/* First scan request failed. Anyways lets set the state to ON and allow user to continue */
			INFO_LOG(UG_NAME_RESP,"Scan request failed\n");
			viewer_manager_header_mode_set(HEADER_MODE_ON);
			viewer_manager_hidden_disable_set(FALSE);
			viewer_manager_hide(VIEWER_WINSET_SEARCHING);
			viewer_manager_hide(VIEWER_WINSET_SUB_CONTENTS);
		}

		motion_stop();
		break;

	case WLAN_MANAGER_RESPONSE_TYPE_POWER_OFF_OK:
		viewer_list_item_clear();
		wlan_manager_reset_connected_AP();
		viewer_manager_header_mode_set(HEADER_MODE_OFF);
		viewer_manager_show(VIEWER_WINSET_SEARCHING);
		viewer_manager_hide(VIEWER_WINSET_SUB_CONTENTS);

		motion_start();
		break;

	case WLAN_MANAGER_RESPONSE_TYPE_POWER_ON_NOT_SUPPORTED:
		viewer_manager_header_mode_set(HEADER_MODE_OFF);
		wlan_manager_network_syspopup_message("Network connection popup", "not support", "notification");
		break;

	case WLAN_MANAGER_RESPONSE_TYPE_POWER_ON_RESTRICTED:
		viewer_manager_header_mode_set(HEADER_MODE_OFF);
		wlan_manager_network_syspopup_message("Network connection popup", "wifi restricted", "popup");
		break;

	case WLAN_MANAGER_RESPONSE_TYPE_SCAN_OK:			/* Manual scan complete response */
		if (HEADER_MODE_DEACTIVATING == header_state || HEADER_MODE_OFF == header_state) {
			break;
		}
		viewer_manager_hidden_disable_set(FALSE);
		viewer_manager_hide(VIEWER_WINSET_SEARCHING);
	case WLAN_MANAGER_RESPONSE_TYPE_SCAN_RESULT_IND:	/* Auto scan complete response */
		INFO_LOG(UG_NAME_RESP,"Scan complete..\n");
		wlan_manager_scanned_profile_refresh();
		break;

	case WLAN_MANAGER_RESPONSE_TYPE_CONNECTING:
		target_item = viewer_manager_move_item_to_top(target_item);
		viewer_manager_header_mode_set(HEADER_MODE_CONNECTING);
		viewer_manager_item_radio_mode_set(NULL, target_item, VIEWER_ITEM_RADIO_MODE_CONNECTING);
		viewer_manager_scroll_to_top();
		break;

	case WLAN_MANAGER_RESPONSE_TYPE_CONNECTION_IND:
	case WLAN_MANAGER_RESPONSE_TYPE_CONNECTION_OK:
		target_item = viewer_manager_move_item_to_top(target_item);
		viewer_manager_item_radio_mode_set(NULL, target_item , VIEWER_ITEM_RADIO_MODE_CONNECTED);
		viewer_manager_header_mode_set(HEADER_MODE_CONNECTED);
		viewer_manager_scroll_to_top();
		break;

	case WLAN_MANAGER_RESPONSE_TYPE_WPS_ENROLL_OK:
		target_item = viewer_manager_move_item_to_top(target_item);
		viewer_manager_item_radio_mode_set(NULL, target_item , VIEWER_ITEM_RADIO_MODE_CONNECTED);
		viewer_manager_header_mode_set(HEADER_MODE_CONNECTED);
		viewer_manager_scroll_to_top();
		if (ug_app_state->passpopup) {	/* This is needed to remove the PBC timer popup */
			common_pswd_popup_destroy(ug_app_state->passpopup);
			ug_app_state->passpopup = NULL;
		}
		break;

	case WLAN_MANAGER_RESPONSE_TYPE_CONNECTION_FAIL_IN_PROGRESS:
	case WLAN_MANAGER_RESPONSE_TYPE_CONNECTION_FAIL_UNKNOWN:
	case WLAN_MANAGER_RESPONSE_TYPE_CONNECTION_FAIL_ALREADY_EXIST:
	case WLAN_MANAGER_RESPONSE_TYPE_CONNECTION_FAIL_ABORTED:
		viewer_manager_header_mode_set(HEADER_MODE_ON);
		viewer_manager_item_radio_mode_set(NULL, target_item, VIEWER_ITEM_RADIO_MODE_OFF);

		if (ug_app_state->passpopup) {
			common_pswd_popup_destroy(ug_app_state->passpopup);
			ug_app_state->passpopup = NULL;
		}
		break;

	case WLAN_MANAGER_RESPONSE_TYPE_CONNECTION_INVALID_KEY:
	case WLAN_MANAGER_RESPONSE_TYPE_CONNECTION_CONNECT_FAILED:
	case WLAN_MANAGER_RESPONSE_TYPE_CONNECTION_FAIL_TIMEOUT:
	case WLAN_MANAGER_RESPONSE_TYPE_CONNECTION_FAIL_UNKNOWN_METHOD:
		viewer_manager_header_mode_set(HEADER_MODE_ON);
		viewer_manager_item_radio_mode_set(NULL, target_item , VIEWER_ITEM_RADIO_MODE_OFF);

		if (ug_app_state->passpopup) {
			common_pswd_popup_destroy(ug_app_state->passpopup);
			ug_app_state->passpopup = NULL;
		}
		break;

	case WLAN_MANAGER_RESPONSE_TYPE_DISCONNECTION_IND:
	case WLAN_MANAGER_RESPONSE_TYPE_DISCONNECTION_OK:
		INFO_LOG(UG_NAME_RESP, "DISCONNECTION IND");

		viewer_manager_item_radio_mode_set(NULL, target_item, VIEWER_ITEM_RADIO_MODE_OFF);
		viewer_manager_header_mode_set(HEADER_MODE_ON);
		break;

	case WLAN_MANAGER_RESPONSE_TYPE_WPS_ENROLL_FAIL:
		viewer_manager_item_radio_mode_set(NULL, target_item, VIEWER_ITEM_RADIO_MODE_OFF);
		viewer_manager_header_mode_set(HEADER_MODE_ON);

		if (ug_app_state->passpopup) {
			common_pswd_popup_destroy(ug_app_state->passpopup);
			ug_app_state->passpopup = NULL;
		}
		break;

	case WLAN_MANAGER_RESPONSE_TYPE_CANCEL_WPS_ENROLL_OK:
		viewer_manager_item_radio_mode_set(NULL, target_item, VIEWER_ITEM_RADIO_MODE_OFF);
		viewer_manager_header_mode_set(HEADER_MODE_ON);
		break;

	case WLAN_MANAGER_RESPONSE_TYPE_CANCEL_WPS_ENROLL_FAIL:
		INFO_LOG(UG_NAME_RESP, "CANCEL_WPS_ENROLL_FAIL");
		break;

	case WLAN_MANAGER_RESPONSE_TYPE_MAC_ID_IND:
		INFO_LOG(UG_NAME_RESP, "MAC_ID_IND");
		break;

	case WLAN_MANAGER_RESPONSE_TYPE_SPECIFIC_SCAN_IND:
		if (ug_app_state->hidden_ap_popup) {
			viewer_manager_specific_scan_response_hlr(user_data);
		}
		break;
	case WLAN_MANAGER_RESPONSE_TYPE_SPECIFIC_SCAN_FAIL:
		if (ug_app_state->hidden_ap_popup) {
			INFO_LOG(UG_NAME_RESP, "Hidden AP destroy \n");
			view_hidden_ap_popup_destroy(ug_app_state->hidden_ap_popup);
			ug_app_state->hidden_ap_popup = NULL;
		}
		break;
	case WLAN_MANAGER_RESPONSE_TYPE_SPECIFIC_SCAN_OK:
		INFO_LOG(UG_NAME_RESP, "WLAN_MANAGER_RESPONSE_TYPE_SPECIFIC_SCAN_OK");
		break;

	case WLAN_MANAGER_RESPONSE_TYPE_NONE:
	default:
		winset_popup_mode_set(ug_app_state->popup_manager, POPUP_OPTION_UNREG_WLAN_EVENT_ERROR, NULL);
	}

	__COMMON_FUNC_EXIT__;
	return;
}

void wlan_engine_refresh_callback(void)
{
	__COMMON_FUNC_ENTER__;
	viewer_manager_refresh();
	__COMMON_FUNC_EXIT__;
}
