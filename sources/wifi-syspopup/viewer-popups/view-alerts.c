/*
 * Copyright (c) 2000-2012 Samsung Electronics Co., Ltd All Rights Reserved.
 *
 * This file is part of Wi-Fi syspopup
 * Written by Sanghoon Cho <sanghoon80.cho@samsung.com>
 *
 * PROPRIETARY/CONFIDENTIAL
 *
 * This software is the confidential and proprietary information of
 * SAMSUNG ELECTRONICS ("Confidential Information").
 * You shall not disclose such Confidential Information and shall
 * use it only in accordance with the terms of the license agreement
 * you entered into with SAMSUNG ELECTRONICS.
 *
 * SAMSUNG make no representations or warranties about the suitability
 * of the software, either express or implied, including but not limited
 * to the implied warranties of merchantability, fitness for a particular
 * purpose, or non-infringement. SAMSUNG shall not be liable for
 * any damages suffered by licensee as a result of using, modifying or
 * distributing this software or its derivatives.
 *
 */



#include "common.h"
#include "wifi-syspopup.h"
#include "view-alerts.h"
#include "i18nmanager.h"

extern wifi_object* app_state;

static void _timeout_cb(void *data, Evas_Object *obj, void *event_info)
{
	if (obj)
		evas_object_del(obj);
}

int view_alerts_powering_on_show(void)
{
	__COMMON_FUNC_ENTER__;
	if(WIFI_SYSPOPUP_SUPPORT_QUICKPANEL == app_state->wifi_syspopup_support){
		__COMMON_FUNC_EXIT__;
		return TRUE;
	}
	if(NULL != app_state->alertpopup) {
		evas_object_del(app_state->alertpopup);
		app_state->alertpopup = NULL;
	}

	Evas_Object *conformant = NULL;
	conformant = elm_conformant_add(app_state->win_main);
	elm_win_conformant_set(app_state->win_main, EINA_TRUE);
	elm_win_resize_object_add(app_state->win_main, conformant);
	evas_object_size_hint_weight_set(conformant, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(conformant, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(conformant);

	Evas_Object *content = elm_layout_add(conformant);
	elm_object_content_set(conformant, content);
	app_state->alertpopup = elm_popup_add(content);

	Evas_Object *box = elm_box_add(app_state->alertpopup);
	evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(box);

	Evas_Object *progressbar = elm_progressbar_add(box);
	elm_object_style_set(progressbar, "list_process");
	evas_object_size_hint_weight_set(progressbar, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(progressbar, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_progressbar_pulse(progressbar, EINA_TRUE);
	evas_object_show(progressbar);
	elm_box_pack_end(box, progressbar);

	Evas_Object *label = elm_label_add(box);
	elm_object_style_set(label, "popup/default");
	evas_object_size_hint_weight_set(label, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(label, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_object_text_set(label, sc(PACKAGE, I18N_TYPE_Activating));
	evas_object_show(label);
	elm_box_pack_end(box, label);

	elm_object_content_set(app_state->alertpopup, box);
	evas_object_size_hint_weight_set(app_state->alertpopup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_show(app_state->alertpopup);

	__COMMON_FUNC_EXIT__;

	return TRUE;
}

int view_alerts_connection_fail_show(void)
{
	__COMMON_FUNC_ENTER__;

	if (WIFI_SYSPOPUP_SUPPORT_QUICKPANEL == app_state->wifi_syspopup_support) {
		__COMMON_FUNC_EXIT__;

		return TRUE;
	}

	if (NULL != app_state->alertpopup) {
		evas_object_del(app_state->alertpopup);
		app_state->alertpopup = NULL;
	}

	app_state->alertpopup = elm_popup_add(app_state->win_main);
	elm_object_text_set(app_state->alertpopup, "Connection attempt failed.<br>Try again.");
	elm_popup_timeout_set(app_state->alertpopup, 2.0f);
	evas_object_smart_callback_add(app_state->alertpopup, "timeout", _timeout_cb, NULL);
	evas_object_size_hint_weight_set(app_state->alertpopup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_show(app_state->alertpopup);

	__COMMON_FUNC_EXIT__;

	return TRUE;
}

int view_alerts_connection_fail_timeout_show(void)
{
	__COMMON_FUNC_ENTER__;

	if (WIFI_SYSPOPUP_SUPPORT_QUICKPANEL == app_state->wifi_syspopup_support) {
		__COMMON_FUNC_EXIT__;

		return TRUE;
	}

	if (NULL != app_state->alertpopup) {
		evas_object_del(app_state->alertpopup);
		app_state->alertpopup = NULL;
	}

	app_state->alertpopup = elm_popup_add(app_state->win_main);
	elm_object_text_set(app_state->alertpopup, "No response from AP");
	elm_popup_timeout_set(app_state->alertpopup, 2.0f);
	evas_object_smart_callback_add(app_state->alertpopup, "timeout", _timeout_cb, NULL);

	evas_object_size_hint_weight_set(app_state->alertpopup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_show(app_state->alertpopup);

	__COMMON_FUNC_EXIT__;

	return TRUE;
}

int view_alerts_password_length_error_show(void)
{
	__COMMON_FUNC_ENTER__;

	if (WIFI_SYSPOPUP_SUPPORT_QUICKPANEL == app_state->wifi_syspopup_support) {
		__COMMON_FUNC_EXIT__;

		return TRUE;
	}

	if (NULL != app_state->alertpopup) {
		evas_object_del(app_state->alertpopup);
		app_state->alertpopup = NULL;
	}

	app_state->alertpopup = elm_popup_add(app_state->win_main);
	elm_object_text_set(app_state->alertpopup, "WPA2 requires 8 - 63 letters for a password.<br>Please, check your input.");
	elm_popup_timeout_set(app_state->alertpopup, 2.0f);
	evas_object_smart_callback_add(app_state->alertpopup, "timeout", _timeout_cb, NULL);

	evas_object_size_hint_weight_set(app_state->alertpopup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_show(app_state->alertpopup);

	__COMMON_FUNC_EXIT__;

	return TRUE;
}
