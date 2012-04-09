/*
 * Copyright (c) 2000-2012 Samsung Electronics Co., Ltd All Rights Reserved.
 *
 * This file is part of Wi-Fi UG
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



#include "wifi.h"
#include "viewer_manager.h"
#include "viewer_list.h"
#include "i18nmanager.h"
#include "wifi-setting.h"
#include "view_ime_hidden.h"
#include "wlan_manager.h"
#include "wifi-ui-list-callbacks.h"
#include "popup.h"


typedef struct viewer_manager_object {
	Evas_Object* nav;
	Elm_Object_Item* scan_button;
	Evas_Object* list;
	Elm_Object_Item *current_selected_item;

	Elm_Object_Item *item_hidden_btn;
	Elm_Object_Item *item_sep_above_hidden_button;
	Elm_Object_Item *item_sep_below_hidden_button;

	Eina_Bool click_enabled;
	Eina_Bool update_enabled;

	char* header_text;
	HEADER_MODES header_mode;
	Elm_Object_Item* item_header;
	Elm_Object_Item* item_bottom;
} viewer_manager_object;
static viewer_manager_object* manager_object = NULL;

extern struct wifi_appdata *app_state;

static Elm_Genlist_Item_Class header_itc;
static Elm_Genlist_Item_Class header_itc_text;
static Elm_Genlist_Item_Class bottom_itc;
static Elm_Genlist_Item_Class bottom_itc_text;
static Elm_Genlist_Item_Class hidden_button_seperator_itc;
static Elm_Genlist_Item_Class hidden_button_itc;

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
			case WLAN_MANAGER_ERR_MOBILE_HOTSPOT_OCCUPIED:
				viewer_manager_header_mode_set(HEADER_MODE_ACTIVATING);
				break;
			default:
				viewer_manager_header_mode_set(HEADER_MODE_OFF);
		}
		break;
	case HEADER_MODE_ON:
	case HEADER_MODE_CONNECTED:
		INFO_LOG(UG_NAME_NORMAL, "wifi state power on/connected");
		viewer_list_item_clear();
		ret = wlan_manager_request_power_off();
		switch (ret) {
		case WLAN_MANAGER_ERR_NONE:
			viewer_manager_hide(VIEWER_WINSET_SUB_CONTENTS);
			viewer_manager_hide(VIEWER_WINSET_SEARCHING);
			viewer_manager_header_mode_set(HEADER_MODE_DEACTIVATING);
			break;
		default:
			break;
		}
		break;
	default:
		INFO_LOG(UG_NAME_NORMAL, "powering on or scanning state, header is not working %d\n", cur_state);
		break;
	}

	switch (ret) {
	case WLAN_MANAGER_ERR_NONE:
		break;
	case WLAN_MANAGER_ERR_MOBILE_HOTSPOT_OCCUPIED:
		winset_popup_mode_set(NULL, POPUP_MODE_POWER_ON_FAILED, POPUP_OPTION_POWER_ON_FAILED_MOBILE_HOTSPOT);
		break;
	default:
		break;
	}

	__COMMON_FUNC_EXIT__;
	return TRUE;
}

static void _hide_finished_cb(void *data, Evas_Object *obj, void *event_info)
{
	Elm_Object_Item* top_it = elm_naviframe_top_item_get(obj);

	const char* title = elm_object_item_text_get((Elm_Object_Item*)data);
	const char* title_top = elm_object_item_text_get(top_it);
	
	if(title == title_top) {
		app_state->current_view = VIEW_MAIN;
	}else if(!strcmp(title_top, "Password")) {
		app_state->current_view = VIEW_PASSWORD;
	}else if(!strcmp(title_top, sc(PACKAGE, I18N_TYPE_Hidden_AP))) {
		app_state->current_view = VIEW_HIDDEN_AP;
	}else if(!strcmp(title_top, sc(PACKAGE, I18N_TYPE_Static_IP))) {
		app_state->current_view = VIEW_STATIC_IP;
	}else if(!strcmp(title_top, sc(PACKAGE, I18N_TYPE_Dynamic_IP))) {
		app_state->current_view = VIEW_DHCP_IP;
	}else {
		app_state->current_view = VIEW_DETAIL;
	}
}

void _back_sk_cb(void *data, Evas_Object *obj, void *event_info)
{
	__COMMON_FUNC_ENTER__;

	if (app_state->bAlive == EINA_FALSE)
		return;

	int cur_state = -1;
	cur_state = viewer_manager_header_mode_get();

	if (app_state->bundle_back_button_show_force_when_connected == EINA_TRUE &&
		cur_state != HEADER_MODE_CONNECTED){
		winset_popup_mode_set(NULL, POPUP_MODE_ETC, POPUP_OPTION_ETC_BACK_ENABLE_WHEN_CONNECTED_ERROR);
	} else {
		wifi_exit();
	}
}

void _refresh_sk_cb(void *data, Evas_Object *obj, void *event_info)
{
	int cur_state = -1;
	cur_state = viewer_manager_header_mode_get();

	int ret = WLAN_MANAGER_ERR_NONE;

	switch(cur_state){
		case HEADER_MODE_OFF:
			power_control();
			break;
		case HEADER_MODE_ON:
		case HEADER_MODE_CONNECTED:
			ret = wlan_manager_request_scan();
			if(WLAN_MANAGER_ERR_NONE == ret) {
				viewer_manager_show(VIEWER_WINSET_SEARCHING);
				viewer_manager_header_mode_set(HEADER_MODE_SEARCHING);
			}
			break;
		
	}
}

static void _header_onoff_cb(void *data, Evas_Object *obj, void *event_info)
{
	__COMMON_FUNC_ENTER__;
	power_control();
	__COMMON_FUNC_EXIT__;
	return;
}

static char *_gl_header_text_get(void *data, Evas_Object *obj, const char *part)
{
	assertm_if(NULL == part, "NULL!!");

	if (part != NULL) {
		if (!strncmp(part, "elm.text", strlen(part))) {
			char* det = NULL;
			det = strdup(manager_object->header_text);
			assertm_if(NULL == det, "NULL!!");
			__COMMON_FUNC_EXIT__;
			return det; 
		}
	}

	return NULL;
}

static Evas_Object *_gl_header_content_get(void *data, Evas_Object *obj, const char *part)
{
	Evas_Object* ret = NULL;
	assertm_if(NULL == part, "NULL!!");
	assertm_if(NULL == obj, "NULL!!");
	assertm_if(NULL == manager_object, "NULL!!");

	Evas_Object *header_onoff = NULL;
	if (!strncmp(part, "elm.icon", strlen(part))) {
		switch (manager_object->header_mode) {
		case HEADER_MODE_OFF:
			header_onoff = elm_check_add(obj);
			assertm_if(NULL == header_onoff, "NULL!!");

			evas_object_propagate_events_set(header_onoff, EINA_FALSE);
			elm_object_style_set(header_onoff, "on&off");
			elm_check_state_set(header_onoff, EINA_FALSE);
			evas_object_show(header_onoff);
			evas_object_smart_callback_add(header_onoff, "changed", (Evas_Smart_Cb)_header_onoff_cb, NULL);
			ret = header_onoff;
			break;
		case HEADER_MODE_ON:
		case HEADER_MODE_CONNECTED:
			header_onoff = elm_check_add(obj);
			assertm_if(NULL == header_onoff, "NULL!!");

			evas_object_propagate_events_set(header_onoff, EINA_FALSE);
			elm_object_style_set(header_onoff, "on&off");
			elm_check_state_set(header_onoff, EINA_TRUE);
			evas_object_show(header_onoff);
			evas_object_smart_callback_add(header_onoff, "changed", (Evas_Smart_Cb)_header_onoff_cb, NULL);
			ret = header_onoff;
			break;
		default:
			break;
		}
	}

	assertm_if(NULL == ret, "NULL!!");
	return ret;
}

static char *_gl_bottom_text_get(void *data, Evas_Object *obj, const char *part)
{
	assertm_if(NULL == part, "NULL!!");

	if (part != NULL) {
		if (!strncmp(part, "elm.text", strlen(part))) {
			char* det = NULL;
			det = strdup(sc(PACKAGE, I18N_TYPE_Network_notification));
			assertm_if(NULL == det, "NULL!!");
			__COMMON_FUNC_EXIT__;
			return det;
		}
	}

	return NULL;
}

static void _bottom_header_callback(void *data, Evas_Object *obj, void *event_info)
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

	__COMMON_FUNC_EXIT__;
}

static Evas_Object *_gl_bottom_content_get(void *data, Evas_Object *obj, const char *part)
{
	__COMMON_FUNC_ENTER__;

	assertm_if(NULL == obj, "NULL!!");

	int ret = -1;
	
	Evas_Object* onoff = elm_check_add(obj);
	elm_object_style_set(onoff, "on&off");

	assertm_if(NULL == onoff, "NULL!!");

	ret = wifi_setting_value_get(VCONFKEY_WIFI_ENABLE_QS);
	switch (ret) {
		case 1:
			elm_check_state_set(onoff, EINA_TRUE);
			break;
		case 0:
			elm_check_state_set(onoff, EINA_FALSE);
			break;
		default:
			assertm_if(TRUE, "Setting fail!!");
			break;
	}

	evas_object_smart_callback_add(onoff, "changed", (Evas_Smart_Cb)_bottom_header_callback, NULL);

	__COMMON_FUNC_EXIT__;
	return onoff;
}

static int _genlist_item_disable_later(void* data)
{
	if(NULL != data) {
		elm_genlist_item_selected_set((Elm_Object_Item*) data, FALSE);
	}
	return FALSE;
}

static void _gl_header_sel_cb(void *data, Evas_Object *obj, void *event_info)
{
	__COMMON_FUNC_ENTER__;
	power_control();
	ecore_idler_add( (Ecore_Task_Cb) _genlist_item_disable_later, manager_object->item_header);
	__COMMON_FUNC_EXIT__;
	return;
}

static void _gl_bottom_sel_cb(void *data, Evas_Object *obj, void *event_info)
{
	__COMMON_FUNC_ENTER__;
	_bottom_header_callback(data, obj, event_info);
	ecore_idler_add( (Ecore_Task_Cb) _genlist_item_disable_later, manager_object->item_bottom);
	elm_genlist_item_item_class_update((Elm_Object_Item*)event_info, &bottom_itc_text);
	__COMMON_FUNC_EXIT__;
	return;
}

static Evas_Object *_gl_hidden_btn_content_get(void *data, Evas_Object *obj, const char *part)
{
	__COMMON_FUNC_ENTER__;

	Evas_Object* ret = NULL;
	assertm_if(NULL == part, "NULL!!");
	assertm_if(NULL == obj, "NULL!!");

	DEBUG_LOG(UG_NAME_NORMAL, "part [%s]", part);

	if (!strncmp(part, "elm.icon", strlen(part))) {
		ret = elm_button_add(obj);
		assertm_if(NULL == ret, "NULL!!");

		elm_object_text_set(ret, sc(PACKAGE, I18N_TYPE_Hidden_AP));
	}

	if (manager_object->item_hidden_btn != NULL) {
		int state = elm_object_item_disabled_get(manager_object->item_hidden_btn);
		elm_object_disabled_set(ret, state);
	}

	__COMMON_FUNC_EXIT__;
	return ret;
}

static void _hiddne_button_callback(void* data, Evas_Object* obj, void* event_info)
{
	__COMMON_FUNC_ENTER__;

	viewer_manager_set_enabled_list_click(EINA_FALSE);
	viewer_manager_set_enabled_list_update(EINA_FALSE);

	view_ime_hidden();

	ecore_idler_add((Ecore_Task_Cb)_genlist_item_disable_later, event_info);

	__COMMON_FUNC_EXIT__;
}

int viewer_manager_header_create(Evas_Object* genlist)
{
	__COMMON_FUNC_ENTER__;

	manager_object->header_text = NULL;
	manager_object->header_text = strdup("Wi-Fi");
	assertm_if(NULL == manager_object->header_text, "NULL!!");

	header_itc.item_style = "dialogue/seperator";
	header_itc.func.text_get = NULL;
	header_itc.func.content_get = NULL;
	header_itc.func.state_get = NULL;
	header_itc.func.del = NULL;

	header_itc_text.item_style = "dialogue/1text.1icon";
	header_itc_text.func.text_get = _gl_header_text_get;
	header_itc_text.func.content_get = _gl_header_content_get;
	header_itc_text.func.state_get = NULL;
	header_itc_text.func.del = NULL;

	Elm_Object_Item* dialoguegroup = elm_genlist_item_append(genlist, &header_itc, NULL, NULL, ELM_GENLIST_ITEM_GROUP, NULL, NULL);
	assertm_if(NULL == dialoguegroup, "NULL!!");
	elm_genlist_item_select_mode_set(dialoguegroup, ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);

	assertm_if(NULL != manager_object->item_header, "ERROR!!");
	manager_object->item_header = elm_genlist_item_append(genlist, &header_itc_text, NULL, NULL, ELM_GENLIST_ITEM_NONE, _gl_header_sel_cb, NULL);
	assertm_if(NULL == manager_object->item_header, "NULL!!");

	__COMMON_FUNC_EXIT__;
	return TRUE;
}

int viewer_manager_bottom_create(Evas_Object* genlist)
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

	Elm_Object_Item* dialoguegroup = elm_genlist_item_append(genlist, &bottom_itc, NULL, NULL, ELM_GENLIST_ITEM_GROUP, NULL, NULL);
	assertm_if(NULL == dialoguegroup, "NULL!!");
	elm_genlist_item_select_mode_set(dialoguegroup, ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);

	manager_object->item_bottom = elm_genlist_item_append(genlist, &bottom_itc_text, NULL, NULL, ELM_GENLIST_ITEM_NONE, _gl_bottom_sel_cb, NULL);
	assertm_if(NULL == manager_object->item_bottom, "NULL!!");

	__COMMON_FUNC_EXIT__;

	return TRUE;
}

int viewer_manager_hidden_button_create(Evas_Object* genlist)
{
	__COMMON_FUNC_ENTER__;

	if(NULL != manager_object->item_sep_above_hidden_button ||
		NULL != manager_object->item_sep_below_hidden_button ||
		NULL != manager_object->item_hidden_btn) {

		__COMMON_FUNC_EXIT__;
		return FALSE;
	}

	assertm_if(NULL == genlist, "NULL!!");

	hidden_button_seperator_itc.item_style = "dialogue/seperator";
	hidden_button_seperator_itc.func.text_get = NULL;
	hidden_button_seperator_itc.func.content_get = NULL;
	hidden_button_seperator_itc.func.state_get = NULL;
	hidden_button_seperator_itc.func.del = NULL;

	hidden_button_itc.item_style = "dialogue/bg/1icon";
	hidden_button_itc.func.text_get = NULL;
	hidden_button_itc.func.content_get = _gl_hidden_btn_content_get;
	hidden_button_itc.func.state_get = NULL;
	hidden_button_itc.func.del = NULL;

	manager_object->item_sep_above_hidden_button = elm_genlist_item_append(genlist, &hidden_button_seperator_itc, NULL, NULL, ELM_GENLIST_ITEM_GROUP, NULL, NULL);
	assertm_if(NULL == manager_object->item_sep_above_hidden_button, "NULL!!");
	elm_genlist_item_select_mode_set(manager_object->item_sep_above_hidden_button, ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);
	
	manager_object->item_hidden_btn = elm_genlist_item_append(genlist, &hidden_button_itc, NULL, NULL, ELM_GENLIST_ITEM_NONE, _hiddne_button_callback, NULL);
	assertm_if(NULL == manager_object->item_hidden_btn, "NULL!!");

	manager_object->item_sep_below_hidden_button = elm_genlist_item_append(genlist, &hidden_button_seperator_itc, NULL, NULL, ELM_GENLIST_ITEM_GROUP, NULL, NULL);
	assertm_if(NULL == manager_object->item_sep_below_hidden_button, "NULL!!");
	elm_genlist_item_select_mode_set(manager_object->item_sep_below_hidden_button, ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);

	__COMMON_FUNC_EXIT__;
	return TRUE;
}

Eina_Bool viewer_manager_scan_button_set(Eina_Bool show_state)
{
	__COMMON_FUNC_ENTER__;

	if(NULL == manager_object) {
		__COMMON_FUNC_EXIT__;
		return EINA_FALSE;
	}
	
	if(show_state == EINA_TRUE) {
		if(app_state->current_view == VIEW_MAIN) {
			INFO_LOG(UG_NAME_NORMAL,"Show directly");
			elm_object_item_disabled_set(manager_object->scan_button, EINA_FALSE);
		} else {
			INFO_LOG(UG_NAME_NORMAL,"Show reserve");
		}
	} else if (show_state == EINA_FALSE) {
		elm_object_item_disabled_set(manager_object->scan_button, EINA_TRUE);
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
	manager_object->current_selected_item = NULL;
	manager_object->click_enabled = EINA_TRUE;
	manager_object->update_enabled = EINA_TRUE;
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

	/* Push in Navigationbar */
	Elm_Object_Item* navi_it = elm_naviframe_item_push(manager_object->nav, sc(PACKAGE, I18N_TYPE_Wi_Fi), button_back, NULL, view_content, NULL);
	evas_object_smart_callback_add(manager_object->nav, "transition,finished", (Evas_Smart_Cb)_hide_finished_cb, navi_it);

	/* Add Toolbar */
	Evas_Object *toolbar = elm_toolbar_add(manager_object->nav);
	elm_toolbar_shrink_mode_set(toolbar, ELM_TOOLBAR_SHRINK_EXPAND);

	manager_object->scan_button = elm_toolbar_item_append(toolbar, NULL, sc(PACKAGE, I18N_TYPE_Scan), _refresh_sk_cb, NULL);
	elm_object_item_disabled_set(elm_toolbar_item_append(toolbar, NULL, "", NULL, NULL), EINA_TRUE);
	elm_object_item_part_content_set(navi_it, "controlbar", toolbar);

	elm_object_style_set(button_back, "naviframe/back_btn/default");
	evas_object_smart_callback_add(button_back, "clicked", (Evas_Smart_Cb)_back_sk_cb, NULL);

	evas_object_show(layout);

	__COMMON_FUNC_EXIT__;
	return layout;
}

