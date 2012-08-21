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
#include "viewer_manager.h"
#include "viewer_list.h"
#include "i18nmanager.h"
#include "wifi-setting.h"
#include "view_ime_hidden.h"
#include "wlan_manager.h"
#include "wifi-ui-list-callbacks.h"
#include "winset_popup.h"
#include "common_utils.h"
#include "common_datamodel.h"

#define LIST_ITEM_CONNECTED_AP_FONT_SIZE	28
#define LIST_ITEM_CONNECTED_AP_FONT_COLOR	"#00FFFF"

typedef struct {
	wlan_security_mode_type_t sec_mode;
	char *ssid;
	Evas_Object *confirmation_popup;
} hidden_ap_data_t;

typedef struct viewer_manager_object {
	Evas_Object* nav;
	Evas_Object* scan_button;
	Evas_Object* next_button;
	Evas_Object* prev_button;
	Evas_Object* list;

	Elm_Object_Item *item_hidden_btn;
	Elm_Object_Item *item_sep_above_hidden_button;
	Elm_Object_Item *item_sep_below_hidden_button;

	char* header_text;
	HEADER_MODES header_mode;
	Elm_Object_Item* item_header;
	Elm_Object_Item* item_bottom;
	Elm_Object_Item* item_bottom_helper_txt;
} viewer_manager_object;
static viewer_manager_object* manager_object = NULL;

extern wifi_appdata *ug_app_state;

static Elm_Genlist_Item_Class header_itc_text;
static Elm_Genlist_Item_Class bottom_itc;
static Elm_Genlist_Item_Class bottom_itc_text;
static Elm_Genlist_Item_Class bottom_itc_helper_text;
static Elm_Genlist_Item_Class hidden_button_itc;

static char *viewer_manager_get_device_status_txt(wifi_device_info_t *wifi_device, VIEWER_ITEM_RADIO_MODES mode);

