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

#include "common.h"
#include "ug_wifi.h"
#include "common_utils.h"
#include "common_generate_pin.h"
#include "connection_manager.h"
#include "wlan_manager.h"
#include "view_detail.h"
#include "viewer_list.h"
#include "viewer_manager.h"
#include "appcoreWrapper.h"
#include "i18nmanager.h"

#define LIST_ITEM_CONNECTED_AP_FONT_SIZE		28

static Evas_Object* viewer_list = NULL;
static Elm_Object_Item* first_item = NULL;
static Elm_Object_Item* last_item = NULL;

static Elm_Genlist_Item_Class itc;
static Elm_Genlist_Item_Class no_wifi_device_itc;
static Elm_Genlist_Item_Class grouptitle_itc;
static Elm_Object_Item* grouptitle = NULL;

extern wifi_appdata *ug_app_state;

struct connecting_cancel_popup_data {
	Evas_Object *popup;
	wifi_ap_h ap;
};

static struct connecting_cancel_popup_data *g_disconnect_popup = NULL;

static void _gl_listview_more_btn_cb(void *data, Evas_Object *obj, void *event_info)
{
	__COMMON_FUNC_ENTER__;

	elm_object_disabled_set(obj, EINA_TRUE);

	wifi_device_info_t *device_info = (wifi_device_info_t *)data;

	if (data == NULL) {
		__COMMON_FUNC_EXIT__;
		return;
	}

	SECURE_DEBUG_LOG(UG_NAME_NORMAL, "ssid [%s]", device_info->ssid);

	view_detail(device_info, ug_app_state->layout_main, obj);

	__COMMON_FUNC_EXIT__;
}

char* ConvertRGBAtoHex(int r, int g, int b, int a)
{
	int hexcolor = 0;
	char* string = NULL;

	string = g_try_malloc0(sizeof(char )* 255);
	if (string != NULL) {
		hexcolor = (r << 24) + (g << 16) + (b << 8) + a;
		sprintf(string, "%08x", hexcolor);
	}

	return string;
}

