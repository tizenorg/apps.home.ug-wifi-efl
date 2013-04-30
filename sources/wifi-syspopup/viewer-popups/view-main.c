/*
 * Wi-Fi
 *
 * Copyright 2012-2013 Samsung Electronics Co., Ltd
 *
 * Licensed under the Flora License, Version 1.1 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://floralicense.org/license
 *
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

#define QS_POPUP_CONNECTION_STATE	"qs_popup_connection_state"

extern wifi_object* syspopup_app_state;

static Evas_Object* list = NULL;
static Elm_Genlist_Item_Class itc;

static ITEM_CONNECTION_MODES view_main_state_get(void)
{
	ITEM_CONNECTION_MODES state;

	state = (ITEM_CONNECTION_MODES)evas_object_data_get(
			list, QS_POPUP_CONNECTION_STATE);

	return state;
}

static void view_main_state_set(ITEM_CONNECTION_MODES state)
{
	evas_object_data_set(list, QS_POPUP_CONNECTION_STATE, (const void *)state);
}

static void __popup_ok_cb(void *data, Evas_Object *obj, void *event_info)
{
	__COMMON_FUNC_ENTER__;

	wifi_ap_h ap = NULL;
	int password_len = 0;
	const char* password = NULL;
	wifi_security_type_e sec_type = WIFI_SECURITY_TYPE_NONE;

	if (syspopup_app_state->passpopup == NULL)
		return;

	ap = passwd_popup_get_ap(syspopup_app_state->passpopup);
	password = passwd_popup_get_txt(syspopup_app_state->passpopup);
	password_len = strlen(password);

	wifi_ap_get_security_type(ap, &sec_type);

	switch (sec_type) {
	case WIFI_SECURITY_TYPE_WEP:
		if (password_len != 5 && password_len != 13 &&
				password_len != 26 && password_len != 10) {
			view_alerts_popup_show(WEP_WRONG_PASSWORD_LEN_ERR_MSG_STR);
			goto popup_ok_exit;
		}
		break;

	case WIFI_SECURITY_TYPE_WPA_PSK:
	case WIFI_SECURITY_TYPE_WPA2_PSK:
		if (password_len < 8 || password_len > 63) {
			view_alerts_popup_show(WPA_WRONG_PASSWORD_LEN_ERR_MSG_STR);
			goto popup_ok_exit;
		}
		break;

	default:
		ERROR_LOG(SP_NAME_ERR, "Wrong security mode: %d", sec_type);
		passwd_popup_free(syspopup_app_state->passpopup);
		break;
	}

	wlan_manager_connect_with_password(ap, password);

	passwd_popup_free(syspopup_app_state->passpopup);
	syspopup_app_state->passpopup = NULL;

popup_ok_exit:
	g_free((gpointer)password);

	__COMMON_FUNC_EXIT__;
}

static void __popup_cancel_cb(void *data, Evas_Object *obj, void *event_info)
{
	__COMMON_FUNC_ENTER__;

	if (syspopup_app_state->passpopup == NULL)
		return;

	passwd_popup_free(syspopup_app_state->passpopup);
	syspopup_app_state->passpopup = NULL;

	__COMMON_FUNC_EXIT__;
}

static void __wps_pbc_popup_cancel_connecting(void *data, Evas_Object *obj,
		void *event_info)
{
	if (syspopup_app_state->passpopup == NULL)
		return;

	wifi_ap_h ap = passwd_popup_get_ap(syspopup_app_state->passpopup);

	int ret = wlan_manager_disconnect(ap);
	if (ret != WLAN_MANAGER_ERR_NONE)
		ERROR_LOG(SP_NAME_ERR, "Failed WPS PBC cancellation [0x%x]", ap);

	passwd_popup_free(syspopup_app_state->passpopup);
	syspopup_app_state->passpopup = NULL;
}

static void _wps_btn_cb(void* data, Evas_Object* obj, void* event_info)
{
	__COMMON_FUNC_ENTER__;
	if (!syspopup_app_state->passpopup) {
		return;
	}

	wifi_ap_h ap = passwd_popup_get_ap(syspopup_app_state->passpopup);
	int ret = wlan_manager_wps_connect(ap);
	if (ret == WLAN_MANAGER_ERR_NONE) {
		create_pbc_popup(syspopup_app_state->passpopup, __wps_pbc_popup_cancel_connecting, NULL);
	} else {
		ERROR_LOG(SP_NAME_ERR, "wlan_manager_wps_connect failed");
		wifi_ap_destroy(ap);
	}

	__COMMON_FUNC_EXIT__;
}

static void __view_main_wifi_connect(syspopup_genlist_data_t *gdata)
{
	int rv;
	bool favorite = false;
	wifi_device_info_t *device_info;
	pswd_popup_create_req_data_t popup_info;
	wifi_security_type_e sec_type = WIFI_SECURITY_TYPE_NONE;

	retm_if(NULL == gdata);

	device_info = gdata->dev_info;
	retm_if(NULL == device_info);

	wifi_ap_is_favorite(device_info->ap, &favorite);

	if (favorite == true) {
		rv = wlan_manager_connect(device_info->ap);
		return;
	}

	wifi_ap_get_security_type(device_info->ap, &sec_type);

	switch (sec_type) {
	case WIFI_SECURITY_TYPE_NONE:
		rv = wlan_manager_connect(device_info->ap);
		break;

	case WIFI_SECURITY_TYPE_WEP:
	case WIFI_SECURITY_TYPE_WPA_PSK:
	case WIFI_SECURITY_TYPE_WPA2_PSK:
		memset(&popup_info, 0, sizeof(pswd_popup_create_req_data_t));

		popup_info.title = gdata->dev_info->ssid;
		popup_info.ok_cb = __popup_ok_cb;
		popup_info.cancel_cb = __popup_cancel_cb;
		popup_info.show_wps_btn = gdata->dev_info->wps_mode;
		popup_info.wps_btn_cb = _wps_btn_cb;
		popup_info.ap = gdata->dev_info->ap;
		popup_info.cb_data = NULL;
		syspopup_app_state->passpopup = create_passwd_popup(
				syspopup_app_state->layout_main, PACKAGE, &popup_info);
		break;

	case WIFI_SECURITY_TYPE_EAP:
		syspopup_app_state->eap_popup = create_eap_popup(
				syspopup_app_state->layout_main, PACKAGE, gdata->dev_info);
		break;

	default:
		ERROR_LOG(SP_NAME_NORMAL, "Unknown security type [%d]", sec_type);
		break;
	}
}

#if 0
/* Unused function */
Elm_Object_Item *__view_main_get_item_in_mode(ITEM_CONNECTION_MODES mode)
{
	Elm_Object_Item* it = NULL;
	it = elm_genlist_first_item_get(list);
	__COMMON_FUNC_ENTER__;
	while (it) {
		syspopup_genlist_data_t *gdata = (syspopup_genlist_data_t *)elm_object_item_data_get(it);
		if (gdata && gdata->connection_mode == mode) {
			INFO_LOG( SP_NAME_NORMAL, "Found Item [%s] in mode[%d]", gdata->dev_info->ssid, mode);
			__COMMON_FUNC_EXIT__;
			return it;
		}
		it = elm_genlist_item_next_get(it);
	}

	__COMMON_FUNC_EXIT__;
	return NULL;
}
#endif

