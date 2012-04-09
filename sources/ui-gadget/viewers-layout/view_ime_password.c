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
#include "view_ime_password.h"
#include "i18nmanager.h"
#include "wlan_manager.h"
#include "viewer_manager.h"
#include "popup.h"


static Elm_Genlist_Item_Class itc;
static Elm_Genlist_Item_Class show_pw_itc;
static Elm_Genlist_Item_Class wps_itc;
static Elm_Genlist_Item_Class wps_sep_itc;

static Evas_Object* button_done = NULL;
static Evas_Object* _entry = NULL;
static int view_ime_password_end = TRUE;

typedef struct _view_ime_password_data {
	char *profile_name;
	int security_mode;
	int wps_mode;
} view_ime_password_data;
view_ime_password_data *_password_data = NULL;


static void _entry_changed_cb(void* data, Evas_Object* obj, void* event_info)
{
	__COMMON_FUNC_ENTER__;

	if (elm_object_focus_get(data)) {
		if (elm_entry_is_empty(obj))
			elm_object_signal_emit(data, "elm,state,eraser,hide", "elm");
		else
			elm_object_signal_emit(data, "elm,state,eraser,show", "elm");
	}

	const char* txt = elm_entry_markup_to_utf8(elm_entry_entry_get(obj));
	int len = 0;

	if (txt != NULL) {
		INFO_LOG(UG_NAME_NORMAL, "* text [%s]", txt);
		len = strlen(txt);

		if (_password_data->security_mode == WLAN_SEC_MODE_WEP) {
			if (len > 0) {
				elm_object_disabled_set(button_done, EINA_FALSE);
			} else {
				elm_object_disabled_set(button_done, EINA_TRUE);
			}
		} else {
			if (len > 7 && len < 64) {
				elm_object_disabled_set(button_done, EINA_FALSE);
			} else {
				elm_object_disabled_set(button_done, EINA_TRUE);
			}
		}
	} else {
		ERROR_LOG(UG_NAME_NORMAL, "Err!");
	}

	__COMMON_FUNC_EXIT__;
}

static void _entry_focused_cb(void *data, Evas_Object *obj, void *event_info)
{
	if (!elm_entry_is_empty(obj))
		elm_object_signal_emit(data, "elm,state,eraser,show", "elm");
	elm_object_signal_emit(data, "elm,state,guidetext,hide", "elm");
}

static void _entry_unfocused_cb(void *data, Evas_Object *obj, void *event_info)
{
	if (elm_entry_is_empty(obj))
		elm_object_signal_emit(data, "elm,state,guidetext,show", "elm");
	elm_object_signal_emit(data, "elm,state,eraser,hide", "elm");
}

static void _eraser_clicked_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	elm_entry_entry_set(data, "");
}

static int _genlist_item_disable_later(void* data)
{
	if(NULL != data) {
		elm_genlist_item_selected_set((Elm_Object_Item*) data, FALSE);
	}
	return FALSE;
}

static void _wps_btn_cb(void* data, Evas_Object* obj, void* event_info)
{
	__COMMON_FUNC_ENTER__;

	if (_password_data == NULL)
		return;

	int ret = wlan_manager_request_wps_connection(_password_data->profile_name);
	if (ret == WLAN_MANAGER_ERR_NONE) {
		viewer_manager_header_mode_set(HEADER_MODE_CONNECTING);
		viewer_manager_item_radio_mode_set(NULL, viewer_manager_current_selected_item_get(), VIEWER_ITEM_RADIO_MODE_WPS_CONNECTING);
		winset_popup_mode_set((void *)_password_data->profile_name, POPUP_MODE_PBC, POPUP_OPTION_NONE);
		__COMMON_FUNC_EXIT__;
	}

	ecore_idler_add( (Ecore_Task_Cb) _genlist_item_disable_later, event_info);

	__COMMON_FUNC_EXIT__;
}

static void _show_pw_changed_cb(void *data, Evas_Object *obj, void *event_info)
{
	if (obj == NULL)
		return;

	Eina_Bool state = elm_check_state_get(obj);
	elm_entry_password_set(_entry, !state);
}