Eina_Bool viewer_manager_destroy()
{
	__COMMON_FUNC_ENTER__;

	viewer_list_destroy();

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
		viewer_list_title_item_set(manager_object->item_bottom);
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

	genlist_data* gdata = (genlist_data*) data;

	if (manager_object->click_enabled == EINA_FALSE) {
		INFO_LOG(UG_NAME_NORMAL, "normal callback cancel by proccesing something");
		__COMMON_FUNC_EXIT__;
		return;
	}

	DEBUG_LOG(UG_NAME_NORMAL, "event_info:[%s]", (char*) elm_object_style_get(obj));

	radio_button_cb(gdata->device_info, obj, event_info);
	elm_genlist_item_selected_set((Elm_Object_Item *)event_info, FALSE);

	__COMMON_FUNC_EXIT__;
	return;
}

Elm_Object_Item* viewer_manager_item_set(void* entry_data,
				const char* ssid,
				const char* ap_image_path,
				VIEWER_ITEM_RADIO_MODES mode,
				VIEWER_CALLBACK_TYPES type,
				void*callback_data)
{
	__COMMON_FUNC_ENTER__;

	Elm_Object_Item *item = NULL;
	assertm_if(NULL == manager_object, "manager_object is NULL!!");
	assertm_if(NULL == manager_object->list, "manager_object->list is NULL!!");

	switch (type) {
	case VIEWER_CALLBACK_TYPE_NORMAL_LIST:
		item = viewer_list_item_set(manager_object->list,
					entry_data,
					ssid,
					ap_image_path,
					mode,
					viewer_manager_genlist_normal_callback,
					callback_data);
		break;
	case VIEWER_CALLBACK_TYPE_NONE_AP_LIST:
		item = viewer_list_item_set(manager_object->list,
					entry_data,
					ssid,
					ap_image_path,
					mode,
					NULL,
					callback_data);
		elm_genlist_item_select_mode_set(item, ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);
		break;
	default:
		break;
	}

	elm_genlist_item_update(item);

	__COMMON_FUNC_EXIT__;
	return item;
}

