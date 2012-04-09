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
#include "view_detail.h"
#include "viewer_list.h"
#include "view_ime_password.h"
#include "view_eap.h"
#include "popup.h"


void radio_button_cb(void *data, Evas_Object *obj, void *event_info)
{
	__COMMON_FUNC_ENTER__;

	int ret = WLAN_MANAGER_ERR_UNKNOWN;
	if (data == NULL) {
		ERROR_LOG(UG_NAME_NORMAL, "Error!!! Invalid argument device_info");
		goto radio_button_cb_process_end;
	}

	//selected UI and it`s data
	Elm_Object_Item *it = (Elm_Object_Item *)event_info;
	wifi_device_info_t *device_info = (wifi_device_info_t *)data;

	if (device_info->ssid == NULL) {
		ERROR_LOG(UG_NAME_NORMAL, "Error!!! Invalid argument device_info ssid");
		goto radio_button_cb_process_end;
	}

	viewer_manager_set_enabled_list_update(EINA_FALSE);
	viewer_manager_current_selected_item_set(it);

	genlist_data* gdata = (genlist_data*) viewer_list_item_data_get((Elm_Object_Item*)it, "data");
	if(NULL == gdata) {
		ERROR_LOG(UG_NAME_NORMAL, "Error!!! list item data null");
		goto radio_button_cb_process_end;
	}

	int item_state = gdata->radio_mode;

	INFO_LOG(UG_NAME_NORMAL, "ssid --- %s", device_info->ssid);
	INFO_LOG(UG_NAME_NORMAL, "current item_state state is --- %d\n", item_state);
	INFO_LOG(UG_NAME_NORMAL, "selected item_state %d", item_state);

	switch (item_state) {
		case VIEWER_ITEM_RADIO_MODE_NULL: //this is OFF too. of course, NULL should be un-state
		case VIEWER_ITEM_RADIO_MODE_OFF:
			break;
		case VIEWER_ITEM_RADIO_MODE_CONNECTED:
			INFO_LOG(UG_NAME_NORMAL, "want to disconnect for connected item");
			viewer_manager_item_radio_mode_set(NULL, it, VIEWER_ITEM_RADIO_MODE_DISCONNECTING);
			ret = wlan_manager_request_disconnection(device_info);
			if(ret == WLAN_MANAGER_ERR_NONE){
				viewer_manager_item_radio_mode_set(NULL, it, VIEWER_ITEM_RADIO_MODE_DISCONNECTING);
				viewer_manager_header_mode_set(HEADER_MODE_DISCONNECTING);
				viewer_manager_item_radio_mode_set(NULL, it, VIEWER_ITEM_RADIO_MODE_DISCONNECTING);

			} else {
				viewer_manager_item_radio_mode_set(NULL, it , VIEWER_ITEM_RADIO_MODE_OFF);
			}

			goto radio_button_cb_process_end;

		case VIEWER_ITEM_RADIO_MODE_CONNECTING:
			INFO_LOG(UG_NAME_NORMAL, "want to cancel connecting for connected item");

			ret = wlan_manager_request_cancel_connecting(device_info->profile_name);
			if(ret == WLAN_MANAGER_ERR_NONE){
				viewer_manager_item_radio_mode_set(NULL, it, VIEWER_ITEM_RADIO_MODE_CANCEL_CONNECTING);
				viewer_manager_header_mode_set(HEADER_MODE_CANCEL_CONNECTING);
				viewer_manager_item_radio_mode_set(NULL, it, VIEWER_ITEM_RADIO_MODE_CANCEL_CONNECTING);
			} 
			goto radio_button_cb_process_end;

		default:
			ret = WLAN_MANAGER_ERR_UNKNOWN;
			goto radio_button_cb_process_end;
	}

	int current_state = -1;
	current_state = viewer_manager_header_mode_get();

	INFO_LOG(UG_NAME_NORMAL, "Clicked AP`s information\n");
	INFO_LOG(UG_NAME_NORMAL, "header mode [%d]", current_state);

	switch (current_state) {
		case HEADER_MODE_OFF:
			INFO_LOG(UG_NAME_NORMAL, "you can not connect when header off");
			break;
		case HEADER_MODE_CONNECTED:
			INFO_LOG(UG_NAME_NORMAL, "header mode connected. disconnect and connect needed");
		case HEADER_MODE_ON:
			viewer_manager_item_radio_mode_set(NULL, it, VIEWER_ITEM_RADIO_MODE_CONNECTING);

			ret = wlan_manager_request_connection(device_info);
			if (ret == WLAN_MANAGER_ERR_NONE) {
				Elm_Object_Item* target_item = viewer_manager_current_selected_item_get();
				viewer_manager_item_radio_mode_set(NULL, target_item, VIEWER_ITEM_RADIO_MODE_CONNECTING);
				viewer_manager_header_mode_set(HEADER_MODE_CONNECTING);
				viewer_manager_item_radio_mode_set(NULL, target_item, VIEWER_ITEM_RADIO_MODE_CONNECTING);
			} else {
				viewer_manager_item_radio_mode_set(NULL, it, VIEWER_ITEM_RADIO_MODE_OFF);
			}
			break;

		case HEADER_MODE_SEARCHING:
		case HEADER_MODE_ACTIVATING:
		case HEADER_MODE_CONNECTING:
		case HEADER_MODE_DISCONNECTING:
		case HEADER_MODE_DEACTIVATING:
			viewer_manager_set_enabled_list_update(EINA_TRUE);
			viewer_manager_current_selected_item_set(NULL);
			INFO_LOG(UG_NAME_NORMAL, "Disable input");
			return;

		default:
			INFO_LOG(UG_NAME_NORMAL, "default");
			break;
	}

radio_button_cb_process_end:

	switch (ret) {
	case WLAN_MANAGER_ERR_NONE:
		INFO_LOG(UG_NAME_NORMAL, "ERROR_NONE");
		break;
	case WLAN_MANAGER_ERR_CONNECT_PASSWORD_NEEDED:
		view_ime_password(device_info);
		break;
	case WLAN_MANAGER_ERR_CONNECT_EAP_SEC_TYPE:
		view_eap(device_info);
		break;
	case WLAN_MANAGER_ERR_NOSERVICE:
		viewer_manager_set_enabled_list_update(EINA_TRUE);
		wlan_manager_scanned_profile_refresh(TRUE);
		break;
	default:
		viewer_manager_set_enabled_list_update(EINA_TRUE);
		winset_popup_mode_set(NULL, POPUP_MODE_CONNECTING_FAILED, POPUP_OPTION_CONNECTING_FAILED_INVALID_OPERATION);
		ERROR_LOG(UG_NAME_NORMAL, "errro code [%d]", ret);
		break;
	}

	__COMMON_FUNC_EXIT__;
}

void list_select_cb(void *data, Evas_Object *obj, void *event_info)
{
	__COMMON_FUNC_ENTER__;

	INFO_LOG(UG_NAME_NORMAL,"=================\n");
	INFO_LOG(UG_NAME_NORMAL," %s %d\n", __func__ ,__LINE__);
	INFO_LOG(UG_NAME_NORMAL,"=================\n");

	if (data == NULL) {
		__COMMON_FUNC_EXIT__;
		return;
	}

	wifi_device_info_t *device_info = (wifi_device_info_t *)data;

	DEBUG_LOG(UG_NAME_NORMAL, "ssid [%s]", device_info->ssid);

	viewer_manager_current_selected_item_set((Elm_Object_Item *)obj);

	view_detail(device_info);

	__COMMON_FUNC_EXIT__;
}

