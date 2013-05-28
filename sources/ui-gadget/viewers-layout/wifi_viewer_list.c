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
#include "common_utils.h"
#include "ug_wifi.h"
#include "wlan_manager.h"
#include "view_detail.h"
#include "viewer_list.h"
#include "viewer_manager.h"
#include "appcoreWrapper.h"
#include "i18nmanager.h"

#define LIST_ITEM_CONNECTED_AP_FONT_SIZE		28
#define LIST_ITEM_CONNECTED_AP_FONT_COLOR		"#3B73B6"
#define LIST_ITEM_CONNECTED_AP_FONT_COLOR_HL	"#FFFFFF"
#define FIRST_ITEM_NUMBER						8


static Evas_Object* viewer_list = NULL;
static Elm_Object_Item* first_item = NULL;
static Elm_Object_Item* last_item = NULL;

static Elm_Genlist_Item_Class itc;
static Elm_Genlist_Item_Class grouptitle_itc;
static Elm_Object_Item* grouptitle = NULL;

extern wifi_appdata *ug_app_state;

static void _gl_listview_more_btn_cb(void *data, Evas_Object *obj, void *event_info)
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

	view_detail(device_info, ug_app_state->layout_main);

	__COMMON_FUNC_EXIT__;
}

static char* _gl_listview_text_get(void *data, Evas_Object *obj, const char *part)
{
	char* det = NULL;

	ug_genlist_data_t* gdata = (ug_genlist_data_t*) data;
	retvm_if(NULL == gdata || NULL == gdata->device_info, NULL);

	assertm_if(NULL == gdata->device_info->ssid, "NULL!!");
	assertm_if(NULL == gdata->device_info->ap_status_txt, "NULL!!");

	if (!strncmp(part, "elm.text.1", strlen(part))) {
		det = g_strdup(gdata->device_info->ssid);
		assertm_if(NULL == det, "NULL!!");
	} else if (!strncmp(part, "elm.text.2", strlen(part))) {
		if (VIEWER_ITEM_RADIO_MODE_CONNECTED == gdata->radio_mode) {
			if (FALSE == gdata->highlighted)
				det = g_strdup_printf("<color=%s><b>%s</b></color>", LIST_ITEM_CONNECTED_AP_FONT_COLOR, gdata->device_info->ap_status_txt);
			else
				det = g_strdup_printf("<color=%s><b>%s</b></color>", LIST_ITEM_CONNECTED_AP_FONT_COLOR_HL, gdata->device_info->ap_status_txt);
		} else
			det = g_strdup(gdata->device_info->ap_status_txt);
		assertm_if(NULL == det, "NULL!!");
	}
	return det;
}