Elm_Object_Item *viewer_manager_current_selected_item_get(void)
{
	__COMMON_FUNC_ENTER__;

	assertm_if(NULL == manager_object, "manager_object is NULL!!");
	assertm_if(NULL == manager_object->current_selected_item, "manager_object->current_selected_item is NULL!!");

	__COMMON_FUNC_EXIT__;
	return manager_object->current_selected_item;
}

void viewer_manager_current_selected_item_set(Elm_Object_Item *item)
{
	__COMMON_FUNC_ENTER__;

	assertm_if(NULL == manager_object, "manager_object is NULL!!");
	assertm_if(NULL == item, "current is NULL!!");

	manager_object->current_selected_item = item;

	__COMMON_FUNC_EXIT__;
}

void viewer_manager_set_enabled_list_click(Eina_Bool enabled)
{
	__COMMON_FUNC_ENTER__;
	if (manager_object == NULL)
		return;
	manager_object->click_enabled = enabled;
	__COMMON_FUNC_EXIT__;
}

void viewer_manager_set_enabled_list_update(Eina_Bool enabled)
{
	__COMMON_FUNC_ENTER__;
	if (manager_object == NULL)
		return;
	manager_object->update_enabled = enabled;
	__COMMON_FUNC_EXIT__;
}

int viewer_manager_item_radio_mode_all_reset()
{
	__COMMON_FUNC_ENTER__;

	Elm_Object_Item *gli = viewer_list_item_first_get(manager_object->list);
	int searched_device_num = wlan_manager_profile_scanned_length_get();
	int i=0;

	for (i=0; i<searched_device_num; i++) {
		viewer_manager_item_radio_mode_set(NULL, gli , VIEWER_ITEM_RADIO_MODE_OFF);
		gli = viewer_list_item_next_get(gli);
	}

	__COMMON_FUNC_EXIT__;
	return TRUE;
}

