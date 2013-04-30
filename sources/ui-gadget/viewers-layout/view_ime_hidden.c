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

#include "ug_wifi.h"
#include "view_ime_hidden.h"
#include "i18nmanager.h"
#include "wlan_manager.h"
#include "viewer_manager.h"
#include "common_utils.h"

struct hiddep_ap_popup_data {
	Evas_Object *win;
	const char *str_pkg_name;
	Evas_Object *popup;
	Evas_Object *popup_entry_lyt;
	Evas_Object *progress_popup;
};

static void view_hidden_ap_popup_ok_cb(void *data, Evas_Object *obj, void *event_info);
static void view_hidden_ap_popup_cancel_cb(void *data, Evas_Object *obj, void *event_info);
static Eina_Bool _enable_scan_updates_cb(void *data);

static void __popup_entry_changed_cb(void* data, Evas_Object* obj, void* event_info)
{
	if (elm_object_focus_get(data)) {
		if (elm_entry_is_empty(obj))
			elm_object_signal_emit(data, "elm,state,eraser,hide", "elm");
		else
			elm_object_signal_emit(data, "elm,state,eraser,show", "elm");
	}
}

static void __popup_entry_focused_cb(void *data, Evas_Object *obj, void *event_info)
{
	if (!elm_entry_is_empty(obj))
		elm_object_signal_emit(data, "elm,state,eraser,show", "elm");
	elm_object_signal_emit(data, "elm,state,guidetext,hide", "elm");
}

static void __popup_entry_unfocused_cb(void *data, Evas_Object *obj, void *event_info)
{
	if (elm_entry_is_empty(obj))
		elm_object_signal_emit(data, "elm,state,guidetext,show", "elm");
	elm_object_signal_emit(data, "elm,state,eraser,hide", "elm");
}

static void __popup_eraser_clicked_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	elm_entry_entry_set(data, "");
}

