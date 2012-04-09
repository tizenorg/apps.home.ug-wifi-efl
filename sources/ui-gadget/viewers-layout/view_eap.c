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
#include "view_eap.h"
#include "i18nmanager.h"
#include "wlan_manager.h"
#include "viewer_manager.h"
#include "popup.h"


static Elm_Genlist_Item_Class seperator_itc;
static Elm_Genlist_Item_Class eap_type_itc;
static Elm_Genlist_Item_Class eap_type_sub_itc;
static Elm_Genlist_Item_Class eap_auth_itc;
static Elm_Genlist_Item_Class eap_auth_sub_itc;
static Elm_Genlist_Item_Class eap_id_itc;
static Elm_Genlist_Item_Class eap_anonyid_itc;
static Elm_Genlist_Item_Class eap_pw_itc;


static int view_eap_end = TRUE;
static Elm_Object_Item *_button_done_item = NULL;
static Evas_Object *_entry[3];
struct _Expand_List {
	char depth;
	char *name;
	Elm_Genlist_Item_Type flags;
};

static struct _Expand_List list_eap_type[] = {
	{1, "PEAP", ELM_GENLIST_ITEM_NONE},
	{1, "TLS", ELM_GENLIST_ITEM_NONE},
	{1, "TTLS", ELM_GENLIST_ITEM_NONE},
	{1, "SIM", ELM_GENLIST_ITEM_NONE},
	{1, "AKA", ELM_GENLIST_ITEM_NONE},
	{1, NULL, ELM_GENLIST_ITEM_NONE}
};

static struct _Expand_List list_eap_auth[] = {
	{1, "NONE", ELM_GENLIST_ITEM_NONE},
	{1, "PAP", ELM_GENLIST_ITEM_NONE},
	{1, "MSCHAP", ELM_GENLIST_ITEM_NONE},
	{1, "MSCHAPV2", ELM_GENLIST_ITEM_NONE},
	{1, "GTC", ELM_GENLIST_ITEM_NONE},
	{1, NULL, ELM_GENLIST_ITEM_NONE}
};

typedef struct _view_eap_data {
	char *profile_name;
	char *ssid;
	int rssi;
	int security_mode;
} view_eap_data;

static int expandable_list_index = 0;
static Elm_Object_Item *eap_type_item = NULL;
static Elm_Object_Item *eap_auth_item = NULL;
static Elm_Object_Item *eap_id_item = NULL;
static Elm_Object_Item *eap_anonyid_item = NULL;
static Elm_Object_Item *eap_pw_item = NULL;

static void _gl_eap_auth_sel(void *data, Evas_Object *obj, void *event_info);
static void view_eap_destroy(void *data);

static Evas_Object* _create_layout(Evas_Object *parent)
{
	assertm_if(NULL == parent, "NULL!!");

	Evas_Object *layout=NULL;
	layout = elm_layout_add(parent);
	assertm_if(NULL == layout, "NULL!!");

	elm_layout_theme_set(layout, "standard", "window", "integration");
	evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	edje_object_signal_emit(elm_layout_edje_get(layout), "elm,state,show,content", "elm");
	edje_object_signal_emit(elm_layout_edje_get(layout), "elm,bg,show,group_list", "elm");

	return layout;
}

static void _set_list(Evas_Object *obj, eap_type_t pre_type, eap_type_t new_type)
{
	switch (new_type) {
	case EAP_SEC_TYPE_PEAP:
	case EAP_SEC_TYPE_TLS:
	case EAP_SEC_TYPE_TTLS:
		if (pre_type > EAP_SEC_TYPE_TTLS) {
			eap_auth_item = elm_genlist_item_append(obj, &eap_auth_itc, NULL, NULL, ELM_GENLIST_ITEM_TREE, _gl_eap_auth_sel, NULL);
			eap_id_item = elm_genlist_item_append(obj, &eap_id_itc, NULL, NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);
			eap_anonyid_item = elm_genlist_item_append(obj, &eap_anonyid_itc, NULL, NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);
			eap_pw_item = elm_genlist_item_append(obj, &eap_pw_itc, NULL, NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);
		}
		break;
	case EAP_SEC_TYPE_SIM:
	case EAP_SEC_TYPE_AKA:
		if (pre_type < EAP_SEC_TYPE_SIM) {
			elm_object_item_del(eap_auth_item);
			elm_object_item_del(eap_id_item);
			elm_object_item_del(eap_anonyid_item);
			elm_object_item_del(eap_pw_item);
		}
		break;
	default:
		break;
	}
}

