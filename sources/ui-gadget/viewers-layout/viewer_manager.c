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

#include <vconf-keys.h>

#include "common.h"
#include "ug_wifi.h"
#include "viewer_list.h"
#include "i18nmanager.h"
#include "common_utils.h"
#include "winset_popup.h"
#include "viewer_manager.h"
#include "view_ime_hidden.h"

typedef struct {
	wlan_security_mode_type_t sec_mode;
	char *ssid;
	Evas_Object *confirmation_popup;
} hidden_ap_data_t;

typedef struct viewer_manager_object {
	Evas_Object *nav;
	Evas_Object *scan_button;
	Evas_Object *next_button;
	Evas_Object *prev_button;
	Evas_Object *list;

	Elm_Object_Item *item_hidden_btn;
	Elm_Object_Item *item_sep_above_hidden_button;
	Elm_Object_Item *item_sep_below_hidden_button;

	char *header_text;
	HEADER_MODES header_mode;
	Elm_Object_Item *item_header;
	Elm_Object_Item *item_bottom;
} viewer_manager_object;

typedef struct {
	Evas_Object *list;
	Elm_Object_Item *last_appended_item;
	int total_items_added;
} view_manager_list_update_info_t;

static viewer_manager_object *manager_object = NULL;

extern wifi_appdata *ug_app_state;

static Elm_Genlist_Item_Class header_itc_text;
static Elm_Genlist_Item_Class bottom_itc_text;
static Elm_Genlist_Item_Class bottom_itc_helper_text;
static Elm_Genlist_Item_Class hidden_button_itc;

void power_control(void)
{
	__COMMON_FUNC_ENTER__;

	int cur_state = -1;
	cur_state = viewer_manager_header_mode_get();

	INFO_LOG(UG_NAME_NORMAL, "current state %d\n", cur_state);

	int ret;

	switch (cur_state) {
	case HEADER_MODE_OFF:
	case HEADER_MODE_ACTIVATING:
		ret = wlan_manager_power_on();
		switch (ret){
		case WLAN_MANAGER_ERR_NONE:
			viewer_manager_show(VIEWER_WINSET_SEARCHING);
			viewer_manager_header_mode_set(HEADER_MODE_ACTIVATING);
			break;

		case WLAN_MANAGER_ERR_WIFI_TETHERING_OCCUPIED:
			winset_popup_mode_set(ug_app_state->popup_manager,
					POPUP_OPTION_POWER_ON_FAILED_TETHERING_OCCUPIED, NULL);
			break;

		case WLAN_MANAGER_ERR_IN_PROGRESS:
			/* Do nothing */
			break;

		default:
			viewer_manager_header_mode_set(HEADER_MODE_OFF);
			INFO_LOG(UG_NAME_NORMAL, "power on failed. ret = %d", ret);
			break;
		}

		break;

	case HEADER_MODE_ON:
	case HEADER_MODE_CONNECTING:
	case HEADER_MODE_CONNECTED:
	case HEADER_MODE_SEARCHING:
		viewer_list_item_clear();

		ret = wlan_manager_power_off();
		switch (ret) {
		case WLAN_MANAGER_ERR_NONE:
			viewer_manager_show(VIEWER_WINSET_SEARCHING);
			viewer_manager_hide(VIEWER_WINSET_SUB_CONTENTS);
			viewer_manager_header_mode_set(HEADER_MODE_DEACTIVATING);

			// Lets ignore all the scan updates because we are powering off now.
			wlan_manager_disable_scan_result_update();
			break;

		case WLAN_MANAGER_ERR_IN_PROGRESS:
			/* Do nothing */
			break;

		default:
			INFO_LOG(UG_NAME_NORMAL, "power off failed. ret = %d", ret);
			break;
		}

		break;

	case HEADER_MODE_DEACTIVATING:
	default:
		INFO_LOG(UG_NAME_NORMAL, "Powering off in progress. Let it complete. \n");
		break;
	}

	__COMMON_FUNC_EXIT__;
}

static void _hide_finished_cb(void *data, Evas_Object *obj, void *event_info)
{
	__COMMON_FUNC_ENTER__;

	view_manager_view_type_t top_view_id;

	top_view_id = (view_manager_view_type_t)evas_object_data_get(
												obj, SCREEN_TYPE_ID_KEY);

	if (data == elm_naviframe_top_item_get(obj)) {
		/* We are now in main view */
		evas_object_data_set(obj, SCREEN_TYPE_ID_KEY,
								(void *)VIEW_MANAGER_VIEW_TYPE_MAIN);
		top_view_id = VIEW_MANAGER_VIEW_TYPE_MAIN;
	}

	INFO_LOG(UG_NAME_NORMAL, "top view id = %d", top_view_id);

	switch(top_view_id) {
	case VIEW_MANAGER_VIEW_TYPE_MAIN:
		ug_app_state->eap_view = NULL;

		/* Lets enable the scan updates */
		wlan_manager_enable_scan_result_update();
		break;

	case VIEW_MANAGER_VIEW_TYPE_DETAIL:
	case VIEW_MANAGER_VIEW_TYPE_EAP:
	default:
		/* Lets disable the scan updates so that the UI is not refreshed */
		wlan_manager_disable_scan_result_update();
		break;
	}

	__COMMON_FUNC_EXIT__;
}

static void _lbutton_click_cb(void *data, Evas_Object *obj, void *event_info)
{
	__COMMON_FUNC_ENTER__;

	service_h service;
	int ret;

	ret = service_create(&service);
	if (ret != SERVICE_ERROR_NONE) {
		INFO_LOG(UG_NAME_ERR, "service_create failed: %d", ret);
		return;
	}

	service_add_extra_data(service, "result", "lbutton_click");
	ug_send_result(ug_app_state->ug, service);

	service_destroy(service);
	ug_destroy_me(ug_app_state->ug);

	__COMMON_FUNC_EXIT__;
}