static void __gl_sel(void *data, Evas_Object *obj, void *event_info)
{
	__COMMON_FUNC_ENTER__;

	assertm_if(NULL == data, "data is NULL!!");
	assertm_if(NULL == obj, "obj is NULL!!");
	assertm_if(NULL == event_info, "event_info is NULL!!");

	Elm_Object_Item *item = (Elm_Object_Item *)event_info;
	ITEM_CONNECTION_MODES state = view_main_state_get();
	syspopup_genlist_data_t *gdata = (syspopup_genlist_data_t *)data;

	switch (state) {
	case ITEM_CONNECTION_MODE_OFF:
		__view_main_wifi_connect(gdata);
		break;

	case ITEM_CONNECTION_MODE_CONNECTING:
	default:
		break;
	}

	elm_genlist_item_selected_set(item, EINA_FALSE);

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
	retvm_if(NULL == gdata, NULL);

	if (!strncmp(part, "elm.text.1", strlen(part))) {
		ret = gdata->dev_info->ssid;
		if (ret == NULL) {
			ERROR_LOG(SP_NAME_NORMAL, "ssid name is NULL!!");
		}
	} else if (!strncmp(part, "elm.text.2", strlen(part))) {
		if (ITEM_CONNECTION_MODE_CONNECTING == gdata->connection_mode) {
			ret = sc(PACKAGE, I18N_TYPE_Connecting);
		} else if (ITEM_CONNECTION_MODE_CONNECTING == gdata->connection_mode) {
			ret = sc(PACKAGE, I18N_TYPE_Disconnecting);
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
		char *temp_str = NULL;
		icon = elm_image_add(obj);
		if (FALSE == gdata->highlighted)
			temp_str = g_strdup_printf("%s.png", gdata->dev_info->ap_image_path);
		else
			temp_str = g_strdup_printf("%s_press.png", gdata->dev_info->ap_image_path);
		elm_image_file_set(icon, temp_str, NULL);
		g_free(temp_str);
		evas_object_size_hint_aspect_set(icon, EVAS_ASPECT_CONTROL_VERTICAL, 5, 5);
	} else if (!strncmp(part, "elm.icon.2", strlen(part))) {
		switch (gdata->connection_mode) {
		case ITEM_CONNECTION_MODE_OFF:
			break;

		case ITEM_CONNECTION_MODE_CONNECTING:
			icon = elm_progressbar_add(obj);
			elm_object_style_set(icon, "list_process");
			evas_object_size_hint_align_set(icon, EVAS_HINT_FILL, 0.5);
			evas_object_size_hint_weight_set(icon, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
			elm_progressbar_pulse(icon, TRUE);
			break;

		default:
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
		DEBUG_LOG(UG_NAME_NORMAL, "del target ssid: [%s]", gdata->dev_info->ssid);
		g_free(gdata->dev_info->ap_image_path);
		g_free(gdata->dev_info->ap_status_txt);
		g_free(gdata->dev_info->ssid);
		wifi_ap_destroy(gdata->dev_info->ap);
		g_free(gdata->dev_info);
	}
	elm_object_item_data_set(gdata->it, NULL);
	g_free(gdata);

	return;
}

static void _gl_highlighted(void *data, Evas_Object *obj, void *event_info)
{
	Elm_Object_Item *item = (Elm_Object_Item *)event_info;
	if (item) {
		syspopup_genlist_data_t *gdata = (syspopup_genlist_data_t *)elm_object_item_data_get(item);
		if (gdata) {
			gdata->highlighted = TRUE;
			elm_genlist_item_fields_update(item, "elm.icon.1", ELM_GENLIST_ITEM_FIELD_CONTENT);
		}
	}
}

static void _gl_unhighlighted(void *data, Evas_Object *obj, void *event_info)
{
	Elm_Object_Item *item = (Elm_Object_Item *)event_info;
	if (item) {
		syspopup_genlist_data_t *gdata = (syspopup_genlist_data_t *)elm_object_item_data_get(item);
		if (gdata) {
			gdata->highlighted = FALSE;
			elm_genlist_item_fields_update(item, "elm.icon.1", ELM_GENLIST_ITEM_FIELD_CONTENT);
		}
	}
}

Evas_Object *view_main_create(Evas_Object* parent)
{
	__COMMON_FUNC_ENTER__;
	assertm_if(NULL == parent, "parent is NULL!!");

	list = elm_genlist_add(parent);
	assertm_if(NULL == list, "list allocation fail!!");

	evas_object_size_hint_weight_set(list, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(list, EVAS_HINT_FILL, EVAS_HINT_FILL);

	evas_object_smart_callback_add(list, "highlighted", _gl_highlighted, NULL);
	evas_object_smart_callback_add(list, "unhighlighted", _gl_unhighlighted, NULL);

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

void view_main_item_state_set(wifi_ap_h ap, ITEM_CONNECTION_MODES state)
{
	char *item_ssid = NULL;
	wifi_security_type_e sec_type;
	wlan_security_mode_type_t item_sec_mode;
	Elm_Object_Item* it = NULL;
	it = elm_genlist_first_item_get(list);
	__COMMON_FUNC_ENTER__;
	if (!it ||
		!ap ||
		(WIFI_ERROR_NONE != wifi_ap_get_essid(ap, &item_ssid)) ||
		(WIFI_ERROR_NONE != wifi_ap_get_security_type(ap, &sec_type))) {
		ERROR_LOG(SP_NAME_NORMAL, "Invalid params");
		__COMMON_FUNC_EXIT__;
		return;
	}
	item_sec_mode = common_utils_get_sec_mode(sec_type);
	INFO_LOG(SP_NAME_NORMAL, "item state set for AP[%s] with sec mode[%d]", item_ssid, item_sec_mode);
	while (it) {
		syspopup_genlist_data_t *gdata = (syspopup_genlist_data_t *)elm_object_item_data_get(it);
		INFO_LOG(SP_NAME_NORMAL, "gdata AP[%s] with sec mode[%d]", gdata->dev_info->ssid, gdata->dev_info->security_mode);
		if (gdata->dev_info->security_mode == item_sec_mode &&
			!g_strcmp0(gdata->dev_info->ssid, item_ssid)) {
			if (gdata->connection_mode != state) {
				gdata->connection_mode = state;
				INFO_LOG(SP_NAME_NORMAL, "State transition from [%d] --> [%d]", view_main_state_get(), state);
				view_main_state_set(state);
				elm_genlist_item_update(it);
			}
			break;
		}

		it = elm_genlist_item_next_get(it);
	}
	g_free(item_ssid);
	__COMMON_FUNC_EXIT__;
	return;
}

static wifi_device_info_t *view_main_item_device_info_create(wifi_ap_h ap)
{
	__COMMON_FUNC_ENTER__;

	wifi_device_info_t *wifi_device = g_new0(wifi_device_info_t, 1);
	wifi_security_type_e sec_type;

	if (WIFI_ERROR_NONE != wifi_ap_clone(&(wifi_device->ap), ap)) {
		g_free(wifi_device);
		return NULL;
	} else if (WIFI_ERROR_NONE != wifi_ap_get_essid(ap, &(wifi_device->ssid))) {
		g_free(wifi_device);
		return NULL;
	} else if (WIFI_ERROR_NONE != wifi_ap_get_rssi(ap, &(wifi_device->rssi))) {
		g_free(wifi_device->ssid);
		g_free(wifi_device);
		return NULL;
	} else if (WIFI_ERROR_NONE != wifi_ap_get_security_type (ap, &sec_type)) {
		g_free(wifi_device->ssid);
		g_free(wifi_device);
		return NULL;
	} else if (WIFI_ERROR_NONE != wifi_ap_is_wps_supported (ap, &(wifi_device->wps_mode))) {
		g_free(wifi_device->ssid);
		g_free(wifi_device);
		return NULL;
	}
	wifi_device->security_mode = common_utils_get_sec_mode(sec_type);
	wifi_device->ap_status_txt = common_utils_get_ap_security_type_info_txt(PACKAGE, wifi_device);
	common_utils_get_device_icon(WIFI_SP_ICON_PATH,
			wifi_device,
			&wifi_device->ap_image_path);

	__COMMON_FUNC_EXIT__;
	return wifi_device;
}

static bool view_main_wifi_found_ap_cb(wifi_ap_h ap, void* user_data)
{
	int *profile_size = (int *)user_data;
	syspopup_genlist_data_t *gdata = g_new0(syspopup_genlist_data_t, 1);
	wifi_connection_state_e state;

	gdata->dev_info = view_main_item_device_info_create(ap);
	if (gdata->dev_info == NULL) {
		g_free(gdata);

		return true;
	}

	wifi_ap_get_connection_state(ap, &state);

	if (WIFI_CONNECTION_STATE_ASSOCIATION == state ||
			WIFI_CONNECTION_STATE_CONFIGURATION == state) {
		gdata->connection_mode = ITEM_CONNECTION_MODE_CONNECTING;
		gdata->it = elm_genlist_item_append(list, &itc, gdata, NULL, ELM_GENLIST_ITEM_NONE, __gl_sel, gdata);
		*profile_size += 1;
		view_main_state_set(ITEM_CONNECTION_MODE_CONNECTING);

		return true;
	}

	gdata->connection_mode = ITEM_CONNECTION_MODE_OFF;
	gdata->it = elm_genlist_item_append(list, &itc, gdata, NULL,
			ELM_GENLIST_ITEM_NONE, __gl_sel, gdata);
	*profile_size += 1;

	return true;
}

gboolean view_main_show(void *data)
{
	__COMMON_FUNC_ENTER__;

	if (list == NULL) {
		ERROR_LOG( SP_NAME_NORMAL, "list is NULL!!" );

		__COMMON_FUNC_EXIT__;
		return FALSE;
	}

	view_main_scan_ui_clear();
	view_main_state_set(ITEM_CONNECTION_MODE_OFF);

	int state = wlan_manager_state_get();
	if (WLAN_MANAGER_ERROR == state || WLAN_MANAGER_OFF == state) {
		return FALSE;
	}

	itc.item_style = "2text.2icon.4";
	itc.func.text_get = _gl_text_get;
	itc.func.content_get = _gl_content_get;
	itc.func.state_get = NULL;
	itc.func.del = _gl_list_del;

	int profiles_list_size = 0;

	wifi_foreach_found_aps(view_main_wifi_found_ap_cb, &profiles_list_size);

	INFO_LOG(SP_NAME_NORMAL, "profiles list count [%d]\n", profiles_list_size);
	if (profiles_list_size <= 0) {
		WARN_LOG(SP_NAME_NORMAL, "scan size is ZERO");

		__COMMON_FUNC_EXIT__;
		return FALSE;
	}

	evas_object_show(list);

	__COMMON_FUNC_EXIT__;
	return FALSE;
}