static Evas_Object *_content_get(void *data, Evas_Object *obj, const char *part)
{
	Evas_Object *layout = NULL;
	assertm_if(NULL == part, "NULL!!");

	DEBUG_LOG(UG_NAME_NORMAL, "part [%s]", part);

	if (!strncmp(part, "elm.icon", strlen(part))) {
		layout = elm_layout_add(obj);
		if (layout) {
			elm_layout_theme_set(layout, "layout", "editfield", "title");
			evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
			evas_object_size_hint_align_set(layout, EVAS_HINT_FILL, EVAS_HINT_FILL);
			_entry = elm_entry_add(obj);
			elm_object_part_content_set(layout, "elm.swallow.content", _entry);
			elm_object_part_text_set(layout, "elm.text", sc(PACKAGE, I18N_TYPE_Input_password));
			elm_entry_entry_set(_entry, "");

			evas_object_smart_callback_add(_entry, "changed", _entry_changed_cb, layout);
			evas_object_smart_callback_add(_entry, "focused", _entry_focused_cb, layout);
			evas_object_smart_callback_add(_entry, "unfocused", _entry_unfocused_cb, layout);
			elm_object_signal_callback_add(layout, "elm,eraser,clicked", "elm", _eraser_clicked_cb, _entry);

			elm_entry_password_set(_entry, TRUE);
			elm_entry_input_panel_layout_set(_entry, ELM_INPUT_PANEL_LAYOUT_URL);
			evas_object_show(_entry);
			evas_object_show(layout);
			elm_object_focus_set(layout, EINA_TRUE);
		}
	}

	return layout;
}

