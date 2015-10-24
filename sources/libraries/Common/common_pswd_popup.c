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
#include <efl_extension.h>

#include "common.h"
#include "common_pswd_popup.h"
#include "common_generate_pin.h"
#include "i18nmanager.h"
#include "ug_wifi.h"

#define EDJ_GRP_POPUP_PBC_BUTTON_LAYOUT "popup_pbc_button_layout"
#define EDJ_GRP_POPUP_WPS_PIN_LAYOUT "popup_wps_pin_layout"

#define MAX_PBC_TIMEOUT_SECS	120	// Time in seconds
#define PASSWORD_LENGTH		64

static Elm_Genlist_Item_Class g_wps_itc;
static Elm_Genlist_Item_Class g_check_box_itc;
static Elm_Genlist_Item_Class g_pswd_entry_itc;
static gboolean wps_options_click = FALSE;
static gboolean keypad_state = FALSE;

static void __popup_entry_changed_cb(void* data, Evas_Object* obj, void* event_info)
{
	Evas_Object *btn_ok = NULL;
	pswd_popup_t *pswd_popup_data = NULL;
	char* entry_str = NULL;
	unsigned short int passwd_length = 0;

	if (!data) {
		return;
	}

	pswd_popup_data = (pswd_popup_t *) data;
	entry_str = passwd_popup_get_txt(pswd_popup_data);
	if (entry_str != NULL)
		passwd_length = strlen(entry_str);

	btn_ok = elm_object_part_content_get(pswd_popup_data->popup, "button2");

	if (pswd_popup_data->sec_type == WIFI_SECURITY_TYPE_WEP) {
		if (passwd_length > 0) {
			elm_object_disabled_set(btn_ok, EINA_FALSE);
			elm_entry_input_panel_return_key_disabled_set(obj,
					EINA_FALSE);
		} else {
			elm_object_disabled_set(btn_ok, EINA_TRUE);
			elm_entry_input_panel_return_key_disabled_set(obj,
					EINA_TRUE);
		}
	} else if (pswd_popup_data->sec_type == WIFI_SECURITY_TYPE_WPA2_PSK ||
			pswd_popup_data->sec_type == WIFI_SECURITY_TYPE_WPA_PSK) {
		if (passwd_length > 7) {
			elm_object_disabled_set(btn_ok, EINA_FALSE);
			elm_entry_input_panel_return_key_disabled_set(obj,
					EINA_FALSE);
		} else {
			elm_object_disabled_set(btn_ok, EINA_TRUE);
			elm_entry_input_panel_return_key_disabled_set(obj,
					EINA_TRUE);
		}
	}

	if (elm_object_part_content_get(obj, "elm.swallow.clear")) {
		if (elm_object_focus_get(obj)) {
			if (elm_entry_is_empty(obj)) {
				elm_object_signal_emit(obj,
						"elm,state,clear,hidden", "");
			} else {
				elm_object_signal_emit(obj,
						"elm,state,clear,visible", "");
			}
		}
	}

	if (entry_str)
		g_free(entry_str);
}

static void __popup_entry_maxlength_reached(void *data, Evas_Object *obj,
		void *event_info)
{
	common_utils_send_message_to_net_popup("Password length",
			"Lengthy Password", "notification", NULL);
}

static void __common_pbc_popup_destroy(pbc_popup_t *pbc_popup_data)
{
	__COMMON_FUNC_ENTER__;
	if (!pbc_popup_data) {
		return;
	}

	if (pbc_popup_data->checker == 0) {
		pbc_popup_data->checker = 1;

		if (pbc_popup_data->timer != NULL) {
			ecore_timer_del(pbc_popup_data->timer);
			pbc_popup_data->timer = NULL;
		}

		if (pbc_popup_data->pin != NULL) {
			g_free(pbc_popup_data->pin);
			pbc_popup_data->pin = NULL;
		}

		if (pbc_popup_data->popup != NULL) {
			evas_object_hide(pbc_popup_data->popup);
			evas_object_del(pbc_popup_data->popup);
			pbc_popup_data->popup = NULL;
		}
		g_free(pbc_popup_data);
	}
	__COMMON_FUNC_EXIT__;
	return;
}

