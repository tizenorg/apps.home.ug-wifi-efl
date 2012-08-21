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



#include "common.h"
#include "view-main.h"
#include "common_pswd_popup.h"
#include "common_utils.h"
#include "view-alerts.h"
#include "i18nmanager.h"
#include "common_eap_connect.h"

extern wifi_object* syspopup_app_state;

static Evas_Object* list = NULL;
static Elm_Genlist_Item_Class itc;

int view_main_item_connection_mode_set(syspopup_genlist_data_t *data, ITEM_CONNECTION_MODES mode)
{
	__COMMON_FUNC_ENTER__;

	if (data == NULL) {
		WARN_LOG(SP_NAME_NORMAL, "NULL == data");
		__COMMON_FUNC_EXIT__;
		return FALSE;
	}

	if (ITEM_CONNECTION_MODE_NULL >= mode || ITEM_CONNECTION_MODE_MAX <= mode) {
		ERROR_LOG(SP_NAME_NORMAL, "mode[%d] is not valid", mode);
		__COMMON_FUNC_EXIT__;
		return FALSE;
	}

	/* mode set */
	data->connection_mode = mode;
	INFO_LOG(SP_NAME_NORMAL, "mode: [%d]", data->connection_mode);

	elm_genlist_item_update(data->it);

	__COMMON_FUNC_EXIT__;

	return TRUE;
}

static ITEM_CONNECTION_MODES view_main_state_get(Evas_Object *glist)
{
	__COMMON_FUNC_ENTER__;

	if (glist == NULL)
		return ITEM_CONNECTION_MODE_NULL;

	Elm_Object_Item* it = NULL;
	it = elm_genlist_first_item_get(glist);
	while (it) {
		syspopup_genlist_data_t *gdata = (syspopup_genlist_data_t *)elm_object_item_data_get(it);
		if (gdata->connection_mode == ITEM_CONNECTION_MODE_CONNECTING) {
			return ITEM_CONNECTION_MODE_CONNECTING;
		}

		it = elm_genlist_item_next_get(it);
	}

	__COMMON_FUNC_EXIT__;
	return ITEM_CONNECTION_MODE_OFF;
}

static void _popup_ok_cb(void *data, Evas_Object *obj, void *event_info)
{
	__COMMON_FUNC_ENTER__;
	if (!syspopup_app_state->passpopup) {
		return;
	}
	assertm_if(NULL == data, "data is NULL!!");
	assertm_if(NULL == obj, "obj is NULL!!");
	assertm_if(NULL == event_info, "event_info is NULL!!");

	syspopup_genlist_data_t *gdata = (syspopup_genlist_data_t *)data;
	char *profile_name = gdata->dev_info->profile_name;
	wlan_security_mode_type_t sec_mode = gdata->dev_info->security_mode;

	char* password = NULL;
	int len_password = 0;
	int ret = WLAN_MANAGER_ERR_UNKNOWN;
	password = common_pswd_popup_get_txt(syspopup_app_state->passpopup);
	len_password = strlen(password);
	INFO_LOG(SP_NAME_NORMAL, "* password len [%d]", len_password);

	switch (sec_mode) {
	case WLAN_SEC_MODE_WEP:
		if (len_password == 5 || len_password == 13 || len_password == 26 || len_password == 10) {
			wlan_manager_password_data* param = (wlan_manager_password_data *)g_malloc0(sizeof(wlan_manager_password_data));
			assertm_if(NULL == param, "param is NULL!!");

			param->wlan_eap_type = WLAN_MANAGER_EAP_TYPE_NONE;
			param->password = password;

			ret = wlan_manager_connect_with_password(profile_name, sec_mode, param);
			g_free(param);

		} else {
			view_alerts_popup_show(WEP_WRONG_PASSWORD_LEN_ERR_MSG_STR);
			goto popup_ok_exit;
		}
		break;

	case WLAN_SEC_MODE_WPA_PSK:
	case WLAN_SEC_MODE_WPA2_PSK:
		if (len_password < 8 || len_password > 63) {
			view_alerts_popup_show(WPA_WRONG_PASSWORD_LEN_ERR_MSG_STR);
			goto popup_ok_exit;
		} else {
			wlan_manager_password_data* param = (wlan_manager_password_data *)g_malloc0(sizeof(wlan_manager_password_data));
			assertm_if(NULL == param, "param is NULL!!");

			param->wlan_eap_type = WLAN_MANAGER_EAP_TYPE_NONE;
			param->password = password;

			ret = wlan_manager_connect_with_password(profile_name, sec_mode, param);
			g_free(param);
		}
		break;

	default:
		ret = WLAN_MANAGER_ERR_UNKNOWN;
		ERROR_LOG(SP_NAME_ERR, "Fatal: Wrong security mode : %d", sec_mode);
		break;
	}

	if (WLAN_MANAGER_ERR_NONE == ret)
		view_main_item_connection_mode_set(gdata, ITEM_CONNECTION_MODE_CONNECTING);
	else
		view_main_item_connection_mode_set(gdata, ITEM_CONNECTION_MODE_OFF);

	common_pswd_popup_destroy(syspopup_app_state->passpopup);
	syspopup_app_state->passpopup = NULL;

popup_ok_exit:
	g_free(password);

	__COMMON_FUNC_EXIT__;
}

