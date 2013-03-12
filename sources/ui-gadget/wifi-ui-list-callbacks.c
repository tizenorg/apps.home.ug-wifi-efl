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
#include "view_detail.h"
#include "viewer_list.h"
#include "common_pswd_popup.h"
#include "common_eap_connect.h"
#include "winset_popup.h"
#include "i18nmanager.h"
#include "common_datamodel.h"

extern wifi_appdata *ug_app_state;

static void _popup_cancel_cb(void *data,  Evas_Object *obj, void *event_info)
{
	__COMMON_FUNC_ENTER__;
	if (!ug_app_state->passpopup || !data) {
		return;
	}

	char *profile_name = (char *)data;
	common_pswd_popup_destroy(ug_app_state->passpopup);
	ug_app_state->passpopup = NULL;
	g_free(profile_name);
	__COMMON_FUNC_EXIT__;
}

static void _popup_ok_cb(void *data, Evas_Object *obj, void *event_info)
{
	__COMMON_FUNC_ENTER__;
	if (!ug_app_state->passpopup || !data) {
		return;
	}

	char *profile_name = (char *)data;
	wlan_security_mode_type_t sec_mode;
	view_datamodel_basic_info_t *basic_data_model = view_basic_detail_datamodel_create(profile_name);
	if (!basic_data_model) {
		common_pswd_popup_destroy(ug_app_state->passpopup);
		ug_app_state->passpopup = NULL;
		g_free(profile_name);
		return;
	}
	int ret = WLAN_MANAGER_ERR_NONE;
	int nLen = 0;
	const char* szPassword = common_pswd_popup_get_txt(ug_app_state->passpopup);
	nLen = strlen(szPassword);
	INFO_LOG(UG_NAME_NORMAL, "password = [%s]", szPassword);

	sec_mode = view_detail_datamodel_sec_mode_get(basic_data_model);
	switch (sec_mode) {
	case WLAN_SEC_MODE_WEP:

		if (nLen != 5 && nLen != 13 && nLen != 26 && nLen != 10) {
			winset_popup_mode_set(ug_app_state->popup_manager, POPUP_OPTION_WEP_PSWD_LEN_ERROR, NULL);
			goto popup_ok_cb_exit;
		}

		break;

	case WLAN_SEC_MODE_WPA_PSK:
	case WLAN_SEC_MODE_WPA2_PSK:

		if (nLen < 8 || nLen > 63) {
			winset_popup_mode_set(ug_app_state->popup_manager, POPUP_OPTION_WPA_PSWD_LEN_ERROR, NULL);
			goto popup_ok_cb_exit;
		}
		break;

	default:
		ERROR_LOG(UG_NAME_SCAN, "Fatal: Wrong security mode : %d", sec_mode);
		common_pswd_popup_destroy(ug_app_state->passpopup);
		ug_app_state->passpopup = NULL;
		g_free(profile_name);
		goto popup_ok_cb_exit;
	}

	common_pswd_popup_destroy(ug_app_state->passpopup);
	ug_app_state->passpopup = NULL;

	INFO_LOG(UG_NAME_SCAN, "connect with password comp");
	wlan_manager_password_data param;
	memset(&param, 0, sizeof(wlan_manager_password_data));
	param.wlan_eap_type = WLAN_MANAGER_EAP_TYPE_NONE;
	param.password = szPassword;
	ret = wlan_manager_connect_with_password(profile_name, sec_mode, &param);
	if (WLAN_MANAGER_ERR_NONE == ret) {
		viewer_manager_header_mode_set(HEADER_MODE_CONNECTING);
	} else {
		ERROR_LOG(UG_NAME_SCAN, "wlan error %d", ret);
		viewer_manager_header_mode_set(HEADER_MODE_ON);
	}

	g_free(profile_name);

popup_ok_cb_exit:
	g_free((gpointer)szPassword);
	view_basic_detail_datamodel_destroy(basic_data_model);

	__COMMON_FUNC_EXIT__;
}

static void _wps_pbc_popup_cancel_connecting(void* data, Evas_Object* obj, void* event_info)
{
	if (!ug_app_state->passpopup || !data) {
		return;
	}

	char *profile_name = (char *)data;
	int ret = wlan_manager_request_cancel_wps_connection(profile_name);
	if (ret == WLAN_MANAGER_ERR_NONE) {
		INFO_LOG(UG_NAME_NORMAL, "WPS conection cancelled successfully for AP[%s]", profile_name);
	} else {
		ERROR_LOG(UG_NAME_NORMAL, "Error!!! wlan_manager_request_cancel_wps_connection failed for AP[%s]", profile_name);
	}
	common_pswd_popup_destroy(ug_app_state->passpopup);
	ug_app_state->passpopup = NULL;
	viewer_manager_header_mode_set(HEADER_MODE_ON);
	g_free(profile_name);
	return;
}

static void _wps_btn_cb(void* data, Evas_Object* obj, void* event_info)
{
	__COMMON_FUNC_ENTER__;
	if (!ug_app_state->passpopup || !data) {
		return;
	}

	char *profile_name = (char *)data;
	int ret = wlan_manager_request_wps_connection(profile_name);
	if (ret == WLAN_MANAGER_ERR_NONE) {
		viewer_manager_header_mode_set(HEADER_MODE_CONNECTING);
		common_pswd_popup_pbc_popup_create(ug_app_state->passpopup, _wps_pbc_popup_cancel_connecting, profile_name);
	} else {
		ERROR_LOG(UG_NAME_NORMAL, "Error!!! wlan_manager_request_wps_connection failed");
		common_pswd_popup_destroy(ug_app_state->passpopup);
		ug_app_state->passpopup = NULL;
		g_free(profile_name);
	}
	__COMMON_FUNC_EXIT__;
}

