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

#include <feedback.h>
#include <efl_assist.h>

#include "ug_wifi.h"
#include "view_ime_hidden.h"
#include "i18nmanager.h"
#include "wlan_manager.h"
#include "viewer_manager.h"
#include "viewer_list.h"
#include "common_utils.h"

struct hiddep_ap_popup_data {
	Evas_Object *win;
	const char *str_pkg_name;
	Evas_Object *popup;
	Evas_Object *entry;
	Evas_Object *ok_btn;
};

static hiddep_ap_popup_data_t *g_hidden_ap_popup_data = NULL;
static Elm_Genlist_Item_Class g_entry_itc;

static Eina_Bool _enable_scan_updates_cb(void *data)
{
	/* Lets enable the scan updates */
	wlan_manager_enable_scan_result_update();

	/* Reset the ecore timer handle */
	common_util_manager_ecore_scan_update_timer_reset();

	return ECORE_CALLBACK_CANCEL;
}

static void __popup_entry_changed_cb(void* data, Evas_Object* obj, void* event_info)
{
	if (g_hidden_ap_popup_data == NULL) {
		return;
	}

	hiddep_ap_popup_data_t *popup_data = g_hidden_ap_popup_data;
	Evas_Object *ok_btn = popup_data->ok_btn;

	if (elm_object_part_content_get(obj, "elm.swallow.clear")) {
		if (elm_object_focus_get(obj)) {
			if (elm_entry_is_empty(obj)) {
				elm_object_signal_emit(obj, "elm,state,clear,hidden", "");
				elm_object_disabled_set(ok_btn, EINA_TRUE);
				elm_entry_input_panel_return_key_disabled_set(obj, EINA_TRUE);
			} else {
				elm_object_signal_emit(obj, "elm,state,clear,visible", "");
				elm_object_disabled_set(ok_btn, EINA_FALSE);
				elm_entry_input_panel_return_key_disabled_set(obj, EINA_FALSE);
			}
		}
	}
}

static void __popup_entry_focused_cb(void *data, Evas_Object *obj, void *event_info)
{
	if (elm_object_part_content_get(obj, "elm.swallow.clear")) {
		if (!elm_entry_is_empty(obj)) {
			elm_object_signal_emit(obj, "elm,state,clear,visible", "");
		} else {
			elm_object_signal_emit(obj, "elm,state,clear,hidden", "");
		}
	}
	elm_object_signal_emit(obj, "elm,state,focus,on", "");
}

static void __popup_entry_unfocused_cb(void *data, Evas_Object *obj, void *event_info)
{
	if (elm_object_part_content_get(obj, "elm.swallow.clear")) {
		elm_object_signal_emit(obj, "elm,state,clear,hidden", "");
	}
	elm_object_signal_emit(obj, "elm,state,focus,off", "");
}

static void __popup_entry_maxlength_reached(void *data, Evas_Object *obj,
		void *event_info)
{
		common_utils_send_message_to_net_popup("Password length",
				"Lengthy Password", "notification", NULL);
}

hiddep_ap_popup_data_t *view_hidden_ap_popup_data_get(void)
{
	return g_hidden_ap_popup_data;
}

void view_hidden_ap_popup_delete(void)
{
	if (g_hidden_ap_popup_data == NULL)
		return;

	if (g_hidden_ap_popup_data->popup) {
		evas_object_del(g_hidden_ap_popup_data->popup);
		g_hidden_ap_popup_data->popup = NULL;
	}

	g_free(g_hidden_ap_popup_data);
	g_hidden_ap_popup_data = NULL;
}

void view_hidden_ap_popup_destroy(void)
{
	view_hidden_ap_popup_delete();

	/* A delay is needed to get the smooth Input panel closing animation effect */
	common_util_managed_ecore_scan_update_timer_add(0.1,
			_enable_scan_updates_cb, NULL);
}

static void view_hidden_ap_popup_ok_cb(void *data, Evas_Object *obj,
		void *event_info)
{
	hiddep_ap_popup_data_t *popup_data = g_hidden_ap_popup_data;
	char *entry_txt = NULL;
	int err;

	if (popup_data == NULL)
		return;

	entry_txt = elm_entry_markup_to_utf8(elm_entry_entry_get(
			popup_data->entry));

	err = wlan_manager_scan_with_ssid(entry_txt, entry_txt);
	if (err != WLAN_MANAGER_ERR_NONE) {
		common_utils_send_message_to_net_popup("Network connection popup",
				"no ap found", "notification", NULL);

		view_hidden_ap_popup_destroy();
		g_free(entry_txt);

		return;
	}

	viewer_manager_show(VIEWER_WINSET_SEARCHING);
	viewer_manager_header_mode_set(HEADER_MODE_SEARCHING);

	evas_object_del(popup_data->popup);
	popup_data->popup = NULL;

	/* A delay is needed to get the smooth Input panel closing animation effect */
	common_util_managed_ecore_scan_update_timer_add(0.1,
			_enable_scan_updates_cb, NULL);
}

static void view_hidden_ap_popup_cancel_cb(void *data, Evas_Object *obj, void *event_info)
{
	viewer_manager_request_scan();
	view_hidden_ap_popup_destroy();
}