static Evas_Object *_show_pw_content_get(void *data, Evas_Object *obj, const char *part)
{
	Evas_Object *check = NULL;

	if (!strncmp(part, "elm.icon", strlen(part))) {
		check = elm_check_add(obj);
		elm_object_text_set(check, sc(PACKAGE, I18N_TYPE_Show_password));
		elm_object_focus_allow_set(check, EINA_FALSE);

		evas_object_size_hint_weight_set(check, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		evas_object_size_hint_align_set(check, EVAS_HINT_FILL, EVAS_HINT_FILL);

		evas_object_smart_callback_add(check, "changed", _show_pw_changed_cb, obj);
	}

	return check;
}

static Evas_Object *_wps_content_get(void *data, Evas_Object *obj, const char *part)
{
	if (obj == NULL)
		return NULL;

	Evas_Object* btn = NULL;

	if(!strncmp(part, "elm.icon", strlen(part))) {
		btn = elm_button_add(obj);
		elm_object_text_set(btn, "WPS connection");
	}

	return btn;
}

static Evas_Object* _create_list(Evas_Object* parent)
{
	assertm_if(NULL == parent, "NULL!!");

	Evas_Object *list = elm_genlist_add(parent);
	elm_genlist_mode_set(list, ELM_LIST_LIMIT);
	evas_object_size_hint_weight_set(list, 
			EVAS_HINT_EXPAND,
			EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(list, 
			EVAS_HINT_FILL, 
			EVAS_HINT_FILL);

	itc.item_style = "1icon";
	itc.func.text_get = NULL;
	itc.func.content_get = _content_get;
	itc.func.state_get = NULL;
	itc.func.del = NULL;

	WLAN_MANAGER_EAP_TYPES det = WLAN_MANAGER_EAP_TYPE_NONE;

	INFO_LOG(UG_NAME_SCAN, "WlanSecMode %d", _password_data->security_mode);
	if (_password_data->security_mode == WLAN_SEC_MODE_IEEE8021X) {
		det =WLAN_MANAGER_EAP_TYPE_ERROR;
	} else {
		det = WLAN_MANAGER_EAP_TYPE_NONE;
	}

	INFO_LOG(UG_NAME_SCAN, "EAP Type %d", det);
	switch (det) {
	case WLAN_MANAGER_EAP_TYPE_NONE:
		elm_genlist_item_append(list, &itc,	NULL, NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);
		break;
	case WLAN_MANAGER_EAP_TYPE_ERROR:
		break;
	default:
		INFO_LOG(UG_NAME_SCAN, "EAP Type error");
		break;
	}

	show_pw_itc.item_style = "1text.1icon.2";
	show_pw_itc.func.text_get = NULL;
	show_pw_itc.func.content_get = _show_pw_content_get;
	show_pw_itc.func.state_get = NULL;
	show_pw_itc.func.del = NULL;
	Elm_Object_Item *item = elm_genlist_item_append(list, &show_pw_itc, NULL, NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);
	elm_genlist_item_select_mode_set(item, ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);
	
	if (_password_data->wps_mode) {
		wps_sep_itc.item_style = "dialogue/separator/20";
		wps_sep_itc.func.text_get = NULL;
		wps_sep_itc.func.content_get = NULL;
		wps_sep_itc.func.state_get = NULL;
		wps_sep_itc.func.del = NULL;
		elm_genlist_item_append(list, &wps_sep_itc, NULL, NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);

		wps_itc.item_style = "dialogue/bg/1icon";
		wps_itc.func.text_get = NULL;
		wps_itc.func.content_get = _wps_content_get;
		wps_itc.func.state_get = NULL;
		wps_itc.func.del = NULL;
		elm_genlist_item_append(list, &wps_itc, NULL, NULL, ELM_GENLIST_ITEM_NONE, _wps_btn_cb, NULL);
	}

	return list;
}

static void view_ime_password_back(void *data,  Evas_Object *obj, void *event_info)
{
	__COMMON_FUNC_ENTER__;

	if(view_ime_password_end == TRUE)
		return;

	view_ime_password_end = TRUE;

	viewer_manager_set_enabled_list_click(EINA_TRUE);
	viewer_manager_set_enabled_list_update(EINA_TRUE);

	view_ime_password_destroy();

	__COMMON_FUNC_EXIT__;
}

static void view_ime_password_done(void *data,  Evas_Object *obj, void *event_info)
{
	__COMMON_FUNC_ENTER__;
	
	if(view_ime_password_end == TRUE) {
		return;
	} 
	view_ime_password_end = TRUE;

	int ret = WLAN_MANAGER_ERR_UNKNOWN;
	wlan_manager_password_data* param = (wlan_manager_password_data*) g_malloc0(sizeof(wlan_manager_password_data));
	param->wlan_eap_type = _password_data->security_mode;

	int nLen = 0;
	const char* szPassword = NULL;
	switch (param->wlan_eap_type) {
	case WLAN_SEC_MODE_WEP:
		szPassword = elm_entry_markup_to_utf8(elm_entry_entry_get(_entry));
		nLen = strlen(szPassword);

		if (nLen == 5 || nLen == 13 || nLen == 26 || nLen == 10) {
			INFO_LOG(UG_NAME_NORMAL, "password = [%s]", szPassword);
		} else {
			winset_popup_mode_set(NULL, POPUP_MODE_CONNECTING_FAILED, POPUP_OPTION_CONNECTIONG_PASSWORD_WEP_ERROR);
			view_ime_password_end = FALSE;
			return;
		}

		param->password = strdup(szPassword);
		viewer_manager_item_radio_mode_all_reset();
		viewer_manager_item_radio_mode_set(NULL, viewer_manager_current_selected_item_get(), VIEWER_ITEM_RADIO_MODE_CONNECTING);
		viewer_manager_header_mode_set(HEADER_MODE_CONNECTING);

		ret = wlan_manager_connect_with_password(_password_data->profile_name, _password_data->security_mode, param);
		break;
	case WLAN_SEC_MODE_WPA_PSK:
	case WLAN_SEC_MODE_WPA2_PSK:
		szPassword = elm_entry_markup_to_utf8(elm_entry_entry_get(_entry));
		nLen = strlen(szPassword);

		if (nLen < 8 || nLen > 63) {
			winset_popup_mode_set(NULL, POPUP_MODE_CONNECTING_FAILED, POPUP_OPTION_CONNECTIONG_PASSWORD_WPAPSK_ERROR);
			view_ime_password_end = FALSE;
			return;
		} else {
			INFO_LOG(UG_NAME_NORMAL, "password = [%s]", szPassword);
		}

		param->password = strdup(szPassword);			
		viewer_manager_item_radio_mode_all_reset();
		viewer_manager_item_radio_mode_set(NULL, viewer_manager_current_selected_item_get(), VIEWER_ITEM_RADIO_MODE_CONNECTING);
		viewer_manager_header_mode_set(HEADER_MODE_CONNECTING);

		ret = wlan_manager_connect_with_password(_password_data->profile_name, _password_data->security_mode, param);
		break;
	default:
		break;
	}

	switch (ret) {
	case WLAN_MANAGER_ERR_NONE:
		INFO_LOG(UG_NAME_SCAN, "connect with password comp");
		break;
	default:
		ERROR_LOG(UG_NAME_SCAN, "wlan error %d", ret);
		viewer_manager_refresh(TRUE);
		break;
	}

	view_ime_password_destroy();

	__COMMON_FUNC_EXIT__;
}

void view_ime_password(wifi_device_info_t *device_info)
{
	__COMMON_FUNC_ENTER__;
	view_ime_password_end = FALSE;

	Evas_Object *navi_frame = viewer_manager_get_naviframe();
	if (navi_frame == NULL)
		return;

	_password_data = (view_ime_password_data *) malloc(sizeof(view_ime_password_data));
	if (_password_data == NULL)
		return;

	memset(_password_data, 0, sizeof(view_ime_password_data));
	_password_data->profile_name = strdup(device_info->profile_name);
	_password_data->security_mode = device_info->security_mode;
	_password_data->wps_mode = device_info->wps_mode;

	Evas_Object* layout = elm_layout_add(navi_frame);
	elm_layout_theme_set(layout, "standard", "window", "integration");
	evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	edje_object_signal_emit(elm_layout_edje_get(layout), "elm,state,show,content", "elm");
	edje_object_signal_emit(elm_layout_edje_get(layout), "elm,bg,show,group_list", "elm");

	Evas_Object* conform = elm_conformant_add(navi_frame);
	elm_object_style_set(conform, "internal_layout");
	elm_object_part_content_set(layout, "elm.swallow.content", conform);

	Evas_Object* list = _create_list(navi_frame);
	elm_object_content_set(conform, list);

	elm_naviframe_prev_btn_auto_pushed_set(navi_frame, EINA_FALSE);
	Elm_Object_Item *navi_it = elm_naviframe_item_push(navi_frame, sc(PACKAGE, I18N_TYPE_Password), NULL, NULL, layout, NULL);
	elm_naviframe_prev_btn_auto_pushed_set(navi_frame, EINA_TRUE);

	Evas_Object* button_back = elm_button_add(navi_frame);
	elm_object_style_set(button_back, "naviframe/title/default");
	elm_object_text_set(button_back, sc(PACKAGE, I18N_TYPE_Back));
	evas_object_smart_callback_add(button_back, "clicked", view_ime_password_back, NULL);
	elm_object_item_part_content_set(navi_it, "title_left_btn", button_back);

	button_done = elm_button_add(navi_frame);
	elm_object_style_set(button_done, "naviframe/title/default");
	elm_object_text_set(button_done, sc(PACKAGE, I18N_TYPE_Done));
	evas_object_smart_callback_add(button_done, "clicked", (Evas_Smart_Cb) view_ime_password_done, NULL);
	elm_object_item_part_content_set(navi_it, "title_right_btn", button_done);

	evas_object_show(layout);

	__COMMON_FUNC_EXIT__;
}

void view_ime_password_destroy()
{
	if (_entry) {
		evas_object_smart_callback_del(_entry, "changed", _entry_changed_cb);
		evas_object_smart_callback_del(_entry, "focused", _entry_focused_cb);
		evas_object_smart_callback_del(_entry, "unfocused", _entry_unfocused_cb);
		_entry = NULL;
	}

	if (_password_data) {
		g_free(_password_data);
		_password_data = NULL;
	}

	elm_naviframe_item_pop(viewer_manager_get_naviframe());
}
