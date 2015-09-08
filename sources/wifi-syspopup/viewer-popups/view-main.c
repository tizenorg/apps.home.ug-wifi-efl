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

#include <utilX.h>
#include "common.h"
#include "view-main.h"
#include "common_pswd_popup.h"
#include "common_generate_pin.h"
#include "common_utils.h"
#include "view-alerts.h"
#include "i18nmanager.h"
#include "common_eap_connect.h"
#define VCONF_SORT_BY "file/private/wifi/sort_by"

#define QS_POPUP_CONNECTION_STATE	"qs_popup_connection_state"
#define WIFI_DEVPKR_EDJ "/usr/apps/wifi-efl-ug/res/edje/wifi-qs/wifi-syspopup-custom.edj"
#define WIFI_SYSPOPUP_EMPTY_GRP "devpkr_no_wifi_networks"

struct connecting_cancel_popup_data {
	Evas_Object *popup;
	wifi_ap_h ap;
};

static struct connecting_cancel_popup_data *g_disconnect_popup = NULL;

extern wifi_object* devpkr_app_state;

static Evas_Object* list = NULL;
static Elm_Genlist_Item_Class itc;
static int profiles_list_size = 0;
static Elm_Genlist_Item_Class grouptitle_itc;
static Elm_Object_Item *grouptitle = NULL;

static GList *wifi_device_list = NULL;

int view_main_get_profile_count(void)
{
	return profiles_list_size;
}

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

	if (devpkr_app_state->passpopup == NULL) {
		return;
	}

	ap = passwd_popup_get_ap(devpkr_app_state->passpopup);
	password = passwd_popup_get_txt(devpkr_app_state->passpopup);
	if(password != NULL)
		password_len = strlen(password);
	else
		password_len = 0;

	wifi_ap_get_security_type(ap, &sec_type);

	switch (sec_type) {
	case WIFI_SECURITY_TYPE_WEP:
		if (password_len != 5 && password_len != 13 &&
				password_len != 26 && password_len != 10) {
			common_utils_send_message_to_net_popup(
					"Network connection popup",
					"wrong password", "toast_popup", NULL);

			if (devpkr_app_state->passpopup->entry) {
				elm_object_focus_set(
						devpkr_app_state->passpopup->entry,
						EINA_TRUE);
			}
			goto popup_ok_exit;
		}
		break;

	case WIFI_SECURITY_TYPE_WPA_PSK:
	case WIFI_SECURITY_TYPE_WPA2_PSK:
		if (password_len < 8 || password_len > 64) {
			common_utils_send_message_to_net_popup(
					"Network connection popup",
					"wrong password", "toast_popup", NULL);

			if (devpkr_app_state->passpopup->entry) {
				elm_object_focus_set(
						devpkr_app_state->passpopup->entry,
						EINA_TRUE);
			}
			goto popup_ok_exit;
		}
		break;

	default:
		ERROR_LOG(SP_NAME_ERR, "Wrong security mode: %d", sec_type);
		passwd_popup_free(devpkr_app_state->passpopup);
		break;
	}

	wlan_manager_connect_with_password(ap, password);

	passwd_popup_free(devpkr_app_state->passpopup);
	devpkr_app_state->passpopup = NULL;

popup_ok_exit:
	g_free((gpointer)password);

	__COMMON_FUNC_EXIT__;
}

static void __popup_cancel_cb(void *data, Evas_Object *obj, void *event_info)
{
	__COMMON_FUNC_ENTER__;

	if (devpkr_app_state->passpopup == NULL) {
		return;
	}

	passwd_popup_free(devpkr_app_state->passpopup);
	devpkr_app_state->passpopup = NULL;

	elm_object_focus_set(devpkr_app_state->popup , EINA_TRUE);

	__COMMON_FUNC_EXIT__;
}

static void __wps_pbc_popup_cancel_connecting(void *data, Evas_Object *obj,
		void *event_info)
{
	if (devpkr_app_state->passpopup == NULL) {
		return;
	}

	wifi_ap_h ap = passwd_popup_get_ap(devpkr_app_state->passpopup);

	int ret = wlan_manager_disconnect(ap);
	if (ret != WLAN_MANAGER_ERR_NONE) {
		ERROR_LOG(SP_NAME_ERR, "Failed WPS PBC cancellation [0x%x]", ap);
	}

	passwd_popup_free(devpkr_app_state->passpopup);
	devpkr_app_state->passpopup = NULL;
}