static void _entry_changed_cb(void* data, Evas_Object* obj, void* event_info)
{
	if (obj == NULL)
		return;

	if (elm_object_focus_get(data)) {
		if (elm_entry_is_empty(obj))
			elm_object_signal_emit(data, "elm,state,eraser,hide", "elm");
		else
			elm_object_signal_emit(data, "elm,state,eraser,show", "elm");
	}
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

static void _gl_eap_type_sel(void *data, Evas_Object *obj, void *event_info)
{
	Elm_Object_Item *item = (Elm_Object_Item *)event_info;
	Eina_Bool expanded = EINA_FALSE;
	if (item)
		elm_genlist_item_selected_set(item, EINA_FALSE);

	expandable_list_index = 0;

	expanded = elm_genlist_item_expanded_get(item);
	elm_genlist_item_expanded_set(item, !expanded);
}

static void _gl_eap_type_sub_sel(void *data, Evas_Object *obj, void *event_info)
{
	Elm_Object_Item *item = (Elm_Object_Item *)event_info;
	Elm_Object_Item *parent_item = elm_genlist_item_parent_get(item);

	int pre_index = (int)elm_object_item_data_get(parent_item);
	int new_index = (int)data;

	elm_object_item_data_set(parent_item, data);

	Eina_Bool expanded = EINA_FALSE;
	expanded = elm_genlist_item_expanded_get(parent_item);
	elm_genlist_item_expanded_set(parent_item, !expanded);

	if (pre_index != new_index) {
		_set_list(obj, pre_index, new_index);
		elm_genlist_item_update(parent_item);
	}
}

static void _gl_eap_auth_sel(void *data, Evas_Object *obj, void *event_info)
{
	Elm_Object_Item *item = (Elm_Object_Item *)event_info;
	Eina_Bool expanded = EINA_FALSE;
	if (item)
		elm_genlist_item_selected_set(item, EINA_FALSE);

	expandable_list_index = 1;

	expanded = elm_genlist_item_expanded_get(item);
	elm_genlist_item_expanded_set(item, !expanded);
}

static void _gl_eap_auth_sub_sel(void *data, Evas_Object *obj, void *event_info)
{
	Elm_Object_Item *item = (Elm_Object_Item *)event_info;
	Elm_Object_Item *parent_item = elm_genlist_item_parent_get(item);

	elm_object_item_data_set(parent_item, data);

	Eina_Bool expanded = EINA_FALSE;
	expanded = elm_genlist_item_expanded_get(parent_item);
	elm_genlist_item_expanded_set(parent_item, !expanded);
}

static Evas_Object *_gl_eap_id_content_get(void *data, Evas_Object *obj, const char *part)
{
	Evas_Object* layout = NULL;
	if (!strncmp(part, "elm.icon", strlen(part))) {
		layout = elm_layout_add(obj);
		if (layout) {
			elm_layout_theme_set(layout, "layout", "editfield", "title");
			evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
			evas_object_size_hint_align_set(layout, EVAS_HINT_FILL, EVAS_HINT_FILL);
			elm_object_part_text_set(layout, "elm.text", _("ID"));

			_entry[0] = elm_entry_add(obj);
			elm_object_part_content_set(layout, "elm.swallow.content", _entry[0]);
			elm_entry_single_line_set(_entry[0], EINA_TRUE);
			elm_entry_scrollable_set(_entry[0], EINA_TRUE);
			elm_entry_entry_set(_entry[0], "");

			evas_object_smart_callback_add(_entry[0], "changed", _entry_changed_cb, layout);
			evas_object_smart_callback_add(_entry[0], "focused", _entry_focused_cb, layout);
			evas_object_smart_callback_add(_entry[0], "unfocused", _entry_unfocused_cb, layout);
			elm_object_signal_callback_add(layout, "elm,eraser,clicked", "elm", _eraser_clicked_cb, _entry[0]);
		}
	}

	return layout;
}

static Evas_Object *_gl_eap_anonyid_content_get(void *data, Evas_Object *obj, const char *part)
{
	Evas_Object* layout = NULL;
	if (!strncmp(part, "elm.icon", strlen(part))) {
		layout = elm_layout_add(obj);
		if (layout) {
			elm_layout_theme_set(layout, "layout", "editfield", "title");
			evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
			evas_object_size_hint_align_set(layout, EVAS_HINT_FILL, EVAS_HINT_FILL);
			elm_object_part_text_set(layout, "elm.text", _("Anonymous ID"));

			_entry[1] = elm_entry_add(obj);
			elm_object_part_content_set(layout, "elm.swallow.content", _entry[1]);
			elm_entry_single_line_set(_entry[1], EINA_TRUE);
			elm_entry_scrollable_set(_entry[1], EINA_TRUE);
			elm_entry_entry_set(_entry[1], "");

			evas_object_smart_callback_add(_entry[1], "changed", _entry_changed_cb, layout);
			evas_object_smart_callback_add(_entry[1], "focused", _entry_focused_cb, layout);
			evas_object_smart_callback_add(_entry[1], "unfocused", _entry_unfocused_cb, layout);
			elm_object_signal_callback_add(layout, "elm,eraser,clicked", "elm", _eraser_clicked_cb, _entry[1]);
		}
	}

	return layout;
}

static Evas_Object *_gl_eap_pw_content_get(void *data, Evas_Object *obj, const char *part)
{
	Evas_Object* layout = NULL;
	if (!strncmp(part, "elm.icon", strlen(part))) {
		layout = elm_layout_add(obj);
		if (layout) {
			elm_layout_theme_set(layout, "layout", "editfield", "title");
			evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
			evas_object_size_hint_align_set(layout, EVAS_HINT_FILL, EVAS_HINT_FILL);
			elm_object_part_text_set(layout, "elm.text", _("Password"));

			_entry[2] = elm_entry_add(obj);
			elm_object_part_content_set(layout, "elm.swallow.content", _entry[2]);
			elm_entry_single_line_set(_entry[2], EINA_TRUE);
			elm_entry_scrollable_set(_entry[2], EINA_TRUE);
			elm_entry_entry_set(_entry[2], "");

			evas_object_smart_callback_add(_entry[2], "changed", _entry_changed_cb, layout);
			evas_object_smart_callback_add(_entry[2], "focused", _entry_focused_cb, layout);
			evas_object_smart_callback_add(_entry[2], "unfocused", _entry_unfocused_cb, layout);
			elm_object_signal_callback_add(layout, "elm,eraser,clicked", "elm", _eraser_clicked_cb, _entry[2]);
		}
	}

	return layout;
}


static char *_gl_eap_type_text_get(void *data, Evas_Object *obj, const char *part)
{
	if (!strcmp(part, "elm.text.1")) {
		return strdup(list_eap_type[(int)data].name);
	} else if (!strcmp(part, "elm.text.2")) {
		return strdup("EAP Type");
	}

	return NULL;
}

static char *_gl_eap_subtext_get(void *data, Evas_Object *obj, const char *part)
{
	if (!strcmp(part, "elm.text")) {
		return strdup(list_eap_type[(int)data].name);
	}

	return NULL;
}

static char *_gl_eap_auth_text_get(void *data, Evas_Object *obj, const char *part)
{
	if (!strcmp(part, "elm.text.1")) {
		return strdup(list_eap_auth[(int)data].name);
	} else if (!strcmp(part, "elm.text.2")) {
		return strdup("Authentication Type");
	}

	return NULL;
}

static char *_gl_eap_auth_subtext_get(void *data, Evas_Object *obj, const char *part)
{
	if (!strcmp(part, "elm.text")) {
		return strdup(list_eap_auth[(int)data].name);
	}

	return NULL;
}

static void _gl_exp(void *data, Evas_Object *obj, void *event_info)
{
	Elm_Object_Item *item = (Elm_Object_Item *)event_info;
	Evas_Object *gl = elm_object_item_widget_get(item);
	if (gl == NULL) {
		ERROR_LOG(UG_NAME_NORMAL, "gl is NULL");
		return;
	}

	evas_object_focus_set(gl, EINA_TRUE);

	int i = 0;
	INFO_LOG(UG_NAME_RESP, "depth = %d", expandable_list_index);
	switch (expandable_list_index) {
	case 0:
		while(list_eap_type[i].name != NULL) {
			elm_genlist_item_append(gl, &eap_type_sub_itc, (void*)i, item, list_eap_type[i].flags, _gl_eap_type_sub_sel, (void*)i);
			i++;
		}
		break;
	case 1:
		while(list_eap_auth[i].name != NULL) {
			elm_genlist_item_append(gl, &eap_auth_sub_itc, (void*)i, item, list_eap_auth[i].flags, _gl_eap_auth_sub_sel, (void*)i);
			i++;
		}
		break;
	default:
		break;
	}
}

static void _gl_con(void *data, Evas_Object *obj, void *event_info)
{
	Elm_Object_Item *item = (Elm_Object_Item *)event_info;
	
	elm_genlist_item_subitems_clear(item);
}

static Evas_Object* _create_list(Evas_Object* parent)
{
	assertm_if(NULL == parent, "NULL!!");

	Evas_Object* view_list = elm_genlist_add(parent);
	assertm_if(NULL == view_list, "NULL!!");

	seperator_itc.item_style = "dialogue/seperator";
	seperator_itc.func.text_get = NULL;
	seperator_itc.func.content_get = NULL;
	seperator_itc.func.state_get = NULL;
	seperator_itc.func.del = NULL;
	Elm_Object_Item* sep = elm_genlist_item_append(view_list, &seperator_itc, 
									NULL, NULL, ELM_GENLIST_ITEM_GROUP, NULL, NULL);

	assertm_if(NULL == sep, "NULL!!");
	elm_genlist_item_select_mode_set(sep, ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);

	eap_type_itc.item_style = "dialogue/2text.2/expandable";
	eap_type_itc.func.text_get = _gl_eap_type_text_get;
	eap_type_itc.func.content_get = NULL;
	eap_type_itc.func.state_get = NULL;
	eap_type_itc.func.del = NULL;
	eap_type_item = elm_genlist_item_append(view_list, &eap_type_itc,
									NULL, NULL, ELM_GENLIST_ITEM_TREE, _gl_eap_type_sel, NULL);

	elm_object_item_data_set(eap_type_item, (void*)0);

	eap_type_sub_itc.item_style = "dialogue/1text/expandable2";
	eap_type_sub_itc.func.text_get = _gl_eap_subtext_get;
	eap_type_sub_itc.func.content_get = NULL;
	eap_type_sub_itc.func.state_get = NULL;
	eap_type_sub_itc.func.del = NULL;

	eap_auth_itc.item_style = "dialogue/2text.2/expandable";
	eap_auth_itc.func.text_get = _gl_eap_auth_text_get;
	eap_auth_itc.func.content_get = NULL;
	eap_auth_itc.func.state_get = NULL;
	eap_auth_itc.func.del = NULL;
	eap_auth_item = elm_genlist_item_append(view_list, &eap_auth_itc,
									NULL, NULL, ELM_GENLIST_ITEM_TREE, _gl_eap_auth_sel, NULL);

	eap_auth_sub_itc.item_style = "dialogue/1text/expandable2";
	eap_auth_sub_itc.func.text_get = _gl_eap_auth_subtext_get;
	eap_auth_sub_itc.func.content_get = NULL;
	eap_auth_sub_itc.func.state_get = NULL;
	eap_auth_sub_itc.func.del = NULL;

	eap_id_itc.item_style = "dialogue/1icon";
	eap_id_itc.func.text_get = NULL;
	eap_id_itc.func.content_get = _gl_eap_id_content_get;
	eap_id_itc.func.state_get = NULL;
	eap_id_itc.func.del = NULL;
	eap_id_item = elm_genlist_item_append(view_list, &eap_id_itc, NULL, NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);

	eap_anonyid_itc.item_style = "dialogue/1icon";
	eap_anonyid_itc.func.text_get = NULL;
	eap_anonyid_itc.func.content_get = _gl_eap_anonyid_content_get;
	eap_anonyid_itc.func.state_get = NULL;
	eap_anonyid_itc.func.del = NULL;
	eap_anonyid_item = elm_genlist_item_append(view_list, &eap_anonyid_itc, NULL, NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);

	eap_pw_itc.item_style = "dialogue/1icon";
	eap_pw_itc.func.text_get = NULL;
	eap_pw_itc.func.content_get = _gl_eap_pw_content_get;
	eap_pw_itc.func.state_get = NULL;
	eap_pw_itc.func.del = NULL;
	eap_pw_item = elm_genlist_item_append(view_list, &eap_pw_itc, NULL, NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);

	evas_object_smart_callback_add(view_list, "expanded", _gl_exp, view_list);
	evas_object_smart_callback_add(view_list, "contracted", _gl_con, view_list);

	return view_list;
}

void view_eap_back(void *data,  Evas_Object *obj, void *event_info)
{
	__COMMON_FUNC_ENTER__;

	if(view_eap_end == TRUE) {
		return;
	}
	view_eap_end = TRUE;

	view_eap_destroy(data);

	viewer_manager_set_enabled_list_click(EINA_TRUE);

	__COMMON_FUNC_EXIT__;
}

void view_eap_done(void *data,  Evas_Object *obj, void *event_info)
{
	__COMMON_FUNC_ENTER__;

	if(view_eap_end == TRUE) {
		return;
	}
	view_eap_end = TRUE;

	char* str_id = NULL;
	char* str_anonyid = NULL;
	char* str_pw = NULL;

	net_wifi_connection_info_t conn_info;
	int sel_index = (int)elm_object_item_data_get(eap_type_item);
	switch (sel_index) {
		case EAP_SEC_TYPE_PEAP:
			conn_info.security_info.sec_mode = WLAN_SEC_MODE_IEEE8021X;
			//conn_info.security_info.authentication.eap.eap_type = WLAN_SEC_EAP_TYPE_PEAP;

			connman_request_connection_open_hidden_ap(&conn_info);
			break;
		case EAP_SEC_TYPE_TLS:
			conn_info.security_info.sec_mode = WLAN_SEC_MODE_IEEE8021X;
			//conn_info.security_info.authentication.eap.eap_type = WLAN_SEC_EAP_TYPE_TLS;

			str_id = elm_entry_markup_to_utf8(elm_entry_entry_get(_entry[0]));
			str_anonyid = elm_entry_markup_to_utf8(elm_entry_entry_get(_entry[1]));
			str_pw = elm_entry_markup_to_utf8(elm_entry_entry_get(_entry[2]));

			connman_request_connection_open_hidden_ap(&conn_info);
			break;
		case EAP_SEC_TYPE_TTLS:
			conn_info.security_info.sec_mode = WLAN_SEC_MODE_IEEE8021X;
			//conn_info.security_info.authentication.eap.eap_type = WLAN_SEC_EAP_TYPE_TTLS;

			str_id = elm_entry_markup_to_utf8(elm_entry_entry_get(_entry[0]));
			str_anonyid = elm_entry_markup_to_utf8(elm_entry_entry_get(_entry[1]));
			str_pw = elm_entry_markup_to_utf8(elm_entry_entry_get(_entry[2]));

			connman_request_connection_open_hidden_ap(&conn_info);
			break;
		case EAP_SEC_TYPE_SIM:
			conn_info.security_info.sec_mode = WLAN_SEC_MODE_IEEE8021X;
			//conn_info.security_info.authentication.eap.eap_type = WLAN_SEC_EAP_TYPE_SIM;

			connman_request_connection_open_hidden_ap(&conn_info);
			break;
		case EAP_SEC_TYPE_AKA:
			conn_info.security_info.sec_mode = WLAN_SEC_MODE_IEEE8021X;
			//conn_info.security_info.authentication.eap.eap_type = WLAN_SEC_EAP_TYPE_AKA;

			connman_request_connection_open_hidden_ap(&conn_info);
			break;

		default:
			break;
	}

	view_eap_destroy(data);

	elm_naviframe_item_pop(viewer_manager_get_naviframe());

	__COMMON_FUNC_EXIT__;
}

void view_eap(wifi_device_info_t *device_info)
{
	__COMMON_FUNC_ENTER__;

	if (device_info == NULL) {
		ERROR_LOG(UG_NAME_NORMAL, "Failed : device_info is NULL");
		return;
	}

	Evas_Object* navi_frame = viewer_manager_get_naviframe();
	if (navi_frame == NULL) {
		ERROR_LOG(UG_NAME_NORMAL, "Failed : get naviframe");
		return;
	}

	view_eap_end = FALSE;

	view_eap_data * eap_data = (view_eap_data *) malloc(sizeof(view_eap_data));
	memset(eap_data, 0, sizeof(view_eap_data));
	eap_data->profile_name = strdup(device_info->profile_name);
	eap_data->security_mode = device_info->security_mode;

	Evas_Object *layout =_create_layout(navi_frame);
	if (layout == NULL)
		return;

	Evas_Object* conform = elm_conformant_add(navi_frame);
	elm_object_style_set(conform, "internal_layout");
	elm_object_part_content_set(layout, "elm.swallow.content", conform);
	evas_object_show(conform);

	Evas_Object* list = _create_list(navi_frame);
	assertm_if(NULL == list, "NULL!!");

	elm_object_content_set(conform, list);

	Elm_Object_Item *navi_it = elm_naviframe_item_push(navi_frame, device_info->ssid, NULL, NULL, layout, NULL);

	Evas_Object *toolbar = elm_toolbar_add(navi_frame);
	elm_toolbar_shrink_mode_set(toolbar, ELM_TOOLBAR_SHRINK_EXPAND);

	_button_done_item = elm_toolbar_item_append(toolbar,
													NULL,
													sc(PACKAGE, I18N_TYPE_Done),
													(Evas_Smart_Cb) view_eap_done,
													eap_data);
	elm_object_item_disabled_set(elm_toolbar_item_append(toolbar, NULL, NULL, NULL, NULL), EINA_TRUE);
	elm_object_item_part_content_set(navi_it, "controlbar", toolbar);

	Evas_Object* button_back = elm_object_item_part_content_get(navi_it, "prev_btn");
	elm_object_focus_allow_set(button_back, EINA_TRUE);
	evas_object_smart_callback_add(button_back, "clicked", (Evas_Smart_Cb)view_eap_back, eap_data);

	evas_object_show(layout);
	__COMMON_FUNC_EXIT__;
}

static void view_eap_destroy(void *data)
{
	if (_entry[0]) {
		evas_object_smart_callback_del(_entry[0], "changed", _entry_changed_cb);
		evas_object_smart_callback_del(_entry[0], "focused", _entry_focused_cb);
		evas_object_smart_callback_del(_entry[0], "unfocused", _entry_unfocused_cb);
		_entry[0] = NULL;
	}

	if (_entry[1]) {
		evas_object_smart_callback_del(_entry[1], "changed", _entry_changed_cb);
		evas_object_smart_callback_del(_entry[1], "focused", _entry_focused_cb);
		evas_object_smart_callback_del(_entry[1], "unfocused", _entry_unfocused_cb);
		_entry[1] = NULL;
	}

	if (_entry[2]) {
		evas_object_smart_callback_del(_entry[2], "changed", _entry_changed_cb);
		evas_object_smart_callback_del(_entry[2], "focused", _entry_focused_cb);
		evas_object_smart_callback_del(_entry[2], "unfocused", _entry_unfocused_cb);
		_entry[2] = NULL;
	}

	if (data  != NULL) {
		g_free(data);
	}
}
