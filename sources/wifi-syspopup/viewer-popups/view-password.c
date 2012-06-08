/*
  * Copyright 2012  Samsung Electronics Co., Ltd
  *
  * Licensed under the Flora License, Version 1.0 (the "License");
  * you may not use this file except in compliance with the License.
  * You may obtain a copy of the License at
  *
  *    http://www.tizenopensource.org/license
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  */



#include "common.h"
#include "wifi-syspopup.h"
#include "view-password.h"
#include "wlan_manager.h"
#include "view-main.h"
#include "view-alerts.h"
#include "i18nmanager.h"


extern wifi_object* app_state;
static Evas_Object* _entry = NULL;

typedef struct _qs_view_password_data {
	char *profile_name;
	int security_mode;
} qs_view_password_data;
static qs_view_password_data *_qs_password_data = NULL;

static void _entry_changed_cb(void* data, Evas_Object* obj, void* event_info)
{
	__COMMON_FUNC_ENTER__;

	if (elm_object_focus_get(data)) {
		if (elm_entry_is_empty(obj))
			elm_object_signal_emit(data, "elm,state,eraser,hide", "elm");
		else
			elm_object_signal_emit(data, "elm,state,eraser,show", "elm");
	}

	__COMMON_FUNC_EXIT__;
}

static void _entry_focused_cb(void* data, Evas_Object* obj, void* event_info)
{
	if (!elm_entry_is_empty(obj))
		elm_object_signal_emit(data, "elm,state,eraser,show", "elm");

	elm_object_signal_emit(data, "elm,state,guidetext,hide", "elm");
}

static void _entry_unfocused_cb(void* data, Evas_Object* obj, void* event_info)
{
	if (elm_entry_is_empty(obj))
		elm_object_signal_emit(data, "elm,state,guidetext,show", "elm");

	elm_object_signal_emit(data, "elm,state,eraser,hide", "elm");
}

static void _eraser_clicked_cb(void* data, Evas_Object* obj, const char* emission, const char* source)
{
	elm_entry_entry_set(data, "");
}

static void _popup_ok_cb(void *data, Evas_Object *obj, void *event_info)
{
	__COMMON_FUNC_ENTER__;

	assertm_if(NULL == data, "data is NULL!!");
	assertm_if(NULL == obj, "obj is NULL!!");
	assertm_if(NULL == event_info, "event_info is NULL!!");

	char* password = NULL;
	int len_password = 0;
	int ret = -1;

	password = elm_entry_markup_to_utf8(elm_entry_entry_get(_entry));
	len_password = strlen(password);
	INFO_LOG(SP_NAME_NORMAL, "* password len [%d]", len_password);

	if(len_password == 5 || (len_password > 7 && len_password < 64) ) {
		wlan_manager_password_data* param= (wlan_manager_password_data*) g_malloc0(sizeof(wlan_manager_password_data));
		assertm_if(NULL == param, "param is NULL!!");

		param->wlan_eap_type = WLAN_MANAGER_EAP_TYPE_NONE;
		param->password = strdup(password);
		g_free(password);

		genlist_data *gdata = (genlist_data *)data;
		ret = wlan_manager_connect_with_password(_qs_password_data->profile_name, _qs_password_data->security_mode, param);
		g_free(param->password);
		g_free(param);

		view_main_item_connection_mode_set(gdata, ITEM_CONNECTION_MODE_CONNECTING);
	} else {
		view_alerts_password_length_error_show();
		return;
	}

	if (_qs_password_data) {
		g_free(_qs_password_data);
		_qs_password_data = NULL;
	}

	evas_object_del(app_state->passpopup);
	app_state->passpopup = NULL;

	__COMMON_FUNC_EXIT__;
}

static void _popup_cancel_cb(void *data, Evas_Object *obj, void *event_info)
{
	__COMMON_FUNC_ENTER__;

	INFO_LOG(SP_NAME_NORMAL, "button cancel");
	if (_qs_password_data) {
		g_free(_qs_password_data);
		_qs_password_data = NULL;
	}

	evas_object_del(app_state->passpopup);
	app_state->passpopup = NULL;

	__COMMON_FUNC_EXIT__;
}

static void _check_changed_cb(void *data, Evas_Object *obj, void *event_info)
{
	if (obj == NULL)
		return;

	Eina_Bool state = elm_check_state_get(obj);
	elm_entry_password_set(_entry, !state);
}