static void _popup_cancel_cb(void *data, Evas_Object *obj, void *event_info)
{
	__COMMON_FUNC_ENTER__;

	INFO_LOG(SP_NAME_NORMAL, "button cancel");

	common_pswd_popup_destroy(syspopup_app_state->passpopup);
	syspopup_app_state->passpopup = NULL;

	__COMMON_FUNC_EXIT__;
}

static void _wps_pbc_popup_cancel_connecting(void* data, Evas_Object* obj, void* event_info)
{
	if (!syspopup_app_state->passpopup || !data) {
		return;
	}

	syspopup_genlist_data_t *gdata = (syspopup_genlist_data_t *)data;
	char *profile_name = gdata->dev_info->profile_name;
	int ret = wlan_manager_request_cancel_wps_connection(profile_name);
	if (ret == WLAN_MANAGER_ERR_NONE) {
		INFO_LOG(SP_NAME_NORMAL, "WPS conection cancelled successfully for AP[%s]", profile_name);
	} else {
		ERROR_LOG(SP_NAME_ERR, "Error!!! wlan_manager_request_cancel_wps_connection failed for AP[%s]", profile_name);
	}
	common_pswd_popup_destroy(syspopup_app_state->passpopup);
	syspopup_app_state->passpopup = NULL;
	return;
}

static void _wps_btn_cb(void* data, Evas_Object* obj, void* event_info)
{
	__COMMON_FUNC_ENTER__;
	if (!syspopup_app_state->passpopup || !data) {
		return;
	}

	syspopup_genlist_data_t *gdata = (syspopup_genlist_data_t *)data;
	int ret = wlan_manager_request_wps_connection(gdata->dev_info->profile_name);
	if (ret == WLAN_MANAGER_ERR_NONE) {
		common_pswd_popup_pbc_popup_create(syspopup_app_state->passpopup, _wps_pbc_popup_cancel_connecting, gdata);
	} else {
		ERROR_LOG(SP_NAME_ERR, "Error!!! wlan_manager_request_wps_connection failed");
	}
	__COMMON_FUNC_EXIT__;
}


static void _gl_sel(void *data, Evas_Object *obj, void *event_info)
{
	__COMMON_FUNC_ENTER__;
	assertm_if(NULL == data, "data is NULL!!");
	assertm_if(NULL == obj, "obj is NULL!!");
	assertm_if(NULL == event_info, "event_info is NULL!!");
	Elm_Object_Item *item = (Elm_Object_Item *)event_info;

	if (view_main_state_get(obj) != ITEM_CONNECTION_MODE_OFF) {
		INFO_LOG(SP_NAME_NORMAL, "In connecting state, nothing can do" );
		elm_genlist_item_selected_set(item, 0);
		__COMMON_FUNC_EXIT__;
		return;
	}

	syspopup_genlist_data_t *gdata = (syspopup_genlist_data_t*) data;

	int ret = WLAN_MANAGER_ERR_UNKNOWN;

	switch (gdata->connection_mode) {
	case ITEM_CONNECTION_MODE_OFF:
		INFO_LOG( SP_NAME_NORMAL, "item state: off");
		ret = wlan_manager_request_connection(gdata->dev_info);
		break;
	case ITEM_CONNECTION_MODE_CONNECTING:
		INFO_LOG( SP_NAME_NORMAL, "item state: connecting");
		break;
	default:
		ERROR_LOG( SP_NAME_NORMAL, "item state: etc [%d]", gdata->connection_mode);
		break;
	}

	switch (ret) {
	case WLAN_MANAGER_ERR_NONE:
		INFO_LOG( SP_NAME_NORMAL, "ERROR_NONE");
		view_main_item_connection_mode_set(gdata, ITEM_CONNECTION_MODE_CONNECTING);
		break;
	case WLAN_MANAGER_ERR_CONNECT_PASSWORD_NEEDED:
		INFO_LOG( SP_NAME_NORMAL, "Password view will show up");
		pswd_popup_create_req_data_t	popup_info;
		popup_info.title = gdata->dev_info->ssid;
		popup_info.ok_cb = _popup_ok_cb;
		popup_info.cancel_cb = _popup_cancel_cb;
		popup_info.show_wps_btn = gdata->dev_info->wps_mode;
		popup_info.wps_btn_cb = _wps_btn_cb;
		popup_info.cb_data = gdata;
		syspopup_app_state->passpopup = common_pswd_popup_create(syspopup_app_state->win_main, PACKAGE, &popup_info);
		break;
	case WLAN_MANAGER_ERR_CONNECT_EAP_SEC_TYPE:
		create_eap_connect(syspopup_app_state->win_main, NULL, PACKAGE, gdata->dev_info, NULL);
		break;
	default:
		view_main_item_connection_mode_set(gdata, ITEM_CONNECTION_MODE_OFF);
		view_main_refresh();
		ERROR_LOG( SP_NAME_NORMAL, "errro code [%d]", ret);
		break;
	}

	elm_genlist_item_selected_set(item, 0);
	elm_genlist_item_update(item);

	__COMMON_FUNC_EXIT__;
}