static char* _gl_listview_text_get(void *data, Evas_Object *obj, const char *part)
{
	char* det = NULL;
	char* buf = NULL;

	ug_genlist_data_t* gdata = (ug_genlist_data_t*) data;
	retvm_if(NULL == gdata || NULL == gdata->device_info, NULL);

	assertm_if(NULL == gdata->device_info->ssid, "NULL!!");

	if (!strcmp("elm.text", part)) {
		det = evas_textblock_text_utf8_to_markup(NULL,
				gdata->device_info->ssid);
		assertm_if(NULL == det, "NULL!!");
		if (VIEWER_ITEM_RADIO_MODE_CONNECTED == gdata->radio_mode) {
			buf = g_strdup_printf("<color=#%s>%s</color>",
				ConvertRGBAtoHex(2, 61, 132, 255), det);

			g_free(det);
			return buf;
		}
	} else if (!strcmp("elm.text.sub", part)
			&& gdata->device_info->ap_status_txt != NULL) {
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
	Evas_Object *btn = NULL;
	Evas_Object *ic = NULL;

	if (gdata->device_info->ap_image_path == NULL) {
		/* if there is no ap_image_path (NO AP Found situation) */
		DEBUG_LOG(UG_NAME_ERR, "Fatal: Image path is NULL");

	} else if (!strcmp("elm.swallow.icon", part)) {
		ic = elm_layout_add(obj);
		elm_layout_theme_set(ic, "layout", "list/B/type.3", "default");

		icon = elm_image_add(ic);
		retvm_if(NULL == icon, NULL);

		/* for strength */
		char *temp_str = NULL;
		temp_str = g_strdup_printf("%s.png", gdata->device_info->ap_image_path);
		elm_image_file_set(icon, CUSTOM_EDITFIELD_PATH, temp_str);
		g_free(temp_str);

		evas_object_color_set(icon, 2, 61, 132, 204);

		evas_object_size_hint_align_set(icon, EVAS_HINT_FILL, EVAS_HINT_FILL);
		evas_object_size_hint_weight_set(icon, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		elm_layout_content_set(ic, "elm.swallow.content", icon);

	} else if (!strcmp("elm.swallow.end", part)) {
		if (VIEWER_ITEM_RADIO_MODE_CONNECTING == gdata->radio_mode ||
				VIEWER_ITEM_RADIO_MODE_CONFIGURATION == gdata->radio_mode) {
			ic = elm_layout_add(obj);
			elm_layout_theme_set(ic, "layout", "list/C/type.2", "default");

			icon = elm_progressbar_add(ic);

			elm_object_style_set(icon, "process_medium");
			evas_object_size_hint_align_set(icon, EVAS_HINT_FILL, 0.5);
			evas_object_size_hint_weight_set(icon, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
			elm_progressbar_pulse(icon, TRUE);
			elm_layout_content_set(ic, "elm.swallow.content", icon);
			evas_object_propagate_events_set(icon, EINA_FALSE);

		} else {
			btn = elm_button_add(obj);
			elm_object_style_set(btn, "circle_custom");
			icon = elm_image_add(btn);
			elm_image_file_set(icon, CUSTOM_EDITFIELD_PATH, "wifi_icon_badge_info.png");
			elm_image_resizable_set(icon, EINA_TRUE, EINA_TRUE);
			elm_object_part_content_set(btn, "elm.swallow.content", icon);
			evas_object_propagate_events_set(btn, EINA_FALSE);
			evas_object_smart_callback_add(btn, "clicked", _gl_listview_more_btn_cb,gdata->device_info);
			evas_object_show(btn);

			return btn;
		}
	}
	return ic;
}

static void _gl_listview_del(void *data, Evas_Object *obj)
{
	if (data == NULL)
		return;

	ug_genlist_data_t *gdata = (ug_genlist_data_t *)data;
	retm_if(NULL == gdata || NULL == gdata->device_info);

	g_free(gdata->device_info->ap_image_path);
	g_free(gdata->device_info->ap_status_txt);
	g_free(gdata->device_info->ssid);
	wifi_ap_destroy(gdata->device_info->ap);
	g_free(gdata->device_info);
	g_free(gdata);
}

static char *_gl_text_available_networks_get(void *data, Evas_Object *obj,
		const char *part)
{
	if (!strcmp("elm.text", part))
		return g_strdup(sc(PACKAGE, I18N_TYPE_Available_networks));

	return NULL;
}

static Evas_Object *_gl_content_scanning_icon_get(void *data, Evas_Object *obj,
		const char *part)
{
	Evas_Object *title_progressbar = NULL;

	if (HEADER_MODE_SEARCHING == viewer_manager_header_mode_get()) {
		title_progressbar  = elm_progressbar_add(obj);
		elm_object_style_set(title_progressbar, "process_small");
		elm_progressbar_horizontal_set(title_progressbar, EINA_TRUE);
		elm_progressbar_pulse(title_progressbar, EINA_TRUE);
	}

	return title_progressbar;
}

Elm_Object_Item* viewer_list_get_first_item(void)
{
	return first_item;
}

Elm_Object_Item* viewer_list_get_last_item(void)
{
	return last_item;
}

#ifdef ACCESSIBLITY_FEATURE
static char *_access_info_cb(void *data, Evas_Object *obj)
{
	char *strength = NULL;
	char *buf = NULL;

	ug_genlist_data_t *gdata = (ug_genlist_data_t *)data;
	if (!gdata) {
		return NULL;
	}

	strength = common_utils_get_rssi_text(PACKAGE, gdata->device_info->rssi);
	buf = g_strdup_printf("%s. %s. %s.", strength, gdata->device_info->ssid, gdata->device_info->ap_status_txt);

	g_free(strength);
	return buf;
}
#endif

static void gl_lang_changed(void *data, Evas_Object *obj, void *event_info)
{
	elm_genlist_realized_items_update(obj);
}

static void _gl_realized(void *data, Evas_Object *obj, void *event_info)
{
#ifdef ACCESSIBLITY_FEATURE
	HEADER_MODES header_mode = viewer_manager_header_mode_get();
	Elm_Object_Item *item = (Elm_Object_Item *)event_info;

	if (header_mode == HEADER_MODE_ACTIVATING ||
		header_mode == HEADER_MODE_DEACTIVATING ||
		header_mode == HEADER_MODE_OFF) {
		return ;
	}

	Elm_Object_Item *first_item = viewer_list_get_first_item();
	Elm_Object_Item *last_item = viewer_list_get_last_item();

	if (first_item == NULL || last_item == NULL) {
		return;
	}

	int index = (int)elm_genlist_item_index_get(item);
	int first_item_index = (int)elm_genlist_item_index_get(first_item);
	int last_item_index = (int)elm_genlist_item_index_get(last_item);
	char buf[100] = "";

	if (first_item_index == -1) {
		int group_index = (int)elm_genlist_item_index_get(grouptitle);
		first_item_index = group_index + 1;
	}

	if (index >= first_item_index && index <= last_item_index &&
			first_item_index < last_item_index) {

		Evas_Object *ao = elm_object_item_access_object_get(item);
		elm_access_info_cb_set(ao, ELM_ACCESS_INFO, _access_info_cb, elm_object_item_data_get(item));
		g_snprintf(buf, sizeof(buf), "%s%s",
						sc(PACKAGE, I18N_TYPE_Double_tap),
						sc(PACKAGE, I18N_TYPE_Connect_to_device));
		elm_access_info_set(ao, ELM_ACCESS_TYPE, buf);
		elm_access_info_set(ao, ELM_ACCESS_STATE, sc(PACKAGE, I18N_TYPE_Wi_Fi_network_info));
	}

	if (ug_app_state->ug_type == UG_VIEW_SETUP_WIZARD &&
			first_item_index == index) {
		viewer_manager_setup_wizard_btns_color_set(TRUE);
	}
#else
	Elm_Object_Item *item = (Elm_Object_Item *)event_info;
	Elm_Object_Item *first_item = viewer_list_get_first_item();

	if (first_item == NULL) {
		return;
	}

	int index = (int)elm_genlist_item_index_get(item);
	int first_item_index = (int)elm_genlist_item_index_get(first_item);

	if (first_item_index == -1) {
		int group_index = (int)elm_genlist_item_index_get(grouptitle);
		first_item_index = group_index + 1;
	}

	if (ug_app_state->ug_type == UG_VIEW_SETUP_WIZARD &&
			first_item_index == index) {
		viewer_manager_setup_wizard_btns_color_set(TRUE);
	}
#endif
}

static void _info_popup_ok_cb(void *data, Evas_Object *obj, void *event_info)
{
	pswd_popup_t *passpopup = (pswd_popup_t *)data;

	if (passpopup != NULL && passpopup->info_popup != NULL) {
		evas_object_del(passpopup->info_popup);
		passpopup->info_popup = NULL;
	}
}

static void __passwd_popup_cancel_cb(void *data,  Evas_Object *obj,
		void *event_info)
{
	__COMMON_FUNC_ENTER__;

	if (ug_app_state->passpopup == NULL) {
		return;
	}

	passwd_popup_free(ug_app_state->passpopup);
	ug_app_state->passpopup = NULL;

	if (data) {
		g_free(data);
	}

	__COMMON_FUNC_EXIT__;
}

static void __passwd_popup_ok_cb(void *data, Evas_Object *obj, void *event_info)
{
	__COMMON_FUNC_ENTER__;

	wifi_ap_h ap = NULL;
	int password_len = 0;
	const char *password = NULL;
	wifi_security_type_e sec_type = WIFI_SECURITY_TYPE_NONE;

	if (ug_app_state->passpopup == NULL) {
		return;
	}

	ap = passwd_popup_get_ap(ug_app_state->passpopup);
	password = passwd_popup_get_txt(ug_app_state->passpopup);
	if (password != NULL)
		password_len = strlen(password);

	wifi_ap_get_security_type(ap, &sec_type);

	switch (sec_type) {
	case WIFI_SECURITY_TYPE_WEP:
		if (password_len != 5 && password_len != 13 &&
				password_len != 26 && password_len != 10) {
			common_utils_send_message_to_net_popup(
					"Network connection popup",
					"wrong password", "toast_popup", NULL);

			if (ug_app_state->passpopup->entry) {
				elm_object_focus_set(
						ug_app_state->passpopup->entry,
						EINA_TRUE);
			}
			goto failure;
		}
		break;

	case WIFI_SECURITY_TYPE_WPA_PSK:
	case WIFI_SECURITY_TYPE_WPA2_PSK:
		if (password_len < 8 || password_len > 64) {
			common_utils_send_message_to_net_popup(
					"Network connection popup",
					"wrong password", "toast_popup", NULL);

			if (ug_app_state->passpopup->entry) {
				elm_object_focus_set(
						ug_app_state->passpopup->entry,
						EINA_TRUE);
			}
			goto failure;
		}
		break;

	default:
		ERROR_LOG(UG_NAME_NORMAL, "Wrong security mode: %d", sec_type);
		passwd_popup_free(ug_app_state->passpopup);
		ug_app_state->passpopup = NULL;

		goto popup_ok_cb_exit;
	}

	bool favorite = FALSE;
	if(wifi_ap_is_favorite(ap, &favorite) == WIFI_ERROR_NONE
		&& favorite == TRUE) {
		wlan_manager_forget(ap);
		wifi_ap_refresh(ap);
	}

	if (ug_app_state->is_hidden) {
		wifi_ap_h hidden_ap;
		char *ssid;
		wifi_ap_get_essid(ap, &ssid);
		wifi_ap_hidden_create(ssid, &hidden_ap);
		g_free(ssid);
		wifi_ap_set_security_type(hidden_ap, sec_type);
		wlan_manager_connect_with_password(hidden_ap, password);
	} else
		wlan_manager_connect_with_password(ap, password);

	passwd_popup_free(ug_app_state->passpopup);
	ug_app_state->passpopup = NULL;

popup_ok_cb_exit:
	if (data) {
		g_free(data);
	}

failure:
	g_free((gpointer)password);

	__COMMON_FUNC_EXIT__;
}

static void __wps_pbc_popup_cancel_connecting(void *data, Evas_Object *obj,
		void *event_info)
{
	__COMMON_FUNC_ENTER__;

	if (ug_app_state->passpopup == NULL) {
		return;
	}

	wifi_ap_h ap = passwd_popup_get_ap(ug_app_state->passpopup);

	int ret = wlan_manager_disconnect(ap);
	if (ret != WLAN_MANAGER_ERR_NONE) {
		ERROR_LOG(UG_NAME_NORMAL, "Failed WPS PBC cancellation [0x%x]", ap);
	}

	passwd_popup_free(ug_app_state->passpopup);
	ug_app_state->passpopup = NULL;

	viewer_manager_header_mode_set(HEADER_MODE_ON);

	__COMMON_FUNC_EXIT__;
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
		create_pbc_popup(ug_app_state->passpopup,
				__wps_pbc_popup_cancel_connecting, NULL,
				POPUP_WPS_BTN, NULL);
	} else {
		ERROR_LOG(UG_NAME_NORMAL, "wlan_manager_wps_connect failed");
		passwd_popup_free(ug_app_state->passpopup);
		ug_app_state->passpopup = NULL;
	}

	__COMMON_FUNC_EXIT__;
}

static void _wps_cancel_cb(void* data, Evas_Object* obj, void* event_info)
{
	__COMMON_FUNC_ENTER__;

	if (ug_app_state->passpopup == NULL) {
		return;
	}

	current_popup_free(ug_app_state->passpopup, POPUP_WPS_OPTIONS);

	__COMMON_FUNC_EXIT__;
}

static void _wps_pin_cb(void* data, Evas_Object* obj, void* event_info)
{
	__COMMON_FUNC_ENTER__;
	unsigned int rpin = 0;
	char npin[9] = { '\0' };
	int pin_len = 0;
	int ret = WLAN_MANAGER_ERR_NONE;
	wifi_ap_h ap = NULL;
	Evas_Object *info_popup = NULL;

	if (!ug_app_state->passpopup) {
		return;
	}

	/* Generate WPS pin */
	rpin = wps_generate_pin();
	if (rpin > 0)
		g_snprintf(npin, sizeof(npin), "%08d", rpin);

	pin_len = strlen(npin);
	if (pin_len != 8) {
		info_popup = common_utils_show_info_ok_popup(
				ug_app_state->layout_main, PACKAGE,
				sc(PACKAGE, I18N_TYPE_Invalid_pin),
				_info_popup_ok_cb,
				ug_app_state->passpopup);
		ug_app_state->passpopup->info_popup = info_popup;

		__COMMON_FUNC_EXIT__;
		return;
	}

	ap = passwd_popup_get_ap(ug_app_state->passpopup);

	ret = wlan_manager_wps_pin_connect(ap, npin);
	if (ret == WLAN_MANAGER_ERR_NONE) {
		INFO_LOG(UG_NAME_NORMAL, "wlan_manager_wps_pin_connect successful");

		create_pbc_popup(ug_app_state->passpopup,
				__wps_pbc_popup_cancel_connecting, NULL,
				POPUP_WPS_PIN, npin);
	} else {
		ERROR_LOG(UG_NAME_NORMAL, "wlan_manager_wps_pin_connect failed");
		passwd_popup_free(ug_app_state->passpopup);
		ug_app_state->passpopup = NULL;
	}

	__COMMON_FUNC_EXIT__;
}

static void __wps_options_popup_cb(void* data, Evas_Object* obj, void* event_info)
{
	__COMMON_FUNC_ENTER__;
	pswd_popup_create_req_data_t popup_info;

	if (!ug_app_state->passpopup) {
		return;
	}

	memset(&popup_info, 0, sizeof(pswd_popup_create_req_data_t));

	popup_info.title = "IDS_WIFI_HEADER_SELECT_WPS_METHOD_ABB";
	popup_info.ok_cb = NULL;
	popup_info.cancel_cb = _wps_cancel_cb;
	popup_info.show_wps_btn = EINA_FALSE;
	popup_info.wps_btn_cb = _wps_btn_cb;
	popup_info.wps_pin_cb = _wps_pin_cb;
	popup_info.ap = passwd_popup_get_ap(ug_app_state->passpopup);
	popup_info.cb_data = NULL;
	create_wps_options_popup(ug_app_state->layout_main, ug_app_state->passpopup,
			&popup_info);

	__COMMON_FUNC_EXIT__;
}

void viewer_list_wifi_reconnect(wifi_device_info_t *device_info)
{
	pswd_popup_create_req_data_t popup_info;
	wifi_security_type_e sec_type = WIFI_SECURITY_TYPE_NONE;
	wifi_device_info_t *device_info_temp = NULL;
	struct ug_data *ugd = (struct ug_data *)ug_app_state->gadget;

	if (device_info == NULL)
		return;

	if (ug_app_state->passpopup != NULL || ug_app_state->eap_view != NULL) {
		INFO_LOG(UG_NAME_ERR, "already launched popup");
		return;
	}

	wifi_ap_get_security_type(device_info->ap, &sec_type);

	switch (sec_type) {
	case WIFI_SECURITY_TYPE_WEP:
	case WIFI_SECURITY_TYPE_WPA_PSK:
	case WIFI_SECURITY_TYPE_WPA2_PSK:
		memset(&popup_info, 0, sizeof(pswd_popup_create_req_data_t));

		device_info_temp = view_list_item_device_info_create(device_info->ap);

		popup_info.title = device_info->ssid;
		popup_info.ok_cb = __passwd_popup_ok_cb;
		popup_info.cancel_cb = __passwd_popup_cancel_cb;
		popup_info.show_wps_btn = device_info->wps_mode;
		popup_info.wps_btn_cb = __wps_options_popup_cb;
		popup_info.ap = device_info->ap;
		popup_info.cb_data = device_info_temp;
		popup_info.sec_type = sec_type;

		/* TODO: parameter with device_info */
		/* TODO: finally parameter with wifi_ap_h, WPA, EAP */
		ug_app_state->passpopup = create_passwd_popup(
				ug_app_state->conformant,
				ug_app_state->layout_main, PACKAGE, &popup_info);

		if (ug_app_state->passpopup == NULL) {
			INFO_LOG(UG_NAME_ERR, "Fail to create password popup");
		}

		break;
	case WIFI_SECURITY_TYPE_EAP:
		ug_app_state->eap_view = create_eap_view(
				ug_app_state->layout_main, ugd->win_main,
				ug_app_state->conformant,
				PACKAGE, device_info,
				viewer_manager_eap_view_deref);
		break;
	default:
		ERROR_LOG(UG_NAME_NORMAL, "Unknown security type [%d]", sec_type);
		return;
	}

}

void viewer_list_wifi_connect(wifi_device_info_t *device_info)
{
	bool favorite = false;
	pswd_popup_create_req_data_t popup_info;
	wifi_security_type_e sec_type = WIFI_SECURITY_TYPE_NONE;
	wifi_device_info_t *dev_info_temp = NULL;
	struct ug_data *ugd = (struct ug_data *)ug_app_state->gadget;

	if (device_info == NULL)
		return;

	wifi_ap_is_favorite(device_info->ap, &favorite);
	if (favorite == true) {
		wlan_manager_connect(device_info->ap);
		return;
	}

	if (ug_app_state->passpopup != NULL || ug_app_state->eap_view != NULL) {
		INFO_LOG(UG_NAME_ERR, "already launched popup");
		return;
	}

	wifi_ap_get_security_type(device_info->ap, &sec_type);

	switch (sec_type) {
	case WIFI_SECURITY_TYPE_NONE:
		if(ug_app_state->is_hidden) {
			wifi_ap_h ap;
			char *ssid;
			wifi_ap_get_essid(device_info->ap, &ssid);
			wifi_ap_hidden_create(ssid, &ap);
			g_free(ssid);
			wifi_ap_set_security_type(ap, WIFI_SECURITY_TYPE_NONE);
			wlan_manager_connect(ap);
		} else
			wlan_manager_connect(device_info->ap);
		break;

	case WIFI_SECURITY_TYPE_WEP:
	case WIFI_SECURITY_TYPE_WPA_PSK:
	case WIFI_SECURITY_TYPE_WPA2_PSK:
		memset(&popup_info, 0, sizeof(pswd_popup_create_req_data_t));

		dev_info_temp = view_list_item_device_info_create(device_info->ap);

		popup_info.title = device_info->ssid;
		popup_info.ok_cb = __passwd_popup_ok_cb;
		popup_info.cancel_cb = __passwd_popup_cancel_cb;
		popup_info.show_wps_btn = device_info->wps_mode;
		popup_info.wps_btn_cb = __wps_options_popup_cb;
		popup_info.ap = device_info->ap;
		popup_info.cb_data = dev_info_temp;
		popup_info.sec_type = sec_type;

		/* TODO: parameter with device_info */
		/* TODO: finally parameter with wifi_ap_h, WPA, EAP */
		ug_app_state->passpopup = create_passwd_popup(
				ug_app_state->conformant,
				ug_app_state->layout_main, PACKAGE, &popup_info);

		if (ug_app_state->passpopup == NULL) {
			INFO_LOG(UG_NAME_ERR, "Fail to create password popup");
		}

		break;

	case WIFI_SECURITY_TYPE_EAP:
		ug_app_state->eap_view = create_eap_view(
				ug_app_state->layout_main, ugd->win_main,
				ug_app_state->conformant,
				PACKAGE, device_info,
				viewer_manager_eap_view_deref);
		break;

	default:
		ERROR_LOG(UG_NAME_NORMAL, "Unknown security type[%d]", sec_type);
		break;
	}
}

void viewer_list_clear_disconnect_popup(wifi_ap_h ap)
{
	if (!g_disconnect_popup)
		return;

	if (ap && g_disconnect_popup->ap &&
			wlan_manager_is_same_network(g_disconnect_popup->ap, ap) != TRUE)
		return;

	if (g_disconnect_popup->popup)
		evas_object_del(g_disconnect_popup->popup);

	if (g_disconnect_popup->ap)
		wifi_ap_destroy(g_disconnect_popup->ap);

	g_free(g_disconnect_popup);
	g_disconnect_popup = NULL;
}

static void __viewer_list_disconnect_cancel_cb(void *data,
		Evas_Object *obj, void *event_info)
{
	struct connecting_cancel_popup_data *popup =
			(struct connecting_cancel_popup_data *)data;
	if (!popup)
		return;

	if (popup->popup)
		evas_object_del(popup->popup);

	if (popup->ap)
		wifi_ap_destroy(popup->ap);

	g_free(popup);
	g_disconnect_popup = NULL;
}

static gboolean __viewer_list_update_ap(gpointer data)
{
	wifi_ap_h ap = (wifi_ap_h)data;

	viewer_manager_update_item_favorite_status(ap);
	wifi_ap_destroy(ap);

	return FALSE;
}

static void __viewer_list_disconnect_ok_cb(void *data,
		Evas_Object *obj, void *event_info)
{
	guint id;
	struct connecting_cancel_popup_data *popup =
			(struct connecting_cancel_popup_data *)data;
	if (!popup)
		return;

	wlan_manager_disconnect(popup->ap);
	wlan_manager_forget(popup->ap);

	if (popup->popup)
		evas_object_del(popup->popup);

	id = common_util_managed_idle_add(__viewer_list_update_ap, (gpointer)popup->ap);
	if (!id) {
		viewer_manager_update_item_favorite_status(popup->ap);
		wifi_ap_destroy(popup->ap);
	}

	g_free(popup);
	g_disconnect_popup = NULL;
}

static void __viewer_list_disconnect_popup(wifi_device_info_t *device_info,
		Evas_Object *win_main)
{
	popup_btn_info_t popup_data;
	struct connecting_cancel_popup_data *popup = NULL;

	memset(&popup_data, 0, sizeof(popup_data));

	popup = g_try_new0(struct connecting_cancel_popup_data, 1);
	if (!popup)
		return;

	g_disconnect_popup = popup;
	wifi_ap_clone(&popup->ap, device_info->ap);

	popup_data.title_txt = "IDS_WIFI_OPT_FORGET_NETWORK";
	popup_data.info_txt = "IDS_WIFI_POP_CURRENT_NETWORK_WILL_BE_DISCONNECTED";
	popup_data.btn1_cb = __viewer_list_disconnect_cancel_cb;
	popup_data.btn1_txt = "IDS_WIFI_SK_CANCEL";
	popup_data.btn1_data = popup;
	popup_data.btn2_cb = __viewer_list_disconnect_ok_cb;
	popup_data.btn2_txt = "IDS_WIFI_SK_FORGET";
	popup_data.btn2_data = popup;

	popup->popup = common_utils_show_info_popup(win_main, &popup_data);
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

	SECURE_INFO_LOG(UG_NAME_NORMAL, "ssid --- %s", device_info->ssid);
	INFO_LOG(UG_NAME_NORMAL, "ap --- 0x%x", device_info->ap);
	INFO_LOG(UG_NAME_NORMAL, "current item_state state is --- %d", item_state);

	if (ug_app_state->is_lbhome == EINA_TRUE &&
			ug_app_state->app_control != NULL) {
		INFO_LOG(UG_NAME_NORMAL, "exit with reply");
		char *bssid = NULL;
		app_control_h reply;

		app_control_create(&reply);
		wifi_ap_get_bssid(device_info->ap, &bssid);
		SECURE_INFO_LOG(UG_NAME_NORMAL, "bssid %s, ssid %s", bssid, device_info->ssid);

		app_control_add_extra_data(reply, "bssid", bssid);
		app_control_add_extra_data(reply, "ssid", device_info->ssid);
		app_control_reply_to_launch_request(reply, ug_app_state->app_control,
				APP_CONTROL_RESULT_SUCCEEDED);
		app_control_destroy(reply);
		g_free(bssid);
		wifi_exit();
		return;
	}

	switch (item_state) {
	case VIEWER_ITEM_RADIO_MODE_OFF:
		current_state = viewer_manager_header_mode_get();

		INFO_LOG(UG_NAME_NORMAL, "Clicked AP information\n");
		INFO_LOG(UG_NAME_NORMAL, "header mode [%d]", current_state);

		switch (current_state) {
		case HEADER_MODE_ON:
		case HEADER_MODE_CONNECTED:
		case HEADER_MODE_CONNECTING:
		case HEADER_MODE_SEARCHING:
			viewer_list_wifi_connect(device_info);
			break;

		case HEADER_MODE_OFF:
		case HEADER_MODE_ACTIVATING:
		case HEADER_MODE_DEACTIVATING:
		default:
			INFO_LOG(UG_NAME_NORMAL, "Ignore click");
			break;
		}
		break;

	case VIEWER_ITEM_RADIO_MODE_CONNECTING:
	case VIEWER_ITEM_RADIO_MODE_CONFIGURATION:
		__viewer_list_disconnect_popup(device_info, ug_app_state->layout_main);
		break;
	case VIEWER_ITEM_RADIO_MODE_CONNECTED:
	default:
		INFO_LOG(UG_NAME_NORMAL, "Ignore click");
		break;
	}

	elm_genlist_item_selected_set((Elm_Object_Item *)event_info, EINA_FALSE);

	__COMMON_FUNC_EXIT__;
}

static char *viewer_list_get_device_status_txt(wifi_device_info_t *wifi_device,
		VIEWER_ITEM_RADIO_MODES mode)
{
	char *status_txt = NULL;
	int current_state = 0;

	current_state = viewer_manager_header_mode_get();
	/* The strings are currently hard coded. It will be replaced with string ids later */
	if (VIEWER_ITEM_RADIO_MODE_CONNECTING == mode) {
		status_txt = g_strdup(sc(PACKAGE, I18N_TYPE_Connecting));
	} else if (VIEWER_ITEM_RADIO_MODE_CONFIGURATION == mode) {
		status_txt = g_strdup(sc(PACKAGE, I18N_TYPE_Obtaining_IP_addr));
	} else if (VIEWER_ITEM_RADIO_MODE_CONNECTED == mode) {
		if (connection_manager_is_wifi_connection_used() ||
				current_state == HEADER_MODE_CONNECTED ) {
			status_txt = g_strdup(sc(PACKAGE, I18N_TYPE_Connected));
		}
	} else if (VIEWER_ITEM_RADIO_MODE_OFF == mode) {
		status_txt = common_utils_get_ap_security_type_info_txt(PACKAGE,
			wifi_device, true);
	} else {
		status_txt = g_strdup(sc(PACKAGE, I18N_TYPE_Unknown));
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

	// To use multiline textblock/entry/editfield in genlist, set height_for_width mode
	// then the item's height is calculated while the item's width fits to genlist width.
	elm_genlist_mode_set(viewer_list, ELM_LIST_COMPRESS);
	elm_genlist_realization_mode_set(viewer_list, EINA_TRUE);

	evas_object_size_hint_weight_set(viewer_list, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(viewer_list, EVAS_HINT_FILL, EVAS_HINT_FILL);

	itc.item_style = WIFI_GENLIST_2LINE_TOP_TEXT_ICON_STYLE;
	itc.func.text_get = _gl_listview_text_get;
	itc.func.content_get = _gl_listview_content_get;
	itc.func.state_get = NULL;
	itc.func.del = _gl_listview_del;

	no_wifi_device_itc.item_style = WIFI_GENLIST_1LINE_TEXT_STYLE;
	no_wifi_device_itc.func.text_get = _gl_listview_text_get;
	no_wifi_device_itc.func.content_get = NULL;
	no_wifi_device_itc.func.state_get = NULL;
	no_wifi_device_itc.func.del = _gl_listview_del;

	first_item = last_item = NULL;

	evas_object_smart_callback_add(viewer_list, "realized",
			_gl_realized, NULL);
	evas_object_smart_callback_add(viewer_list, "language,changed",
			gl_lang_changed, NULL);

	__COMMON_FUNC_EXIT__;
	return viewer_list;
}

void viewer_list_title_item_del(void)
{
	if (grouptitle != NULL) {
		elm_object_item_del(grouptitle);
		grouptitle = NULL;
	}
}

void viewer_list_title_item_update(void)
{
	if (grouptitle != NULL)
		elm_genlist_item_update(grouptitle);
}

void viewer_list_title_item_set(Elm_Object_Item *item_header)
{
	if (item_header == NULL)
		return;

	if (grouptitle != NULL) {
		elm_genlist_item_update(grouptitle);
		return;
	}

	memset(&grouptitle_itc, 0, sizeof(grouptitle_itc));
	grouptitle_itc.item_style = WIFI_GENLIST_GROUP_INDEX_STYLE;
	grouptitle_itc.func.text_get = _gl_text_available_networks_get;
	grouptitle_itc.func.content_get = _gl_content_scanning_icon_get;

	grouptitle = elm_genlist_item_insert_after(viewer_list,
			&grouptitle_itc, NULL, NULL, item_header,
			ELM_GENLIST_ITEM_NONE, NULL, NULL);

	assertm_if(NULL == grouptitle, "NULL!!");

	elm_genlist_item_select_mode_set(grouptitle,
			ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);
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
		SECURE_INFO_LOG(UG_NAME_NORMAL, "[%s] is already in requested state", gdata->device_info->ssid);
		return FALSE;
	}

	SECURE_INFO_LOG(UG_NAME_NORMAL, "[%s] AP Item State Transition from [%d] --> [%d]", gdata->device_info->ssid, gdata->radio_mode, mode);
	gdata->radio_mode = mode;
	if (gdata->device_info->ap_status_txt) {
		g_free(gdata->device_info->ap_status_txt);
		gdata->device_info->ap_status_txt = viewer_list_get_device_status_txt(gdata->device_info, mode);
	}

	if(item != NULL)
		elm_genlist_item_update(item);

	__COMMON_FUNC_EXIT__;
	return TRUE;
}

static wifi_connection_state_e _convert_wifi_state_to_viewer_radio_mode(
		wifi_connection_state_e state)
{
	switch (state) {
		case WIFI_CONNECTION_STATE_ASSOCIATION:
			return VIEWER_ITEM_RADIO_MODE_CONNECTING;
		case WIFI_CONNECTION_STATE_CONFIGURATION:
			return VIEWER_ITEM_RADIO_MODE_CONFIGURATION;
		case WIFI_CONNECTION_STATE_CONNECTED:
			return VIEWER_ITEM_RADIO_MODE_CONNECTED;
		default:
			return VIEWER_ITEM_RADIO_MODE_OFF;
	}
}

Elm_Object_Item *viewer_list_item_insert_after(wifi_device_info_t *wifi_device,
		Elm_Object_Item *after)
{
	Elm_Object_Item* ret = NULL;
	ug_genlist_data_t* gdata = NULL;
	wifi_device_info_t *no_wifi_device = NULL;
	wifi_connection_state_e state = WIFI_CONNECTION_STATE_DISCONNECTED;
	VIEWER_ITEM_RADIO_MODES rad_mode = VIEWER_ITEM_RADIO_MODE_OFF;
	retvm_if(NULL == viewer_list, NULL);

	if (wifi_device != NULL) {
		if (wifi_ap_get_connection_state(wifi_device->ap, &state) ==
				WIFI_ERROR_NONE) {
			rad_mode = _convert_wifi_state_to_viewer_radio_mode(state);
		}
		wifi_device->ap_status_txt = viewer_list_get_device_status_txt(
						wifi_device,
						rad_mode);
	} else {
		if (ug_app_state->is_first_scan == true) {
			int scan_result = wlan_manager_scan();
			if (scan_result != WLAN_MANAGER_ERR_NONE) {
				viewer_manager_hide(VIEWER_WINSET_SEARCHING);
				viewer_manager_header_mode_set(viewer_manager_header_mode_get());
			}
			ug_app_state->is_first_scan = false;
			return NULL;
		}
		no_wifi_device = g_try_new0(wifi_device_info_t, 1);
		if (no_wifi_device == NULL)
			return NULL;

		no_wifi_device->ssid = g_strdup(sc(PACKAGE, I18N_TYPE_No_Wi_Fi_AP_Found));

		wifi_device = no_wifi_device;
	}

	gdata = g_try_new0(ug_genlist_data_t, 1);
	retvm_if(NULL == gdata, NULL);
	gdata->device_info = wifi_device;
	gdata->radio_mode = VIEWER_ITEM_RADIO_MODE_OFF;

	if (!after) {
		/* If the after item is NULL then insert it as first item */
		after = grouptitle;
	}

	if (no_wifi_device == NULL) {
		ret = elm_genlist_item_insert_after(
				viewer_list, /*obj*/
				&itc,/*itc*/
				gdata,/*data*/
				NULL,/*parent*/
				after, /*after than*/
				ELM_GENLIST_ITEM_NONE, /*flags*/
				__viewer_list_item_clicked_cb,/*func*/
				NULL);/*func_data*/
	} else {
		ret = elm_genlist_item_insert_after(
				viewer_list, /*obj*/
				&no_wifi_device_itc,/*itc*/
				gdata,/*data*/
				NULL,/*parent*/
				after, /*after than*/
				ELM_GENLIST_ITEM_NONE, /*flags*/
				__viewer_list_item_clicked_cb,/*func*/
				NULL);/*func_data*/
	}

	if (!ret) {
		assertm_if(NULL == ret, "NULL!!");
		g_free(gdata);
	} else {
		/* SECURE_DEBUG_LOG(UG_NAME_NORMAL,
				"* item add complete item [0x%x] ssid:[%s] security[%d] size:[%d]",
				ret,
				wifi_device->ssid,
				wifi_device->security_mode,
				viewer_list_item_size_get()); */

		if (after == grouptitle) {
			first_item = ret;
			if (!last_item) {
				last_item = ret;
			}
		} else {
			last_item = ret;
			if (!first_item) {
				first_item = ret;
			}
		}

		if(ret != NULL)
			elm_genlist_item_update(ret);
	}

	return ret;
}

void viewer_list_item_del(Elm_Object_Item *it)
{
	if (it == NULL) {
		return;
	}

	if (it == first_item) {
		first_item = elm_genlist_item_next_get(first_item);
	} else if (it == last_item) {
		last_item = elm_genlist_item_prev_get(last_item);
	}
	elm_object_item_del(it);
}

int viewer_list_item_size_get()
{
	int ret = 0;
	Elm_Object_Item *it = first_item;

	while(it) {
		ret++;
		if (it == last_item) {
			break;
		}
		it = elm_genlist_item_next_get(it);
	}

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
		if (it == last_item) {
			break;
		}
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

		if (it == last_item) {
			break;
		}

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

		if (it == last_item) {
			break;
		}

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
			if (!g_strcmp0(device_info->ssid, essid) && device_info->security_mode == sec_mode) {
				break;
			}
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

Elm_Object_Item* item_get_for_ssid(const char *ssid)
{
	Elm_Object_Item *it = first_item;
	while (it) {
		ug_genlist_data_t* gdata = elm_object_item_data_get(it);
		wifi_device_info_t *device_info = NULL;
		if (gdata && (device_info = gdata->device_info)) {
			if (!g_strcmp0(device_info->ssid, ssid)) {
				break;
			}
		}
		if (it == last_item) {
			it = NULL;
			break;
		}
		it = elm_genlist_item_next_get(it);
	}

	return it;
}
