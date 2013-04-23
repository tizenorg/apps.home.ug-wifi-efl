/*
 * Wi-Fi
 *
 * Copyright 2012 Samsung Electronics Co., Ltd
 *
 * Licensed under the Flora License, Version 1.0 (the "License");
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
#include "common_pswd_popup.h"
#include "i18nmanager.h"
#include "common_utils.h"
#include "ug_wifi.h"

#define PBC_TIMEOUT_MSG_STR	"one click connection failed"
#define MAX_PBC_TIMEOUT_SECS	120	// Time in seconds

typedef struct {
	/* PBC popup related attributes */
	Evas_Object* popup;
	Evas_Object* progressbar;
	Evas_Object* timer_label;
	Ecore_Timer *timer;
	int checker;
	double value;
} pbc_popup_t;

struct pswd_popup {
	/* Password popup related attributes */
	const char *str_pkg_name;
	Evas_Object *win;
	Evas_Object *popup;
	Evas_Object *popup_entry_lyt;
	pbc_popup_t *pbc_popup_data;
	wifi_ap_h ap;
};

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

static void _check_changed_cb(void *data, Evas_Object *obj, void *event_info)
{
	if (obj == NULL)
		return;

	Eina_Bool state = elm_check_state_get(obj);
	common_utils_entry_password_set(data, !state);
}

