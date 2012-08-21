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
#include "view_ime_hidden.h"
#include "i18nmanager.h"
#include "wlan_manager.h"
#include "viewer_manager.h"
#include "common_utils.h"

struct hiddep_ap_popup_data {
	Evas_Object *win;
	const char *str_pkg_name;
	char *ssid;
	Evas_Object *popup_conformant;
	Evas_Object *popup_entry_lyt;
	Evas_Object *progress_popup;
};

static void view_hidden_ap_popup_ok_cb(void *data, Evas_Object *obj, void *event_info);
static void view_hidden_ap_popup_cancel_cb(void *data, Evas_Object *obj, void *event_info);
static Eina_Bool _enable_scan_updates_cb(void *data);

hiddep_ap_popup_data_t *view_hidden_ap_popup_create(Evas_Object *win_main, const char *str_pkg_name)
{
	__COMMON_FUNC_ENTER__;

	if (!win_main) {
		INFO_LOG(UG_NAME_ERR, "Invalid argument passed.");
		return NULL;
	}

	/* Lets disable the scan updates so that the UI is not refreshed un necessarily */
	wlan_manager_disable_scan_result_update();

	hiddep_ap_popup_data_t *hidden_ap_popup_data = (hiddep_ap_popup_data_t *)g_malloc0(sizeof(hiddep_ap_popup_data_t));
	popup_btn_info_t	popup_btn_data;
	common_utils_entry_info_t entry_info;
	Evas_Object *conformant = NULL;
	conformant = elm_conformant_add(win_main);
	elm_win_conformant_set(win_main, EINA_TRUE);
	elm_win_resize_object_add(win_main, conformant);
	evas_object_size_hint_weight_set(conformant, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(conformant, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(conformant);

	Evas_Object *content = elm_layout_add(conformant);
	elm_object_content_set(conformant, content);

	memset(&popup_btn_data, 0, sizeof(popup_btn_data));
	popup_btn_data.title_txt = sc(str_pkg_name,I18N_TYPE_Find_Hidden_Network);
	popup_btn_data.btn1_cb = view_hidden_ap_popup_ok_cb;
	popup_btn_data.btn1_data = hidden_ap_popup_data;
	popup_btn_data.btn2_cb = view_hidden_ap_popup_cancel_cb;
	popup_btn_data.btn2_data = hidden_ap_popup_data;
	popup_btn_data.btn1_txt = sc(str_pkg_name, I18N_TYPE_Ok);
	popup_btn_data.btn2_txt = sc(str_pkg_name, I18N_TYPE_Cancel);
	Evas_Object *passpopup = common_utils_show_info_popup(content, &popup_btn_data);

	elm_object_content_set(content, passpopup);

	Evas_Object *box = elm_box_add(passpopup);
	evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(box);

	common_utils_set_entry_info(&entry_info, sc(str_pkg_name,I18N_TYPE_Enter_Ssid) , "", sc(str_pkg_name, I18N_TYPE_Ssid), ELM_INPUT_PANEL_LAYOUT_URL);
	Evas_Object *ly_editfield = common_utils_add_edit_box(box, &entry_info);
	evas_object_show(ly_editfield);

	elm_box_pack_end(box, ly_editfield);

	elm_object_content_set(passpopup, box);
	evas_object_show(passpopup);
	hidden_ap_popup_data->win = win_main;
	hidden_ap_popup_data->str_pkg_name = str_pkg_name;
	hidden_ap_popup_data->popup_conformant = conformant;
	hidden_ap_popup_data->popup_entry_lyt = ly_editfield;
	Evas_Object *popup_entry = common_utils_entry_layout_get_entry(ly_editfield);
	elm_object_focus_set(popup_entry, EINA_TRUE);

	__COMMON_FUNC_EXIT__;

	return hidden_ap_popup_data;
}

void view_hidden_ap_popup_destroy(hiddep_ap_popup_data_t *popup_data)
{
	if (!popup_data) {
		INFO_LOG(UG_NAME_ERR, "Invalid argument passed.");
		return;
	}

	evas_object_del(popup_data->popup_conformant);
	popup_data->popup_conformant = NULL;

	evas_object_del(popup_data->progress_popup);
	popup_data->progress_popup = NULL;

	g_free(popup_data->ssid);
	g_free(popup_data);

	/* A delay is needed to get the smooth Input panel closing animation effect */
	ecore_timer_add(0.1, _enable_scan_updates_cb, NULL);

	return;
}

const char *view_ime_hidden_popup_get_ssid(hiddep_ap_popup_data_t *popup_data)
{
	if (!popup_data) {
		INFO_LOG(UG_NAME_ERR, "Invalid argument passed.");
		return NULL;
	}

	return popup_data->ssid;
}

static void view_hidden_ap_popup_ok_cb(void *data, Evas_Object *obj, void *event_info)
{
	hiddep_ap_popup_data_t *popup_data = (hiddep_ap_popup_data_t *)data;
	char *entry_txt = common_utils_entry_layout_get_text(popup_data->popup_entry_lyt);
	if (WLAN_MANAGER_ERR_NONE != connman_request_specific_scan(entry_txt)) {
		char *disp_msg = g_strdup_printf("Unable to find %s", entry_txt);
		common_utils_show_info_ok_popup(popup_data->win, popup_data->str_pkg_name, disp_msg);
		g_free(disp_msg);
		g_free(entry_txt);
		view_hidden_ap_popup_destroy(popup_data);
	} else {
		popup_data->ssid = entry_txt;

		/* Show progress idication popup */
		popup_data->progress_popup = common_utils_show_info_ok_popup(popup_data->win, popup_data->str_pkg_name, "Please Wait...");

		evas_object_del(popup_data->popup_conformant);
		popup_data->popup_conformant = NULL;

		/* A delay is needed to get the smooth Input panel closing animation effect */
		ecore_timer_add(0.1, _enable_scan_updates_cb, NULL);
	}

	return;
}

static void view_hidden_ap_popup_cancel_cb(void *data, Evas_Object *obj, void *event_info)
{
	hiddep_ap_popup_data_t *hidden_ap_popup_data = (hiddep_ap_popup_data_t *)data;
	view_hidden_ap_popup_destroy(hidden_ap_popup_data);
	return;
}

static Eina_Bool _enable_scan_updates_cb(void *data)
{
	/* Lets enable the scan updates */
	wlan_manager_enable_scan_result_update();

	return ECORE_CALLBACK_CANCEL;
}