int power_control()
{
	__COMMON_FUNC_ENTER__;

	int cur_state = -1;
	cur_state = viewer_manager_header_mode_get();

	INFO_LOG(UG_NAME_NORMAL, "current state %d\n", cur_state);

	int ret = TRUE;

	switch (cur_state) {
	case HEADER_MODE_OFF:
	case HEADER_MODE_ACTIVATING:

		INFO_LOG(UG_NAME_NORMAL, "wifi state power off/powering off");

		ret = wlan_manager_request_power_on();
		switch (ret){
			case WLAN_MANAGER_ERR_NONE:
				INFO_LOG(UG_NAME_NORMAL, "power on ok");
				viewer_manager_header_mode_set(HEADER_MODE_ACTIVATING);
				break;
			case WLAN_MANAGER_ERR_MOBILE_HOTSPOT_OCCUPIED:
				viewer_manager_header_mode_set(HEADER_MODE_ACTIVATING);
				winset_popup_mode_set(ug_app_state->popup_manager, POPUP_OPTION_POWER_ON_FAILED_MOBILE_HOTSPOT, NULL);
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
	case HEADER_MODE_DISCONNECTING:
	case HEADER_MODE_CANCEL_CONNECTING:
	case HEADER_MODE_CONNECTED:
	case HEADER_MODE_SEARCHING:

		viewer_list_item_clear();
		INFO_LOG(UG_NAME_NORMAL, "wifi state power on/connected");
		ret = wlan_manager_request_power_off();
		switch (ret) {
		case WLAN_MANAGER_ERR_NONE:
			viewer_manager_show(VIEWER_WINSET_SEARCHING);
			viewer_manager_hide(VIEWER_WINSET_SUB_CONTENTS);
			viewer_manager_header_mode_set(HEADER_MODE_DEACTIVATING);
			wlan_manager_disable_scan_result_update();	// Lets ignore all the scan updates because we are powering off now.
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
	return TRUE;
}

static void _hide_finished_cb(void *data, Evas_Object *obj, void *event_info)
{
	view_manager_view_type_t top_view_id = (view_manager_view_type_t)evas_object_data_get(obj, SCREEN_TYPE_ID_KEY);

	if (data == elm_naviframe_top_item_get(obj)) {
		/* We are now in main view */
		evas_object_data_set(obj, SCREEN_TYPE_ID_KEY, (void *)VIEW_MANAGER_VIEW_TYPE_MAIN);
		top_view_id = VIEW_MANAGER_VIEW_TYPE_MAIN;
	}

	INFO_LOG(UG_NAME_NORMAL, "top view id = %d", top_view_id);

	switch(top_view_id) {
	case VIEW_MANAGER_VIEW_TYPE_MAIN:
		/* Lets enable the scan updates */
		wlan_manager_enable_scan_result_update();
		break;

	case VIEW_MANAGER_VIEW_TYPE_DETAIL:
	case VIEW_MANAGER_VIEW_TYPE_EAP:
	default:
		/* Lets disable the scan updates so that the UI is not refreshed un-necessarily */
		wlan_manager_disable_scan_result_update();
		break;
	}
}

void _lbutton_click_cb(void *data, Evas_Object *obj, void *event_info)
{
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

	wifi_exit();
}

void _rbutton_click_cb(void *data, Evas_Object *obj, void *event_info)
{
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

	wifi_exit();
}

void _back_sk_cb(void *data, Evas_Object *obj, void *event_info)
{
	__COMMON_FUNC_ENTER__;

	if (ug_app_state->bAlive == EINA_FALSE)
		return;

	int cur_state = -1;
	cur_state = viewer_manager_header_mode_get();

	wifi_exit();
}

void _refresh_sk_cb(void *data, Evas_Object *obj, void *event_info)
{
	int cur_state = -1;
	cur_state = viewer_manager_header_mode_get();

	int ret = WLAN_MANAGER_ERR_NONE;

	switch(cur_state){
		case HEADER_MODE_DEACTIVATING:
		case HEADER_MODE_OFF:
			viewer_manager_show(VIEWER_WINSET_SEARCHING);
			break;

		case HEADER_MODE_ON:
		case HEADER_MODE_CONNECTED:
			ret = wlan_manager_request_scan();
			if(WLAN_MANAGER_ERR_NONE == ret) {
				viewer_manager_show(VIEWER_WINSET_SEARCHING);
				viewer_manager_header_mode_set(HEADER_MODE_SEARCHING);
			} else {
				INFO_LOG(COMMON_NAME_ERR, "Manual scan failed. Err = %d", ret);
			}
			break;
		default:
			INFO_LOG(UG_NAME_NORMAL, "Manual scan requested in wrong state: %d", cur_state);
			break;
	}
}

static int _genlist_item_disable_later(void* data)
{
	if(NULL != data) {
		elm_genlist_item_selected_set((Elm_Object_Item*) data, FALSE);
	}
	return FALSE;
}

static char *_gl_header_text_get(void *data, Evas_Object *obj, const char *part)
{
	char* det = NULL;

	__COMMON_FUNC_ENTER__;

	if (manager_object == NULL)
		return NULL;

	if (!strncmp(part, "elm.text", strlen(part))) {
		det = g_strdup(manager_object->header_text);
		assertm_if(NULL == det, "NULL!!");
	}

	__COMMON_FUNC_EXIT__;
	return det;
}

static void _gl_header_sel_cb(void *data, Evas_Object *obj, void *event_info)
{
	__COMMON_FUNC_ENTER__;
	const HEADER_MODES header_mode = viewer_manager_header_mode_get();
	if (HEADER_MODE_ACTIVATING != header_mode && HEADER_MODE_DEACTIVATING != header_mode)
		power_control();

	ecore_idler_add( (Ecore_Task_Cb) _genlist_item_disable_later, manager_object->item_header);

	elm_genlist_item_update(manager_object->item_header);

	__COMMON_FUNC_EXIT__;
	return;
}

static Evas_Object *_gl_header_content_get(void *data, Evas_Object *obj, const char *part)
{
	if (manager_object == NULL)
		return NULL;

	Evas_Object* ret = NULL;
	Evas_Object *toggle_btn = (Evas_Object *)data;
	Evas_Object *icon = NULL;

	__COMMON_FUNC_ENTER__;

	assertm_if(NULL == toggle_btn, "NULL!!");

	if (!strncmp(part, "elm.icon", strlen(part))) {
		switch (manager_object->header_mode) {
		case HEADER_MODE_OFF:
			/* Show WiFi off indication button */
			elm_check_state_set(toggle_btn, EINA_FALSE);
			ret = toggle_btn;
			break;

		case HEADER_MODE_ACTIVATING:
		case HEADER_MODE_DEACTIVATING:
			/* Dont display the WiFi on/off indication while it is Activating/Deactivating */
			icon = elm_progressbar_add(obj);
			elm_object_style_set(icon, "list_process");
			evas_object_size_hint_align_set(icon, EVAS_HINT_FILL, 0.5);
			evas_object_size_hint_weight_set(icon, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
			elm_progressbar_pulse(icon, TRUE);
			ret = icon;
			break;

		default:	/* Show WiFi on indication button */
			elm_check_state_set(toggle_btn, EINA_TRUE);
			ret = toggle_btn;
			break;
		}
	}

	__COMMON_FUNC_EXIT__;
	return ret;
}

static void _gl_header_del(void* data, Evas_Object* obj)
{
	if (data == NULL)
		return;

	__COMMON_FUNC_ENTER__;
	evas_object_unref(data);
	__COMMON_FUNC_EXIT__;

	return;
}

static char *_gl_bottom_text_get(void *data, Evas_Object *obj, const char *part)
{
	char* det = NULL;

	__COMMON_FUNC_ENTER__;

	if (!strncmp(part, "elm.text", strlen(part))) {
		det = g_strdup(sc(PACKAGE, I18N_TYPE_Network_notification));
		assertm_if(NULL == det, "NULL!!");
	}

	__COMMON_FUNC_EXIT__;
	return det;
}

static char *_gl_bottom_helper_text_get(void *data, Evas_Object *obj, const char *part)
{
	char* det = NULL;

	__COMMON_FUNC_ENTER__;

	det = g_strdup(sc(PACKAGE, I18N_TYPE_Network_notify_me_later));

	__COMMON_FUNC_EXIT__;
	return det;
}

static void _gl_bottom_sel_cb(void *data, Evas_Object *obj, void *event_info)
{
	__COMMON_FUNC_ENTER__;
	int ret = -1;
	int bottom_ret = (int)elm_check_state_get(obj);

	INFO_LOG(UG_NAME_NORMAL, "bottom state[%d] is different", bottom_ret);

	ret = wifi_setting_value_get(VCONFKEY_WIFI_ENABLE_QS);
	switch (ret) {
	case 1:
		if (wifi_setting_value_set(VCONFKEY_WIFI_ENABLE_QS, VCONFKEY_WIFI_QS_DISABLE) < 0) {
			ERROR_LOG(UG_NAME_NORMAL, "Failed to set vconf - VCONFKEY_WIFI_QS_DISABLE");
		}
		break;
	case 0:
		if (wifi_setting_value_set(VCONFKEY_WIFI_ENABLE_QS, VCONFKEY_WIFI_QS_ENABLE) < 0) {
			ERROR_LOG(UG_NAME_NORMAL, "Failed to set vconf - VCONFKEY_WIFI_QS_ENABLE");
		}
		break;
	default:
		ERROR_LOG(UG_NAME_NORMAL, "Failed to get vconf value - VCONFKEY_WIFI_ENABLE_QS");
		break;
	}

	ecore_idler_add( (Ecore_Task_Cb)_genlist_item_disable_later, manager_object->item_bottom);
	elm_genlist_item_update(manager_object->item_bottom);

	__COMMON_FUNC_EXIT__;
}

static Evas_Object *_gl_bottom_content_get(void *data, Evas_Object *obj, const char *part)
{
	__COMMON_FUNC_ENTER__;

	if (manager_object == NULL || obj == NULL)
		return NULL;

	int ret = -1;
	
	Evas_Object *toggle_btn = elm_check_add(obj);
	assertm_if(NULL == toggle_btn, "NULL!!");
	elm_object_style_set(toggle_btn, "on&off");
	evas_object_propagate_events_set(toggle_btn, EINA_TRUE);
	ret = wifi_setting_value_get(VCONFKEY_WIFI_ENABLE_QS);
	switch (ret) {
		case 1:
			elm_check_state_set(toggle_btn, EINA_TRUE);
			break;
		case 0:
			elm_check_state_set(toggle_btn, EINA_FALSE);
			break;
		default:
			assertm_if(TRUE, "Setting fail!!");
			break;
	}

	__COMMON_FUNC_EXIT__;
	return toggle_btn;
}

static Evas_Object *_gl_hidden_btn_content_get(void *data, Evas_Object *obj, const char *part)
{
	Evas_Object *find_hidden_ap_btn = elm_button_add(obj);
	elm_object_text_set(find_hidden_ap_btn, sc(PACKAGE, I18N_TYPE_Find_Hidden_Network));

	return find_hidden_ap_btn;
}

static void _hidden_button_callback(void* data, Evas_Object* obj, void* event_info)
{
	__COMMON_FUNC_ENTER__;


	ug_app_state->hidden_ap_popup = view_hidden_ap_popup_create(ug_app_state->win_main, PACKAGE);

	ecore_idler_add((Ecore_Task_Cb)_genlist_item_disable_later, event_info);

	__COMMON_FUNC_EXIT__;
}

static int viewer_manager_header_create(Evas_Object* genlist)
{
	__COMMON_FUNC_ENTER__;
	Evas_Object *toggle_btn = NULL;
	manager_object->header_text = g_strdup(sc(PACKAGE, I18N_TYPE_Wi_Fi));

	header_itc_text.item_style = "dialogue/1text.1icon";
	header_itc_text.func.text_get = _gl_header_text_get;
	header_itc_text.func.content_get = _gl_header_content_get;
	header_itc_text.func.state_get = NULL;
	header_itc_text.func.del = _gl_header_del;

	common_utils_add_dialogue_separator(genlist, "dialogue/separator");

	toggle_btn = elm_check_add(genlist);
	elm_object_style_set(toggle_btn, "on&off");
	evas_object_propagate_events_set(toggle_btn, EINA_TRUE);
	elm_check_state_set(toggle_btn, EINA_FALSE);
	evas_object_show(toggle_btn);
	evas_object_ref(toggle_btn);

	assertm_if(NULL != manager_object->item_header, "ERROR!!");
	manager_object->item_header = elm_genlist_item_append(genlist, &header_itc_text, toggle_btn, NULL, ELM_GENLIST_ITEM_NONE, _gl_header_sel_cb, NULL);
	assertm_if(NULL == manager_object->item_header, "NULL!!");

	__COMMON_FUNC_EXIT__;
	return TRUE;
}

static int viewer_manager_bottom_create(Evas_Object* genlist)
{
	__COMMON_FUNC_ENTER__;
	assertm_if(NULL == genlist, "NULL!!");

	bottom_itc.item_style = "grouptitle.dialogue.seperator";
	bottom_itc.func.text_get = NULL;
	bottom_itc.func.content_get = NULL;
	bottom_itc.func.state_get = NULL;
	bottom_itc.func.del = NULL;

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

	Elm_Object_Item* dialoguegroup = elm_genlist_item_append(genlist, &bottom_itc, NULL, NULL, ELM_GENLIST_ITEM_GROUP, NULL, NULL);
	assertm_if(NULL == dialoguegroup, "NULL!!");
	elm_genlist_item_select_mode_set(dialoguegroup, ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);

	manager_object->item_bottom = elm_genlist_item_append(genlist, &bottom_itc_text, NULL, NULL, ELM_GENLIST_ITEM_NONE, _gl_bottom_sel_cb, NULL);
	assertm_if(NULL == manager_object->item_bottom, "NULL!!");

	elm_genlist_mode_set(genlist, ELM_LIST_COMPRESS);
	manager_object->item_bottom_helper_txt = elm_genlist_item_append(genlist, &bottom_itc_helper_text, NULL, NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);
	elm_genlist_item_select_mode_set(manager_object->item_bottom_helper_txt, ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);

	__COMMON_FUNC_EXIT__;

	return TRUE;
}

static int viewer_manager_hidden_button_create(Evas_Object* genlist)
{
	__COMMON_FUNC_ENTER__;

	if(NULL != manager_object->item_sep_above_hidden_button ||
		NULL != manager_object->item_sep_below_hidden_button ||
		NULL != manager_object->item_hidden_btn) {

		__COMMON_FUNC_EXIT__;
		return FALSE;
	}

	assertm_if(NULL == genlist, "NULL!!");

	hidden_button_itc.item_style = "dialogue/bg/1icon";
	hidden_button_itc.func.text_get = NULL;
	hidden_button_itc.func.content_get = _gl_hidden_btn_content_get;
	hidden_button_itc.func.state_get = NULL;
	hidden_button_itc.func.del = NULL;

	manager_object->item_sep_above_hidden_button = common_utils_add_dialogue_separator(genlist, "dialogue/separator/20");

	manager_object->item_hidden_btn = elm_genlist_item_append(genlist, &hidden_button_itc, NULL, NULL, ELM_GENLIST_ITEM_NONE, _hidden_button_callback, NULL);
	assertm_if(NULL == manager_object->item_hidden_btn, "NULL!!");

	common_utils_add_dialogue_separator(genlist, "dialogue/separator/end");

	__COMMON_FUNC_EXIT__;
	return TRUE;
}

static Eina_Bool viewer_manager_scan_button_set(Eina_Bool show_state)
{
	__COMMON_FUNC_ENTER__;

	if(NULL == manager_object) {
		__COMMON_FUNC_EXIT__;
		return EINA_FALSE;
	}

	if(show_state == EINA_TRUE) {
		Evas_Object* navi_frame = viewer_manager_get_naviframe();
		view_manager_view_type_t top_view_id = (view_manager_view_type_t)evas_object_data_get(navi_frame, SCREEN_TYPE_ID_KEY);
		if(VIEW_MANAGER_VIEW_TYPE_MAIN == top_view_id) {
			INFO_LOG(UG_NAME_NORMAL,"Show directly");
			elm_object_item_disabled_set((Elm_Object_Item *)manager_object->scan_button, EINA_FALSE);
		} else {
			INFO_LOG(UG_NAME_NORMAL,"Show reserve");
		}
	} else if (show_state == EINA_FALSE) {
		elm_object_item_disabled_set((Elm_Object_Item *)manager_object->scan_button, EINA_TRUE);
	}

	__COMMON_FUNC_EXIT__;

	return EINA_TRUE;
}

Evas_Object* viewer_manager_create(Evas_Object* _parent)
{
	__COMMON_FUNC_ENTER__;

	if (manager_object != NULL || _parent == NULL) {
		__COMMON_FUNC_EXIT__;
		return NULL;
	}

	manager_object = (viewer_manager_object*) g_malloc0(sizeof(viewer_manager_object));
	memset(manager_object, 0, sizeof(viewer_manager_object));

	manager_object->list = NULL;
	manager_object->item_hidden_btn = NULL;
	manager_object->item_sep_above_hidden_button = NULL;
	manager_object->item_sep_below_hidden_button = NULL;
	manager_object->item_header = NULL;

	/* Add Full Layout */
	Evas_Object* layout = elm_layout_add(_parent);
	elm_layout_theme_set(layout, "layout", "application", "default");
	evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_win_resize_object_add(_parent, layout);
	edje_object_signal_emit(elm_layout_edje_get(layout), "elm,state,show,content", "elm");
	edje_object_signal_emit(elm_layout_edje_get(layout), "elm,bg,show,group_list", "elm");
	edje_object_signal_emit(elm_layout_edje_get(layout), "elm,state,show,indicator", "elm");

	/* Add Naviframe */
	manager_object->nav = elm_naviframe_add(layout);
	elm_object_part_content_set(layout, "elm.swallow.content", manager_object->nav);

	/* Add back button on Navigationbar */
	Evas_Object* button_back = elm_button_add(manager_object->nav);

	/* Add MainView Layout */
	Evas_Object* view_content = elm_layout_add(manager_object->nav);
	elm_layout_theme_set(view_content, "standard", "window", "integration");
	edje_object_signal_emit(elm_layout_edje_get(view_content), "elm,state,show,content", "elm");
	edje_object_signal_emit(elm_layout_edje_get(view_content), "elm,bg,show,group_list", "elm");

	/* Add Conformant */
	Evas_Object *conform = elm_conformant_add(manager_object->nav);
	elm_object_style_set(conform, "internal_layout");
	elm_object_part_content_set(view_content, "elm.swallow.content", conform);
	evas_object_show(conform);

	/* Add Genlist */
	manager_object->list = viewer_list_create(manager_object->nav);
	assertm_if(NULL == manager_object->list, "manager_object->list is NULL!!");
	viewer_manager_header_create(manager_object->list);
	viewer_manager_bottom_create(manager_object->list);
	viewer_manager_hidden_button_create(manager_object->list);

	elm_object_content_set(conform, manager_object->list);

	if (ug_app_state->ug_type == UG_VIEW_SETUP_WIZARD) {
		Elm_Object_Item* navi_it = elm_naviframe_item_push(manager_object->nav, sc(PACKAGE, I18N_TYPE_Wi_Fi), NULL, NULL, view_content, NULL);
		evas_object_data_set(manager_object->nav, SCREEN_TYPE_ID_KEY, (void *)VIEW_MANAGER_VIEW_TYPE_MAIN);
		evas_object_smart_callback_add(manager_object->nav, "transition,finished", _hide_finished_cb, navi_it);

		Evas_Object *toolbar = elm_toolbar_add(manager_object->nav);
		elm_toolbar_shrink_mode_set(toolbar, ELM_TOOLBAR_SHRINK_EXPAND);

		if(ug_app_state->lbutton_setup_wizard_prev_icon != NULL && ug_app_state->lbutton_setup_wizard != NULL) {
		    manager_object->prev_button = (Evas_Object *)elm_toolbar_item_append(toolbar, ug_app_state->lbutton_setup_wizard_prev_icon, ug_app_state->lbutton_setup_wizard, _lbutton_click_cb, NULL);
		    elm_object_item_disabled_set(elm_toolbar_item_append(toolbar, NULL, "", NULL, NULL), EINA_TRUE);
		}
		if(ug_app_state->rbutton_setup_wizard_scan_icon != NULL) {
		    manager_object->scan_button = (Evas_Object *)elm_toolbar_item_append(toolbar, ug_app_state->rbutton_setup_wizard_scan_icon, sc(PACKAGE, I18N_TYPE_Scan), _refresh_sk_cb, NULL);
		    elm_object_item_disabled_set(elm_toolbar_item_append(toolbar, NULL, "", NULL, NULL), EINA_TRUE);
		}
		if(ug_app_state->rbutton_setup_wizard_skip_icon != NULL && ug_app_state->rbutton_setup_wizard_skip != NULL) {
		    manager_object->next_button = (Evas_Object *)elm_toolbar_item_append(toolbar, ug_app_state->rbutton_setup_wizard_skip_icon, ug_app_state->rbutton_setup_wizard_skip, _rbutton_click_cb, NULL);
		}
		elm_object_item_part_content_set(navi_it, "controlbar", toolbar);

	} else {
		Elm_Object_Item* navi_it = elm_naviframe_item_push(manager_object->nav, sc(PACKAGE, I18N_TYPE_Wi_Fi), button_back, NULL, view_content, NULL);
		evas_object_data_set(manager_object->nav, SCREEN_TYPE_ID_KEY, (void *)VIEW_MANAGER_VIEW_TYPE_MAIN);
		evas_object_smart_callback_add(manager_object->nav, "transition,finished", _hide_finished_cb, navi_it);

		Evas_Object *toolbar = elm_toolbar_add(manager_object->nav);
		elm_toolbar_shrink_mode_set(toolbar, ELM_TOOLBAR_SHRINK_EXPAND);

		manager_object->scan_button = (Evas_Object *)elm_toolbar_item_append(toolbar, NULL, sc(PACKAGE, I18N_TYPE_Scan), _refresh_sk_cb, NULL);
		elm_object_item_disabled_set(elm_toolbar_item_append(toolbar, NULL, "", NULL, NULL), EINA_TRUE);
		elm_object_item_part_content_set(navi_it, "controlbar", toolbar);

		elm_object_style_set(button_back, "naviframe/back_btn/default");
		evas_object_smart_callback_add(button_back, "clicked", _back_sk_cb, NULL);
	}

	evas_object_show(layout);

	__COMMON_FUNC_EXIT__;
	return layout;
}

Eina_Bool viewer_manager_destroy()
{
	__COMMON_FUNC_ENTER__;

	viewer_list_destroy();

	if (manager_object->header_text) {
		g_free(manager_object->header_text);
		manager_object->header_text = NULL;
	}
	if (manager_object != NULL) {
		g_free(manager_object);
		manager_object = NULL;
	}

	__COMMON_FUNC_EXIT__;
	return EINA_TRUE;
}

Eina_Bool viewer_manager_show(VIEWER_WINSETS winset)
{
	__COMMON_FUNC_ENTER__;

	assertm_if(NULL == manager_object, "NULL!!");

	switch (winset) {
	case VIEWER_WINSET_SEARCHING:
		viewer_manager_scan_button_set(EINA_FALSE);
		viewer_list_item_disable_all();
		break;
	case VIEWER_WINSET_SUB_CONTENTS:
		assertm_if(NULL == manager_object->list, "NULL!!");
		viewer_list_title_item_set(manager_object->item_bottom_helper_txt);
		viewer_manager_hidden_button_create(manager_object->list);
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
			common_pswd_popup_destroy(ug_app_state->passpopup);
			ug_app_state->passpopup = NULL;
		}
		if (ug_app_state->eap_view) {
			eap_view_close(ug_app_state->eap_view);
			ug_app_state->eap_view = NULL;
		}
		viewer_list_title_item_del();
		assertm_if(NULL == manager_object->item_sep_above_hidden_button, "NULL!!");
		assertm_if(NULL == manager_object->item_sep_below_hidden_button, "NULL!!");
		assertm_if(NULL == manager_object->item_hidden_btn, "NULL!!");
		elm_object_item_del(manager_object->item_sep_above_hidden_button);
		elm_object_item_del(manager_object->item_sep_below_hidden_button);
		elm_object_item_del(manager_object->item_hidden_btn);
		manager_object->item_sep_above_hidden_button = NULL;
		manager_object->item_sep_below_hidden_button = NULL;
		manager_object->item_hidden_btn = NULL;
		break;
	default:
		/* Err */
		assertm_if(TRUE, "Err!!");
		break;
	}

	__COMMON_FUNC_EXIT__;
	return EINA_TRUE;
}

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

static void viewer_manager_genlist_normal_callback(void* data, Evas_Object* obj, void* event_info)
{
	__COMMON_FUNC_ENTER__;
	assertm_if(NULL == manager_object, "manager_object is NULL!!");
	assertm_if(NULL == event_info, "event_info is NULL!!");
	assertm_if(NULL == data, "data is NULL!!");
	assertm_if(NULL == obj, "obj is NULL!!");

	wifi_device_info_t *device_info = (wifi_device_info_t *) data;

	DEBUG_LOG(UG_NAME_NORMAL, "event_info:[%s]", (char*) elm_object_style_get(obj));

	radio_button_cb(device_info, obj, event_info);
	elm_genlist_item_selected_set((Elm_Object_Item *)event_info, FALSE);

	__COMMON_FUNC_EXIT__;
	return;
}

int viewer_manager_item_radio_mode_set(void* object, Elm_Object_Item* item, VIEWER_ITEM_RADIO_MODES mode)
{
	__COMMON_FUNC_ENTER__;
	assertm_if(NULL == manager_object, "manager_object is NULL!!");
	assertm_if(NULL == item, "item is NULL!!");

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
		INFO_LOG(UG_NAME_NORMAL, "[%s] is already in requested state", gdata->device_info->ssid);
		return FALSE;
	}

	INFO_LOG(UG_NAME_NORMAL, "[%s] AP Item State Transition from [%d] --> [%d]", gdata->device_info->ssid, gdata->radio_mode, mode);
	gdata->radio_mode = mode;
	if (gdata->device_info->ap_status_txt) {
		g_free(gdata->device_info->ap_status_txt);
		gdata->device_info->ap_status_txt = viewer_manager_get_device_status_txt(gdata->device_info, mode);
	}

	elm_genlist_item_update(item);

	__COMMON_FUNC_EXIT__;
	return TRUE;
}

int viewer_manager_hidden_disable_set(int mode)
{
	__COMMON_FUNC_ENTER__;
	assertm_if(NULL == manager_object->item_hidden_btn, "NULL!!");
	
	elm_object_item_disabled_set(manager_object->item_hidden_btn, mode);
	elm_genlist_item_update(manager_object->item_hidden_btn);

	__COMMON_FUNC_EXIT__;
	return TRUE;
}

static Eina_Bool _gl_bring_in(void *data)
{
	if (manager_object == NULL)
		return ECORE_CALLBACK_CANCEL;

	if (manager_object->item_header == NULL)
		return ECORE_CALLBACK_CANCEL;

	elm_genlist_item_bring_in(manager_object->item_header, ELM_GENLIST_ITEM_SCROLLTO_IN);
	return ECORE_CALLBACK_CANCEL;
}

void viewer_manager_scroll_to_top()
{
	if (manager_object->item_header == NULL)
		return;

	ecore_idler_add((Ecore_Task_Cb)_gl_bring_in, NULL);
}

static void viewer_manager_setup_wizard_button_controller(HEADER_MODES mode)
{
	switch (mode) {
	case HEADER_MODE_OFF:
	case HEADER_MODE_ON:
	    if (manager_object->next_button != NULL && ug_app_state->rbutton_setup_wizard_skip_icon != NULL)
	    	elm_object_item_text_set((Elm_Object_Item *)manager_object->next_button, ug_app_state->rbutton_setup_wizard_skip);

	    if (manager_object->next_button != NULL && ug_app_state->rbutton_setup_wizard_skip_icon != NULL)
	    	elm_toolbar_item_icon_set((Elm_Object_Item *)manager_object->next_button, (const char *)ug_app_state->rbutton_setup_wizard_skip_icon);
	    break;
	case HEADER_MODE_CONNECTED:
	    if (manager_object->next_button != NULL && ug_app_state->rbutton_setup_wizard_next != NULL)
	    	elm_object_item_text_set((Elm_Object_Item *)manager_object->next_button, ug_app_state->rbutton_setup_wizard_next);

	    if (manager_object->next_button != NULL && ug_app_state->rbutton_setup_wizard_next_icon != NULL)
	    	elm_toolbar_item_icon_set((Elm_Object_Item *)manager_object->next_button, (const char *) ug_app_state->rbutton_setup_wizard_next_icon);
	    break;
	default:
		break;
	}
}

int viewer_manager_header_mode_set(HEADER_MODES mode)
{
	__COMMON_FUNC_ENTER__;

	assertm_if(NULL == manager_object, "NULL!!");
	assertm_if(NULL == manager_object->item_header, "NULL!!");
	assertm_if(HEADER_MODE_OFF > mode || HEADER_MODE_MAX <= mode, "Err!!");

	if (manager_object->header_mode == mode) {
		return FALSE;
	}

	DEBUG_LOG(UG_NAME_NORMAL, "Header mode changing from %d --> %d", manager_object->header_mode, mode);
	manager_object->header_mode = mode;
	if (manager_object->header_text) {
		g_free(manager_object->header_text);
	}

	switch (mode) {
	case HEADER_MODE_OFF:
	case HEADER_MODE_ON:
	case HEADER_MODE_CONNECTED:
	    viewer_manager_hidden_disable_set(FALSE);
	    viewer_manager_scan_button_set(EINA_TRUE);
	    manager_object->header_text = g_strdup(sc(PACKAGE, I18N_TYPE_Wi_Fi));
	    break;
	case HEADER_MODE_ACTIVATING:
	    viewer_manager_hidden_disable_set(TRUE);
	    manager_object->header_text = g_strdup(sc(PACKAGE, I18N_TYPE_Activating_WiFi));
	    break;
	case HEADER_MODE_DEACTIVATING:
	    viewer_manager_hidden_disable_set(TRUE);
	    manager_object->header_text = g_strdup(sc(PACKAGE, I18N_TYPE_Deactivating));
	    break;
	case HEADER_MODE_CONNECTING:
	case HEADER_MODE_DISCONNECTING:
	case HEADER_MODE_CANCEL_CONNECTING:
	case HEADER_MODE_SEARCHING:
	    viewer_manager_hidden_disable_set(TRUE);
	    manager_object->header_text = g_strdup(sc(PACKAGE, I18N_TYPE_Wi_Fi));
	    break;
	default:
	    assertm_if(TRUE, "Err!!");
	    break;
	}


	elm_genlist_item_update(manager_object->item_header);
	viewer_list_title_item_update();


  if (ug_app_state->ug_type == UG_VIEW_SETUP_WIZARD) {
	    viewer_manager_setup_wizard_button_controller(mode);
	}

	__COMMON_FUNC_EXIT__;

	return TRUE;
}

HEADER_MODES viewer_manager_header_mode_get(void){
	__COMMON_FUNC_ENTER__;
	assertm_if(HEADER_MODE_OFF > manager_object->header_mode || 
			HEADER_MODE_MAX <= manager_object->header_mode, "Err!");
	__COMMON_FUNC_EXIT__;

	return manager_object->header_mode;
}

Evas_Object* viewer_manager_get_naviframe()
{
	return manager_object->nav;
}

static char *viewer_manager_get_device_status_txt(wifi_device_info_t *wifi_device, VIEWER_ITEM_RADIO_MODES mode)
{
	char *status_txt = NULL;
	char *ret_txt = NULL;
	/* The strings are currently hard coded. It will be replaced with string ids later */
	if (VIEWER_ITEM_RADIO_MODE_CONNECTING == mode || VIEWER_ITEM_RADIO_MODE_WPS_CONNECTING == mode) {
		status_txt = g_strdup(sc(PACKAGE, I18N_TYPE_Connecting));
	} else if (VIEWER_ITEM_RADIO_MODE_CONNECTED == mode) {
		status_txt = g_strdup_printf("<color=%s><b>%s</b></color>", LIST_ITEM_CONNECTED_AP_FONT_COLOR, sc(PACKAGE, I18N_TYPE_Connected));
	} else if (VIEWER_ITEM_RADIO_MODE_DISCONNECTING == mode) {
		status_txt = g_strdup(sc(PACKAGE, I18N_TYPE_Disconnecting));
	} else if (VIEWER_ITEM_RADIO_MODE_OFF == mode) {
		status_txt = common_utils_get_ap_security_type_info_txt(PACKAGE, wifi_device);
	} else {
		status_txt = g_strdup(WIFI_UNKNOWN_DEVICE_STATUS_STR);
		INFO_LOG(UG_NAME_NORMAL, "Invalid mode: %d", mode);
	}
	ret_txt = g_strdup_printf("<font_size=%d>%s</font_size>", LIST_ITEM_CONNECTED_AP_FONT_SIZE, status_txt);
	g_free(status_txt);
	return ret_txt;
}

Elm_Object_Item *viewer_manager_move_item_to_top(Elm_Object_Item *item)
{
	__COMMON_FUNC_ENTER__;
	if (!item) {
		return NULL;
	}
	ug_genlist_data_t *gdata = elm_object_item_data_get(item);
	if (!gdata) {
		return NULL;
	}
	wifi_device_info_t *old_wifi_device = gdata->device_info;
	if (!old_wifi_device) {
		return NULL;
	}
	wifi_device_info_t *wifi_device = (wifi_device_info_t*)g_malloc0(sizeof(wifi_device_info_t));
	wifi_device->profile_name = g_strdup(old_wifi_device->profile_name);
	wifi_device->ssid = g_strdup(old_wifi_device->ssid);
	wifi_device->ap_image_path = g_strdup(old_wifi_device->ap_image_path);
	wifi_device->ap_status_txt = g_strdup(old_wifi_device->ap_status_txt);
	wifi_device->rssi = old_wifi_device->rssi;
	wifi_device->security_mode = old_wifi_device->security_mode;
	wifi_device->wps_mode = old_wifi_device->wps_mode;

	viewer_list_item_del(item);
	item = viewer_list_item_insert_after(manager_object->list,
			wifi_device,
			NULL,
			viewer_manager_genlist_normal_callback,
			wifi_device);
	__COMMON_FUNC_EXIT__;
	return item;
}

Elm_Object_Item *viewer_manager_add_new_item(const char *profile_name)
{
	Elm_Object_Item *item = NULL;
	if (!manager_object)
		return NULL;

	/* Looks like we have received a connecting/connected event and the AP item is not present. Lets add the item to the list. */
	view_datamodel_basic_info_t *data_model = view_basic_detail_datamodel_create(profile_name);
	if (data_model) {
		wifi_device_info_t *wifi_device = (wifi_device_info_t*)g_malloc0(sizeof(wifi_device_info_t));
		wifi_device->profile_name = view_detail_datamodel_basic_info_profile_name_get(data_model);
		wifi_device->ssid = view_detail_datamodel_ap_name_get(data_model);
		wifi_device->rssi = (int)view_detail_datamodel_sig_strength_get(data_model);
		wifi_device->security_mode = view_detail_datamodel_sec_mode_get(data_model);
		wifi_device->wps_mode = (boolean)view_detail_datamodel_wps_support_get(data_model);
		wifi_device->ap_image_path = common_utils_get_device_icon(WIFI_APP_IMAGE_DIR, wifi_device);
		wifi_device->ap_status_txt = viewer_manager_get_device_status_txt(wifi_device, VIEWER_ITEM_RADIO_MODE_OFF);

		item = viewer_list_item_insert_after(manager_object->list,
				wifi_device,
				NULL,
				viewer_manager_genlist_normal_callback,
				wifi_device);
		view_basic_detail_datamodel_destroy(data_model);
	}
	return item;
}

static void viewer_manager_update_list_all()
{
	int i = 0;
	Elm_Object_Item *item = NULL;

	__COMMON_FUNC_ENTER__;

	net_profile_info_t *profiles_list = wlan_manager_profile_table_get();
	if (profiles_list == NULL)
		return;

	int profiles_list_size = wlan_manager_profile_scanned_length_get();
	INFO_LOG(UG_NAME_NORMAL, "profiles list count [%d]\n", profiles_list_size);

	if (profiles_list_size > 0) {
		for (i = 0; i < profiles_list_size; i++) {
			net_profile_info_t *profile_info = profiles_list+i;
			wifi_device_info_t *wifi_device = (wifi_device_info_t*)g_malloc0(sizeof(wifi_device_info_t));
			wifi_device->profile_name = g_strdup(profile_info->ProfileName);
			wifi_device->ssid = g_strdup(profile_info->ProfileInfo.Wlan.essid);
			wifi_device->rssi = (int)profile_info->ProfileInfo.Wlan.Strength;
			wifi_device->security_mode = profile_info->ProfileInfo.Wlan.security_info.sec_mode;
			wifi_device->wps_mode = (int)profile_info->ProfileInfo.Wlan.security_info.wps_support;
			wifi_device->ap_image_path = common_utils_get_device_icon(WIFI_APP_IMAGE_DIR, wifi_device);
			wifi_device->ap_status_txt = viewer_manager_get_device_status_txt(wifi_device, VIEWER_ITEM_RADIO_MODE_OFF);

			item = viewer_list_item_insert_after(manager_object->list,
					wifi_device,
					item,
					viewer_manager_genlist_normal_callback,
					wifi_device);
		}
	} else if (profiles_list_size == 0) {
		/* if there is no scan_data, generate No-AP item */
		wifi_device_info_t *device_info = NULL;
		device_info = (wifi_device_info_t *)wlan_manager_profile_device_info_blank_create();
		device_info->ap_status_txt = g_strdup("");
		device_info->ap_image_path = NULL;
		item = viewer_list_item_insert_after(manager_object->list,
				device_info,
				NULL,
				NULL,
				NULL);
		elm_genlist_item_select_mode_set(item, ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);
	} else {
		ERROR_LOG(UG_NAME_NORMAL, "Err!");
	}

	__COMMON_FUNC_EXIT__;
}

Eina_Bool viewer_manager_refresh(void)
{
	INFO_LOG(UG_NAME_SCAN, "UI update start");
	int profile_state;

	if (manager_object == NULL)
		return EINA_FALSE;

	/* Remove the list */
	viewer_list_item_clear();

	char profile_name[NET_PROFILE_NAME_LEN_MAX+1] = "";
	profile_state = wlan_manager_state_get(profile_name);
	if (WLAN_MANAGER_ERROR == profile_state || WLAN_MANAGER_OFF == profile_state) {
		/* Some body requested to refresh the list while the WLAN manager is OFF or Unable to get the profile state */
		INFO_LOG(UG_NAME_ERR, "Refresh requested in wrong state or Unable to get the state !!! ");

		Elm_Object_Item* target_item = item_get_for_profile_name(profile_name);
		viewer_manager_header_mode_set(HEADER_MODE_OFF);
		viewer_manager_item_radio_mode_set(NULL, target_item, VIEWER_ITEM_RADIO_MODE_OFF);
		return EINA_FALSE;
	}

	viewer_manager_update_list_all();

	if (WLAN_MANAGER_CONNECTING == profile_state) {
		INFO_LOG(UG_NAME_NORMAL, "Profile is connecting...");
		Elm_Object_Item* target_item = item_get_for_profile_name(profile_name);
		viewer_manager_header_mode_set(HEADER_MODE_CONNECTING);
		viewer_manager_item_radio_mode_set(NULL, target_item, VIEWER_ITEM_RADIO_MODE_CONNECTING);
	} else if (WLAN_MANAGER_CONNECTED == profile_state) {
		INFO_LOG(UG_NAME_NORMAL, "Profile is connected");
		Elm_Object_Item* target_item = item_get_for_profile_name(profile_name);
		target_item = viewer_manager_move_item_to_top(target_item);
		viewer_manager_header_mode_set(HEADER_MODE_CONNECTED);
		viewer_manager_item_radio_mode_set(NULL, target_item, VIEWER_ITEM_RADIO_MODE_CONNECTED);
	} else if (WLAN_MANAGER_DISCONNECTING == profile_state) {
		INFO_LOG(UG_NAME_NORMAL, "Profile is disconnecting");
		Elm_Object_Item* target_item = item_get_for_profile_name(profile_name);
		viewer_manager_header_mode_set(HEADER_MODE_DISCONNECTING);
		viewer_manager_item_radio_mode_set(NULL, target_item, VIEWER_ITEM_RADIO_MODE_DISCONNECTING);
	} else {
		INFO_LOG(UG_NAME_NORMAL, "Profile state = %d", profile_state);
		viewer_manager_header_mode_set(HEADER_MODE_ON);
	}

	INFO_LOG(UG_NAME_SCAN, "UI update finish");

	return EINA_TRUE;
}

static void hidden_ap_connect_ok_cb (void *data, Evas_Object *obj, void *event_info)
{
	__COMMON_FUNC_ENTER__;

	hidden_ap_data_t *hidden_ap_data = (hidden_ap_data_t *)data;
	if (!hidden_ap_data)
		return;

	char* szPassword = NULL;
	net_wifi_connection_info_t conninfo;
	memset(&conninfo, 0, sizeof(conninfo));
	g_strlcpy(conninfo.essid, hidden_ap_data->ssid, NET_WLAN_ESSID_LEN);
	conninfo.security_info.sec_mode = hidden_ap_data->sec_mode;
	conninfo.wlan_mode = NETPM_WLAN_CONNMODE_INFRA;
	INFO_LOG(UG_NAME_NORMAL, "Hidden AP[%s]. Sec mode = %d. Connect ok cb", conninfo.essid, conninfo.security_info.sec_mode);

	switch (conninfo.security_info.sec_mode) {
	case WLAN_SEC_MODE_NONE:
		INFO_LOG(UG_NAME_NORMAL, "This hidden AP is Open. event info = %x; passpopup = %x", event_info, ug_app_state->passpopup);
		evas_object_del(hidden_ap_data->confirmation_popup);
		hidden_ap_data->confirmation_popup = NULL;
		break;

	case WLAN_SEC_MODE_WEP:
		szPassword = common_pswd_popup_get_txt(ug_app_state->passpopup);
		g_strlcpy(conninfo.security_info.authentication.wep.wepKey, szPassword, NETPM_WLAN_MAX_WEP_KEY_LEN);
		g_free(szPassword);
		INFO_LOG(UG_NAME_NORMAL, "Hidden AP WEP paswd = [%s]", conninfo.security_info.authentication.wep.wepKey);
		common_pswd_popup_destroy(ug_app_state->passpopup);
		ug_app_state->passpopup = NULL;
		break;

	case WLAN_SEC_MODE_WPA_PSK:
	case WLAN_SEC_MODE_WPA2_PSK:
		szPassword = common_pswd_popup_get_txt(ug_app_state->passpopup);
		g_strlcpy(conninfo.security_info.authentication.psk.pskKey, szPassword, NETPM_WLAN_MAX_PSK_PASSPHRASE_LEN);
		g_free(szPassword);
		INFO_LOG(UG_NAME_NORMAL, "Hidden AP PSK paswd = [%s]", conninfo.security_info.authentication.psk.pskKey);
		common_pswd_popup_destroy(ug_app_state->passpopup);
		ug_app_state->passpopup = NULL;
		break;

	default:
		INFO_LOG(UG_NAME_NORMAL, "Fatal: Unknown Sec mode: %d", hidden_ap_data->sec_mode);
		goto hidden_ap_connect_end;
	}

	connman_request_connection_open_hidden_ap(&conninfo);

hidden_ap_connect_end:
	g_free(hidden_ap_data->ssid);
	g_free(hidden_ap_data);
	__COMMON_FUNC_EXIT__;
	return;
}

static void hidden_ap_connect_cacel_cb (void *data, Evas_Object *obj, void *event_info)
{
	__COMMON_FUNC_ENTER__;
	hidden_ap_data_t *hidden_ap_data = (hidden_ap_data_t *)data;
	if (!hidden_ap_data)
		return;

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
		common_pswd_popup_destroy(ug_app_state->passpopup);
		ug_app_state->passpopup = NULL;
		break;

	default:
		INFO_LOG(UG_NAME_NORMAL, "Fatal: Unknown Sec mode: %d", hidden_ap_data->sec_mode);
		break;
	}

	g_free(hidden_ap_data->ssid);
	g_free(hidden_ap_data);
	__COMMON_FUNC_EXIT__;
	return;
}

void viewer_manager_specific_scan_response_hlr(GSList *bss_info_list)
{
	hidden_ap_data_t *hidden_ap_data = NULL;
	const char *ssid = view_ime_hidden_popup_get_ssid(ug_app_state->hidden_ap_popup);
	wlan_security_mode_type_t sec_mode;
	if (!ssid) {
		return;
	}
	INFO_LOG(UG_NAME_RESP, "Specific scan complete response received for AP[%s]", ssid);
	int ap_count = g_slist_length(bss_info_list);
	net_wifi_connection_info_t *bss_info = NULL;

	if (ap_count == 1) {
		bss_info = g_slist_nth_data(bss_info_list, 0);
		if (!bss_info || g_strcmp0(ssid, bss_info->essid)) {
			INFO_LOG(UG_NAME_RESP, "Fatal: Bss info is NULL OR response received for wrong ssid. ", ssid);
			/* Bss info not available or Response recieved for wrong ssid */
			ap_count = 0;
		} else {
			sec_mode = bss_info->security_info.sec_mode;
		}
	}

	if (ap_count == 1) {
		/* Only if there is one AP found then we need Users further action */
		switch (sec_mode) {
		case WLAN_SEC_MODE_NONE:
			INFO_LOG(UG_NAME_NORMAL, "One AP item with ssid[%s] found. Its security is Open.", ssid);
			/* This is an Open AP. Ask for confirmation to connect. */
			hidden_ap_data = g_try_new0(hidden_ap_data_t, 1);

			popup_btn_info_t popup_btn_data;
			memset(&popup_btn_data, 0, sizeof(popup_btn_data));
			popup_btn_data.info_txt = "Wi-Fi network detected. Connect?";
			popup_btn_data.btn1_cb = hidden_ap_connect_ok_cb;
			popup_btn_data.btn2_cb = hidden_ap_connect_cacel_cb;
			hidden_ap_data->sec_mode = WLAN_SEC_MODE_NONE;
			hidden_ap_data->ssid = g_strdup(ssid);
			popup_btn_data.btn2_data = popup_btn_data.btn1_data = hidden_ap_data;
			popup_btn_data.btn1_txt = sc(PACKAGE, I18N_TYPE_Connect);
			popup_btn_data.btn2_txt = sc(PACKAGE, I18N_TYPE_Cancel);
			hidden_ap_data->confirmation_popup = common_utils_show_info_popup(ug_app_state->win_main, &popup_btn_data);
			break;
		case WLAN_SEC_MODE_IEEE8021X:
			INFO_LOG(UG_NAME_NORMAL, "One AP item with ssid[%s] found. Its security is EAP.", ssid);
			/* This is a EAP secured AP. Ask for confirmation to connect. */
			Evas_Object* navi_frame = viewer_manager_get_naviframe();
			wifi_device_info_t device_info;
			if (navi_frame == NULL) {
				ERROR_LOG(UG_NAME_NORMAL, "Failed : get naviframe");
				return;
			}
			memset(&device_info, 0, sizeof(device_info));
			device_info.security_mode = sec_mode;
			device_info.ssid = (char *)ssid;

			ug_app_state->eap_view = create_eap_connect(ug_app_state->win_main, navi_frame, PACKAGE, &device_info, eap_view_close_cb);
			break;
		case WLAN_SEC_MODE_WEP:
		case WLAN_SEC_MODE_WPA_PSK:
		case WLAN_SEC_MODE_WPA2_PSK:
			INFO_LOG(UG_NAME_NORMAL, "One AP item with ssid[%s] found. Its security is %d", ssid, sec_mode);
			/* This is a WEP/WPA/WPA-2 secured AP. Ask for confirmation to connect. */
			hidden_ap_data = g_try_new0(hidden_ap_data_t, 1);
			pswd_popup_create_req_data_t	popup_info;
			hidden_ap_data->sec_mode = sec_mode;
			hidden_ap_data->ssid = g_strdup(ssid);
			popup_info.title = bss_info->essid;
			popup_info.ok_cb = hidden_ap_connect_ok_cb;
			popup_info.cancel_cb = hidden_ap_connect_cacel_cb;
			popup_info.show_wps_btn = FALSE;
			popup_info.wps_btn_cb = NULL;
			popup_info.cb_data = hidden_ap_data;
			INFO_LOG(UG_NAME_NORMAL, "Going to create a popup. ug_app_state = 0x%x", ug_app_state);
			ug_app_state->passpopup = common_pswd_popup_create(ug_app_state->win_main, PACKAGE, &popup_info);
			INFO_LOG(UG_NAME_NORMAL, "After create a popup");
			if (ug_app_state->passpopup == NULL) {
				INFO_LOG(UG_NAME_ERR, "pass popup create failed !");
			}
			break;
		default:
			INFO_LOG(UG_NAME_NORMAL, "Unkown security mode: %d", sec_mode);
			break;
		}
	} else if (ap_count == 0) {
		INFO_LOG(UG_NAME_NORMAL, "No AP item with ssid[%s] found", ssid);
		char *disp_msg = g_strdup_printf("Unable to find %s", ssid);
		common_utils_show_info_ok_popup(ug_app_state->win_main, PACKAGE, disp_msg);
		g_free(disp_msg);
	} else {
		INFO_LOG(UG_NAME_NORMAL, "More than one AP items with ssid[%s] found", ssid);
	}

	/* If the hidden AP found on first and second scan OR not found even after first and second scan then delete the popup */
	view_hidden_ap_popup_destroy(ug_app_state->hidden_ap_popup);
	ug_app_state->hidden_ap_popup = NULL;
	return;
}