static void _wps_btn_cb(void* data, Evas_Object* obj, void* event_info)
{
	__COMMON_FUNC_ENTER__;

	if (!devpkr_app_state->passpopup) {
		return;
	}

	wifi_ap_h ap = passwd_popup_get_ap(devpkr_app_state->passpopup);
	int ret = wlan_manager_wps_connect(ap);
	if (ret == WLAN_MANAGER_ERR_NONE) {
		create_pbc_popup(devpkr_app_state->passpopup,
				__wps_pbc_popup_cancel_connecting, NULL,
				POPUP_WPS_BTN, NULL);
	} else {
		ERROR_LOG(SP_NAME_ERR, "wlan_manager_wps_connect failed");
		wifi_ap_destroy(ap);

		passwd_popup_free(devpkr_app_state->passpopup);
		devpkr_app_state->passpopup = NULL;
	}

	__COMMON_FUNC_EXIT__;
}

static void _wps_cancel_cb(void* data, Evas_Object* obj, void* event_info)
{
	__COMMON_FUNC_ENTER__;

	if (devpkr_app_state->passpopup == NULL) {
		return;
	}

	current_popup_free(devpkr_app_state->passpopup, POPUP_WPS_OPTIONS);

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

	if (!devpkr_app_state->passpopup) {
		return;
	}

	/* Generate WPS pin */
	rpin = wps_generate_pin();
	if (rpin > 0)
		g_snprintf(npin, sizeof(npin), "%08d", rpin);

	pin_len = strlen(npin);
	if (pin_len != 8) {
		view_alerts_popup_show(sc(PACKAGE, I18N_TYPE_Invalid_pin));

		__COMMON_FUNC_EXIT__;
		return;
	}

	ap = passwd_popup_get_ap(devpkr_app_state->passpopup);

	ret = wlan_manager_wps_pin_connect(ap, npin);
	if (ret == WLAN_MANAGER_ERR_NONE) {
		INFO_LOG(UG_NAME_NORMAL, "wlan_manager_wps_pin_connect successful");

		create_pbc_popup(devpkr_app_state->passpopup,
				__wps_pbc_popup_cancel_connecting, NULL,
				POPUP_WPS_PIN, npin);
	} else {
		ERROR_LOG(UG_NAME_NORMAL, "wlan_manager_wps_pin_connect failed");

		passwd_popup_free(devpkr_app_state->passpopup);
		devpkr_app_state->passpopup = NULL;
	}

	__COMMON_FUNC_EXIT__;
}

static void __popup_wps_options_cb(void* data, Evas_Object* obj, void* event_info)
{
	__COMMON_FUNC_ENTER__;
	pswd_popup_create_req_data_t popup_info;

	if (!devpkr_app_state->passpopup) {
		return;
	}

	memset(&popup_info, 0, sizeof(pswd_popup_create_req_data_t));

	popup_info.title = g_strdup(sc(PACKAGE, I18N_TYPE_Select_WPS_Method));
	popup_info.ok_cb = NULL;
	popup_info.cancel_cb = _wps_cancel_cb;
	popup_info.show_wps_btn = EINA_FALSE;
	popup_info.wps_btn_cb = _wps_btn_cb;
	popup_info.wps_pin_cb = _wps_pin_cb;
	popup_info.ap = passwd_popup_get_ap(devpkr_app_state->passpopup);
	popup_info.cb_data = NULL;
	create_wps_options_popup(devpkr_app_state->layout_main,
			devpkr_app_state->passpopup, &popup_info);

	__COMMON_FUNC_EXIT__;
}

void view_main_eap_view_deref(void)
{
	devpkr_app_state->eap_popup = NULL;
}