static Evas_Object *_gl_listview_content_get(void *data, Evas_Object *obj, const char *part)
{
	ug_genlist_data_t *gdata = (ug_genlist_data_t *)data;
	retvm_if(NULL == gdata, NULL);

	Evas_Object *icon = NULL;

	if (gdata->device_info->ap_image_path == NULL) {
		/* if there is no ap_image_path (NO AP Found situation) */
		DEBUG_LOG(UG_NAME_ERR, "Fatal: Image path is NULL");
	} else if (!strncmp(part, "elm.icon.1", strlen(part))) {
		/* for strength */
		char *temp_str = NULL;
		icon = elm_image_add(obj);
		retvm_if(NULL == icon, NULL);

		if (FALSE == gdata->highlighted)
			temp_str = g_strdup_printf("%s.png", gdata->device_info->ap_image_path);
		else
			temp_str = g_strdup_printf("%s_press.png", gdata->device_info->ap_image_path);
		elm_image_file_set(icon, temp_str, NULL);
		g_free(temp_str);
	} else if (!strncmp(part, "elm.icon.2", strlen(part))) {
		if (VIEWER_ITEM_RADIO_MODE_CONNECTING == gdata->radio_mode) {
			icon = elm_progressbar_add(obj);
			retvm_if(NULL == icon, NULL);

			elm_object_style_set(icon, "list_process");
			evas_object_size_hint_align_set(icon, EVAS_HINT_FILL, 0.5);
			evas_object_size_hint_weight_set(icon, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
			elm_progressbar_pulse(icon, TRUE);
		} else {
			icon = elm_button_add(obj);
			retvm_if(NULL == icon, NULL);

			elm_object_style_set(icon, "reveal");
			evas_object_smart_callback_add(icon, "clicked", _gl_listview_more_btn_cb, gdata->device_info);
			evas_object_propagate_events_set(icon, EINA_FALSE);
		}
	}

	return icon;
}

static void _gl_listview_del(void* data, Evas_Object* obj)
{
	if (data == NULL)
		return;

	ug_genlist_data_t* gdata = (ug_genlist_data_t*) data;
	retm_if(NULL == gdata || NULL == gdata->device_info);

	DEBUG_LOG(UG_NAME_NORMAL, "del target ssid:[%s]", gdata->device_info->ssid);

	g_free(gdata->device_info->ap_image_path);
	g_free(gdata->device_info->ap_status_txt);
	g_free(gdata->device_info->ssid);
	wifi_ap_destroy(gdata->device_info->ap);
	g_free(gdata->device_info);
	g_free(gdata);

	return;
}

static char* _gl_text_title_get(void *data, Evas_Object *obj, const char *part)
{
	assertm_if(NULL == obj, "NULL!!");
	assertm_if(NULL == part, "NULL!!");

	return (char*) g_strdup(sc(PACKAGE, I18N_TYPE_WiFi_network));
}

static Evas_Object *_gl_content_title_get(void *data, Evas_Object *obj, const char *part)
{
	__COMMON_FUNC_ENTER__;

	Evas_Object *title_progressbar = NULL;

	if (HEADER_MODE_SEARCHING == viewer_manager_header_mode_get()) {
		if (!strcmp(part, "elm.icon")) {
			title_progressbar  = elm_progressbar_add(obj);
			elm_object_style_set(title_progressbar, "list_process_small");
			elm_progressbar_horizontal_set(title_progressbar, EINA_TRUE);
			elm_progressbar_pulse(title_progressbar, EINA_TRUE);
		}
	}

	__COMMON_FUNC_EXIT__;
	return title_progressbar;
}

Elm_Object_Item* viewer_list_get_first_item(void)
{
	return first_item;
}

static Elm_Object_Item* viewer_list_get_last_item(void)
{
	return last_item;
}

static char *_access_info_cb(void *data, Evas_Object *obj)
{
	char *strength = NULL;
	char *buf = NULL;

	ug_genlist_data_t *gdata = (ug_genlist_data_t *)data;
	if (!gdata)
		return NULL;

	strength = common_utils_get_rssi_text(PACKAGE, gdata->device_info->rssi);
	buf = g_strdup_printf("%s %s %s", strength, gdata->device_info->ssid, gdata->device_info->ap_status_txt);

	g_free(strength);
	return buf;
}

static void _gl_realized(void *data, Evas_Object *obj, void *event_info)
{
	HEADER_MODES header_mode = viewer_manager_header_mode_get();

	if (header_mode == HEADER_MODE_ACTIVATING ||
		header_mode == HEADER_MODE_DEACTIVATING ||
		header_mode == HEADER_MODE_OFF)
		return ;

	Elm_Object_Item *item = (Elm_Object_Item *)event_info;
	int index = (int)elm_genlist_item_index_get(item);
	int first_item_index = (int)elm_genlist_item_index_get(viewer_list_get_first_item());
	int last_item_index = (int)elm_genlist_item_index_get(viewer_list_get_last_item());

	if (last_item_index == FIRST_ITEM_NUMBER)
		return ;

	if (first_item_index == -1) {
		int group_index = (int)elm_genlist_item_index_get(grouptitle);
		first_item_index = group_index+1;
	}

	if (first_item_index <= index && first_item_index - last_item_index) {
		if(index == first_item_index)
			elm_object_item_signal_emit(item, "elm,state,top", "");
		else if (index == last_item_index)
			elm_object_item_signal_emit(item, "elm,state,bottom", "");
		else
			elm_object_item_signal_emit(item, "elm,state,center", "");

		Evas_Object *ao = elm_object_item_access_object_get(item);
		elm_access_info_cb_set(ao, ELM_ACCESS_INFO, _access_info_cb, elm_object_item_data_get(item));
		elm_access_info_set(ao, ELM_ACCESS_TYPE, "double tap to connect device");
		elm_access_info_set(ao, ELM_ACCESS_STATE, "More button");
	}

}

static void _gl_highlighted(void *data, Evas_Object *obj, void *event_info)
{
	Elm_Object_Item *item = (Elm_Object_Item *)event_info;
	if (item) {
		ug_genlist_data_t *gdata = (ug_genlist_data_t *)elm_object_item_data_get(item);
		if (gdata) {
			gdata->highlighted = TRUE;
			elm_genlist_item_fields_update(item, "elm.icon.1", ELM_GENLIST_ITEM_FIELD_CONTENT);
			elm_genlist_item_fields_update(item, "elm.text.2", ELM_GENLIST_ITEM_FIELD_TEXT);
		}
	}
}

static void _gl_unhighlighted(void *data, Evas_Object *obj, void *event_info)
{
	Elm_Object_Item *item = (Elm_Object_Item *)event_info;
	if (item) {
		ug_genlist_data_t *gdata = (ug_genlist_data_t *)elm_object_item_data_get(item);
		if (gdata) {
			gdata->highlighted = FALSE;
			elm_genlist_item_fields_update(item, "elm.icon.1", ELM_GENLIST_ITEM_FIELD_CONTENT);
			elm_genlist_item_fields_update(item, "elm.text.2", ELM_GENLIST_ITEM_FIELD_TEXT);
		}
	}
}

static void __passwd_popup_cancel_cb(void *data,  Evas_Object *obj,
		void *event_info)
{
	__COMMON_FUNC_ENTER__;

	if (ug_app_state->passpopup == NULL)
		return;

	passwd_popup_free(ug_app_state->passpopup);
	ug_app_state->passpopup = NULL;

	__COMMON_FUNC_EXIT__;
}

static void __passwd_popup_ok_cb(void *data, Evas_Object *obj, void *event_info)
{
	__COMMON_FUNC_ENTER__;

	wifi_ap_h ap = NULL;
	int password_len = 0;
	const char *password = NULL;
	wifi_security_type_e sec_type = WIFI_SECURITY_TYPE_NONE;

	if (ug_app_state->passpopup == NULL)
		return;

	ap = passwd_popup_get_ap(ug_app_state->passpopup);
	password = passwd_popup_get_txt(ug_app_state->passpopup);
	password_len = strlen(password);

	wifi_ap_get_security_type(ap, &sec_type);

	switch (sec_type) {
	case WIFI_SECURITY_TYPE_WEP:
		if (password_len != 5 && password_len != 13 &&
				password_len != 26 && password_len != 10) {
			winset_popup_mode_set(ug_app_state->popup_manager,
					POPUP_OPTION_WEP_PSWD_LEN_ERROR, NULL);

			goto popup_ok_cb_exit;
		}
		break;

	case WIFI_SECURITY_TYPE_WPA_PSK:
	case WIFI_SECURITY_TYPE_WPA2_PSK:
		if (password_len < 8 || password_len > 63) {
			winset_popup_mode_set(ug_app_state->popup_manager,
					POPUP_OPTION_WPA_PSWD_LEN_ERROR, NULL);

			goto popup_ok_cb_exit;
		}
		break;

	default:
		ERROR_LOG(UG_NAME_NORMAL, "Wrong security mode: %d", sec_type);
		passwd_popup_free(ug_app_state->passpopup);
		ug_app_state->passpopup = NULL;

		goto popup_ok_cb_exit;
	}

	wlan_manager_connect_with_password(ap, password);

	passwd_popup_free(ug_app_state->passpopup);
	ug_app_state->passpopup = NULL;

popup_ok_cb_exit:
	g_free((gpointer)password);

	__COMMON_FUNC_EXIT__;
}

static void __wps_pbc_popup_cancel_connecting(void *data, Evas_Object *obj,
		void *event_info)
{
	if (ug_app_state->passpopup == NULL)
		return;

	wifi_ap_h ap = passwd_popup_get_ap(ug_app_state->passpopup);;

	int ret = wlan_manager_disconnect(ap);
	if (ret != WLAN_MANAGER_ERR_NONE)
		ERROR_LOG(UG_NAME_NORMAL, "Failed WPS PBC cancellation [0x%x]", ap);

	passwd_popup_free(ug_app_state->passpopup);
	ug_app_state->passpopup = NULL;

	viewer_manager_header_mode_set(HEADER_MODE_ON);
}

static void _wps_btn_cb(void* data, Evas_Object* obj, void* event_info)
{
	__COMMON_FUNC_ENTER__;
	if (!ug_app_state->passpopup) {
		return;
	}

	wifi_ap_h ap = passwd_popup_get_ap(ug_app_state->passpopup);
	int ret = wlan_manager_wps_connect(ap);
	if (ret == WLAN_MANAGER_ERR_NONE) {
		create_pbc_popup(ug_app_state->passpopup, __wps_pbc_popup_cancel_connecting, NULL);
	} else {
		ERROR_LOG(UG_NAME_NORMAL, "wlan_manager_wps_connect failed");
		passwd_popup_free(ug_app_state->passpopup);
		ug_app_state->passpopup = NULL;
	}

	__COMMON_FUNC_EXIT__;
}

static void __viewer_list_wifi_connect(wifi_device_info_t *device_info)
{
	int rv;
	bool favorite = false;
	Evas_Object* navi_frame = NULL;
	pswd_popup_create_req_data_t popup_info;
	wifi_security_type_e sec_type = WIFI_SECURITY_TYPE_NONE;

	if (device_info == NULL)
		return;

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

		popup_info.title = device_info->ssid;
		popup_info.ok_cb = __passwd_popup_ok_cb;
		popup_info.cancel_cb = __passwd_popup_cancel_cb;
		popup_info.show_wps_btn = device_info->wps_mode;
		popup_info.wps_btn_cb = _wps_btn_cb;
		popup_info.ap = device_info->ap;
		popup_info.cb_data = NULL;

		/* TODO: parameter with device_info */
		/* TODO: finally parameter with wifi_ap_h, WPA, EAP */
		ug_app_state->passpopup = create_passwd_popup(
				ug_app_state->layout_main, PACKAGE, &popup_info);

		if (ug_app_state->passpopup == NULL)
			INFO_LOG(UG_NAME_ERR, "Fail to create password popup");

		break;

	case WIFI_SECURITY_TYPE_EAP:
		navi_frame = viewer_manager_get_naviframe();
		ug_app_state->eap_view = create_eap_view(
				ug_app_state->layout_main, navi_frame, PACKAGE, device_info);
		break;

	default:
		ERROR_LOG(UG_NAME_NORMAL, "Unknown security type [%d]", sec_type);
		break;
	}
}