static Evas_Object *_gl_entry_item_content_get(void *data,
		Evas_Object *obj, const char *part)
{
	if (obj == NULL) {
		return NULL;
	}

	int return_key_type;
	Evas_Object *entry = NULL;

	static Elm_Entry_Filter_Limit_Size limit_filter_data;

	if (!g_strcmp0(part, "elm.icon.entry")) {
		entry = ea_editfield_add(obj, EA_EDITFIELD_SCROLL_SINGLELINE);
		evas_object_size_hint_weight_set(entry, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		evas_object_size_hint_align_set(entry, EVAS_HINT_FILL, EVAS_HINT_FILL);
		if (!entry)
			return NULL;

		elm_entry_cnp_mode_set(entry, ELM_CNP_MODE_PLAINTEXT);
		elm_entry_input_panel_layout_set(entry, ELM_INPUT_PANEL_LAYOUT_NORMAL);
		elm_entry_prediction_allow_set(entry, EINA_FALSE);
		elm_entry_autocapital_type_set(entry, ELM_AUTOCAPITAL_TYPE_NONE);
		elm_object_domain_translatable_part_text_set(entry, "elm.guide",
				PACKAGE, "IDS_ST_BODY_NETWORK_SSID");
		elm_object_signal_emit(entry, "elm,action,hide,search_icon", "");

		limit_filter_data.max_char_count = 32;
		elm_entry_markup_filter_append(entry, elm_entry_filter_limit_size,
				&limit_filter_data);

		return_key_type = ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_DONE;
		elm_entry_input_panel_return_key_type_set(entry, return_key_type);
		elm_entry_input_panel_return_key_disabled_set(entry, EINA_TRUE);

		g_hidden_ap_popup_data->entry = entry;

		evas_object_smart_callback_add(entry, "activated",
				view_hidden_ap_popup_ok_cb, NULL);
		evas_object_smart_callback_add(entry, "changed",
				__popup_entry_changed_cb, NULL);
		evas_object_smart_callback_add(entry, "preedit,changed",
				__popup_entry_changed_cb, NULL);
		evas_object_smart_callback_add(entry, "focused",
				__popup_entry_focused_cb, NULL);
		evas_object_smart_callback_add(entry, "unfocused",
				__popup_entry_unfocused_cb, NULL);
		evas_object_smart_callback_add(entry, "maxlength,reached",
				__popup_entry_maxlength_reached, NULL);

		elm_entry_input_panel_show_on_demand_set(entry, EINA_TRUE);

		return entry;
	}

	return NULL;
}

static void popup_animation_finish_cb(void *data, Evas_Object *obj, void *event_info)
{
	__COMMON_FUNC_ENTER__;
	Elm_Object_Item *item = (Elm_Object_Item *) data;
	Evas_Object *entry = NULL;

	entry = elm_object_item_part_content_get(item, "elm.icon.entry");
	elm_entry_input_panel_show(entry);
	elm_entry_input_panel_show_on_demand_set(entry, EINA_FALSE);
	elm_object_focus_set(entry, EINA_TRUE);

	__COMMON_FUNC_EXIT__;
}

void view_hidden_ap_popup_create(Evas_Object *win_main, const char *str_pkg_name)
{
	__COMMON_FUNC_ENTER__;

	Evas_Object *passpopup = NULL;
	Evas_Object *genlist = NULL;
	popup_btn_info_t popup_data;

	if (win_main == NULL) {
		INFO_LOG(UG_NAME_ERR, "Invalid argument passed.");
		return;
	}

	if (g_hidden_ap_popup_data != NULL) {
		if (g_hidden_ap_popup_data->popup != NULL) {
			evas_object_del(g_hidden_ap_popup_data->popup);
			g_hidden_ap_popup_data->popup = NULL;
		}
		g_free(g_hidden_ap_popup_data);
	}

	g_hidden_ap_popup_data = g_try_new0(hiddep_ap_popup_data_t, 1);
	if (g_hidden_ap_popup_data == NULL) {
		INFO_LOG(UG_NAME_ERR, "Memory allocation error.");
		return;
	}

	/* Lets disable the scan updates so that the UI is not refreshed un necessarily */
	wlan_manager_disable_scan_result_update();

	memset(&popup_data, 0, sizeof(popup_data));
	popup_data.title_txt = "IDS_WIFI_BUTTON_FIND_HIDDEN_NETWORK";
	popup_data.btn1_cb = view_hidden_ap_popup_cancel_cb;
	popup_data.btn1_data = NULL;
	popup_data.btn1_txt = "IDS_WIFI_SK_CANCEL";
	popup_data.btn2_cb = view_hidden_ap_popup_ok_cb;
	popup_data.btn2_data = NULL;
	popup_data.btn2_txt = "IDS_COM_BODY_FIND";

	passpopup = common_utils_show_info_popup(win_main, &popup_data);
	g_hidden_ap_popup_data->ok_btn = popup_data.btn;
	elm_object_disabled_set(g_hidden_ap_popup_data->ok_btn, EINA_TRUE);

	g_hidden_ap_popup_data->win = win_main;
	g_hidden_ap_popup_data->str_pkg_name = str_pkg_name;
	g_hidden_ap_popup_data->popup = passpopup;

	genlist = elm_genlist_add(passpopup);
	evas_object_size_hint_weight_set(genlist, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(genlist, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_genlist_realization_mode_set(genlist, EINA_TRUE);
	elm_scroller_content_min_limit(genlist, EINA_FALSE, EINA_TRUE);

	g_entry_itc.item_style = "entry";
	g_entry_itc.func.text_get = NULL;
	g_entry_itc.func.content_get = _gl_entry_item_content_get;
	g_entry_itc.func.state_get = NULL;
	g_entry_itc.func.del = NULL;

	Elm_Object_Item * entry_item = elm_genlist_item_append(genlist,
			&g_entry_itc, NULL, NULL, ELM_GENLIST_ITEM_NONE,
			NULL, NULL);

	evas_object_smart_callback_add(passpopup, "show,finished",
			popup_animation_finish_cb, entry_item);

	evas_object_show(genlist);
	elm_object_content_set(passpopup, genlist);
	evas_object_show(passpopup);

	__COMMON_FUNC_EXIT__;
}