void view_main_wifi_reconnect(devpkr_gl_data_t *gdata)
{
	wifi_device_info_t *device_info;
	pswd_popup_create_req_data_t popup_info;
	wifi_security_type_e sec_type = WIFI_SECURITY_TYPE_NONE;

	retm_if(NULL == gdata);

	device_info = gdata->dev_info;
	retm_if(NULL == device_info);

	if (devpkr_app_state->passpopup != NULL ||
			devpkr_app_state->eap_popup != NULL) {
		INFO_LOG(SP_NAME_ERR, "already launched popup");
		return;
	}

	wifi_ap_get_security_type(device_info->ap, &sec_type);

	switch (sec_type) {
	case WIFI_SECURITY_TYPE_WEP:
	case WIFI_SECURITY_TYPE_WPA_PSK:
	case WIFI_SECURITY_TYPE_WPA2_PSK:
		memset(&popup_info, 0, sizeof(pswd_popup_create_req_data_t));

		popup_info.title = gdata->dev_info->ssid;
		popup_info.ok_cb = __popup_ok_cb;
		popup_info.cancel_cb = __popup_cancel_cb;
		popup_info.show_wps_btn = gdata->dev_info->wps_mode;
		popup_info.wps_btn_cb = __popup_wps_options_cb;
		popup_info.ap = gdata->dev_info->ap;
		popup_info.cb_data = NULL;
		popup_info.sec_type = sec_type;

		devpkr_app_state->passpopup = create_passwd_popup(
				devpkr_app_state->conformant,
				devpkr_app_state->layout_main, PACKAGE,
				&popup_info);
		if (devpkr_app_state->passpopup == NULL) {
			ERROR_LOG(SP_NAME_NORMAL, "Password popup creation failed");
		}
		break;

	case WIFI_SECURITY_TYPE_EAP:
		devpkr_app_state->eap_popup = create_eap_view(
				devpkr_app_state->layout_main,
				devpkr_app_state->win_main,
				devpkr_app_state->conformant,
				PACKAGE, gdata->dev_info, view_main_eap_view_deref);
		break;

	default:
		ERROR_LOG(SP_NAME_NORMAL, "Unknown security type [%d]", sec_type);
		break;
	}
}

void view_main_wifi_connect(devpkr_gl_data_t *gdata)
{
	bool favorite = false;
	wifi_device_info_t *device_info;
	pswd_popup_create_req_data_t popup_info;
	wifi_security_type_e sec_type = WIFI_SECURITY_TYPE_NONE;

	retm_if(NULL == gdata);

	device_info = gdata->dev_info;
	retm_if(NULL == device_info);

	wifi_ap_is_favorite(device_info->ap, &favorite);

	if (favorite == true) {
		wlan_manager_connect(device_info->ap);
		return;
	}

	if (devpkr_app_state->passpopup != NULL ||
			devpkr_app_state->eap_popup != NULL) {
		INFO_LOG(SP_NAME_ERR, "already launched popup");
		return;
	}

	wifi_ap_get_security_type(device_info->ap, &sec_type);

	switch (sec_type) {
	case WIFI_SECURITY_TYPE_NONE:
		wlan_manager_connect(device_info->ap);
		break;

	case WIFI_SECURITY_TYPE_WEP:
	case WIFI_SECURITY_TYPE_WPA_PSK:
	case WIFI_SECURITY_TYPE_WPA2_PSK:
		memset(&popup_info, 0, sizeof(pswd_popup_create_req_data_t));

		popup_info.title = gdata->dev_info->ssid;
		popup_info.ok_cb = __popup_ok_cb;
		popup_info.cancel_cb = __popup_cancel_cb;
		popup_info.show_wps_btn = gdata->dev_info->wps_mode;
		popup_info.wps_btn_cb = __popup_wps_options_cb;
		popup_info.ap = gdata->dev_info->ap;
		popup_info.cb_data = NULL;
		popup_info.sec_type = sec_type;

		devpkr_app_state->passpopup = create_passwd_popup(
				devpkr_app_state->conformant,
				devpkr_app_state->layout_main, PACKAGE,
				&popup_info);
		if (devpkr_app_state->passpopup == NULL) {
			ERROR_LOG(SP_NAME_NORMAL, "Password popup creation failed");
		}
		break;

	case WIFI_SECURITY_TYPE_EAP:
		devpkr_app_state->eap_popup = create_eap_view(
				devpkr_app_state->layout_main,
				devpkr_app_state->win_main,
				devpkr_app_state->conformant,
				PACKAGE, gdata->dev_info, view_main_eap_view_deref);
		break;

	default:
		ERROR_LOG(SP_NAME_NORMAL, "Unknown security type [%d]", sec_type);
		break;
	}
}