static void __viewer_list_item_clicked_cb(void *data, Evas_Object *obj,
		void *event_info)
{
	__COMMON_FUNC_ENTER__;

	assertm_if(NULL == event_info, "event_info is NULL!!");
	assertm_if(NULL == obj, "obj is NULL!!");

	Elm_Object_Item *it = (Elm_Object_Item *)event_info;
	ug_genlist_data_t *gdata = NULL;
	wifi_device_info_t *device_info = NULL;

	gdata = (ug_genlist_data_t *)elm_object_item_data_get(it);
	retm_if(NULL == gdata);

	device_info = gdata->device_info;
	retm_if(NULL == device_info || NULL == device_info->ssid);

	int item_state = gdata->radio_mode;
	int current_state = 0;

	INFO_LOG(UG_NAME_NORMAL, "ssid --- %s", device_info->ssid);
	INFO_LOG(UG_NAME_NORMAL, "ap --- 0x%x", device_info->ap);
	INFO_LOG(UG_NAME_NORMAL, "current item_state state is --- %d\n", item_state);

	switch (item_state) {
	case VIEWER_ITEM_RADIO_MODE_OFF:
		current_state = viewer_manager_header_mode_get();

		INFO_LOG(UG_NAME_NORMAL, "Clicked AP information\n");
		INFO_LOG(UG_NAME_NORMAL, "header mode [%d]", current_state);

		switch (current_state) {
		case HEADER_MODE_ON:
		case HEADER_MODE_CONNECTED:
		case HEADER_MODE_CONNECTING:
			__viewer_list_wifi_connect(device_info);
			break;

		case HEADER_MODE_OFF:
		case HEADER_MODE_SEARCHING:
		case HEADER_MODE_ACTIVATING:
		case HEADER_MODE_DEACTIVATING:
		default:
			INFO_LOG(UG_NAME_NORMAL, "Ignore click");
			break;
		}
		break;

	case VIEWER_ITEM_RADIO_MODE_CONNECTING:
	case VIEWER_ITEM_RADIO_MODE_CONNECTED:
	default:
		INFO_LOG(UG_NAME_NORMAL, "Ignore click");
		break;
	}

	elm_genlist_item_selected_set((Elm_Object_Item *)event_info, EINA_FALSE);

	__COMMON_FUNC_EXIT__;
}