int view_password_show(genlist_data *gdata)
{
	__COMMON_FUNC_ENTER__;

	if (gdata == NULL)
		return FALSE;

	assertm_if(NULL == app_state, "app_state is NULL!!");
	assertm_if(NULL == app_state->win_main, "app_state->win_main is NULL!!");

	_qs_password_data = (qs_view_password_data *) malloc(sizeof(qs_view_password_data));
	if (_qs_password_data == NULL)
		return FALSE;

	memset(_qs_password_data, 0, sizeof(qs_view_password_data));
	_qs_password_data->profile_name = strdup(gdata->dev_info->profile_name);
	_qs_password_data->security_mode = gdata->dev_info->security_mode;

	Evas_Object *conformant = NULL;
	conformant = elm_conformant_add(app_state->win_main);
	assertm_if(NULL == conformant, "conformant is NULL!!");
	elm_win_conformant_set(app_state->win_main, EINA_TRUE);
	elm_win_resize_object_add(app_state->win_main, conformant);
	evas_object_size_hint_weight_set(conformant, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(conformant, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(conformant);

	Evas_Object *content = elm_layout_add(conformant);
	elm_object_content_set(conformant, content);
	app_state->passpopup = NULL;
	app_state->passpopup = elm_popup_add(content);
	assertm_if(NULL == app_state->syspopup, "app_state->syspopup is NULL!!");
	elm_object_part_text_set(app_state->passpopup, "title,text", sc(PACKAGE, I18N_TYPE_Password));

	Evas_Object *box = elm_box_add(app_state->syspopup);
	evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(box);

	Evas_Object *ly_editfield = elm_layout_add(box);
	elm_layout_theme_set(ly_editfield, "layout", "editfield", "title");
	evas_object_size_hint_weight_set(ly_editfield, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(ly_editfield, EVAS_HINT_FILL, EVAS_HINT_FILL);
	_entry = elm_entry_add(box);
	elm_object_part_content_set(ly_editfield, "elm.swallow.content", _entry);
	elm_object_part_text_set(ly_editfield, "elm.text", sc(PACKAGE, I18N_TYPE_Input_password));
	elm_entry_single_line_set(_entry, EINA_TRUE);
	elm_entry_scrollable_set(_entry, EINA_TRUE);

	evas_object_smart_callback_add(_entry, "changed", _entry_changed_cb, ly_editfield);
	evas_object_smart_callback_add(_entry, "focused", _entry_focused_cb, ly_editfield);
	evas_object_smart_callback_add(_entry, "unfocused", _entry_unfocused_cb, ly_editfield);
	elm_object_signal_callback_add(ly_editfield, "elm,eraser,clicked", "elm", _eraser_clicked_cb, _entry);

	elm_entry_password_set(_entry, TRUE);
	evas_object_show(_entry);
	evas_object_show(ly_editfield);
	elm_box_pack_end(box, ly_editfield);

	Evas_Object *check = elm_check_add(box);
	elm_object_text_set(check, sc(PACKAGE, I18N_TYPE_Show_password));
	elm_object_focus_allow_set(check, EINA_FALSE);
	evas_object_size_hint_weight_set(check, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(check, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_smart_callback_add(check, "changed", _check_changed_cb, NULL);
	evas_object_show(check);
	elm_box_pack_end(box, check);

	elm_object_content_set(app_state->passpopup, box);
	Evas_Object *btn_ok = elm_button_add(app_state->passpopup);
	elm_object_text_set(btn_ok, sc(PACKAGE, I18N_TYPE_Ok));
	elm_object_part_content_set(app_state->passpopup, "button1", btn_ok);
	evas_object_smart_callback_add(btn_ok, "clicked", _popup_ok_cb, gdata);
	Evas_Object *btn_cancel = elm_button_add(app_state->passpopup);
	elm_object_text_set(btn_cancel, sc(PACKAGE, I18N_TYPE_Cancel));
	elm_object_part_content_set(app_state->passpopup, "button2", btn_cancel);
	evas_object_smart_callback_add(btn_cancel, "clicked", _popup_cancel_cb, gdata);
	evas_object_show(app_state->passpopup);

	__COMMON_FUNC_EXIT__;

	return TRUE;
}