static void _rbutton_click_cb(void *data, Evas_Object *obj, void *event_info)
{
	__COMMON_FUNC_ENTER__;

	service_h service;
	int ret;

	ret = service_create(&service);
	if (ret != SERVICE_ERROR_NONE) {
		INFO_LOG(UG_NAME_ERR, "service_create failed: %d", ret);
		return;
	}

	service_add_extra_data(service, "result", "rbutton_click");
	ug_send_result(ug_app_state->ug, service);

	service_destroy(service);
	ug_destroy_me(ug_app_state->ug);

	__COMMON_FUNC_EXIT__;
}

static void _back_sk_cb(void *data, Evas_Object *obj, void *event_info)
{
	__COMMON_FUNC_ENTER__;

	if (ug_app_state->bAlive == EINA_FALSE)
		return;

	wifi_exit();

	__COMMON_FUNC_EXIT__;
}

static void __refresh_scan_callback(void *data,
		Evas_Object *obj, void *event_info)
{
	__COMMON_FUNC_ENTER__;

	int scan_result;
	HEADER_MODES current_state;
	current_state = viewer_manager_header_mode_get();

	switch (current_state) {
	case HEADER_MODE_DEACTIVATING:
	case HEADER_MODE_OFF:
		power_control();
		break;

	case HEADER_MODE_ON:
	case HEADER_MODE_CONNECTED:
		viewer_manager_show(VIEWER_WINSET_SEARCHING);
		viewer_manager_header_mode_set(HEADER_MODE_SEARCHING);

		scan_result = wlan_manager_scan();
		if (scan_result != WLAN_MANAGER_ERR_NONE) {
			viewer_manager_hide(VIEWER_WINSET_SEARCHING);
			viewer_manager_header_mode_set(current_state);
		}
		break;

	default:
		break;
	}

	__COMMON_FUNC_EXIT__;
}

static char *_gl_header_text_get(void *data, Evas_Object *obj, const char *part)
{
	char* det = NULL;

	__COMMON_FUNC_ENTER__;

	retvm_if(NULL == manager_object, NULL);

	if (!strncmp(part, "elm.text", strlen(part))) {
		det = g_strdup(manager_object->header_text);
		assertm_if(NULL == det, "NULL!!");
	}

	__COMMON_FUNC_EXIT__;
	return det;
}

static void __gl_power_onoff_cb(void *data, Evas_Object *obj, void *event_info)
{
	__COMMON_FUNC_ENTER__;

	Elm_Object_Item *item;
	const char *object_type;
	HEADER_MODES current_mode;

	item = (Elm_Object_Item *)event_info;
	object_type = evas_object_type_get(obj);
	current_mode = viewer_manager_header_mode_get();

	if (current_mode == HEADER_MODE_ACTIVATING ||
					current_mode == HEADER_MODE_DEACTIVATING) {
		elm_genlist_item_selected_set(item, EINA_FALSE);
		__COMMON_FUNC_EXIT__;
		return;
	}

	if (g_strcmp0(object_type, "elm_check") == 0) {
		Eina_Bool check_mode = elm_check_state_get(obj);

		if (check_mode == TRUE && current_mode == HEADER_MODE_OFF)
			power_control();
		else if (check_mode != TRUE && current_mode != HEADER_MODE_OFF)
			power_control();
	} else if (g_strcmp0(object_type, "elm_genlist") == 0) {
		power_control();
	}

	if (item) {
		elm_genlist_item_update(item);
		elm_genlist_item_selected_set(item, EINA_FALSE);
	}

	__COMMON_FUNC_EXIT__;
}