static Eina_Bool _fn_pb_timer_bar(void *data)
{
	double val = 120.0;
	time_t current_time;
	int time_diff;
	time(&current_time);

	pswd_popup_t *pswd_popup_data = (pswd_popup_t *)data;
	pbc_popup_t *pbc_popup_data = pswd_popup_data->pbc_popup_data;
	if (!pbc_popup_data || pbc_popup_data->timer == NULL ||
			pbc_popup_data->progressbar == NULL) {
		return ECORE_CALLBACK_CANCEL;
	}
	time_diff = difftime(current_time, pswd_popup_data->start_time);
	pbc_popup_data->value = (double)(time_diff/(double)MAX_PBC_TIMEOUT_SECS);
	if (pbc_popup_data->value >= 1) {
		if (pbc_popup_data->checker == 0) {
			__COMMON_FUNC_ENTER__;

			Evas_Object *cancel_btn = elm_object_part_content_get(
					pbc_popup_data->popup, "button1");
			evas_object_smart_callback_call(cancel_btn, "clicked", NULL);

			__COMMON_FUNC_EXIT__;
		}
		return ECORE_CALLBACK_CANCEL;
	}
	val = val - time_diff;
	int remain_mins = (int)(val);
	int remain_secs = 0;
	remain_secs = remain_mins % 60;
	remain_mins /= 60;

	char *remaining_time_str = g_strdup_printf(
			"<font_size=40><align=center>%02d:%02d</align></font_size>",
			remain_mins, remain_secs);
	elm_object_text_set(pbc_popup_data->timer_label, remaining_time_str);
	/* INFO_LOG(UG_NAME_NORMAL, "pbc_popup_data->value = %lf;"
			"remain_mins = %d; remain_secs = %d; remaining_time_str = %s",
			pbc_popup_data->value, remain_mins, remain_secs, remaining_time_str); */
	g_free(remaining_time_str);

	elm_progressbar_value_set(pbc_popup_data->progressbar, pbc_popup_data->value);

	return ECORE_CALLBACK_RENEW;
}

static Eina_Bool _enable_scan_updates_cb(void *data)
{
	/* Lets enable the scan updates */
	wlan_manager_enable_scan_result_update();

	/* Reset the ecore timer handle */
	common_util_manager_ecore_scan_update_timer_reset();

	return ECORE_CALLBACK_CANCEL;
}

static void __pbc_popup_language_changed_cb(void *data, Evas_Object *obj, void *event_info)
{
	__COMMON_FUNC_ENTER__;

	pbc_popup_t *pbc_data = (pbc_popup_t *)data;
	Evas_Object *layout = NULL;
	char str[1024];

	retm_if(pbc_data == NULL || pbc_data->popup == NULL);
	layout = elm_object_content_get(pbc_data->popup);

	if (pbc_data->wps_type == POPUP_WPS_BTN) {
		g_snprintf(str, 1024, sc(PACKAGE,
				I18N_TYPE_Press_WPS_On_Your_Wi_Fi_Access_Point), 2);
		elm_object_domain_translatable_part_text_set(layout,
				"elm.text.description", PACKAGE, str);
	} else if (pbc_data->wps_type == POPUP_WPS_PIN) {
		g_snprintf(str, 1024, sc(PACKAGE,
				I18N_TYPE_Enter_PIN_number_on_your_WIFI_access_point),
				pbc_data->pin, 2);
		elm_object_domain_translatable_part_text_set(layout,
				"elm.text.description", PACKAGE, str);
	}
	__COMMON_FUNC_EXIT__;
}