hiddep_ap_popup_data_t *view_hidden_ap_popup_create(Evas_Object *win_main, const char *str_pkg_name)
{
	__COMMON_FUNC_ENTER__;

	if (!win_main) {
		INFO_LOG(UG_NAME_ERR, "Invalid argument passed.");
		return NULL;
	}

	/* Lets disable the scan updates so that the UI is not refreshed un necessarily */
	wlan_manager_disable_scan_result_update();

	hiddep_ap_popup_data_t *hidden_ap_popup_data = g_new0(hiddep_ap_popup_data_t, 1);
	popup_btn_info_t	popup_btn_data;

	memset(&popup_btn_data, 0, sizeof(popup_btn_data));
	popup_btn_data.title_txt = sc(str_pkg_name,I18N_TYPE_Find_Hidden_Network);
	popup_btn_data.btn1_cb = view_hidden_ap_popup_ok_cb;
	popup_btn_data.btn1_data = hidden_ap_popup_data;
	popup_btn_data.btn2_cb = view_hidden_ap_popup_cancel_cb;
	popup_btn_data.btn2_data = hidden_ap_popup_data;
	popup_btn_data.btn1_txt = sc(str_pkg_name, I18N_TYPE_Ok);
	popup_btn_data.btn2_txt = sc(str_pkg_name, I18N_TYPE_Cancel);
	Evas_Object *passpopup = common_utils_show_info_popup(win_main, &popup_btn_data);

	Evas_Object *box = elm_box_add(passpopup);
	evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(box);

	Evas_Object *entry_ly = elm_layout_add(box);
	Evas_Object *entry = NULL;
	Elm_Entry_Filter_Limit_Size limit_filter_data;

	elm_layout_file_set(entry_ly, CUSTOM_EDITFIELD_PATH, "custom_editfield");
	evas_object_size_hint_weight_set(entry_ly, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(entry_ly, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_object_part_text_set(entry_ly, "elm.text", sc(str_pkg_name, I18N_TYPE_Enter_Ssid));
	elm_object_part_text_set(entry_ly, "elm.guidetext", sc(str_pkg_name, I18N_TYPE_Ssid));

	entry = elm_entry_add(entry_ly);
	elm_entry_scrollable_set(entry, EINA_TRUE);
	elm_entry_single_line_set(entry, EINA_TRUE);
	elm_entry_input_panel_layout_set(entry, ELM_INPUT_PANEL_LAYOUT_URL);
	elm_object_part_content_set(entry_ly, "elm.swallow.content", entry);

	limit_filter_data.max_char_count = 32;
	elm_entry_markup_filter_append(entry, elm_entry_filter_limit_size, &limit_filter_data);

	evas_object_smart_callback_add(entry, "changed", __popup_entry_changed_cb, entry_ly);
	evas_object_smart_callback_add(entry, "focused", __popup_entry_focused_cb, entry_ly);
	evas_object_smart_callback_add(entry, "unfocused", __popup_entry_unfocused_cb, entry_ly);
	elm_object_signal_callback_add(entry_ly, "elm,eraser,clicked", "elm", __popup_eraser_clicked_cb, entry);
	evas_object_show(entry);
	evas_object_show(entry_ly);

	elm_box_pack_end(box, entry_ly);

	elm_object_content_set(passpopup, box);
	evas_object_show(passpopup);
	hidden_ap_popup_data->win = win_main;
	hidden_ap_popup_data->str_pkg_name = str_pkg_name;
	hidden_ap_popup_data->popup = passpopup;
	hidden_ap_popup_data->popup_entry_lyt = entry_ly;
	elm_object_focus_set(entry, EINA_TRUE);
	__COMMON_FUNC_EXIT__;

	return hidden_ap_popup_data;
}

void view_hidden_ap_popup_destroy(hiddep_ap_popup_data_t *popup_data)
{
	if (!popup_data) {
		INFO_LOG(UG_NAME_ERR, "Invalid argument passed.");
		return;
	}

	if (popup_data->popup) {
		evas_object_del(popup_data->popup);
		popup_data->popup = NULL;
	}

	if (popup_data->progress_popup) {
		evas_object_del(popup_data->progress_popup);
		popup_data->progress_popup = NULL;
	}

	g_free(popup_data);

	/* A delay is needed to get the smooth Input panel closing animation effect */
	ecore_timer_add(0.1, _enable_scan_updates_cb, NULL);
}

static void view_hidden_ap_popup_ok_cb(
		void *data, Evas_Object *obj, void *event_info)
{
	hiddep_ap_popup_data_t *popup_data = (hiddep_ap_popup_data_t *)data;

	char *entry_txt = common_utils_entry_layout_get_text(
									popup_data->popup_entry_lyt);

	if (WLAN_MANAGER_ERR_NONE != wlan_manager_scan_with_ssid(entry_txt, entry_txt)) {
		char *disp_msg = g_strdup_printf("%s : %s", sc(popup_data->str_pkg_name,I18N_TYPE_Find_Hidden_Network), entry_txt);

		common_utils_show_info_ok_popup(popup_data->win, popup_data->str_pkg_name, disp_msg);

		g_free(disp_msg);
		g_free(entry_txt);

		view_hidden_ap_popup_destroy(popup_data);
	} else {
		/* Show progress indication popup */
		popup_data->progress_popup = common_utils_show_info_ok_popup(popup_data->win, popup_data->str_pkg_name, sc(popup_data->str_pkg_name,I18N_TYPE_Wait));

		evas_object_del(popup_data->popup);
		popup_data->popup = NULL;

		/* A delay is needed to get the smooth Input panel closing animation effect */
		ecore_timer_add(0.1, _enable_scan_updates_cb, NULL);
	}
}

static void view_hidden_ap_popup_cancel_cb(void *data, Evas_Object *obj, void *event_info)
{
	hiddep_ap_popup_data_t *hidden_ap_popup_data;

	hidden_ap_popup_data = (hiddep_ap_popup_data_t *)data;
	view_hidden_ap_popup_destroy(hidden_ap_popup_data);
}

static Eina_Bool _enable_scan_updates_cb(void *data)
{
	/* Lets enable the scan updates */
	wlan_manager_enable_scan_result_update();

	return ECORE_CALLBACK_CANCEL;
}