void eap_view_close_cb(void)
{
	ug_app_state->eap_view = NULL;
}

void radio_button_cb(void *data, Evas_Object *obj, void *event_info)
{
	__COMMON_FUNC_ENTER__;

	int ret = WLAN_MANAGER_ERR_UNKNOWN;
	Elm_Object_Item *it = (Elm_Object_Item *)event_info;
	wifi_device_info_t *device_info = (wifi_device_info_t *)data;

	if (!it || !device_info || device_info->ssid == NULL) {
		ERROR_LOG(UG_NAME_NORMAL, "Error!!! Invalid inout params");
		__COMMON_FUNC_EXIT__;
		return;
	}

	ug_genlist_data_t *gdata = (ug_genlist_data_t *) elm_object_item_data_get((Elm_Object_Item*)it);
	if(NULL == gdata) {
		ERROR_LOG(UG_NAME_NORMAL, "Error!!! list item data null");
		__COMMON_FUNC_EXIT__;
		return;
	}

	int item_state = gdata->radio_mode;
	int current_state = 0;

	INFO_LOG(UG_NAME_NORMAL, "ssid --- %s", device_info->ssid);
	INFO_LOG(UG_NAME_NORMAL, "current item_state state is --- %d\n", item_state);

	switch (item_state) {
		case VIEWER_ITEM_RADIO_MODE_OFF:
			current_state = viewer_manager_header_mode_get();

			INFO_LOG(UG_NAME_NORMAL, "Clicked AP`s information\n");
			INFO_LOG(UG_NAME_NORMAL, "header mode [%d]", current_state);

			switch (current_state) {
				case HEADER_MODE_CONNECTED:
				case HEADER_MODE_ON:
					ret = wlan_manager_request_connection(device_info);
					if (ret == WLAN_MANAGER_ERR_NONE) {
						viewer_manager_header_mode_set(HEADER_MODE_CONNECTING);
					}
					break;

				case HEADER_MODE_OFF:
				case HEADER_MODE_SEARCHING:
				case HEADER_MODE_ACTIVATING:
				case HEADER_MODE_CONNECTING:
				case HEADER_MODE_DISCONNECTING:
				case HEADER_MODE_DEACTIVATING:
				default:
					INFO_LOG(UG_NAME_NORMAL, "Ignore the item click");
					break;
			}
			break;

		case VIEWER_ITEM_RADIO_MODE_CONNECTED:
			INFO_LOG(UG_NAME_NORMAL, "want to disconnect for connected item");
			ret = wlan_manager_request_disconnection(device_info);
			if(ret == WLAN_MANAGER_ERR_NONE){
				viewer_manager_header_mode_set(HEADER_MODE_DISCONNECTING);
			}
			break;

		case VIEWER_ITEM_RADIO_MODE_CONNECTING:
			INFO_LOG(UG_NAME_NORMAL, "want to cancel connecting for connected item");
			ret = wlan_manager_request_cancel_connecting(device_info->profile_name);
			if(ret == WLAN_MANAGER_ERR_NONE){
				viewer_manager_header_mode_set(HEADER_MODE_CANCEL_CONNECTING);
			} 
			break;

		default:
			ret = WLAN_MANAGER_ERR_UNKNOWN;
			break;
	}

	switch (ret) {
	case WLAN_MANAGER_ERR_NONE:
		INFO_LOG(UG_NAME_NORMAL, "ERROR_NONE");
		break;
	case WLAN_MANAGER_ERR_CONNECT_PASSWORD_NEEDED:
	{
		pswd_popup_create_req_data_t	popup_info;
		popup_info.title = device_info->ssid;
		popup_info.ok_cb = _popup_ok_cb;
		popup_info.cancel_cb = _popup_cancel_cb;
		popup_info.show_wps_btn = device_info->wps_mode;
		popup_info.wps_btn_cb = _wps_btn_cb;
		popup_info.cb_data = g_strdup(gdata->device_info->profile_name);
		INFO_LOG(UG_NAME_NORMAL, "Going to create a popup. ug_app_state = 0x%x", ug_app_state);
		ug_app_state->passpopup = common_pswd_popup_create(ug_app_state->win_main, PACKAGE, &popup_info);
		INFO_LOG(UG_NAME_NORMAL, "After create a popup");
		if (ug_app_state->passpopup == NULL) {
			INFO_LOG(UG_NAME_ERR, "pass popup create failed !");
		}
	}
	break;

	case WLAN_MANAGER_ERR_CONNECT_EAP_SEC_TYPE:
	{
		Evas_Object* navi_frame = viewer_manager_get_naviframe();
		if (navi_frame == NULL) {
			ERROR_LOG(UG_NAME_NORMAL, "Failed : get naviframe");
			return;
		}
		ug_app_state->eap_view = create_eap_connect(ug_app_state->win_main, navi_frame, PACKAGE, device_info, eap_view_close_cb);
	}
	break;

	case WLAN_MANAGER_ERR_NOSERVICE:
		break;

	default:
		ERROR_LOG(UG_NAME_NORMAL, "errro code [%d]", ret);
		break;
	}

	__COMMON_FUNC_EXIT__;
	return;
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

	view_detail(device_info, ug_app_state->win_main);

	__COMMON_FUNC_EXIT__;
}