void create_pbc_popup(pswd_popup_t *pswd_popup_data, Evas_Smart_Cb cancel_cb,
		void *cancel_cb_data, popup_type_t type, char *pin)
{
	if (!pswd_popup_data) {
		return;
	}
	Evas_Object *popup = NULL;
	Evas_Object *progressbar = NULL;
	Evas_Object *timer_label = NULL;
	Evas_Object *layout;
	char buf[512] = "";

	pbc_popup_t *pbc_popup_data = NULL;
	pbc_popup_data = g_new0(pbc_popup_t, 1);

	popup_btn_info_t popup_btn_data;
	memset(&popup_btn_data, 0, sizeof(popup_btn_data));
	popup_btn_data.btn1_txt = "IDS_WIFI_SK_CANCEL";
	popup_btn_data.btn1_cb = cancel_cb;
	popup_btn_data.btn1_data = cancel_cb_data;
	popup = common_utils_show_info_popup(pswd_popup_data->win, &popup_btn_data);

	if (type == POPUP_WPS_BTN) {
		elm_object_domain_translatable_part_text_set(popup, "title,text",
				PACKAGE, "IDS_WIFI_BUTTON_WPS_BUTTON");
	} else if (type == POPUP_WPS_PIN) {
		elm_object_domain_translatable_part_text_set(popup, "title,text",
				PACKAGE, "IDS_WIFI_SK_WPS_PIN" );
	}

	layout = elm_layout_add(popup);
	if (layout == NULL) {
		return;
	}

	if (type == POPUP_WPS_BTN) {
		elm_layout_file_set(layout, CUSTOM_EDITFIELD_PATH,
				EDJ_GRP_POPUP_PBC_BUTTON_LAYOUT);
		g_snprintf(buf, sizeof(buf), sc(pswd_popup_data->str_pkg_name,
				I18N_TYPE_Press_WPS_On_Your_Wi_Fi_Access_Point), 2);
		elm_object_domain_translatable_part_text_set(layout,
				"elm.text.description", PACKAGE, buf);
	} else if (type == POPUP_WPS_PIN) {
		elm_layout_file_set(layout, CUSTOM_EDITFIELD_PATH,
				EDJ_GRP_POPUP_WPS_PIN_LAYOUT);
		g_snprintf(buf, sizeof(buf),
				sc(pswd_popup_data->str_pkg_name,
					I18N_TYPE_Enter_PIN_number_on_your_WIFI_access_point),
					pin, 2);
		elm_object_domain_translatable_part_text_set(layout,
				"elm.text.description", PACKAGE, buf);
	}
	evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

	progressbar = elm_progressbar_add(layout);
	elm_progressbar_horizontal_set(progressbar, EINA_TRUE);
	evas_object_size_hint_align_set(progressbar, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_size_hint_weight_set(progressbar, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_progressbar_value_set(progressbar, 0.0);

	timer_label = elm_label_add(layout);
	elm_label_line_wrap_set(timer_label, ELM_WRAP_MIXED);
	elm_object_text_set(timer_label, _("<font_size=40><align=center>02:00</align></font_size>"));
	evas_object_size_hint_weight_set(timer_label, EVAS_HINT_EXPAND, 0.0);
	evas_object_size_hint_align_set(timer_label, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(timer_label);

	elm_object_part_content_set(layout, "slider", progressbar);
	elm_object_part_content_set(layout, "timer_label", timer_label);

	pbc_popup_data->checker = 0;
	pbc_popup_data->value = 0.0;
	pbc_popup_data->progressbar = progressbar;
	pbc_popup_data->timer_label = timer_label;
	pbc_popup_data->popup = popup;
	pbc_popup_data->wps_type = type;
	pbc_popup_data->pin = g_strdup(pin);
	time(&(pswd_popup_data->start_time));
	pbc_popup_data->timer = ecore_timer_add(1.0, _fn_pb_timer_bar, pswd_popup_data);
	evas_object_show(progressbar);
	evas_object_smart_callback_add(popup, "language,changed",
			__pbc_popup_language_changed_cb, pbc_popup_data);
	evas_object_show(popup);
	elm_object_content_set(popup, layout);
	pswd_popup_data->pbc_popup_data = pbc_popup_data;

	evas_object_hide(pswd_popup_data->popup);

	return;
}

static char *_gl_wps_text_get(void *data, Evas_Object *obj, const char *part)
{
	if (!strcmp("elm.text", part)) {
		char buf[1024];
		snprintf(buf, 1023, "%s", sc(PACKAGE, (int)data));
		return g_strdup(dgettext(PACKAGE, buf));
	}
	return NULL;
}

static void _gl_realized(void *data, Evas_Object *obj, void *event_info)
{
	if (!event_info)
		return;

	int index = (int)elm_object_item_data_get(event_info);

	if (index == I18N_TYPE_WPS_PIN) {
		elm_object_item_signal_emit(event_info, "elm,state,bottomline,hide", "");
	}
}

void create_wps_options_popup(Evas_Object *win_main,
		pswd_popup_t *pswd_popup_data,
		pswd_popup_create_req_data_t *popup_info)
{
	if (!win_main || !popup_info || !pswd_popup_data) {
		return;
	}

	Evas_Object *popup = NULL;
	Evas_Object *genlist = NULL;
	static Elm_Genlist_Item_Class wps_itc;
	char *txt = NULL;
	popup = elm_popup_add(win_main);
	if (!popup)
		return;

	ecore_imf_input_panel_hide();

	txt = evas_textblock_text_utf8_to_markup(NULL, popup_info->title);
	elm_object_domain_translatable_part_text_set(popup,
			"title,text", PACKAGE, txt);
	g_free(txt);
	evas_object_size_hint_weight_set(popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

	genlist = elm_genlist_add(popup);
	evas_object_size_hint_weight_set(genlist, EVAS_HINT_EXPAND,
			EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(genlist, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_scroller_content_min_limit(genlist, EINA_FALSE, EINA_TRUE);
	evas_object_smart_callback_add(genlist, "realized", _gl_realized, NULL);

	wps_itc.item_style = WIFI_GENLIST_1LINE_TEXT_STYLE;
	wps_itc.func.text_get = _gl_wps_text_get;
	wps_itc.func.content_get = NULL;
	wps_itc.func.state_get = NULL;
	wps_itc.func.del = NULL;

	elm_genlist_item_append(genlist, &wps_itc,
			(void*)I18N_TYPE_WPS_Button, NULL,
			ELM_GENLIST_ITEM_NONE, popup_info->wps_btn_cb,
			popup_info->cb_data);
	elm_genlist_item_append(genlist, &wps_itc,
			(void*)I18N_TYPE_WPS_PIN, NULL,
			ELM_GENLIST_ITEM_NONE, popup_info->wps_pin_cb,
			popup_info->cb_data);

	evas_object_show(genlist);

	eext_object_event_callback_add(popup, EEXT_CALLBACK_BACK,
			popup_info->cancel_cb, popup_info->cb_data);
	elm_object_content_set(popup, genlist);
	evas_object_show(popup);
	elm_object_focus_set(popup, EINA_TRUE);

	pswd_popup_data->wps_options_popup = popup;

	evas_object_hide(pswd_popup_data->popup);
}

static char *_passwd_popup_wps_item_text_get(void *data, Evas_Object *obj,
		const char *part)
{
	if (!strcmp("elm.text", part)) {
		char buf[1024];
		snprintf(buf, 1023, "%s", sc(PACKAGE, I18N_TYPE_WPS));
		return strdup(buf);
	}
	return NULL;
}

static Evas_Object *_passwd_popup_wps_item_content_get(void *data,
		Evas_Object *obj, const char *part)
{
	Evas_Object *icon = NULL;
	Evas_Object *ic = NULL;

	ic = elm_layout_add(obj);
	retvm_if(NULL == ic, NULL);

	if (!strcmp("elm.swallow.icon", part)) {
		elm_layout_theme_set(ic, "layout", "list/B/type.3", "default");

		/* image */
		icon = elm_image_add(ic);
		retvm_if(NULL == icon, NULL);

		elm_image_file_set(icon, CUSTOM_EDITFIELD_PATH, "wifi_icon_wps.png");
		evas_object_color_set(icon, 2, 61, 132, 153);

		evas_object_size_hint_align_set(icon, EVAS_HINT_FILL, EVAS_HINT_FILL);
		evas_object_size_hint_weight_set(icon, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		elm_object_part_content_set(ic, "elm.swallow.content", icon);
	}
	return ic;
}

static void _entry_edit_mode_show_cb(void *data, Evas *e, Evas_Object *obj,
		void *event_info)
{
	evas_object_event_callback_del(obj, EVAS_CALLBACK_SHOW,
			_entry_edit_mode_show_cb);

	elm_object_focus_set(obj, EINA_TRUE);
}

static void __popup_entry_activated_cb(void *data, Evas_Object *obj, void *event_info)
{
	if (!obj)
		return;

	elm_object_focus_set(obj, EINA_FALSE);
}

static Evas_Object* _gl_pswd_entry_item_content_get(void *data,
		Evas_Object *obj, const char *part)
{
	if (obj == NULL || data == NULL) {
		return NULL;
	}

	int return_key_type;
	Evas_Object *entry = NULL;
	Evas_Object *editfield = NULL;

	static Elm_Entry_Filter_Limit_Size limit_filter_data;
	pswd_popup_t *pswd_popup_data = (pswd_popup_t *)data;

	if (!g_strcmp0(part, "elm.icon.entry")) {
		editfield = elm_layout_add(obj);
		elm_layout_theme_set(editfield, "layout", "editfield", "singleline");
		evas_object_size_hint_align_set(editfield, EVAS_HINT_FILL, 0.0);
		evas_object_size_hint_weight_set(editfield, EVAS_HINT_EXPAND, 0.0);
		entry = elm_entry_add(editfield);
		elm_entry_single_line_set(entry, EINA_TRUE);
		elm_entry_scrollable_set(entry, EINA_TRUE);
		elm_entry_password_set(entry, EINA_TRUE);
		eext_entry_selection_back_event_allow_set(entry, EINA_TRUE);
		evas_object_size_hint_weight_set(entry, EVAS_HINT_EXPAND, 0.0);
		evas_object_size_hint_align_set(entry, EVAS_HINT_FILL, 0.0);
		if (!g_strcmp0(pswd_popup_data->str_pkg_name, "wifi-qs")) {
			elm_entry_input_panel_imdata_set(entry,
					"type=systempopup", 16);
		}
		elm_entry_cnp_mode_set(entry, ELM_CNP_MODE_PLAINTEXT);
		elm_entry_input_panel_layout_set(entry,
				ELM_INPUT_PANEL_LAYOUT_PASSWORD);
		elm_object_domain_translatable_part_text_set(entry, "elm.guide",
			PACKAGE, "IDS_WIFI_HEADER_PASSWORD");
		elm_object_signal_emit(entry, "elm,action,hide,search_icon", "");

		limit_filter_data.max_char_count = PASSWORD_LENGTH;
		elm_entry_markup_filter_append(entry,
		elm_entry_filter_limit_size, &limit_filter_data);

		return_key_type = ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_DONE;
		elm_entry_input_panel_return_key_type_set(entry, return_key_type);
		elm_entry_input_panel_return_key_disabled_set(entry, EINA_TRUE);
		evas_object_smart_callback_add(entry, "activated",
				__popup_entry_activated_cb, data);
		evas_object_smart_callback_add(entry, "changed",
				__popup_entry_changed_cb, data);
		evas_object_smart_callback_add(entry, "preedit,changed",
				__popup_entry_changed_cb, data);
		evas_object_smart_callback_add(entry, "maxlength,reached",
				__popup_entry_maxlength_reached, NULL);
		evas_object_event_callback_add(entry, EVAS_CALLBACK_SHOW,
				_entry_edit_mode_show_cb, NULL);
		elm_object_part_content_set(editfield, "elm.swallow.content", entry);

		pswd_popup_data->entry = entry;

		elm_entry_input_panel_show(entry);
		return editfield;
	}

	return NULL;
}

static void _chk_changed_cb(void *data, Evas_Object *obj, void *ei)
{
	if (obj == NULL) {
		return;
	}

	Eina_Bool state = elm_check_state_get(obj);
	if (state) {
		elm_entry_password_set((Evas_Object *)data, EINA_FALSE);
	} else {
		elm_entry_password_set((Evas_Object *)data, EINA_TRUE);
	}
	elm_entry_cursor_end_set((Evas_Object *)data);
}
static char *_gl_pswd_check_box_item_text_get(void *data, Evas_Object *obj,
		const char *part)
{
	if (!strcmp("elm.text", part)) {
		char buf[1024];
		snprintf(buf, 1023, "%s", sc(PACKAGE, I18N_TYPE_Show_password));
		return strdup(buf);
	}
	return NULL;

}

static Evas_Object *_gl_pswd_check_box_item_content_get(void *data,
		Evas_Object *obj, const char *part)
{
	Evas_Object *check = NULL;
	pswd_popup_t *pswd_popup_data = (pswd_popup_t *)data;

	if (!strcmp("elm.swallow.end", part)) {
		check = elm_check_add(obj);
		evas_object_propagate_events_set(check, EINA_FALSE);

		evas_object_size_hint_align_set(check, EVAS_HINT_FILL, EVAS_HINT_FILL);
		evas_object_size_hint_weight_set(check, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		evas_object_smart_callback_add(check, "changed",
				_chk_changed_cb, pswd_popup_data->entry);

		elm_object_focus_allow_set(check, EINA_FALSE);

		return check;
	}
	return NULL;
}

static void _gl_pswd_check_box_sel(void *data, Evas_Object *obj, void *ei)
{
	__COMMON_FUNC_ENTER__;
	Elm_Object_Item *item = NULL;

	item = (Elm_Object_Item *)ei;
	if (item == NULL) {
		return;
	}

	pswd_popup_t *pswd_popup_data = (pswd_popup_t *)data;

	Evas_Object *ck = elm_object_item_part_content_get(ei, "elm.icon.right");

	elm_genlist_item_selected_set(item, EINA_FALSE);

	Eina_Bool state = elm_check_state_get(ck);
	elm_check_state_set(ck, !state);
	if (pswd_popup_data) {
		_chk_changed_cb(pswd_popup_data->entry, ck, NULL);
	}
}

static void _passwd_popup_keypad_off_cb(void *data, Evas_Object *obj,
		void *event_info)
{
	if (data == NULL) {
		return;
	}

	pswd_popup_t *pop_info = (pswd_popup_t *)data;

	keypad_state = FALSE;

	if (wps_options_click == TRUE) {
		wps_options_click = FALSE;

		if (pop_info->wps_btn_cb != NULL) {
			pop_info->wps_btn_cb(NULL, NULL, NULL);
		}
	}
	INFO_LOG(UG_NAME_NORMAL,"Keypad is down");
}

static void _passwd_popup_keypad_on_cb(void *data, Evas_Object *obj,
		void *event_info)
{
	if (data == NULL) {
		return;
	}

	keypad_state = TRUE;
	INFO_LOG(UG_NAME_NORMAL,"Keypad is up");
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

static void _common_wps_options_popup_cb(void* data, Evas_Object* obj,
		void* event_info)
{
	__COMMON_FUNC_ENTER__;

	if (!event_info || !data) {
		return;
	}

	pswd_popup_t *pop_info = (pswd_popup_t *) data;
	Elm_Object_Item *item = event_info;
	elm_genlist_item_selected_set(item, EINA_FALSE);

	if (keypad_state == FALSE) {
		if (pop_info->wps_btn_cb != NULL)
			pop_info->wps_btn_cb(NULL, NULL, NULL);
	} else {
		wps_options_click = TRUE;
		ecore_imf_input_panel_hide();
	}

	__COMMON_FUNC_EXIT__;
}

pswd_popup_t *create_passwd_popup(Evas_Object *conformant,Evas_Object *win_main,
		const char *pkg_name, pswd_popup_create_req_data_t *popup_info)
{
	Evas_Object *passpopup = NULL;
	Evas_Object *genlist = NULL;
	Evas_Object *btn_ok = NULL;

	__COMMON_FUNC_ENTER__;

	if (!win_main || !popup_info || !pkg_name) {
		return NULL;
	}

	pswd_popup_t *pswd_popup_data = g_new0(pswd_popup_t, 1);

	if (popup_info->ap) {
		if (WIFI_ERROR_NONE !=
				wifi_ap_clone(&(pswd_popup_data->ap), popup_info->ap)) {
			g_free(pswd_popup_data);

			return NULL;
		}
	} else {
		/* Hidden Wi-Fi network does not have handle */
	}

	wps_options_click = FALSE;
	/* Lets disable the scan updates so that the UI is not refreshed un necessarily */
	wlan_manager_disable_scan_result_update();

	popup_btn_info_t popup_btn_data;
	memset(&popup_btn_data, 0, sizeof(popup_btn_data));

	popup_btn_data.title_txt = popup_info->title;
	popup_btn_data.btn1_cb = popup_info->cancel_cb;
	popup_btn_data.btn1_data = popup_info->cb_data;
	popup_btn_data.btn1_txt = "IDS_WIFI_SK_CANCEL";
	popup_btn_data.btn2_cb = popup_info->ok_cb;
	popup_btn_data.btn2_data = popup_info->cb_data;
	popup_btn_data.btn2_txt = "IDS_WIFI_BODY_CONNECT";

	passpopup = common_utils_show_info_popup(win_main, &popup_btn_data);

	if (!passpopup) {
		ERROR_LOG(UG_NAME_ERR, "Could not initialize popup");
		return NULL;
	}

	pswd_popup_data->win = win_main;
	pswd_popup_data->conf = conformant;
	pswd_popup_data->str_pkg_name = pkg_name;
	pswd_popup_data->popup = passpopup;
	pswd_popup_data->sec_type = popup_info->sec_type;
	pswd_popup_data->curr_ap_name = g_strdup(popup_info->title);
	pswd_popup_data->show_wps_btn = popup_info->show_wps_btn;
	pswd_popup_data->wps_btn_cb = popup_info->wps_btn_cb;

	/* Hide the Okay button until the password is entered */
	btn_ok = elm_object_part_content_get(passpopup, "button2");
	elm_object_disabled_set(btn_ok, TRUE);
#ifdef ACCESSIBLITY_FEATURE
	Evas_Object *eo = NULL;
	Evas_Object *ao = NULL;
	eo = elm_layout_edje_get(passpopup);
	const Evas_Object *po = edje_object_part_object_get(eo, "elm.text.title");
	ao = elm_access_object_get(po);
	elm_access_info_set(ao, ELM_ACCESS_INFO, popup_info->title);
#endif

	genlist = elm_genlist_add(passpopup);
	elm_genlist_mode_set(genlist, ELM_LIST_COMPRESS);
	elm_scroller_content_min_limit(genlist, EINA_FALSE, EINA_TRUE);
	evas_object_size_hint_weight_set(genlist,
			EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(genlist, EVAS_HINT_FILL, EVAS_HINT_FILL);

	/* Entry genlist item */
	g_pswd_entry_itc.item_style = "entry";
	g_pswd_entry_itc.func.text_get = NULL;
	g_pswd_entry_itc.func.content_get = _gl_pswd_entry_item_content_get;
	g_check_box_itc.func.state_get = NULL;
	g_check_box_itc.func.del = NULL;

	Elm_Object_Item * entry_item = elm_genlist_item_append(genlist,
			&g_pswd_entry_itc, pswd_popup_data,
			NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);

	/* Checkbox genlist item */
	g_check_box_itc.item_style = WIFI_GENLIST_1LINE_TEXT_ICON_STYLE;
	g_check_box_itc.func.text_get = _gl_pswd_check_box_item_text_get;
	g_check_box_itc.func.content_get = _gl_pswd_check_box_item_content_get;
	g_check_box_itc.func.state_get = NULL;
	g_check_box_itc.func.del = NULL;

	elm_genlist_item_append(genlist, &g_check_box_itc, pswd_popup_data,
			NULL, ELM_GENLIST_ITEM_NONE,
			_gl_pswd_check_box_sel, pswd_popup_data);

	if (popup_info->show_wps_btn) {
		/* WPS options genlist item */
		g_wps_itc.item_style = WIFI_GENLIST_1LINE_TEXT_ICON_STYLE;
		g_wps_itc.func.text_get = _passwd_popup_wps_item_text_get;
		g_wps_itc.func.content_get = _passwd_popup_wps_item_content_get;
		g_wps_itc.func.state_get = NULL;
		g_wps_itc.func.del = NULL;

		pswd_popup_data->wps_options_item = elm_genlist_item_append(genlist,
				&g_wps_itc, NULL, NULL, ELM_GENLIST_ITEM_NONE,
				_common_wps_options_popup_cb, pswd_popup_data);
	}

	elm_genlist_realization_mode_set(genlist, EINA_TRUE);
	evas_object_show(genlist);

	elm_object_content_set(passpopup, genlist);

	evas_object_smart_callback_add(passpopup, "show,finished",
			popup_animation_finish_cb, entry_item);
	evas_object_show(passpopup);

	evas_object_smart_callback_add(pswd_popup_data->conf,
			"virtualkeypad,state,on", _passwd_popup_keypad_on_cb,
			pswd_popup_data);
	evas_object_smart_callback_add(pswd_popup_data->conf,
			"virtualkeypad,state,off", _passwd_popup_keypad_off_cb,
			pswd_popup_data);

	__COMMON_FUNC_EXIT__;
	return pswd_popup_data;
}

char *passwd_popup_get_txt(pswd_popup_t *pswd_popup_data)
{
	if (pswd_popup_data) {
		Evas_Object *entry = pswd_popup_data->entry;
		return elm_entry_markup_to_utf8(elm_entry_entry_get(entry));
	}

	return NULL;
}

wifi_ap_h passwd_popup_get_ap(pswd_popup_t *pswd_popup_data)
{
	if (pswd_popup_data) {
		return pswd_popup_data->ap;
	}

	return NULL;
}

void current_popup_free(pswd_popup_t *pswd_popup_data, popup_type_t type)
{
	if (pswd_popup_data == NULL) {
		return;
	}

	int rotation = -1;
	char buf[1024];
	Evas_Object *curr_popup = NULL;

	if (type == POPUP_WPS_OPTIONS) {
		if (pswd_popup_data->wps_options_popup) {
			evas_object_hide(pswd_popup_data->wps_options_popup);
			evas_object_del(pswd_popup_data->wps_options_popup);
			pswd_popup_data->wps_options_popup = NULL;
		}
		evas_object_show(pswd_popup_data->popup);
		curr_popup = pswd_popup_data->popup;
	}

	rotation = common_utils_get_rotate_angle(APPCORE_RM_UNKNOWN);
	if (rotation != -1) {
		g_snprintf(buf, sizeof(buf), "elm,state,orient,%d", rotation);
		SECURE_INFO_LOG(UG_NAME_NORMAL, "Rotation signal - %s", buf);
		elm_layout_signal_emit(curr_popup, buf, "elm");
	}
}

void passwd_popup_hide(pswd_popup_t *pswd_popup_data)
{
	if (pswd_popup_data == NULL) {
		return;
	}

	if (pswd_popup_data->popup != NULL) {
		evas_object_hide(pswd_popup_data->popup);
	}
}

void passwd_popup_show(pswd_popup_t *pswd_popup_data)
{
	if (pswd_popup_data == NULL) {
		return;
	}

	if (pswd_popup_data->popup != NULL) {
		evas_object_show(pswd_popup_data->popup);
	}
}

void passwd_popup_free(pswd_popup_t *pswd_popup_data)
{
	__COMMON_FUNC_ENTER__;

	if (pswd_popup_data == NULL) {
		return;
	}

	if (pswd_popup_data->conf) {
		if (keypad_state == TRUE)
			keypad_state = FALSE;
		evas_object_smart_callback_del(pswd_popup_data->conf,
				"virtualkeypad,state,on",
				_passwd_popup_keypad_on_cb);
		evas_object_smart_callback_del(pswd_popup_data->conf,
				"virtualkeypad,state,off",
				_passwd_popup_keypad_off_cb);
	}

	if (pswd_popup_data->curr_ap_name != NULL) {
		g_free(pswd_popup_data->curr_ap_name);
		pswd_popup_data->curr_ap_name = NULL;
	}

	if (pswd_popup_data->info_popup != NULL) {
		evas_object_del(pswd_popup_data->info_popup);
		pswd_popup_data->info_popup = NULL;
		INFO_LOG(UG_NAME_NORMAL, "info popup deleted");
	}

	if (pswd_popup_data->pbc_popup_data) {
		__common_pbc_popup_destroy(pswd_popup_data->pbc_popup_data);
		pswd_popup_data->pbc_popup_data = NULL;
		INFO_LOG(UG_NAME_NORMAL, "wps pbc popup deleted");
	}

	if (pswd_popup_data->popup != NULL) {
		evas_object_del(pswd_popup_data->popup);
		pswd_popup_data->popup = NULL;
		INFO_LOG(UG_NAME_NORMAL, "password popup deleted");
	}

	if (pswd_popup_data->wps_options_popup != NULL) {
		evas_object_del(pswd_popup_data->wps_options_popup);
		pswd_popup_data->wps_options_popup = NULL;
		INFO_LOG(UG_NAME_NORMAL, "wps option popup deleted");
	}

	/* Hidden Wi-Fi network does not have handle */
	if (pswd_popup_data->ap)
		wifi_ap_destroy(pswd_popup_data->ap);

	g_free(pswd_popup_data);

	/* A delay is needed to get the smooth Input panel closing animation effect */
	common_util_managed_ecore_scan_update_timer_add(0.1,
			_enable_scan_updates_cb, NULL);

	__COMMON_FUNC_EXIT__;
}