Elm_Object_Item *view_main_item_get_for_ap(wifi_ap_h ap)
{
	__COMMON_FUNC_ENTER__;
	if (!ap || !list) {
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

	Elm_Object_Item *it = elm_genlist_first_item_get(list);
	wlan_security_mode_type_t sec_mode = common_utils_get_sec_mode(type);
	while (it) {
		devpkr_gl_data_t* gdata = elm_object_item_data_get(it);
		wifi_device_info_t *device_info = NULL;
		if (gdata && (device_info = gdata->dev_info)) {
			if (!g_strcmp0(device_info->ssid, essid) && device_info->security_mode == sec_mode) {
				break;
			}
		}

		it = elm_genlist_item_next_get(it);
	}

	g_free(essid);
	__COMMON_FUNC_EXIT__;
	return it;
}

#if 0
/* Unused function */
Elm_Object_Item *__view_main_get_item_in_mode(ITEM_CONNECTION_MODES mode)
{
	Elm_Object_Item* it = NULL;
	it = elm_genlist_first_item_get(list);
	__COMMON_FUNC_ENTER__;
	while (it) {
		devpkr_gl_data_t *gdata = (devpkr_gl_data_t *)elm_object_item_data_get(it);
		if (gdata && gdata->connection_mode == mode) {
			SECURE_INFO_LOG( SP_NAME_NORMAL, "Found Item [%s] in mode[%d]", gdata->dev_info->ssid, mode);
			__COMMON_FUNC_EXIT__;
			return it;
		}
		it = elm_genlist_item_next_get(it);
	}

	__COMMON_FUNC_EXIT__;
	return NULL;
}
#endif

void view_main_clear_disconnect_popup(wifi_ap_h ap)
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

static void __view_main_disconnect_cancel_cb(void *data,
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

static gboolean __view_main_update_ap(gpointer data)
{
	wifi_ap_h ap = (wifi_ap_h)data;

	wifi_ap_destroy(ap);

	return FALSE;
}

static void __view_main_disconnect_ok_cb(void *data,
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

	id = common_util_managed_idle_add(__view_main_update_ap, (gpointer)popup->ap);
	if (!id) {
		wifi_ap_destroy(popup->ap);
	}

	g_free(popup);
	g_disconnect_popup = NULL;
}

static void __view_main_disconnect_popup(wifi_device_info_t *device_info,
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
	popup_data.btn1_cb = __view_main_disconnect_cancel_cb;
	popup_data.btn1_txt = "IDS_WIFI_SK_CANCEL";
	popup_data.btn1_data = popup;
	popup_data.btn2_cb = __view_main_disconnect_ok_cb;
	popup_data.btn2_txt = "IDS_WIFI_SK_FORGET";
	popup_data.btn2_data = popup;

	popup->popup = common_utils_show_info_popup(win_main, &popup_data);
}

static void __gl_sel(void *data, Evas_Object *obj, void *event_info)
{
	__COMMON_FUNC_ENTER__;

	assertm_if(NULL == obj, "obj is NULL!!");
	assertm_if(NULL == event_info, "event_info is NULL!!");

	Elm_Object_Item *item = (Elm_Object_Item *)event_info;
	devpkr_gl_data_t *gdata = (devpkr_gl_data_t *)elm_object_item_data_get(item);
	if (!gdata || !gdata->dev_info) {
		__COMMON_FUNC_EXIT__;
		return;
	}

	switch (gdata->connection_mode) {
	case ITEM_CONNECTION_MODE_OFF:
		view_main_wifi_connect(gdata);
		break;

	case ITEM_CONNECTION_MODE_CONNECTING:
	case ITEM_CONNECTION_MODE_CONFIGURATION:
		__view_main_disconnect_popup(gdata->dev_info, devpkr_app_state->layout_main);
		break;

	default:
		break;
	}

	elm_genlist_item_selected_set(item, EINA_FALSE);

	__COMMON_FUNC_EXIT__;
}

static char *_gl_text_get(void *data, Evas_Object *obj, const char *part)
{
	char *ret = NULL;
	char *txt = NULL;

	assertm_if(NULL == data, "data param is NULL!!");
	assertm_if(NULL == obj, "obj param is NULL!!");
	assertm_if(NULL == part, "part param is NULL!!");

	devpkr_gl_data_t *gdata = (devpkr_gl_data_t *) data;
	retvm_if(NULL == gdata, NULL);

	if (!strncmp(part, "elm.text.main.left.top", strlen(part))) {
		txt = evas_textblock_text_utf8_to_markup(NULL, gdata->dev_info->ssid);
		ret = g_strdup(txt);
		if (ret == NULL) {
			ERROR_LOG(SP_NAME_NORMAL, "ssid name is NULL!!");
		}
		g_free(txt);
	} else if (!strncmp(part, "elm.text.sub.left.bottom", strlen(part))) {
		if (ITEM_CONNECTION_MODE_CONNECTING == gdata->connection_mode) {
			ret = g_strdup(sc(PACKAGE, I18N_TYPE_Connecting));
		} else if (ITEM_CONNECTION_MODE_CONFIGURATION == gdata->connection_mode) {
			ret = g_strdup(sc(PACKAGE, I18N_TYPE_Obtaining_IP_addr));
		} else {
			ret = g_strdup(gdata->dev_info->ap_status_txt);
		}

		if (ret == NULL) {
			ERROR_LOG(SP_NAME_NORMAL, "ap_status_txt is NULL!!");
		}
	}

	return ret;
}

static Evas_Object *_gl_content_get(void *data, Evas_Object *obj, const char *part)
{
	if (data == NULL) {
		return NULL;
	}

	devpkr_gl_data_t *gdata = (devpkr_gl_data_t *) data;

	Evas_Object* icon = NULL;
	Evas_Object *ic = NULL;

	if (!strncmp(part, "elm.icon.1", strlen(part))) {
		char *temp_str = NULL;
		ic = elm_layout_add(obj);

		icon = elm_image_add(ic);
		retvm_if(NULL == icon, NULL);

		elm_layout_theme_set(ic, "layout", "list/B/type.3", "default");

		temp_str = g_strdup_printf("%s.png", gdata->dev_info->ap_image_path);
		elm_image_file_set(icon, CUSTOM_EDITFIELD_PATH, temp_str);
		g_free(temp_str);

		ea_theme_object_color_set(icon, "AO001");

		evas_object_size_hint_align_set(icon, EVAS_HINT_FILL, EVAS_HINT_FILL);
		evas_object_size_hint_weight_set(icon, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		elm_layout_content_set(ic, "elm.swallow.content", icon);
	} else if (!strncmp(part, "elm.icon.2", strlen(part))) {
		if (gdata->connection_mode == ITEM_CONNECTION_MODE_CONNECTING ||
				gdata->connection_mode == ITEM_CONNECTION_MODE_CONFIGURATION) {
			ic = elm_layout_add(obj);
			elm_layout_theme_set(ic, "layout", "list/C/type.2", "default");

			icon = elm_progressbar_add(obj);
			elm_object_style_set(icon, "process_medium");
			evas_object_size_hint_align_set(icon, EVAS_HINT_FILL, 0.5);
			evas_object_size_hint_weight_set(icon, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
			elm_progressbar_pulse(icon, TRUE);

			elm_layout_content_set(ic, "elm.swallow.content", icon);
		}
	}

	return ic;
}

static void _gl_list_del(void* data, Evas_Object* obj)
{
	if (data == NULL) {
		return;
	}

	devpkr_gl_data_t* gdata = (devpkr_gl_data_t *) data;

	if (gdata->dev_info) {
		SECURE_DEBUG_LOG(UG_NAME_NORMAL, "del target ssid: [%s]", gdata->dev_info->ssid);
		g_free(gdata->dev_info->ap_image_path);
		g_free(gdata->dev_info->ap_status_txt);
		g_free(gdata->dev_info->ssid);
		wifi_ap_destroy(gdata->dev_info->ap);
		g_free(gdata->dev_info);
	}

	g_free(gdata);

	return;
}

static Evas_Object *_create_genlist(Evas_Object* parent)
{
	__COMMON_FUNC_ENTER__;
	assertm_if(NULL == parent, "parent is NULL!!");

	list = elm_genlist_add(parent);
	assertm_if(NULL == list, "list allocation fail!!");
	elm_genlist_mode_set(list, ELM_LIST_COMPRESS);
	elm_genlist_homogeneous_set(list, EINA_TRUE);

	evas_object_size_hint_weight_set(list, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(list, EVAS_HINT_FILL, EVAS_HINT_FILL);

	itc.item_style = "2line.top";
	itc.func.text_get = _gl_text_get;
	itc.func.content_get = _gl_content_get;
	itc.func.state_get = NULL;
	itc.func.del = _gl_list_del;

	__COMMON_FUNC_EXIT__;

	return list;
}

static void view_main_scan_ui_clear(void)
{
	__COMMON_FUNC_ENTER__;

	if (list == NULL) {
		return;
	}
	elm_genlist_clear(list);

	__COMMON_FUNC_EXIT__;
}

void view_main_item_state_set(wifi_ap_h ap, ITEM_CONNECTION_MODES state)
{
	__COMMON_FUNC_ENTER__;

	char *item_ssid = NULL;
	wifi_security_type_e sec_type;
	wlan_security_mode_type_t item_sec_mode;
	Elm_Object_Item* it = NULL;

	it = elm_genlist_first_item_get(list);
	if (!it ||
		!ap ||
		(WIFI_ERROR_NONE != wifi_ap_get_essid(ap, &item_ssid)) ||
		(WIFI_ERROR_NONE != wifi_ap_get_security_type(ap, &sec_type))) {
		ERROR_LOG(SP_NAME_NORMAL, "Invalid params");
		__COMMON_FUNC_EXIT__;
		return;
	}
	item_sec_mode = common_utils_get_sec_mode(sec_type);
	SECURE_INFO_LOG(SP_NAME_NORMAL, "item state set for AP[%s] with sec mode[%d]", item_ssid, item_sec_mode);
	while (it) {
		devpkr_gl_data_t *gdata = (devpkr_gl_data_t *)elm_object_item_data_get(it);
		if (gdata != NULL) {
			SECURE_INFO_LOG(SP_NAME_NORMAL, "gdata AP[%s] with sec mode[%d]",
					gdata->dev_info->ssid, gdata->dev_info->security_mode);
		}

		if (gdata && gdata->dev_info->security_mode == item_sec_mode &&
			!g_strcmp0(gdata->dev_info->ssid, item_ssid)) {
			if (gdata->connection_mode != state) {
				gdata->connection_mode = state;
				INFO_LOG(SP_NAME_NORMAL, "State transition from [%d] --> [%d]", view_main_state_get(), state);
				view_main_state_set(state);
				if(it != NULL)
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
	wifi_device_info_t *wifi_device = g_try_new0(wifi_device_info_t, 1);
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
	} else if (WIFI_ERROR_NONE != wifi_ap_get_security_type(ap, &sec_type)) {
		g_free(wifi_device->ssid);
		g_free(wifi_device);
		return NULL;
	} else if (WIFI_ERROR_NONE != wifi_ap_is_wps_supported(ap, &(wifi_device->wps_mode))) {
		g_free(wifi_device->ssid);
		g_free(wifi_device);
		return NULL;
	}

	wifi_device->security_mode = common_utils_get_sec_mode(sec_type);
	wifi_device->ap_status_txt = common_utils_get_ap_security_type_info_txt(PACKAGE,
		wifi_device, true);
	common_utils_get_device_icon(wifi_device, &wifi_device->ap_image_path);

	return wifi_device;
}

static bool view_main_wifi_insert_found_ap(wifi_device_info_t *wifi_device)
{
	devpkr_gl_data_t *gdata = g_try_new0(devpkr_gl_data_t, 1);
	wifi_connection_state_e state;

	assertm_if(NULL == list, "list is NULL");

	if (gdata == NULL)
		return false;

	gdata->dev_info = wifi_device;
	if (gdata->dev_info == NULL) {
		g_free(gdata);
		return true;
	}

	wifi_ap_get_connection_state(wifi_device->ap, &state);

	if (WIFI_CONNECTION_STATE_ASSOCIATION == state ||
			WIFI_CONNECTION_STATE_CONFIGURATION == state) {
		gdata->connection_mode = ITEM_CONNECTION_MODE_CONNECTING;
		gdata->it = elm_genlist_item_append(list, &itc, gdata,
				NULL, ELM_GENLIST_ITEM_NONE, __gl_sel,
				NULL);
		view_main_state_set(ITEM_CONNECTION_MODE_CONNECTING);

		return true;
	}

	gdata->connection_mode = ITEM_CONNECTION_MODE_OFF;

	gdata->it = elm_genlist_item_append(list, &itc, gdata, NULL,
			ELM_GENLIST_ITEM_NONE, __gl_sel, NULL);

	return true;
}

static gint compare(gconstpointer a, gconstpointer b)
{
	bool favorite1 = false, favorite2 = false;
	wifi_connection_state_e state1 = 0, state2 = 0;

	wifi_device_info_t *wifi_device1 = (wifi_device_info_t*)a;
	wifi_device_info_t *wifi_device2 = (wifi_device_info_t*)b;

	wifi_ap_get_connection_state(wifi_device1->ap, &state1);
	wifi_ap_get_connection_state(wifi_device2->ap, &state2);

	if (state1 != state2) {
		if (state1 == WIFI_CONNECTION_STATE_CONNECTED)
			return -1;
		if (state2 == WIFI_CONNECTION_STATE_CONNECTED)
			return 1;

		if (state1 == WIFI_CONNECTION_STATE_CONFIGURATION)
			return -1;
		if (state2 == WIFI_CONNECTION_STATE_CONFIGURATION)
			return 1;

		if (state1 == WIFI_CONNECTION_STATE_ASSOCIATION)
			return -1;
		if (state2 == WIFI_CONNECTION_STATE_ASSOCIATION)
			return 1;
	}

	wifi_ap_is_favorite(wifi_device1->ap, &favorite1);
	wifi_ap_is_favorite(wifi_device2->ap, &favorite2);

	if (favorite1 != favorite2) {
		if (favorite1 == true)
			return -1;
		if (favorite2 == true)
			return 1;
	}
	if (devpkr_app_state->sort_type == I18N_TYPE_Signal_Strength) {
		/*Sort in descending order of signal strength*/
		return wifi_device2->rssi - wifi_device1->rssi;
	}
	return strcasecmp((const char *)wifi_device1->ssid,
			(const char *)wifi_device2->ssid);
}

static bool view_main_wifi_found_ap_cb(wifi_ap_h ap, void* user_data)
{
	int *profile_size = (int *)user_data;
	wifi_device_info_t *wifi_device = NULL;

	wifi_device = view_main_item_device_info_create(ap);
	if (wifi_device == NULL)
		return true;

	wifi_device_list = g_list_insert_sorted(wifi_device_list, wifi_device, compare);
	(*profile_size)++;

	return true;
}

static Evas_Object *_gl_content_title_get(void *data, Evas_Object *obj, const char *part)
{
	Evas_Object *title_progressbar = NULL;

	if (FALSE == wifi_devpkr_get_scan_status())
		return NULL;

	title_progressbar = elm_progressbar_add(obj);
	elm_object_style_set(title_progressbar, "process_small");
	elm_progressbar_horizontal_set(title_progressbar, EINA_TRUE);
	elm_progressbar_pulse(title_progressbar, EINA_TRUE);

	return title_progressbar;
}

static char* _gl_text_title_get(void *data, Evas_Object *obj,const char *part)
{
	if (g_strcmp0(part, "elm.text.main") == 0) {
		return (char*) g_strdup(sc(PACKAGE, I18N_TYPE_Available_networks));
	}

	return NULL;
}

static void view_main_add_group_title(void)
{
	grouptitle_itc.item_style = "groupindex";
	grouptitle_itc.func.text_get = _gl_text_title_get;
	grouptitle_itc.func.content_get = _gl_content_title_get;

	grouptitle = elm_genlist_item_append(list,
			&grouptitle_itc,
			NULL,
			NULL,
			ELM_GENLIST_ITEM_NONE,
			NULL,
			NULL);
	assertm_if(NULL == grouptitle, "NULL!!");

	elm_genlist_item_select_mode_set(grouptitle,
			ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);
}

void view_main_update_group_title(gboolean is_bg_scan)
{
	Evas_Object *box = NULL;
	Evas_Object *main_list = NULL;

	if (list != NULL) {
		if (!is_bg_scan) {
			Elm_Object_Item *it = elm_genlist_first_item_get(list);

			while (it) {
				elm_object_item_disabled_set(it, EINA_TRUE);
				it = elm_genlist_item_next_get(it);
			}
		}

		if(grouptitle != NULL)
			elm_genlist_item_update(grouptitle);
	} else {
		box = elm_object_content_get(devpkr_app_state->popup);

		main_list = _create_genlist(box);
		view_main_add_group_title();
		elm_box_pack_start(box, main_list);

		evas_object_show(main_list);
		evas_object_show(box);

		wifi_devpkr_redraw();

		evas_object_show(devpkr_app_state->popup);
	}

	return;
}

static void view_main_create_empty_layout(void)
{
	__COMMON_FUNC_ENTER__;

	Evas_Object *box = NULL;
	Evas_Object *layout = NULL;
	Evas_Object *prev_box = NULL;

	prev_box = elm_object_content_get(devpkr_app_state->popup);
	if (prev_box != NULL) {
		evas_object_del(prev_box);
		list = NULL;
		grouptitle = NULL;
	}

	box = elm_box_add(devpkr_app_state->popup);
	evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);

	layout = elm_layout_add(devpkr_app_state->popup);
	elm_layout_file_set(layout, WIFI_DEVPKR_EDJ, WIFI_SYSPOPUP_EMPTY_GRP);
	evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(layout, EVAS_HINT_FILL, EVAS_HINT_FILL);

	elm_object_domain_translatable_part_text_set(layout, "text", PACKAGE,
		sc(PACKAGE, I18N_TYPE_No_Wi_Fi_AP_Found));

	elm_box_pack_end(box, layout);
	evas_object_show(layout);
	evas_object_show(box);
	elm_object_content_set(devpkr_app_state->popup, box);

	__COMMON_FUNC_EXIT__;
}

void view_main_create_main_list(void)
{
	__COMMON_FUNC_ENTER__;

	Evas_Object *box = NULL;
	Evas_Object *main_list = NULL;
	Evas_Object *prev_box = NULL;

	prev_box = elm_object_content_get(devpkr_app_state->popup);
	if (prev_box != NULL) {
		evas_object_del(prev_box);
		list = NULL;
		grouptitle = NULL;
	}

	box = elm_box_add(devpkr_app_state->popup);
	evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);

	main_list = _create_genlist(box);
	view_main_add_group_title();

	elm_box_pack_end(box, main_list);
	evas_object_show(main_list);
	evas_object_show(box);
	elm_object_content_set(devpkr_app_state->popup, box);

	__COMMON_FUNC_EXIT__;
}

void view_main_refresh_ap_info(Elm_Object_Item *item)
{
	if (!item) {
		return;
	}

	devpkr_gl_data_t *gdata = elm_object_item_data_get(item);
	if (!gdata) {
		return;
	}
	wifi_device_info_t *wifi_device = gdata->dev_info;
	if (!wifi_device) {
		return;
	}

	wifi_ap_refresh(wifi_device->ap);
}

static gboolean __view_main_scroll_to_top(void *data)
{
	if (data)
		elm_genlist_item_bring_in((Elm_Object_Item *)data,
				ELM_GENLIST_ITEM_SCROLLTO_TOP);

	return FALSE;
}

Elm_Object_Item *view_main_move_item_to_top(Elm_Object_Item *old_item)
{
	__COMMON_FUNC_ENTER__;

	Elm_Object_Item *title_item = elm_genlist_first_item_get(list);
	Elm_Object_Item *first_item = elm_genlist_item_next_get(title_item);
	devpkr_gl_data_t *old_it_gdata = NULL, *first_it_gdata = NULL;

	if (!old_item || !first_item) {
		__COMMON_FUNC_EXIT__;
		return NULL;
	}

	old_it_gdata = elm_object_item_data_get(old_item);
	if (!old_it_gdata || !old_it_gdata->dev_info) {
		__COMMON_FUNC_EXIT__;
		return NULL;
	}

	if (old_item != first_item) {
		first_it_gdata = elm_object_item_data_get(first_item);
		elm_object_item_data_set(first_item, old_it_gdata);
		elm_object_item_data_set(old_item, first_it_gdata);

		elm_genlist_item_update(first_item);
		elm_genlist_item_update(old_item);
	}

	common_util_managed_idle_add(__view_main_scroll_to_top, first_item);

	__COMMON_FUNC_EXIT__;
	return first_item;
}

gboolean view_main_show(void *data)
{
	__COMMON_FUNC_ENTER__;

	int i;
	wifi_device_info_t *wifi_device = NULL;
	GList* list_of_device = NULL;

	int state = wlan_manager_state_get();
	if (WLAN_MANAGER_ERROR == state || WLAN_MANAGER_OFF == state) {
		INFO_LOG(SP_NAME_NORMAL, "Wi-Fi state is OFF");
		view_main_create_empty_layout();
		goto exit;
	} else if (WLAN_MANAGER_CONNECTED == state) {
		__COMMON_FUNC_EXIT__;
		return FALSE;
	}

	wifi_devpkr_enable_scan_btn();

	/* If previous profile list exists then just clear the genlist */
	if (profiles_list_size) {
		view_main_scan_ui_clear();
		view_main_add_group_title();
	} else {
		view_main_create_main_list();
	}

	view_main_state_set(ITEM_CONNECTION_MODE_OFF);

	profiles_list_size = 0;

	devpkr_app_state->sort_type = I18N_TYPE_Alphabetical;
	if (common_util_get_system_registry(VCONF_SORT_BY) == 1)
		devpkr_app_state->sort_type = I18N_TYPE_Signal_Strength;

	wifi_foreach_found_aps(view_main_wifi_found_ap_cb, &profiles_list_size);
	INFO_LOG(SP_NAME_NORMAL, "profiles list count [%d]\n", profiles_list_size);

	list_of_device = wifi_device_list;
	for (i = 0; i < profiles_list_size && list_of_device != NULL; i++) {
		wifi_device = (wifi_device_info_t*)list_of_device->data;

		view_main_wifi_insert_found_ap(wifi_device);

		list_of_device = list_of_device->next;
	}

	if (wifi_device_list != NULL) {
		g_list_free(wifi_device_list);
		wifi_device_list = NULL;
	}

	if (profiles_list_size <= 0)
		view_main_create_empty_layout();
	else
		evas_object_show(list);

exit:
	wifi_devpkr_redraw();

	if (devpkr_app_state->passpopup == NULL &&
			devpkr_app_state->eap_popup == NULL)
		evas_object_show(devpkr_app_state->popup);
	evas_object_show(devpkr_app_state->win_main);

	__COMMON_FUNC_EXIT__;
	return FALSE;
}