static Evas_Object *_gl_header_content_get(void *data,
		Evas_Object *obj, const char *part)
{
	__COMMON_FUNC_ENTER__;

	if (manager_object == NULL)
		return NULL;

	Evas_Object *icon = NULL;
	Evas_Object *ao = NULL;

	if (!strncmp(part, "elm.icon", strlen(part))) {
		switch (manager_object->header_mode) {
		case HEADER_MODE_OFF:
			/* Wi-Fi off indication button */
			icon = elm_check_add(obj);
			elm_object_style_set(icon, "on&off");
			evas_object_propagate_events_set(icon, EINA_FALSE);
			elm_check_state_set(icon, EINA_FALSE);
			evas_object_smart_callback_add(icon, "changed",
					__gl_power_onoff_cb, NULL);
			evas_object_show(icon);

			ao = elm_object_item_access_object_get(manager_object->item_header);
			elm_access_info_set(ao, ELM_ACCESS_TYPE, "on/off button");
			elm_access_info_set(ao, ELM_ACCESS_STATE, "off");
			break;

		case HEADER_MODE_ACTIVATING:
		case HEADER_MODE_DEACTIVATING:
			/* Progress animation */
			icon = elm_progressbar_add(obj);
			elm_object_style_set(icon, "list_process");
			evas_object_size_hint_align_set(icon, EVAS_HINT_FILL, 0.5);
			evas_object_size_hint_weight_set(icon,
					EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
			elm_progressbar_pulse(icon, TRUE);
			break;

		default:
			/* Wi-Fi on indication button */
			icon = elm_check_add(obj);
			elm_object_style_set(icon, "on&off");
			evas_object_propagate_events_set(icon, EINA_FALSE);
			elm_check_state_set(icon, EINA_TRUE);
			evas_object_smart_callback_add(icon, "changed",
					__gl_power_onoff_cb, NULL);
			evas_object_show(icon);

			ao = elm_object_item_access_object_get(manager_object->item_header);
			elm_access_info_set(ao, ELM_ACCESS_TYPE, "on/off button");
			elm_access_info_set(ao, ELM_ACCESS_STATE, "on");
			break;
		}
	}

	__COMMON_FUNC_EXIT__;
	return icon;
}

static char *_gl_bottom_text_get(void *data, Evas_Object *obj, const char *part)
{
	__COMMON_FUNC_ENTER__;

	char *det = NULL;

	if (!strncmp(part, "elm.text", strlen(part))) {
		det = g_strdup(sc(PACKAGE, I18N_TYPE_Network_notification));
		assertm_if(NULL == det, "NULL!!");
	}

	__COMMON_FUNC_EXIT__;
	return det;
}

static char *_gl_bottom_helper_text_get(void *data, Evas_Object *obj, const char *part)
{
	__COMMON_FUNC_ENTER__;

	char *det = NULL;

	det = g_strdup(sc(PACKAGE, I18N_TYPE_Network_notify_me_later));

	__COMMON_FUNC_EXIT__;
	return det;
}

static void __gl_net_notification_onoff_cb(void *data,
		Evas_Object *obj, void *event_info)
{
	__COMMON_FUNC_ENTER__;

	int value;
	Elm_Object_Item *item;
	const char *object_type;

	item = (Elm_Object_Item *)event_info;
	object_type = evas_object_type_get(obj);

	value = common_util_get_system_registry(VCONFKEY_WIFI_ENABLE_QS);

	if (g_strcmp0(object_type, "elm_check") == 0) {
		Eina_Bool check_enable = elm_check_state_get(obj);

		common_util_set_system_registry(VCONFKEY_WIFI_ENABLE_QS,
										(int)check_enable);
	} else if (g_strcmp0(object_type, "elm_genlist") == 0) {
		if (value == VCONFKEY_WIFI_QS_ENABLE)
			value = VCONFKEY_WIFI_QS_DISABLE;
		else
			value = VCONFKEY_WIFI_QS_ENABLE;

		common_util_set_system_registry(VCONFKEY_WIFI_ENABLE_QS, value);
	}

	elm_genlist_item_update(item);
	elm_genlist_item_selected_set(item, EINA_FALSE);

	__COMMON_FUNC_EXIT__;
}

static Evas_Object *_gl_bottom_content_get(void *data,
		Evas_Object *obj, const char *part)
{
	__COMMON_FUNC_ENTER__;

	int ret;

	if (manager_object == NULL || obj == NULL)
		return NULL;

	Evas_Object *ao = NULL;
	Evas_Object *toggle_btn = elm_check_add(obj);
	retvm_if(NULL == toggle_btn, NULL);

	elm_object_style_set(toggle_btn, "on&off");
	evas_object_propagate_events_set(toggle_btn, EINA_FALSE);

	ret = common_util_get_system_registry(VCONFKEY_WIFI_ENABLE_QS);
	switch (ret) {
	case 1:
		ao = elm_object_item_access_object_get(manager_object->item_bottom);
		elm_access_info_set(ao, ELM_ACCESS_TYPE, "on/off button");
		elm_access_info_set(ao, ELM_ACCESS_STATE, "on");
		elm_check_state_set(toggle_btn, EINA_TRUE);
		break;
	case 0:
		ao = elm_object_item_access_object_get(manager_object->item_bottom);
		elm_access_info_set(ao, ELM_ACCESS_TYPE, "on/off button");
		elm_access_info_set(ao, ELM_ACCESS_STATE, "off");
		elm_check_state_set(toggle_btn, EINA_FALSE);
		break;
	default:
		ERROR_LOG(COMMON_NAME_ERR, "Setting fail!!");
		break;
	}

	evas_object_smart_callback_add(toggle_btn, "changed",
			__gl_net_notification_onoff_cb, NULL);

	__COMMON_FUNC_EXIT__;
	return toggle_btn;
}

static void _hidden_button_callback(void* data, Evas_Object* obj, void* event_info)
{
	__COMMON_FUNC_ENTER__;

	ug_app_state->hidden_ap_popup = view_hidden_ap_popup_create(ug_app_state->layout_main, PACKAGE);

	__COMMON_FUNC_EXIT__;
}

static Evas_Object *_gl_hidden_btn_content_get(void *data, Evas_Object *obj, const char *part)
{
	Evas_Object *find_hidden_ap_btn = elm_button_add(obj);
	evas_object_smart_callback_add(find_hidden_ap_btn, "clicked", _hidden_button_callback, NULL);
	evas_object_propagate_events_set(find_hidden_ap_btn, EINA_FALSE);
	elm_object_style_set(find_hidden_ap_btn, "style2");
	elm_object_text_set(find_hidden_ap_btn, sc(PACKAGE, I18N_TYPE_Find_Hidden_Network));

	switch (viewer_manager_header_mode_get()) {
	case HEADER_MODE_ACTIVATING:
	case HEADER_MODE_CONNECTING:
	case HEADER_MODE_DEACTIVATING:
	case HEADER_MODE_SEARCHING:
		elm_object_disabled_set(find_hidden_ap_btn, EINA_TRUE);
		break;
	default:
		elm_object_disabled_set(find_hidden_ap_btn, EINA_FALSE);
		break;
	}

	return find_hidden_ap_btn;
}

static void __viewer_manager_header_create(Evas_Object* genlist)
{
	__COMMON_FUNC_ENTER__;

	manager_object->header_text = g_strdup(sc(PACKAGE, I18N_TYPE_Wi_Fi));

	header_itc_text.item_style = "dialogue/1text.1icon";
	header_itc_text.func.text_get = _gl_header_text_get;
	header_itc_text.func.content_get = _gl_header_content_get;
	header_itc_text.func.state_get = NULL;
	header_itc_text.func.del = NULL;

	common_utils_add_dialogue_separator(genlist, "dialogue/separator");

	assertm_if(NULL != manager_object->item_header, "Header item already set");
	manager_object->item_header = elm_genlist_item_append(genlist,
			&header_itc_text, NULL, NULL, ELM_GENLIST_ITEM_NONE,
			__gl_power_onoff_cb, NULL);
	assertm_if(NULL == manager_object->item_header, "Header item NULL");

	common_utils_add_dialogue_separator(genlist, "dialogue/separator");

	__COMMON_FUNC_EXIT__;
}

static void __viewer_manager_bottom_create(Evas_Object* genlist)
{
	__COMMON_FUNC_ENTER__;

	assertm_if(NULL == genlist, "NULL!!");

	bottom_itc_text.item_style = "dialogue/1text.1icon";
	bottom_itc_text.func.text_get = _gl_bottom_text_get;
	bottom_itc_text.func.content_get = _gl_bottom_content_get;
	bottom_itc_text.func.state_get = NULL;
	bottom_itc_text.func.del = NULL;

	bottom_itc_helper_text.item_style = "multiline/1text";
	bottom_itc_helper_text.func.text_get = _gl_bottom_helper_text_get;
	bottom_itc_helper_text.func.content_get = NULL;
	bottom_itc_helper_text.func.state_get = NULL;
	bottom_itc_helper_text.func.del = NULL;

	manager_object->item_bottom = elm_genlist_item_append(genlist, &bottom_itc_text, NULL, NULL,
			ELM_GENLIST_ITEM_NONE, __gl_net_notification_onoff_cb, NULL);

	elm_genlist_mode_set(genlist, ELM_LIST_COMPRESS);
	Elm_Object_Item *item = elm_genlist_item_append(genlist,
			&bottom_itc_helper_text, NULL, NULL,
			ELM_GENLIST_ITEM_NONE, NULL, NULL);
	elm_genlist_item_select_mode_set(item, ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);

	common_utils_add_dialogue_separator(genlist, "dialogue/separator");

	__COMMON_FUNC_EXIT__;
}

static int viewer_manager_hidden_button_create(Evas_Object* genlist)
{
	__COMMON_FUNC_ENTER__;

	if (NULL != manager_object->item_sep_above_hidden_button ||
		NULL != manager_object->item_sep_below_hidden_button ||
		NULL != manager_object->item_hidden_btn) {

		__COMMON_FUNC_EXIT__;
		return FALSE;
	}

	assertm_if(NULL == genlist, "NULL!!");

	hidden_button_itc.item_style = "1icon";
	hidden_button_itc.func.text_get = NULL;
	hidden_button_itc.func.content_get = _gl_hidden_btn_content_get;
	hidden_button_itc.func.state_get = NULL;
	hidden_button_itc.func.del = NULL;

	manager_object->item_sep_above_hidden_button = common_utils_add_dialogue_separator(genlist, "dialogue/separator");

	manager_object->item_hidden_btn = elm_genlist_item_append(genlist, &hidden_button_itc, NULL, NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);
	assertm_if(NULL == manager_object->item_hidden_btn, "Hidden button is NULL");

	manager_object->item_sep_below_hidden_button = common_utils_add_dialogue_separator(genlist, "dialogue/separator");

	__COMMON_FUNC_EXIT__;
	return TRUE;
}

static Eina_Bool viewer_manager_scan_button_set(Eina_Bool show_state)
{
	__COMMON_FUNC_ENTER__;

	if (NULL == manager_object) {
		__COMMON_FUNC_EXIT__;
		return EINA_FALSE;
	}

	/* TODO: need to check VIEW_MANAGER_VIEW_TYPE_MAIN ?
	 * Evas_Object* navi_frame = viewer_manager_get_naviframe();
	 * view_manager_view_type_t top_view_id =
	 *			(view_manager_view_type_t)evas_object_data_get(navi_frame,
	 *											SCREEN_TYPE_ID_KEY);
	 */

	if (show_state == EINA_TRUE)
		elm_object_disabled_set(manager_object->scan_button, EINA_FALSE);
	else if (show_state == EINA_FALSE)
		elm_object_disabled_set(manager_object->scan_button, EINA_TRUE);

	__COMMON_FUNC_EXIT__;
	return EINA_TRUE;
}

Evas_Object* viewer_manager_create(Evas_Object* _parent)
{
	__COMMON_FUNC_ENTER__;

	retvm_if(NULL != manager_object || NULL == _parent, NULL);

	manager_object = g_new0(viewer_manager_object, 1);
	retvm_if(NULL == manager_object, NULL);

	/* Add Full Layout */
	Evas_Object *layout = elm_layout_add(_parent);
	elm_layout_theme_set(layout, "layout", "application", "default");
	evas_object_size_hint_weight_set(layout,
			EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	edje_object_signal_emit(elm_layout_edje_get(layout),
			"elm,state,show,content", "elm");
	edje_object_signal_emit(elm_layout_edje_get(layout),
			"elm,bg,show,group_list", "elm");
	edje_object_signal_emit(elm_layout_edje_get(layout),
			"elm,state,show,indicator", "elm");

	/* Add Naviframe */
	manager_object->nav = elm_naviframe_add(layout);
	assertm_if(NULL == manager_object->nav, "manager_object->nav is NULL");
	elm_object_part_content_set(layout,
			"elm.swallow.content", manager_object->nav);

	/* Add MainView Layout */
	Evas_Object* view_content = elm_layout_add(manager_object->nav);
	elm_layout_theme_set(view_content, "standard", "window", "integration");
	edje_object_signal_emit(elm_layout_edje_get(view_content),
			"elm,state,show,content", "elm");
	edje_object_signal_emit(elm_layout_edje_get(view_content),
			"elm,bg,show,group_list", "elm");

	/* Add genlist */
	manager_object->list = viewer_list_create(view_content);
	assertm_if(NULL == manager_object->list, "manager_object->list is NULL");
	__viewer_manager_header_create(manager_object->list);
	__viewer_manager_bottom_create(manager_object->list);

	elm_object_part_content_set(view_content,
			"elm.swallow.content", manager_object->list);

	if (ug_app_state->ug_type == UG_VIEW_SETUP_WIZARD) {
		Elm_Object_Item* navi_it = elm_naviframe_item_push(manager_object->nav,
				sc(PACKAGE, I18N_TYPE_Wi_Fi), NULL, NULL, view_content, NULL);
		evas_object_data_set(manager_object->nav, SCREEN_TYPE_ID_KEY,
				(void *)VIEW_MANAGER_VIEW_TYPE_MAIN);
		evas_object_smart_callback_add(manager_object->nav,
				"transition,finished", _hide_finished_cb, navi_it);

		manager_object->prev_button = elm_button_add(manager_object->nav);
		elm_object_style_set(manager_object->prev_button,
				"naviframe/toolbar/default");
		elm_object_text_set(manager_object->prev_button,
				ug_app_state->lbutton_setup_wizard_prev);
		evas_object_smart_callback_add(manager_object->prev_button,
				"clicked", _lbutton_click_cb, NULL);
		elm_object_item_part_content_set(navi_it, "toolbar_button1",
				manager_object->prev_button);

		manager_object->next_button = elm_button_add(manager_object->nav);
		elm_object_style_set(manager_object->next_button,
				"naviframe/toolbar/default");
		elm_object_text_set(manager_object->next_button,
				ug_app_state->rbutton_setup_wizard_next);
		evas_object_smart_callback_add(manager_object->next_button,
				"clicked", _rbutton_click_cb, NULL);
		elm_object_item_part_content_set(navi_it, "toolbar_button2",
				manager_object->next_button);
	} else {
		Evas_Object*	back_btn = elm_button_add(manager_object->nav);
		elm_object_style_set(back_btn, "naviframe/back_btn/default");
		evas_object_smart_callback_add(back_btn, "clicked", _back_sk_cb, NULL);

		Elm_Object_Item* navi_it = elm_naviframe_item_push(manager_object->nav,
				sc(PACKAGE, I18N_TYPE_Wi_Fi), back_btn, NULL,
				view_content, NULL);
		evas_object_data_set(manager_object->nav, SCREEN_TYPE_ID_KEY,
				(void *)VIEW_MANAGER_VIEW_TYPE_MAIN);
		evas_object_smart_callback_add(manager_object->nav,
				"transition,finished", _hide_finished_cb, navi_it);

		manager_object->scan_button = elm_button_add(manager_object->nav);
		elm_object_style_set(manager_object->scan_button,
				"naviframe/toolbar/default");
		elm_object_text_set(manager_object->scan_button,
				sc(PACKAGE, I18N_TYPE_Scan));
		evas_object_smart_callback_add(manager_object->scan_button,
				"clicked", __refresh_scan_callback, NULL);
		elm_object_item_part_content_set(navi_it,
				"toolbar_button1", manager_object->scan_button);
	}

	evas_object_show(layout);

	__COMMON_FUNC_EXIT__;
	return layout;
}

Eina_Bool viewer_manager_destroy()
{
	__COMMON_FUNC_ENTER__;

	viewer_list_destroy();
	if (manager_object) {
		if (manager_object->header_text) {
			g_free(manager_object->header_text);
			manager_object->header_text = NULL;
		}
		g_free(manager_object);
		manager_object = NULL;
	}
	__COMMON_FUNC_EXIT__;
	return EINA_TRUE;
}

Eina_Bool viewer_manager_show(VIEWER_WINSETS winset)
{
	__COMMON_FUNC_ENTER__;

	assertm_if(NULL == manager_object, "Manager object is NULL");

	switch (winset) {
	case VIEWER_WINSET_SEARCHING:
		viewer_manager_scan_button_set(EINA_FALSE);
		viewer_list_item_disable_all();
		break;

	case VIEWER_WINSET_SUB_CONTENTS:
		assertm_if(NULL == manager_object->list, "List is NULL");

		viewer_list_title_item_set();
		viewer_manager_hidden_button_create(manager_object->list);
		break;

	default:
		break;
	}

	__COMMON_FUNC_EXIT__;
	return EINA_TRUE;
}

Eina_Bool viewer_manager_hide(VIEWER_WINSETS winset)
{
	__COMMON_FUNC_ENTER__;

	switch (winset) {
	case VIEWER_WINSET_SEARCHING:
		/* searching view */
		viewer_manager_scan_button_set(EINA_TRUE);
		viewer_list_item_enable_all();
		break;

	case VIEWER_WINSET_SUB_CONTENTS:
		/* hidden AP and WPS PBC */
		if (ug_app_state->passpopup) {
			passwd_popup_free(ug_app_state->passpopup);
			ug_app_state->passpopup = NULL;
		}

		if (ug_app_state->eap_view) {
			eap_connect_data_free(ug_app_state->eap_view);
			ug_app_state->eap_view = NULL;
		}

		viewer_list_title_item_del();

		elm_object_item_del(manager_object->item_sep_above_hidden_button);
		elm_object_item_del(manager_object->item_sep_below_hidden_button);
		elm_object_item_del(manager_object->item_hidden_btn);

		manager_object->item_sep_above_hidden_button = NULL;
		manager_object->item_sep_below_hidden_button = NULL;
		manager_object->item_hidden_btn = NULL;
		break;

	default:
		break;
	}

	__COMMON_FUNC_EXIT__;
	return EINA_TRUE;
}

#if 0
/* Unused function */
Eina_Bool viewer_manager_genlist_item_update(Elm_Object_Item* item)
{
	__COMMON_FUNC_ENTER__;
	if (item == NULL) {
		__COMMON_FUNC_EXIT__;
		return EINA_FALSE;
	}

	elm_genlist_item_update(item);

	__COMMON_FUNC_EXIT__;
	return EINA_FALSE;
}
#endif

void viewer_manager_update_hidden_btn(void)
{
	__COMMON_FUNC_ENTER__;
	retm_if(NULL == manager_object->item_hidden_btn);

	elm_genlist_item_update(manager_object->item_hidden_btn);

	__COMMON_FUNC_EXIT__;
}

static void viewer_manager_setup_wizard_button_controller(HEADER_MODES mode)
{
	switch (mode) {
	case HEADER_MODE_OFF:
	case HEADER_MODE_ON:
		if (manager_object->next_button != NULL && ug_app_state->rbutton_setup_wizard_skip != NULL)
			elm_object_text_set(manager_object->next_button, ug_app_state->rbutton_setup_wizard_skip);
		break;

	case HEADER_MODE_CONNECTED:
		if (manager_object->next_button != NULL && ug_app_state->rbutton_setup_wizard_next != NULL)
			elm_object_text_set(manager_object->next_button, ug_app_state->rbutton_setup_wizard_next);
		break;

	default:
		break;
	}
}

void viewer_manager_header_mode_set(HEADER_MODES new_mode)
{
	__COMMON_FUNC_ENTER__;

	HEADER_MODES old_mode;

	retm_if(NULL == manager_object);

	assertm_if(NULL == manager_object->item_header, "NULL!!");
	assertm_if(HEADER_MODE_OFF > new_mode ||
			HEADER_MODE_MAX <= new_mode, "Err!!");

	old_mode = manager_object->header_mode;
	if (old_mode == new_mode)
		return;

	DEBUG_LOG(UG_NAME_NORMAL, "Header mode %d --> %d", old_mode, new_mode);

	manager_object->header_mode = new_mode;

	if (manager_object->header_text) {
		g_free(manager_object->header_text);
		manager_object->header_text = NULL;
	}

	switch (new_mode) {
	case HEADER_MODE_OFF:
	case HEADER_MODE_ON:
	case HEADER_MODE_CONNECTED:
		viewer_manager_scan_button_set(EINA_TRUE);
		manager_object->header_text = g_strdup(sc(PACKAGE, I18N_TYPE_Wi_Fi));
		break;

	case HEADER_MODE_ACTIVATING:
		manager_object->header_text = g_strdup(sc(PACKAGE, I18N_TYPE_Activating_WiFi));
		break;

	case HEADER_MODE_DEACTIVATING:
		manager_object->header_text = g_strdup(sc(PACKAGE, I18N_TYPE_Deactivating));
		break;

	case HEADER_MODE_CONNECTING:
	case HEADER_MODE_SEARCHING:
		manager_object->header_text = g_strdup(sc(PACKAGE, I18N_TYPE_Wi_Fi));
		break;

	default:
		break;
	}

	viewer_manager_update_hidden_btn();
	elm_genlist_item_update(manager_object->item_header);
	viewer_list_title_item_update();

	if (ug_app_state->ug_type == UG_VIEW_SETUP_WIZARD)
		viewer_manager_setup_wizard_button_controller(new_mode);

	__COMMON_FUNC_EXIT__;
}

HEADER_MODES viewer_manager_header_mode_get(void)
{
	return manager_object->header_mode;
}

Evas_Object* viewer_manager_get_naviframe()
{
	return manager_object->nav;
}

void viewer_manager_refresh_ap_info(Elm_Object_Item *item)
{
	if (!item) {
		return;
	}

	ug_genlist_data_t *gdata = elm_object_item_data_get(item);
	if (!gdata) {
		return;
	}
	wifi_device_info_t *wifi_device = gdata->device_info;
	if (!wifi_device) {
		return;
	}

	wifi_ap_refresh(wifi_device->ap);

	return;
}

Elm_Object_Item *viewer_manager_move_item_to_top(Elm_Object_Item *old_item)
{
	__COMMON_FUNC_ENTER__;

	Elm_Object_Item *first_item = viewer_list_get_first_item();
	ug_genlist_data_t *gdata = NULL, *first_it_gdata = NULL;

	if (!old_item || !first_item) {
		__COMMON_FUNC_EXIT__;
		return NULL;
	}

	gdata = elm_object_item_data_get(old_item);
	if (!gdata || !gdata->device_info) {
		__COMMON_FUNC_EXIT__;
		return NULL;
	}

	if (old_item != first_item) {
		first_it_gdata = elm_object_item_data_get(first_item);
		elm_object_item_data_set(first_item, gdata);
		elm_object_item_data_set(old_item, first_it_gdata);

		elm_genlist_item_update(first_item);
		elm_genlist_item_update(old_item);
	}

	__COMMON_FUNC_EXIT__;
	return first_item;
}

void viewer_manager_update_rssi(void)
{
	int ret;
	wifi_ap_h ap;

	ret = wifi_get_connected_ap(&ap);
	if (WIFI_ERROR_NONE != ret)
		return;

	Elm_Object_Item *item = item_get_for_ap(ap);
	if (!item) {
		wifi_ap_destroy(ap);
		return;
	}

	ug_genlist_data_t* gdata = elm_object_item_data_get(item);
	if (gdata && gdata->device_info) {
		int rssi = 0;

		if (WIFI_ERROR_NONE != wifi_ap_get_rssi(ap, &rssi)) {
			wifi_ap_destroy(ap);
			return;
		} else if (gdata->device_info->rssi != rssi) {
			wifi_ap_refresh(gdata->device_info->ap);
			gdata->device_info->rssi = rssi;

			g_free(gdata->device_info->ap_image_path);

			common_utils_get_device_icon(WIFI_APP_IMAGE_DIR,
					gdata->device_info,
					&gdata->device_info->ap_image_path);

			elm_genlist_item_update(item);
		}
	}

	wifi_ap_destroy(ap);
}

static bool wifi_update_list_for_each_ap(wifi_ap_h ap, void *user_data)
{
	Elm_Object_Item *item;;
	view_manager_list_update_info_t *update_info;

	update_info = (view_manager_list_update_info_t *)user_data;

	item = viewer_list_item_insert_after(ap, update_info->last_appended_item);
	if (item) {
		update_info->last_appended_item = item;
		update_info->total_items_added++;
	}

	return true;
}

static void viewer_manager_update_list_all()
{
	Elm_Object_Item *item = NULL;

	__COMMON_FUNC_ENTER__;

	view_manager_list_update_info_t update_info;
	memset(&update_info, 0, sizeof(update_info));

	wifi_foreach_found_aps (wifi_update_list_for_each_ap, &update_info);
	DEBUG_LOG(UG_NAME_NORMAL, "total items added = %d", update_info.total_items_added);

	if (0 == update_info.total_items_added) {
		/* if there is no scan_data, generate No-AP item */
		item = viewer_list_item_insert_after(NULL, NULL);
		if (item) {
			elm_genlist_item_select_mode_set(item, ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);
		}
	}

	__COMMON_FUNC_EXIT__;
}

Eina_Bool viewer_manager_refresh(void)
{
	int state;

	INFO_LOG(UG_NAME_SCAN, "viewer manager refresh");

	if (manager_object == NULL)
		return EINA_FALSE;

	if (viewer_manager_header_mode_get() == HEADER_MODE_CONNECTING)
		return EINA_FALSE;

	/* Remove the list */
	viewer_list_item_disable_all();
	viewer_list_item_clear();

	state = wlan_manager_state_get();
	if (WLAN_MANAGER_ERROR == state || WLAN_MANAGER_OFF == state) {
		/* Some body requested to refresh the list
		 * while the WLAN manager is OFF or Unable to get the profile state
		 */
		INFO_LOG(UG_NAME_ERR, "WLAN Manager state: %d", state);

		viewer_manager_header_mode_set(HEADER_MODE_OFF);

		return EINA_FALSE;
	}

	wifi_ap_h ap = wlan_manager_get_ap_with_state(state);
	viewer_manager_update_list_all();

	if (WLAN_MANAGER_CONNECTING == state) {
		INFO_LOG(UG_NAME_NORMAL, "Wi-Fi is connecting");

		Elm_Object_Item* target_item = item_get_for_ap(ap);

		viewer_manager_header_mode_set(HEADER_MODE_CONNECTING);
		viewer_list_item_radio_mode_set(target_item,
								VIEWER_ITEM_RADIO_MODE_CONNECTING);
	} else if (WLAN_MANAGER_CONNECTED == state) {
		INFO_LOG(UG_NAME_NORMAL, "Wi-Fi is connected");

		Elm_Object_Item* target_item = item_get_for_ap(ap);

		target_item = viewer_manager_move_item_to_top(target_item);
		viewer_manager_header_mode_set(HEADER_MODE_CONNECTED);
		viewer_list_item_radio_mode_set(target_item,
								VIEWER_ITEM_RADIO_MODE_CONNECTED);
	} else {
		INFO_LOG(UG_NAME_NORMAL, "Wi-Fi state: %d", state);

		viewer_manager_header_mode_set(HEADER_MODE_ON);
	}
	wifi_ap_destroy(ap);

	INFO_LOG(UG_NAME_SCAN, "viewer manager refresh finished");

	return EINA_TRUE;
}

static void hidden_ap_connect_ok_cb (void *data,
		Evas_Object *obj, void *event_info)
{
	__COMMON_FUNC_ENTER__;

	hidden_ap_data_t *hidden_ap_data = (hidden_ap_data_t *)data;
	if (hidden_ap_data == NULL) {
		__COMMON_FUNC_EXIT__;
		return;
	}

	char* szPassword = NULL;
	wifi_ap_h ap;

	int ret = wifi_ap_create(hidden_ap_data->ssid, &ap);
	if (WIFI_ERROR_NONE != ret) {
		ERROR_LOG(UG_NAME_ERR, "Failed to create an AP handle. Err = %d", ret);

		__COMMON_FUNC_EXIT__;
		return;
	}

	INFO_LOG(UG_NAME_NORMAL, "Hidden AP[%s]. Sec mode = %d. Connect ok cb",
			hidden_ap_data->ssid, hidden_ap_data->sec_mode);

	switch (hidden_ap_data->sec_mode) {
	case WLAN_SEC_MODE_NONE:
		INFO_LOG(UG_NAME_NORMAL, "OPEN: event %x; passpopup %x",
				event_info, ug_app_state->passpopup);

		wifi_ap_set_security_type(ap, WIFI_SECURITY_TYPE_NONE);
		evas_object_del(hidden_ap_data->confirmation_popup);
		hidden_ap_data->confirmation_popup = NULL;
		break;

	case WLAN_SEC_MODE_WEP:
	case WLAN_SEC_MODE_WPA_PSK:
	case WLAN_SEC_MODE_WPA2_PSK:
		szPassword = passwd_popup_get_txt(ug_app_state->passpopup);

		if (WLAN_SEC_MODE_WEP == hidden_ap_data->sec_mode)
			wifi_ap_set_security_type(ap, WIFI_SECURITY_TYPE_WEP);
		else if (WLAN_SEC_MODE_WPA_PSK == hidden_ap_data->sec_mode)
			wifi_ap_set_security_type(ap, WIFI_SECURITY_TYPE_WPA_PSK);
		else
			wifi_ap_set_security_type(ap, WIFI_SECURITY_TYPE_WPA2_PSK);

		wifi_ap_set_passphrase(ap, szPassword);
		g_free(szPassword);

		passwd_popup_free(ug_app_state->passpopup);
		ug_app_state->passpopup = NULL;
		break;

	default:
		INFO_LOG(UG_NAME_NORMAL, "Fatal: Unknown Sec mode: %d",
				hidden_ap_data->sec_mode);

		goto hidden_ap_connect_end;
	}

	wlan_manager_connect(ap);

hidden_ap_connect_end:
	wifi_ap_destroy(ap);

	g_free(hidden_ap_data->ssid);
	g_free(hidden_ap_data);

	__COMMON_FUNC_EXIT__;
}

static void hidden_ap_connect_cacel_cb(void *data, Evas_Object *obj,
		void *event_info)
{
	__COMMON_FUNC_ENTER__;

	hidden_ap_data_t *hidden_ap_data = (hidden_ap_data_t *)data;
	if (hidden_ap_data == NULL) {
		__COMMON_FUNC_EXIT__;
		return;
	}

	switch (hidden_ap_data->sec_mode) {
	case WLAN_SEC_MODE_NONE:
		INFO_LOG(UG_NAME_NORMAL, "This hidden AP is Open");

		evas_object_del(hidden_ap_data->confirmation_popup);
		hidden_ap_data->confirmation_popup = NULL;
		break;

	case WLAN_SEC_MODE_WEP:
	case WLAN_SEC_MODE_WPA_PSK:
	case WLAN_SEC_MODE_WPA2_PSK:
		INFO_LOG(UG_NAME_NORMAL, "Hidden AP Secured");

		passwd_popup_free(ug_app_state->passpopup);
		ug_app_state->passpopup = NULL;
		break;

	default:
		INFO_LOG(UG_NAME_NORMAL, "Fatal: Unknown Sec mode: %d",
				hidden_ap_data->sec_mode);
		break;
	}

	g_free(hidden_ap_data->ssid);
	g_free(hidden_ap_data);

	__COMMON_FUNC_EXIT__;
	return;
}

void viewer_manager_specific_scan_response_hlr(
		GSList *bss_info_list, void *user_data)
{
	hidden_ap_data_t *hidden_ap_data = NULL;
	const char *ssid = (const char *)user_data;
	wlan_security_mode_type_t sec_mode;

	if (ug_app_state->hidden_ap_popup == NULL) {
		ERROR_LOG(UG_NAME_RESP, "Popup is already destroyed \n");

		g_free(user_data);
		return;
	}

	if (ssid == NULL) {
		ERROR_LOG(UG_NAME_RESP, "SSID is empty \n");

		view_hidden_ap_popup_destroy(ug_app_state->hidden_ap_popup);
		ug_app_state->hidden_ap_popup = NULL;

		return;
	}

	int ap_count = g_slist_length(bss_info_list);
	net_wifi_connection_info_t *bss_info = NULL;

	INFO_LOG(UG_NAME_RESP, "Find %s(%d) hidden Wi-Fi networks", ssid, ap_count);

	if (ap_count == 1) {
		bss_info = g_slist_nth_data(bss_info_list, 0);

		if (bss_info == NULL || g_strcmp0(ssid, bss_info->essid) != 0) {
			INFO_LOG(UG_NAME_RESP,
					"Bss info is NULL OR response received for wrong ssid",
					ssid);

			/* Bss info not available or Response recieved for wrong ssid */
			ap_count = 0;
		} else
			sec_mode = bss_info->security_info.sec_mode;
	}

	if (ap_count == 1) {
		/* Only if there is one AP found then we need Users further action */

		switch (sec_mode) {
		case WLAN_SEC_MODE_NONE:
			INFO_LOG(UG_NAME_NORMAL, "%s open network found", ssid);

			hidden_ap_data = g_new0(hidden_ap_data_t, 1);
			hidden_ap_data->sec_mode = WLAN_SEC_MODE_NONE;
			hidden_ap_data->ssid = g_strdup(ssid);

			popup_btn_info_t popup_btn_data;
			memset(&popup_btn_data, 0, sizeof(popup_btn_data));
			popup_btn_data.info_txt = OPEN_HIDDEN_NETWORK_STR;
			popup_btn_data.btn1_cb = hidden_ap_connect_ok_cb;
			popup_btn_data.btn2_cb = hidden_ap_connect_cacel_cb;
			popup_btn_data.btn2_data = popup_btn_data.btn1_data = hidden_ap_data;
			popup_btn_data.btn1_txt = sc(PACKAGE, I18N_TYPE_Connect);
			popup_btn_data.btn2_txt = sc(PACKAGE, I18N_TYPE_Cancel);
			hidden_ap_data->confirmation_popup =
					common_utils_show_info_popup(ug_app_state->layout_main,
							&popup_btn_data);
			break;

		case WLAN_SEC_MODE_IEEE8021X:
			INFO_LOG(UG_NAME_NORMAL, "%s IEEE8021X found", ssid);

			Evas_Object* navi_frame = viewer_manager_get_naviframe();
			wifi_device_info_t device_info;
			wifi_ap_h ap;

			wifi_ap_create(ssid, &ap);
			wifi_ap_set_security_type(ap, WIFI_SECURITY_TYPE_EAP);

			memset(&device_info, 0, sizeof(device_info));
			device_info.security_mode = WIFI_SECURITY_TYPE_EAP;
			device_info.ssid = (char *)ssid;
			device_info.ap = ap;
			ug_app_state->eap_view =
					create_eap_view(ug_app_state->layout_main, navi_frame,
							PACKAGE, &device_info);

			wifi_ap_destroy(ap);
			break;

		case WLAN_SEC_MODE_WEP:
		case WLAN_SEC_MODE_WPA_PSK:
		case WLAN_SEC_MODE_WPA2_PSK:
			INFO_LOG(UG_NAME_NORMAL, "Secured(%d) %s found", sec_mode, ssid);

			pswd_popup_create_req_data_t popup_info;

			hidden_ap_data = g_new0(hidden_ap_data_t, 1);
			hidden_ap_data->sec_mode = sec_mode;
			hidden_ap_data->ssid = g_strdup(ssid);

			memset(&popup_info, 0, sizeof(pswd_popup_create_req_data_t));
			popup_info.title = (char *)ssid;
			popup_info.ok_cb = hidden_ap_connect_ok_cb;
			popup_info.cancel_cb = hidden_ap_connect_cacel_cb;
			popup_info.show_wps_btn = FALSE;
			popup_info.wps_btn_cb = NULL;
			popup_info.cb_data = hidden_ap_data;
			popup_info.ap = NULL;

			ug_app_state->passpopup =
					create_passwd_popup(ug_app_state->layout_main,
							PACKAGE, &popup_info);

			if (ug_app_state->passpopup == NULL)
				INFO_LOG(UG_NAME_ERR, "Fail to create password popup");

			break;

		default:
			INFO_LOG(UG_NAME_NORMAL, "Unknown security mode: %d", sec_mode);
			break;
		}
	} else if (ap_count == 0) {
		char *disp_msg = g_strdup_printf("Unable to find %s", ssid);
		common_utils_show_info_ok_popup(ug_app_state->layout_main, PACKAGE, disp_msg);
		g_free(disp_msg);
	}

	/* If the hidden AP found on first and
	 * second scan OR not found even after first and second scan
	 * then delete the popup
	 */
	g_free(user_data);

	view_hidden_ap_popup_destroy(ug_app_state->hidden_ap_popup);
	ug_app_state->hidden_ap_popup = NULL;
}