static char *viewer_list_get_device_status_txt(wifi_device_info_t *wifi_device, VIEWER_ITEM_RADIO_MODES mode)
{
	char *status_txt = NULL;
	/* The strings are currently hard coded. It will be replaced with string ids later */
	if (VIEWER_ITEM_RADIO_MODE_CONNECTING == mode) {
		status_txt = g_strdup(sc(PACKAGE, I18N_TYPE_Connecting));
	} else if (VIEWER_ITEM_RADIO_MODE_CONNECTED == mode) {
		status_txt = g_strdup(sc(PACKAGE, I18N_TYPE_Connected));
	} else if (VIEWER_ITEM_RADIO_MODE_OFF == mode) {
		status_txt = common_utils_get_ap_security_type_info_txt(PACKAGE, wifi_device);
	} else {
		status_txt = g_strdup(WIFI_UNKNOWN_DEVICE_STATUS_STR);
		INFO_LOG(UG_NAME_NORMAL, "Invalid mode: %d", mode);
	}
	return status_txt;
}

Evas_Object* viewer_list_create(Evas_Object *win)
{
	__COMMON_FUNC_ENTER__;

	assertm_if(NULL == win, "NULL!!");
	assertm_if(NULL != viewer_list, "Err!!");

	viewer_list = elm_genlist_add(win);
	assertm_if(NULL == viewer_list, "NULL!!");

	elm_object_style_set(viewer_list, "dialogue");
	elm_genlist_mode_set(viewer_list, ELM_LIST_LIMIT);

	evas_object_size_hint_weight_set(viewer_list, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(viewer_list, EVAS_HINT_FILL, EVAS_HINT_FILL);

	itc.item_style = "dialogue/2text.2icon.3.tb";
	itc.func.text_get = _gl_listview_text_get;
	itc.func.content_get = _gl_listview_content_get;
	itc.func.state_get = NULL;
	itc.func.del = _gl_listview_del;

	first_item = last_item = NULL;

	evas_object_smart_callback_add(viewer_list, "realized", _gl_realized, NULL);
	evas_object_smart_callback_add(viewer_list, "highlighted", _gl_highlighted, NULL);
	evas_object_smart_callback_add(viewer_list, "unhighlighted", _gl_unhighlighted, NULL);

	__COMMON_FUNC_EXIT__;
	return viewer_list;
}

int viewer_list_destroy(void)
{
	__COMMON_FUNC_ENTER__;

	assertm_if(NULL == viewer_list, "NULL!!");
	viewer_list_item_clear();
	evas_object_del(viewer_list);
	viewer_list = NULL;

	__COMMON_FUNC_EXIT__;
	return TRUE;
}

void viewer_list_title_item_del(void)
{
	if (grouptitle) {
		elm_object_item_del(grouptitle);
		grouptitle = NULL;
	}
}

void viewer_list_title_item_update(void)
{
	elm_genlist_item_update(grouptitle);
}

void viewer_list_title_item_set(void)
{
	if (grouptitle != NULL || viewer_list_item_size_get() != 0)
		return;

	// To use multiline textblock/entry/editfield in genlist, set height_for_width mode
	// then the item's height is calculated while the item's width fits to genlist width.
	elm_genlist_mode_set(viewer_list, ELM_LIST_COMPRESS);

	grouptitle_itc.item_style = "dialogue/title";
	grouptitle_itc.func.text_get = _gl_text_title_get;
	grouptitle_itc.func.content_get = _gl_content_title_get;
	grouptitle_itc.func.state_get = NULL;
	grouptitle_itc.func.del = NULL;

	assertm_if(NULL != grouptitle, "Err!!");

	grouptitle = elm_genlist_item_append(viewer_list,
			&grouptitle_itc,
			NULL,
			NULL,
			ELM_GENLIST_ITEM_NONE,
			NULL,
			NULL);

	assertm_if(NULL == grouptitle, "NULL!!");

	elm_genlist_item_select_mode_set(grouptitle, ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);
}

int viewer_list_item_radio_mode_set(Elm_Object_Item* item,
		VIEWER_ITEM_RADIO_MODES mode)
{
	__COMMON_FUNC_ENTER__;

	if (NULL == item) {
		INFO_LOG(COMMON_NAME_ERR, "item is NULL");
		return FALSE;
	}

	ug_genlist_data_t* gdata = (ug_genlist_data_t *) elm_object_item_data_get(item);
	if (NULL == gdata || NULL == gdata->device_info) {
		INFO_LOG(COMMON_NAME_ERR, "gdata or device_info is NULL");
		return FALSE;
	}

	if (gdata->radio_mode == mode) {
		INFO_LOG(UG_NAME_NORMAL, "[%s] is already in requested state", gdata->device_info->ssid);
		return FALSE;
	}

	INFO_LOG(UG_NAME_NORMAL, "[%s] AP Item State Transition from [%d] --> [%d]", gdata->device_info->ssid, gdata->radio_mode, mode);
	gdata->radio_mode = mode;
	if (gdata->device_info->ap_status_txt) {
		g_free(gdata->device_info->ap_status_txt);
		gdata->device_info->ap_status_txt = viewer_list_get_device_status_txt(gdata->device_info, mode);
	}

	elm_genlist_item_update(item);

	__COMMON_FUNC_EXIT__;
	return TRUE;
}

Elm_Object_Item* viewer_list_item_insert_after(wifi_ap_h ap, Elm_Object_Item *after)
{
	Elm_Object_Item* ret = NULL;
	wifi_security_type_e sec_type;

	retvm_if(NULL == viewer_list, NULL);

	wifi_device_info_t *wifi_device = NULL;

	if (ap == NULL) {
		wifi_device = wlan_manager_profile_device_info_blank_create();
		retvm_if(NULL == wifi_device, NULL);
	} else {
		wifi_device = g_new0(wifi_device_info_t, 1);
		retvm_if(NULL == wifi_device, NULL);

		if (WIFI_ERROR_NONE != wifi_ap_clone(&(wifi_device->ap), ap)) {
			goto FREE_DEVICE_INFO;
		} else if (WIFI_ERROR_NONE != wifi_ap_get_essid(ap, &(wifi_device->ssid))) {
			goto FREE_DEVICE_INFO;
		} else if (WIFI_ERROR_NONE != wifi_ap_get_rssi(ap, &(wifi_device->rssi))) {
			goto FREE_DEVICE_INFO;
		} else if (WIFI_ERROR_NONE != wifi_ap_get_security_type(ap, &sec_type)) {
			goto FREE_DEVICE_INFO;
		} else if (WIFI_ERROR_NONE != wifi_ap_is_wps_supported(ap, &(wifi_device->wps_mode))) {
			goto FREE_DEVICE_INFO;
		}
		wifi_device->security_mode = common_utils_get_sec_mode(sec_type);
		wifi_device->ap_status_txt = viewer_list_get_device_status_txt(wifi_device, VIEWER_ITEM_RADIO_MODE_OFF);
		common_utils_get_device_icon(WIFI_APP_IMAGE_DIR,
				wifi_device,
				&wifi_device->ap_image_path);

	}
	ug_genlist_data_t* gdata = g_new0(ug_genlist_data_t, 1);
	retvm_if(NULL == gdata, NULL);

	gdata->device_info = wifi_device;
	gdata->radio_mode = VIEWER_ITEM_RADIO_MODE_OFF;

	if (!after) {	/* If the after item is NULL then insert it as first item */
		after = grouptitle;
	}

	ret = elm_genlist_item_insert_after(
			viewer_list, /*obj*/
			&itc,/*itc*/
			gdata,/*data*/
			NULL,/*parent*/
			after, /*after than*/
			ELM_GENLIST_ITEM_NONE, /*flags*/
			__viewer_list_item_clicked_cb,/*func*/
			NULL);/*func_data*/

	if (!ret) {
		assertm_if(NULL == ret, "NULL!!");
		g_free(gdata);
	} else {
		DEBUG_LOG(UG_NAME_NORMAL,
				"* item add complete item [0x%x] ssid:[%s] security[%d] size:[%d]",
				ret,
				wifi_device->ssid,
				wifi_device->security_mode,
				viewer_list_item_size_get());

		if (after == grouptitle) {
			first_item = ret;
			if (!last_item)
				last_item = ret;
		} else {
			last_item = ret;
			if (!first_item)
				first_item = ret;
		}

		elm_genlist_item_update(ret);
	}

FREE_DEVICE_INFO:
	if (!ret && wifi_device) {
		wifi_ap_destroy(wifi_device->ap);
		g_free(wifi_device->ap_image_path);
		g_free(wifi_device->ap_status_txt);
		g_free(wifi_device->ssid);
		g_free(wifi_device);
	}

	return ret;
}

void viewer_list_item_del(Elm_Object_Item *it)
{
	if (it == NULL)
		return;

	if (it == first_item) {
		first_item = elm_genlist_item_next_get(first_item);
	} else if (it == last_item) {
		last_item = elm_genlist_item_prev_get(last_item);
	}
	elm_object_item_del(it);
}

int viewer_list_item_size_get()
{
	__COMMON_FUNC_ENTER__;
	int ret = 0;
	Elm_Object_Item *it = first_item;

	while(it) {
		ret++;
		if (it == last_item)
			break;
		it = elm_genlist_item_next_get(it);
	}

	__COMMON_FUNC_EXIT__;
	return ret;
}

void viewer_list_item_clear(void)
{
	__COMMON_FUNC_ENTER__;

	Elm_Object_Item *it = first_item;
	Elm_Object_Item *nxt = NULL;

	while(it) {
		nxt = elm_genlist_item_next_get(it);
		elm_object_item_del(it);
		if (it == last_item)
			break;
		it = nxt;
	}

	first_item = last_item = NULL;

	__COMMON_FUNC_EXIT__;
}

void viewer_list_item_enable_all(void)
{
	__COMMON_FUNC_ENTER__;

	Elm_Object_Item *it = first_item;

	while(it) {
		elm_object_item_disabled_set(it, EINA_FALSE);

		if (it == last_item)
			break;

		it = elm_genlist_item_next_get(it);
	}

	__COMMON_FUNC_EXIT__;
}

void viewer_list_item_disable_all(void)
{
	__COMMON_FUNC_ENTER__;

	Elm_Object_Item *it = first_item;

	while(it) {
		elm_object_item_disabled_set(it, EINA_TRUE);

		if (it == last_item)
			break;

		it = elm_genlist_item_next_get(it);
	}

	__COMMON_FUNC_EXIT__;
}

Elm_Object_Item* item_get_for_ap(wifi_ap_h ap)
{
	__COMMON_FUNC_ENTER__;
	if (!ap) {
		__COMMON_FUNC_EXIT__;
		return NULL;
	}
	
	char *essid = NULL;
	wifi_security_type_e type = WIFI_SECURITY_TYPE_NONE;

	if (WIFI_ERROR_NONE != wifi_ap_get_essid(ap, &essid)) {
		__COMMON_FUNC_EXIT__;
		return NULL;
	} else if (WIFI_ERROR_NONE != wifi_ap_get_security_type(ap, &type)) {
		__COMMON_FUNC_EXIT__;
		return NULL;
	}

	Elm_Object_Item *it = first_item;
	wlan_security_mode_type_t sec_mode = common_utils_get_sec_mode(type);
	while(it) {
		ug_genlist_data_t* gdata = elm_object_item_data_get(it);
		wifi_device_info_t *device_info = NULL;
		if (gdata && (device_info = gdata->device_info)) {
			if (!g_strcmp0(device_info->ssid, essid) && device_info->security_mode == sec_mode)
				break;
		}
		if (it == last_item) {
			it = NULL;
			break;
		}
		it = elm_genlist_item_next_get(it);
	}

	g_free(essid);
	__COMMON_FUNC_EXIT__;
	return it;
}