int viewer_manager_item_radio_mode_set(void* object, Elm_Object_Item* item, VIEWER_ITEM_RADIO_MODES mode)
{
	assertm_if(NULL == manager_object, "manager_object is NULL!!");
	assertm_if(NULL == item, "item is NULL!!");

	if (NULL == item) {
		return FALSE;
	}

	INFO_LOG(UG_NAME_NORMAL, "radio mode %d\n", mode);
	 
	genlist_data* gdata = (genlist_data*) viewer_list_item_data_get(item, "data");
	if (NULL == gdata) {
		return FALSE;
	}
	gdata->radio_mode = mode;

	DEBUG_LOG(UG_NAME_NORMAL, "ssid:[%s], radiomode:[%d]", gdata->ssid, gdata->radio_mode);

	switch (mode) {
	case VIEWER_ITEM_RADIO_MODE_NULL:
		INFO_LOG(UG_NAME_NORMAL, "radio mode NULL");
		manager_object->click_enabled = EINA_TRUE;
		break;

	case VIEWER_ITEM_RADIO_MODE_OFF:
		INFO_LOG(UG_NAME_NORMAL, "radio mode off");
		manager_object->click_enabled = EINA_TRUE;
		break;

	case VIEWER_ITEM_RADIO_MODE_CONNECTED:
		INFO_LOG(UG_NAME_NORMAL, "radio mode connected");
		manager_object->click_enabled = EINA_TRUE;
		break;

	case VIEWER_ITEM_RADIO_MODE_CONNECTING:
		INFO_LOG(UG_NAME_NORMAL, "radio mode connecting");
		manager_object->click_enabled = EINA_TRUE;
		break;

	case VIEWER_ITEM_RADIO_MODE_CANCEL_CONNECTING:
		INFO_LOG(UG_NAME_NORMAL, "radio mode cancel connecting");
		manager_object->click_enabled = EINA_FALSE;
		break;

	case VIEWER_ITEM_RADIO_MODE_DISCONNECTING:
		INFO_LOG(UG_NAME_NORMAL, "radio mode disconnecting");
		manager_object->click_enabled = EINA_FALSE;
		break;

	default:
		INFO_LOG(UG_NAME_NORMAL, "unsupported modea [%d]", mode);
		break;
	}

	viewer_list_item_data_set(item, "data", gdata);
	elm_genlist_item_update(item);

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

void viewer_manager_scroll_to_top()
{
	if (manager_object->item_header == NULL)
		return;

	ecore_idler_add((Ecore_Task_Cb)elm_genlist_item_bring_in, manager_object->item_header);
}

int viewer_manager_header_mode_set(HEADER_MODES mode)
{
	__COMMON_FUNC_ENTER__;

	assertm_if(NULL == manager_object, "NULL!!");
	assertm_if(NULL == manager_object->item_header, "NULL!!");
	assertm_if(HEADER_MODE_OFF > mode || HEADER_MODE_MAX <= mode, "Err!!");

	DEBUG_LOG(UG_NAME_NORMAL, "mode [%d]", mode);
	manager_object->header_mode = mode;

	if (NULL != manager_object->header_text) {
		g_free(manager_object->header_text);
		manager_object->header_text = NULL;
	}

	char buf[30] = {0,};
	switch (mode) {
		case HEADER_MODE_OFF:
			viewer_manager_hidden_disable_set(FALSE);
			viewer_manager_scan_button_set(EINA_TRUE);
			manager_object->header_text = strdup(sc(PACKAGE, I18N_TYPE_Wi_Fi));
			break;
		case HEADER_MODE_ON:
			viewer_manager_hidden_disable_set(FALSE);
			viewer_manager_scan_button_set(EINA_TRUE);
			manager_object->header_text = strdup(sc(PACKAGE, I18N_TYPE_Wi_Fi));
			break;
		case HEADER_MODE_ACTIVATING:
			viewer_manager_hidden_disable_set(TRUE);
			snprintf(buf, sizeof(buf), "%s %s", sc(PACKAGE, I18N_TYPE_Wi_Fi), sc(PACKAGE, I18N_TYPE_Activating));
			manager_object->header_text = strdup(buf);
			break;
		case HEADER_MODE_CONNECTING:
			viewer_manager_hidden_disable_set(TRUE);
			snprintf(buf, sizeof(buf), "%s %s", sc(PACKAGE, I18N_TYPE_Wi_Fi), sc(PACKAGE, I18N_TYPE_Connecting));
			manager_object->header_text = strdup(buf);
			break;
		case HEADER_MODE_CONNECTED:
			viewer_manager_hidden_disable_set(FALSE);
			viewer_manager_scan_button_set(EINA_TRUE);
			snprintf(buf, sizeof(buf), "%s %s", sc(PACKAGE, I18N_TYPE_Wi_Fi), sc(PACKAGE, I18N_TYPE_Connected));
			manager_object->header_text = strdup(buf);
			break;
		case HEADER_MODE_DISCONNECTING:
			viewer_manager_hidden_disable_set(TRUE);
			snprintf(buf, sizeof(buf), "%s %s", sc(PACKAGE, I18N_TYPE_Wi_Fi), sc(PACKAGE, I18N_TYPE_Disconnecting));
			manager_object->header_text = strdup(buf);
			break;
		case HEADER_MODE_DEACTIVATING:
			viewer_manager_hidden_disable_set(TRUE);
			snprintf(buf, sizeof(buf), "%s %s", sc(PACKAGE, I18N_TYPE_Wi_Fi), sc(PACKAGE, I18N_TYPE_Deactivating));
			manager_object->header_text = strdup(buf);
			break;
		case HEADER_MODE_CANCEL_CONNECTING:
			viewer_manager_hidden_disable_set(TRUE);
			manager_object->header_text = strdup("Wi-Fi cancel connecting...");
			break;
		case HEADER_MODE_SEARCHING:
			viewer_manager_hidden_disable_set(TRUE);
			snprintf(buf, sizeof(buf), "%s %s", sc(PACKAGE, I18N_TYPE_Wi_Fi), sc(PACKAGE, I18N_TYPE_Searching));
			manager_object->header_text = strdup(buf);
			break;
		default:
			assertm_if(TRUE, "Err!!");
			break;
	}

	elm_genlist_item_update(manager_object->item_header);

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

static char* viewer_manager_get_device_icon(int security_mode, int rssi)
{
	char tmp_str[128] = {0,};
	char *ret;

	sprintf(tmp_str, "%s/37_wifi_icon", WIFI_APP_IMAGE_DIR);

	if (security_mode != WLAN_SEC_MODE_NONE) {
		sprintf(tmp_str,"%s_lock", tmp_str);
	}

	switch (wlan_manager_get_signal_strength(rssi)) {
	case SIGNAL_STRENGTH_TYPE_EXCELLENT:
		sprintf(tmp_str,"%s_03", tmp_str);
		break;
	case SIGNAL_STRENGTH_TYPE_GOOD:
		sprintf(tmp_str,"%s_02", tmp_str);
		break;
	case SIGNAL_STRENGTH_TYPE_WEAK:
		sprintf(tmp_str,"%s_01", tmp_str);
		break;
	case SIGNAL_STRENGTH_TYPE_VERY_WEAK:
	case SIGNAL_STRENGTH_TYPE_NULL:
		sprintf(tmp_str,"%s_00", tmp_str);
		break;
	}

	sprintf(tmp_str, "%s.png", tmp_str);
	
	ret = strdup(tmp_str);
	return ret;
}

static void viewer_manager_update_list_one(net_profile_info_t *profile_info)
{
	__COMMON_FUNC_ENTER__;

	wifi_device_info_t *wifi_device = (wifi_device_info_t*)malloc(sizeof(wifi_device_info_t));
	memset(wifi_device, 0, sizeof(wifi_device_info_t));

	wifi_device->profile_name = strdup(profile_info->ProfileName);
	wifi_device->ssid = strdup(profile_info->ProfileInfo.Wlan.essid);
	wifi_device->rssi = (int)profile_info->ProfileInfo.Wlan.Strength;
	wifi_device->security_mode = (int)profile_info->ProfileInfo.Wlan.security_info.sec_mode;
	wifi_device->wps_mode = (int)profile_info->ProfileInfo.Wlan.security_info.wps_support;

	char *ap_image = NULL;
	VIEWER_ITEM_RADIO_MODES mode = VIEWER_ITEM_RADIO_MODE_NULL;
	ap_image = viewer_manager_get_device_icon(wifi_device->security_mode, wifi_device->rssi);
	mode = VIEWER_ITEM_RADIO_MODE_NULL;

	if (ap_image != NULL) {
		viewer_manager_item_set(wifi_device, wifi_device->ssid, ap_image, mode, VIEWER_CALLBACK_TYPE_NORMAL_LIST, wifi_device);
		g_free(ap_image);
	}

	__COMMON_FUNC_EXIT__;
}

static void viewer_manager_update_list_one_connected(net_profile_info_t *profile_info, const char *connected_ssid)
{
	__COMMON_FUNC_ENTER__;

	wifi_device_info_t *wifi_device = (wifi_device_info_t*)malloc(sizeof(wifi_device_info_t));
	memset(wifi_device, 0, sizeof(wifi_device_info_t));

	wifi_device->profile_name = strdup(profile_info->ProfileName);
	wifi_device->ssid = strdup(profile_info->ProfileInfo.Wlan.essid);
	wifi_device->rssi = (int)profile_info->ProfileInfo.Wlan.Strength;
	wifi_device->security_mode = (int)profile_info->ProfileInfo.Wlan.security_info.sec_mode;
	wifi_device->wps_mode = (int)profile_info->ProfileInfo.Wlan.security_info.wps_support;

	char *ap_image = NULL;
	VIEWER_ITEM_RADIO_MODES mode = VIEWER_ITEM_RADIO_MODE_NULL;
	if (strcmp(wifi_device->ssid, connected_ssid) == 0) {
		INFO_LOG(UG_NAME_NORMAL, "update ssid [%s] : connected\n", wifi_device->ssid);
		viewer_manager_header_mode_set(HEADER_MODE_CONNECTED);
		ap_image = viewer_manager_get_device_icon(wifi_device->security_mode, wifi_device->rssi);
		mode = VIEWER_ITEM_RADIO_MODE_CONNECTED;
	} else {
		INFO_LOG(UG_NAME_NORMAL, "update ssid [%s]\n", wifi_device->ssid);
		ap_image = viewer_manager_get_device_icon(wifi_device->security_mode, wifi_device->rssi);
		mode = VIEWER_ITEM_RADIO_MODE_NULL;
	}

	if (ap_image != NULL) {
		viewer_manager_item_set(wifi_device, wifi_device->ssid, ap_image, mode, VIEWER_CALLBACK_TYPE_NORMAL_LIST, wifi_device);
		g_free(ap_image);
	}

	__COMMON_FUNC_EXIT__;
}

static void viewer_manager_update_list_all()
{
	int i = 0;
	struct wifi_appdata *ad = app_state;

	__COMMON_FUNC_ENTER__;

	viewer_list_item_clear();

	net_profile_info_t *profiles_list = wlan_manager_profile_table_get();
	if (profiles_list == NULL)
		return;

	int profiles_list_size = wlan_manager_profile_scanned_length_get();
	INFO_LOG(UG_NAME_NORMAL, "profiles list count [%d]\n", profiles_list_size);

	if (profiles_list_size > 0) {
		const char *connected_ssid = wlan_manager_get_connected_ssid();
		INFO_LOG(UG_NAME_NORMAL, "connected ssid [%s]\n", connected_ssid);
		if (connected_ssid == NULL) {
			for (i = 0; i < profiles_list_size; i++) {
				viewer_manager_update_list_one(profiles_list+i);
			}
		} else {
			for (i = 0; i < profiles_list_size; i++) {
				viewer_manager_update_list_one_connected(profiles_list+i, connected_ssid);
			}
		}
	} else if (profiles_list_size == 0) {
		/* if there is no scan_data, generate No-AP item */
		wifi_device_info_t *device_info = NULL;
		device_info = (wifi_device_info_t *)wlan_manager_profile_device_info_blank_create();
		const char* ssid_name = device_info->ssid;
		const char* ap_image = NULL;
		viewer_manager_item_set(device_info, ssid_name, ap_image, VIEWER_ITEM_RADIO_MODE_NULL, VIEWER_CALLBACK_TYPE_NONE_AP_LIST, ad);
	} else {
		ERROR_LOG(UG_NAME_NORMAL, "Err!");
	}

	__COMMON_FUNC_EXIT__;
}

Eina_Bool viewer_manager_refresh(int is_scan)
{
	INFO_LOG(UG_NAME_SCAN, "UI update start");

	if (manager_object == NULL)
		return EINA_FALSE;

	if (manager_object->update_enabled == EINA_FALSE) {
		INFO_LOG(UG_NAME_SCAN, "UI update disabled");
		return EINA_FALSE;
	}

	if (is_scan) {
		viewer_manager_hide(VIEWER_WINSET_SEARCHING);
		viewer_manager_header_mode_set(HEADER_MODE_ON);
		viewer_manager_update_list_all();
	} else {
		viewer_manager_update_list_all();

		char profile_name[NET_PROFILE_NAME_LEN_MAX+1] = "";
		if (wlan_manager_state_get(profile_name) == WLAN_MANAGER_CONNECTING) {
			Elm_Object_Item* target_item = item_get_for_profile_name(profile_name);
			viewer_manager_header_mode_set(HEADER_MODE_CONNECTING);
			viewer_manager_item_radio_mode_set(NULL, target_item, VIEWER_ITEM_RADIO_MODE_CONNECTING);
		}
	}

	INFO_LOG(UG_NAME_SCAN, "UI update finish");

	return EINA_TRUE;
}