static void __common_pbc_popup_destroy(pbc_popup_t *pbc_popup_data)
{
	__COMMON_FUNC_ENTER__;
	if (!pbc_popup_data)
		return;

	if (pbc_popup_data->checker == 0) {
		pbc_popup_data->checker = 1;

		if (pbc_popup_data->timer != NULL) {
			ecore_timer_del(pbc_popup_data->timer);
			pbc_popup_data->timer = NULL;
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
	pswd_popup_t *pswd_popup_data = (pswd_popup_t *)data;
	pbc_popup_t *pbc_popup_data = pswd_popup_data->pbc_popup_data;
	if (!pbc_popup_data || pbc_popup_data->timer == NULL || pbc_popup_data->progressbar == NULL) {
		return ECORE_CALLBACK_CANCEL;
	}
	const double diff = (double)1/(double)MAX_PBC_TIMEOUT_SECS;
	pbc_popup_data->value = elm_progressbar_value_get(pbc_popup_data->progressbar);
	pbc_popup_data->value += diff;
	if (pbc_popup_data->value >= 1) {
		if (pbc_popup_data->checker == 0) {
			__COMMON_FUNC_ENTER__;
			common_utils_show_info_timeout_popup(pswd_popup_data->win, PBC_TIMEOUT_MSG_STR, 3.0f);
			Evas_Object *cancel_btn = elm_object_part_content_get(pbc_popup_data->popup, "button1");
			evas_object_smart_callback_call(cancel_btn, "clicked", NULL);
			__COMMON_FUNC_EXIT__;
		}
		return ECORE_CALLBACK_CANCEL;
	}

	int remain_mins = (int)(MAX_PBC_TIMEOUT_SECS * (1 - pbc_popup_data->value));
	int remain_secs = 0;
	remain_secs = remain_mins % 60;
	remain_mins /= 60;

	char *remaining_time_str = g_strdup_printf("<font_size=40><align=center>%02d:%02d</align></font_size>", remain_mins, remain_secs);
	elm_object_text_set(pbc_popup_data->timer_label, remaining_time_str);
	//INFO_LOG(UG_NAME_NORMAL, "pbc_popup_data->value = %lf; remain_mins = %d; remain_secs = %d; remaining_time_str = %s", pbc_popup_data->value, remain_mins, remain_secs, remaining_time_str);
	g_free(remaining_time_str);

	elm_progressbar_value_set(pbc_popup_data->progressbar, pbc_popup_data->value);

	return ECORE_CALLBACK_RENEW;
}

static Eina_Bool _enable_scan_updates_cb(void *data)
{
	/* Lets enable the scan updates */
	wlan_manager_enable_scan_result_update();

	return ECORE_CALLBACK_CANCEL;
}

void create_pbc_popup(pswd_popup_t *pswd_popup_data, Evas_Smart_Cb cancel_cb, void *cancel_cb_data)
{
	if (!pswd_popup_data) {
		return;
	}
	Evas_Object *popup = NULL,*progressbar = NULL;
	Evas_Object *label = NULL, *timer_label = NULL;
	Evas_Object *separator1 = NULL, *separator2 = NULL;

	pbc_popup_t *pbc_popup_data = NULL;
	pbc_popup_data = g_new0(pbc_popup_t, 1);

	popup_btn_info_t popup_btn_data;
	memset(&popup_btn_data, 0, sizeof(popup_btn_data));
	popup_btn_data.btn1_txt = sc(pswd_popup_data->str_pkg_name, I18N_TYPE_Cancel);
	popup_btn_data.btn1_cb = cancel_cb;
	popup_btn_data.btn1_data = cancel_cb_data;
	popup = common_utils_show_info_popup(pswd_popup_data->win, &popup_btn_data);

	label = elm_label_add(popup);
	elm_object_style_set(label, "popup/default");
	elm_label_line_wrap_set(label, ELM_WRAP_MIXED);
	elm_object_text_set(label, sc(pswd_popup_data->str_pkg_name,I18N_TYPE_Press_WPS_On_Your_Wi_Fi_Access_Point));
	evas_object_size_hint_weight_set(label, EVAS_HINT_EXPAND, 0.0);
	evas_object_size_hint_align_set(label, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(label);

	separator1 = elm_separator_add(popup);

	progressbar = elm_progressbar_add(popup);
	elm_object_style_set(progressbar, "list_progress");
	elm_progressbar_horizontal_set(progressbar, EINA_TRUE);
	evas_object_size_hint_align_set(progressbar, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_size_hint_weight_set(progressbar, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_progressbar_value_set(progressbar, 0.0);

	separator2 = elm_separator_add(popup);

	timer_label = elm_label_add(popup);
	elm_object_style_set(timer_label, "label3");
	elm_label_line_wrap_set(timer_label, ELM_WRAP_MIXED);
	elm_object_text_set(timer_label, _("<font_size=40><align=center>02:00</align></font_size>"));
	evas_object_size_hint_weight_set(timer_label, EVAS_HINT_EXPAND, 0.0);
	evas_object_size_hint_align_set(timer_label, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(timer_label);

	pbc_popup_data->checker = 0;
	pbc_popup_data->value = 0.0;
	pbc_popup_data->progressbar = progressbar;
	pbc_popup_data->timer_label = timer_label;
	pbc_popup_data->popup = popup;
	pbc_popup_data->timer = ecore_timer_add(1.0, _fn_pb_timer_bar, pswd_popup_data);
	evas_object_show(progressbar);

	Evas_Object *box = elm_box_add(popup);
	evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(box);
	elm_box_pack_end(box, label);
	elm_box_pack_end(box, separator1);
	elm_box_pack_end(box, progressbar);
	elm_box_pack_end(box, separator2);
	elm_box_pack_end(box, timer_label);

	evas_object_show(box);
	elm_object_content_set(popup, box);
	pswd_popup_data->pbc_popup_data = pbc_popup_data;

	/* Delete the password popup */
	evas_object_hide(pswd_popup_data->popup);
	evas_object_del(pswd_popup_data->popup);

	return;
}

pswd_popup_t *create_passwd_popup(Evas_Object *win_main,
		const char *pkg_name, pswd_popup_create_req_data_t *popup_info)
{
	__COMMON_FUNC_ENTER__;

	if (!win_main || !popup_info || !pkg_name)
		return NULL;

	pswd_popup_t *pswd_popup_data = g_new0(pswd_popup_t, 1);

	if (popup_info->ap) {
		if (WIFI_ERROR_NONE !=
				wifi_ap_clone(&(pswd_popup_data->ap), popup_info->ap)) {
			g_free(pswd_popup_data);

			return NULL;
		}
	} else {
		/* It can be NULL in case of hidden AP */
	}
	/* Lets disable the scan updates so that the UI is not refreshed un necessarily */
	wlan_manager_disable_scan_result_update();

	popup_btn_info_t popup_btn_data;
	memset(&popup_btn_data, 0, sizeof(popup_btn_data));

	popup_btn_data.title_txt = popup_info->title;
	popup_btn_data.btn1_cb = popup_info->ok_cb;
	popup_btn_data.btn1_data = popup_info->cb_data;
	popup_btn_data.btn2_cb = popup_info->cancel_cb;
	popup_btn_data.btn2_data = popup_info->cb_data;
	popup_btn_data.btn1_txt = sc(pkg_name, I18N_TYPE_Ok);
	popup_btn_data.btn2_txt = sc(pkg_name, I18N_TYPE_Cancel);
	Evas_Object *passpopup = common_utils_show_info_popup(win_main, &popup_btn_data);

	Evas_Object *eo = elm_layout_edje_get(passpopup);
	const Evas_Object *po = edje_object_part_object_get(eo, "elm.text.title");
	Evas_Object *ao = elm_access_object_get(po);
	elm_access_info_set(ao, ELM_ACCESS_INFO, popup_info->title);

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
	elm_object_part_text_set(entry_ly, "elm.text", sc(pkg_name, I18N_TYPE_Enter_password));

	entry = elm_entry_add(entry_ly);
	elm_entry_scrollable_set(entry, EINA_TRUE);
	elm_entry_single_line_set(entry, EINA_TRUE);
	elm_entry_input_panel_layout_set(entry, ELM_INPUT_PANEL_LAYOUT_PASSWORD);
	elm_object_part_content_set(entry_ly, "elm.swallow.content", entry);

	limit_filter_data.max_char_count = 32;
	elm_entry_markup_filter_append(entry,
			elm_entry_filter_limit_size, &limit_filter_data);

	evas_object_smart_callback_add(entry, "changed",
			__popup_entry_changed_cb, entry_ly);
	evas_object_smart_callback_add(entry, "focused",
			__popup_entry_focused_cb, entry_ly);
	evas_object_smart_callback_add(entry, "unfocused",
			__popup_entry_unfocused_cb, entry_ly);
	elm_object_signal_callback_add(entry_ly, "elm,eraser,clicked",
			"elm", __popup_eraser_clicked_cb, entry);
	evas_object_show(entry);

	elm_entry_password_set(entry, EINA_TRUE);
	evas_object_show(entry_ly);
	elm_box_pack_end(box, entry_ly);

	Evas_Object *check = elm_check_add(box);
	elm_object_text_set(check, sc(pkg_name, I18N_TYPE_Show_password));
	elm_object_focus_allow_set(check, EINA_FALSE);
	evas_object_size_hint_weight_set(check, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(check, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_smart_callback_add(check, "changed", _check_changed_cb, entry_ly);

	evas_object_show(check);
	elm_box_pack_end(box, check);

	if (popup_info->show_wps_btn) {
		Evas_Object *btn = elm_button_add(box);
		elm_object_text_set(btn, sc(pkg_name, I18N_TYPE_WPS_Button_Connection));
		evas_object_size_hint_weight_set(btn,
				EVAS_HINT_EXPAND,
				EVAS_HINT_EXPAND);
		evas_object_size_hint_align_set(btn,
				EVAS_HINT_FILL,
				EVAS_HINT_FILL);
		evas_object_smart_callback_add(btn, "clicked",
				popup_info->wps_btn_cb, popup_info->cb_data);
		elm_box_pack_end(box, btn);
		evas_object_show(btn);
	}

	elm_object_content_set(passpopup, box);
	evas_object_show(passpopup);
	pswd_popup_data->win = win_main;
	pswd_popup_data->str_pkg_name = pkg_name;
	pswd_popup_data->popup = passpopup;
	pswd_popup_data->popup_entry_lyt = entry_ly;
	elm_object_focus_set(entry, EINA_TRUE);

	__COMMON_FUNC_EXIT__;
	return pswd_popup_data;
}

char *passwd_popup_get_txt(pswd_popup_t *pswd_popup_data)
{
	if (pswd_popup_data)
		return common_utils_entry_layout_get_text(
					pswd_popup_data->popup_entry_lyt);

	return NULL;
}

wifi_ap_h passwd_popup_get_ap(pswd_popup_t *pswd_popup_data)
{
	if (pswd_popup_data)
		return pswd_popup_data->ap;

	return NULL;
}

void passwd_popup_free(pswd_popup_t *pswd_popup_data)
{
	if (pswd_popup_data == NULL)
		return;

	if (pswd_popup_data->pbc_popup_data) {
		__common_pbc_popup_destroy(pswd_popup_data->pbc_popup_data);
		pswd_popup_data->pbc_popup_data = NULL;
	}

	evas_object_hide(pswd_popup_data->popup);
	evas_object_del(pswd_popup_data->popup);
	pswd_popup_data->popup = NULL;
	pswd_popup_data->popup_entry_lyt = NULL;

	wifi_ap_destroy(pswd_popup_data->ap);

	g_free(pswd_popup_data);

	/* A delay is needed to get the smooth Input panel closing animation effect */
	ecore_timer_add(0.1, _enable_scan_updates_cb, NULL);
}
