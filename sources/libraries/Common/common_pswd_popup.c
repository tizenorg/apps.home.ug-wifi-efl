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
#include "common_pswd_popup.h"
#include "i18nmanager.h"
#include "common_utils.h"
#include "wifi.h"

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
	Evas_Object *popup_conformant;
	Evas_Object *popup_entry_lyt;
	pbc_popup_t *pbc_popup_data;
};

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

void common_pswd_popup_pbc_popup_create(pswd_popup_t *pswd_popup_data, Evas_Smart_Cb cancel_cb, void *cancel_cb_data)
{
	if (!pswd_popup_data) {
		return;
	}
	Evas_Object *popup = NULL,*progressbar = NULL;
	Evas_Object *label = NULL, *timer_label = NULL;

	pbc_popup_t *pbc_popup_data = NULL;
	pbc_popup_data = g_malloc0(sizeof(pbc_popup_t));

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

	progressbar = elm_progressbar_add(popup);
	elm_object_style_set(progressbar, "list_progress");
	elm_progressbar_horizontal_set(progressbar, EINA_TRUE);
	evas_object_size_hint_align_set(progressbar, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_size_hint_weight_set(progressbar, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_progressbar_value_set(progressbar, 0.0);

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
	elm_box_pack_end(box, progressbar);
	elm_box_pack_end(box, timer_label);

	evas_object_show(box);
	elm_object_content_set(popup, box);
	pswd_popup_data->pbc_popup_data = pbc_popup_data;

	/* Delete the password popup */
	evas_object_hide(pswd_popup_data->popup_conformant);
	evas_object_del(pswd_popup_data->popup_conformant);

	return;
}

pswd_popup_t *common_pswd_popup_create(Evas_Object *win_main, const char *pkg_name, pswd_popup_create_req_data_t *popup_info)
{
	__COMMON_FUNC_ENTER__;

	if (!win_main || !popup_info || !pkg_name)
		return NULL;

	/* Lets disable the scan updates so that the UI is not refreshed un necessarily */
	wlan_manager_disable_scan_result_update();

	pswd_popup_t *pswd_popup_data = (pswd_popup_t *)g_malloc0(sizeof(pswd_popup_t));
	popup_btn_info_t	popup_btn_data;
	common_utils_entry_info_t entry_info;
	Evas_Object *conformant = NULL;

	conformant = elm_conformant_add(win_main);
	assertm_if(NULL == conformant, "conformant is NULL!!");
	elm_win_conformant_set(win_main, EINA_TRUE);
	elm_win_resize_object_add(win_main, conformant);
	evas_object_size_hint_weight_set(conformant, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(conformant, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(conformant);

	Evas_Object *content = elm_layout_add(conformant);
	elm_object_content_set(conformant, content);

	memset(&popup_btn_data, 0, sizeof(popup_btn_data));
	popup_btn_data.title_txt = popup_info->title;
	popup_btn_data.btn1_cb = popup_info->ok_cb;
	popup_btn_data.btn1_data = popup_info->cb_data;
	popup_btn_data.btn2_cb = popup_info->cancel_cb;
	popup_btn_data.btn2_data = popup_info->cb_data;
	popup_btn_data.btn1_txt = sc(pkg_name, I18N_TYPE_Ok);
	popup_btn_data.btn2_txt = sc(pkg_name, I18N_TYPE_Cancel);
	Evas_Object *passpopup = common_utils_show_info_popup(content, &popup_btn_data);

	elm_object_content_set(content, passpopup);

	Evas_Object *box = elm_box_add(passpopup);
	evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(box);

	common_utils_set_entry_info(&entry_info, sc(pkg_name, I18N_TYPE_Enter_password), "", sc(pkg_name, I18N_TYPE_Enter_password), ELM_INPUT_PANEL_LAYOUT_URL);
	Evas_Object *ly_editfield = common_utils_add_edit_box(box, &entry_info);
	common_utils_entry_password_set(ly_editfield, TRUE);
	evas_object_show(ly_editfield);

	elm_box_pack_end(box, ly_editfield);

	Evas_Object *check = elm_check_add(box);
	elm_object_text_set(check, sc(pkg_name, I18N_TYPE_Show_password));
	elm_object_focus_allow_set(check, EINA_FALSE);
	evas_object_size_hint_weight_set(check, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(check, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_smart_callback_add(check, "changed", _check_changed_cb, ly_editfield);
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
		evas_object_smart_callback_add(btn, "clicked", popup_info->wps_btn_cb, popup_info->cb_data);
		elm_box_pack_end(box, btn);
		evas_object_show(btn);
	}
	elm_object_content_set(passpopup, box);
	evas_object_show(passpopup);
	pswd_popup_data->win = win_main;
	pswd_popup_data->str_pkg_name = pkg_name;
	pswd_popup_data->popup_conformant = conformant;
	pswd_popup_data->popup_entry_lyt = ly_editfield;
	Evas_Object *popup_entry = common_utils_entry_layout_get_entry(ly_editfield);
	elm_object_focus_set(popup_entry, EINA_TRUE);
	__COMMON_FUNC_EXIT__;

	return pswd_popup_data;
}

char *common_pswd_popup_get_txt(pswd_popup_t *pswd_popup_data)
{
	if (pswd_popup_data)
		return common_utils_entry_layout_get_text(pswd_popup_data->popup_entry_lyt);

	return NULL;
}

void common_pswd_popup_destroy(pswd_popup_t *pswd_popup_data)
{
	if (pswd_popup_data) {
		if (pswd_popup_data->pbc_popup_data) {
			__common_pbc_popup_destroy(pswd_popup_data->pbc_popup_data);
			pswd_popup_data->pbc_popup_data = NULL;
		}
		evas_object_hide(pswd_popup_data->popup_conformant);
		evas_object_del(pswd_popup_data->popup_conformant);
		pswd_popup_data->popup_conformant = NULL;
		pswd_popup_data->popup_entry_lyt = NULL;
		g_free(pswd_popup_data);

		/* A delay is needed to get the smooth Input panel closing animation effect */
		ecore_timer_add(0.1, _enable_scan_updates_cb, NULL);
	}
	return;
}