static char *_gl_text_get(void *data, Evas_Object *obj, const char *part)
{
	__COMMON_FUNC_ENTER__;
	char *ret = NULL;
	assertm_if(NULL == data, "data param is NULL!!");
	assertm_if(NULL == obj, "obj param is NULL!!");
	assertm_if(NULL == part, "part param is NULL!!");

	syspopup_genlist_data_t *gdata = (syspopup_genlist_data_t *) data;
	if (!strncmp(part, "elm.text.1", strlen(part))) {
		ret = gdata->dev_info->ssid;
		if (ret == NULL) {
			ERROR_LOG(SP_NAME_NORMAL, "ssid name is NULL!!");
		}
	} else if (!strncmp(part, "elm.text.2", strlen(part))) {
		if (ITEM_CONNECTION_MODE_CONNECTING == gdata->connection_mode) {
			ret = sc(PACKAGE, I18N_TYPE_Connecting);
		} else {
			ret = gdata->dev_info->ap_status_txt;
		}
		if (ret == NULL) {
			ERROR_LOG(SP_NAME_NORMAL, "ap_status_txt is NULL!!");
		}
	}

	__COMMON_FUNC_EXIT__;

	return g_strdup(ret);
}

static Evas_Object *_gl_content_get(void *data, Evas_Object *obj, const char *part)
{
	__COMMON_FUNC_ENTER__;
	if (data == NULL)
		return NULL;

	syspopup_genlist_data_t *gdata = (syspopup_genlist_data_t *) data;

	const char* ssid_name = gdata->dev_info->ssid;
	INFO_LOG(SP_NAME_NORMAL, "ssid name [%s]", ssid_name);

	Evas_Object* icon = NULL;

	if (!strncmp(part, "elm.icon.1", strlen(part))) {
		icon = elm_icon_add(obj);
		elm_icon_file_set(icon, gdata->dev_info->ap_image_path, NULL);
		evas_object_size_hint_aspect_set(icon, EVAS_ASPECT_CONTROL_VERTICAL, 5, 5);
	} else if (!strncmp(part, "elm.icon.2", strlen(part))) {
		switch (gdata->connection_mode) {
		case ITEM_CONNECTION_MODE_OFF:
		default:
			break;
		case ITEM_CONNECTION_MODE_CONNECTING:
			icon = elm_progressbar_add(obj);
			elm_object_style_set(icon, "list_process");
			evas_object_size_hint_align_set(icon, EVAS_HINT_FILL, 0.5);
			evas_object_size_hint_weight_set(icon, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
			elm_progressbar_pulse(icon, TRUE);
			break;
		}

	}

	__COMMON_FUNC_EXIT__;
	return icon;
}

static void _gl_list_del(void* data, Evas_Object* obj)
{
	if (data == NULL)
		return;

	syspopup_genlist_data_t* gdata = (syspopup_genlist_data_t *) data;

	if (gdata->dev_info) {
		DEBUG_LOG(UG_NAME_NORMAL, "del target ssid:[%s]", gdata->dev_info->ssid);
		g_free(gdata->dev_info->ap_image_path);
		g_free(gdata->dev_info->ap_status_txt);
		g_free(gdata->dev_info->ssid);
		g_free(gdata->dev_info);
	}
	elm_object_item_data_set(gdata->it, NULL);
	g_free(gdata);

	return;
}

Evas_Object *view_main_create(Evas_Object* parent)
{
	__COMMON_FUNC_ENTER__;
	assertm_if(NULL == parent, "parent is NULL!!");

	list = elm_genlist_add(parent); //syspopup_app_state->win_main);
	assertm_if(NULL == list, "list allocation fail!!");

	evas_object_size_hint_weight_set(list, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(list, EVAS_HINT_FILL, EVAS_HINT_FILL);

	__COMMON_FUNC_EXIT__;

	return list;
}

int view_main_destroy(void)
{
	__COMMON_FUNC_ENTER__;
	if(NULL != list) {
		evas_object_del(list);
		list = NULL;
	}

	__COMMON_FUNC_EXIT__;
	return TRUE;
}

static void view_main_scan_ui_clear(void)
{
	__COMMON_FUNC_ENTER__;

	if (list == NULL)
		return;
	elm_genlist_clear(list);

	__COMMON_FUNC_EXIT__;
}

void *view_main_item_set(net_profile_info_t *profile_info)
{
	__COMMON_FUNC_ENTER__;

	wifi_device_info_t *wifi_device = (wifi_device_info_t*)g_malloc0(sizeof(wifi_device_info_t));

	wifi_device->profile_name = g_strdup(profile_info->ProfileName);
	wifi_device->ssid = g_strdup(profile_info->ProfileInfo.Wlan.essid);
	wifi_device->rssi = (int)profile_info->ProfileInfo.Wlan.Strength;
	wifi_device->security_mode = (int)profile_info->ProfileInfo.Wlan.security_info.sec_mode;
	wifi_device->wps_mode = (int)profile_info->ProfileInfo.Wlan.security_info.wps_support;
	wifi_device->ap_status_txt = common_utils_get_ap_security_type_info_txt(PACKAGE, wifi_device);
	wifi_device->ap_image_path = common_utils_get_device_icon(WIFI_SP_ICON_PATH, wifi_device);

	__COMMON_FUNC_EXIT__;
	return wifi_device;
}

int view_main_refresh()
{
	view_main_show(NULL);

	return TRUE;
}

Eina_Bool view_main_show(void *data)
{
	__COMMON_FUNC_ENTER__;

	if (list == NULL) {
		ERROR_LOG( SP_NAME_NORMAL, "list is NULL!!" );
		return ECORE_CALLBACK_CANCEL;
	}

	view_main_scan_ui_clear();

	char profile_name[NET_PROFILE_NAME_LEN_MAX+1] = "";
	int state = wlan_manager_state_get(profile_name);

	if (WLAN_MANAGER_ERROR == state || WLAN_MANAGER_OFF == state) {
		return ECORE_CALLBACK_CANCEL;
	}

	net_profile_info_t *profiles_list = wlan_manager_profile_table_get();
	if (profiles_list == NULL)
		return ECORE_CALLBACK_CANCEL;

	int profiles_list_size = wlan_manager_profile_scanned_length_get();
	INFO_LOG(SP_NAME_NORMAL, "profiles list count [%d]\n", profiles_list_size);
	if (profiles_list_size <= 0) {
		WARN_LOG(SP_NAME_NORMAL, "scan size is ZERO");
		return ECORE_CALLBACK_CANCEL;
	}

	itc.item_style = "2text.2icon.4";
	itc.func.text_get = _gl_text_get;
	itc.func.content_get = _gl_content_get;
	itc.func.state_get = NULL;
	itc.func.del = _gl_list_del;

	int i = 0;
	if (WLAN_MANAGER_CONNECTING == state) {
		for (; i < profiles_list_size; i++) {
			syspopup_genlist_data_t *gdata = (syspopup_genlist_data_t *)g_malloc0(sizeof(syspopup_genlist_data_t));
			gdata->dev_info = (wifi_device_info_t *)view_main_item_set(profiles_list+i);
			if (!g_strcmp0((profiles_list+i)->ProfileName, profile_name)) {
				gdata->connection_mode = ITEM_CONNECTION_MODE_CONNECTING;
				gdata->it = elm_genlist_item_append(list, &itc, gdata, NULL, ELM_GENLIST_ITEM_NONE, _gl_sel, gdata);
				/* We have found the connecting AP, so lets break here and continue list
				 * the remaining in the below for loop. The below for loop doesnt have comparison.
				 * So it makes things little fast.
				 */
				break;
			}
			gdata->connection_mode = ITEM_CONNECTION_MODE_OFF;
			gdata->it = elm_genlist_item_append(list, &itc, gdata, NULL, ELM_GENLIST_ITEM_NONE, _gl_sel, gdata);
		}
	}

	for (; i < profiles_list_size; i++) {
		syspopup_genlist_data_t *gdata = (syspopup_genlist_data_t *)g_malloc0(sizeof(syspopup_genlist_data_t));
		gdata->dev_info = (wifi_device_info_t *)view_main_item_set(profiles_list+i);
		gdata->connection_mode = ITEM_CONNECTION_MODE_OFF;

		gdata->it = elm_genlist_item_append(list, &itc, gdata, NULL, ELM_GENLIST_ITEM_NONE, _gl_sel, gdata);
		elm_object_item_data_set(gdata->it, gdata);
	}

	evas_object_show(list);
	__COMMON_FUNC_EXIT__;

	return ECORE_CALLBACK_CANCEL;
}
