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
#include "view_ime_hidden.h"
#include "i18nmanager.h"
#include "wlan_manager.h"
#include "viewer_manager.h"
#include "popup.h"


static Elm_Genlist_Item_Class seperator_itc;
static Elm_Genlist_Item_Class itc;
static Elm_Genlist_Item_Class type_sel_itc;
static Elm_Genlist_Item_Class type_itc;
static Elm_Genlist_Item_Class password_itc;
static Elm_Genlist_Item_Class eap_itc, eap_auth_itc, eap_id_itc, eap_anonyid_itc, eap_pw_itc;
static Elm_Genlist_Item_Class eap_sub_itc, eap_auth_sub_itc;

static int view_ime_hidden_end = TRUE;
static Evas_Object *_button_save = NULL;
static Evas_Object *_entry[4];
struct _Expand_List {
	char depth;
	char *name;
	Elm_Genlist_Item_Type flags;
};

static struct _Expand_List list_sec_type[] = {
	{1, "Public", ELM_GENLIST_ITEM_NONE},
	{1, "WEP", ELM_GENLIST_ITEM_NONE},
	{1, "WPA/WPA2 PSK", ELM_GENLIST_ITEM_NONE},
	{1, "802.1x Enterprise", ELM_GENLIST_ITEM_NONE},
	{1, "802.1x Dynamic WEP", ELM_GENLIST_ITEM_NONE},
	{1, NULL, ELM_GENLIST_ITEM_NONE}
};

static struct _Expand_List list_eap_type[] = {
	{1, "PEAP", ELM_GENLIST_ITEM_NONE},
	{1, "TLS", ELM_GENLIST_ITEM_NONE},
	{1, "TTLS", ELM_GENLIST_ITEM_NONE},
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

static int expandable_list_index = 0;
static Elm_Object_Item* security_type_item = NULL;
static Elm_Object_Item* eap_items[5+1] = {NULL, };

/* static */
static void _gl_sel(void *data, Evas_Object *obj, void *event_info);
static void view_ime_hidden_destroy();



/* implementation */
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

static void _network_name_entry_changed_cb(void* data, Evas_Object* obj, void* event_info)
{
	__COMMON_FUNC_ENTER__;

	if (elm_object_focus_get(data)) {
		if (elm_entry_is_empty(obj))
			elm_object_signal_emit(data, "elm,state,eraser,hide", "elm");
		else
			elm_object_signal_emit(data, "elm,state,eraser,show", "elm");
	}

	const char* txt = elm_entry_markup_to_utf8(elm_entry_entry_get(_entry[0]));
	int len = 0;

	if(txt) {
		len = strlen(txt);
		if(len > 0) {
			elm_object_disabled_set(_button_save, EINA_FALSE);
		} else {
			elm_object_disabled_set(_button_save, EINA_TRUE);
		}
	} else {
		ERROR_LOG(UG_NAME_NORMAL, "Err!");
	}

	__COMMON_FUNC_EXIT__;
}

static void _entry_changed_cb(void* data, Evas_Object* obj, void* event_info)
{
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

static void _gl_eap_sel(void *data, Evas_Object *obj, void *event_info)
{
	Elm_Object_Item *item = (Elm_Object_Item *)event_info;
	Eina_Bool expanded = EINA_FALSE;
	if (item)
		elm_genlist_item_selected_set(item, EINA_FALSE);

	expandable_list_index = 1;
	
	expanded = elm_genlist_item_expanded_get(item);
	elm_genlist_item_expanded_set(item, !expanded);
}

static void _gl_eap_type_sel(void *data, Evas_Object *obj, void *event_info)
{
	Elm_Object_Item *item = (Elm_Object_Item *)event_info;
	Elm_Object_Item *parent_item = elm_genlist_item_parent_get(item);

	elm_object_item_data_set(parent_item, data);
	
	Eina_Bool expanded = EINA_FALSE;
	expanded = elm_genlist_item_expanded_get(parent_item);
	elm_genlist_item_expanded_set(parent_item, !expanded);
}

static void _gl_eap_auth_sel(void *data, Evas_Object *obj, void *event_info)
{
	Elm_Object_Item *item = (Elm_Object_Item *)event_info;
	Eina_Bool expanded = EINA_FALSE;
	if (item)
		elm_genlist_item_selected_set(item, EINA_FALSE);

	expandable_list_index = 2;
	
	expanded = elm_genlist_item_expanded_get(item);
	elm_genlist_item_expanded_set(item, !expanded);
}

static void _gl_eap_auth_type_sel(void *data, Evas_Object *obj, void *event_info)
{
	Elm_Object_Item *item = (Elm_Object_Item *)event_info;
	Elm_Object_Item *parent_item = elm_genlist_item_parent_get(item);

	elm_object_item_data_set(parent_item, data);
	
	Eina_Bool expanded = EINA_FALSE;
	expanded = elm_genlist_item_expanded_get(parent_item);
	elm_genlist_item_expanded_set(parent_item, !expanded);
}

static void _set_list(Evas_Object *obj, hidden_ap_security_type_t prev_type, hidden_ap_security_type_t new_type)
{
	switch (new_type) {
		case HIDDEN_AP_SECURITY_PUBLIC:
			if (prev_type < HIDDEN_AP_SECURITY_ENTERPRISE) {
				elm_object_item_del(elm_genlist_last_item_get(obj));
			} else {
				int i = 0;
				while (eap_items[i]) {
					elm_object_item_del(eap_items[i]);
					i++;
				}
			}
			break;
		case HIDDEN_AP_SECURITY_WEP:
			if (prev_type == HIDDEN_AP_SECURITY_PUBLIC) {
				elm_genlist_item_append(obj, &password_itc,
						NULL, NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);
			} else if (prev_type == HIDDEN_AP_SECURITY_WPAPSK) {
			} else {
				int i = 0;
				while (eap_items[i]) {
					elm_object_item_del(eap_items[i]);
					i++;
				}

				elm_genlist_item_append(obj, &password_itc,
						NULL, NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);
			}
			break;
		case HIDDEN_AP_SECURITY_WPAPSK:
			if (prev_type == HIDDEN_AP_SECURITY_PUBLIC) {
				elm_genlist_item_append(obj, &password_itc,
						NULL, NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);
			} else if (prev_type == HIDDEN_AP_SECURITY_WEP) {
			} else {
				int i = 0;
				while (eap_items[i]) {
					elm_object_item_del(eap_items[i]);
					i++;
				}

				elm_genlist_item_append(obj, &password_itc,
						NULL, NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);
			}
			break;
		case HIDDEN_AP_SECURITY_ENTERPRISE:
			if (prev_type == HIDDEN_AP_SECURITY_PUBLIC) {	
				eap_items[0] = elm_genlist_item_append(obj, &eap_itc,
									NULL, NULL, ELM_GENLIST_ITEM_TREE, _gl_eap_sel, NULL);
				eap_items[1] = elm_genlist_item_append(obj, &eap_auth_itc,
									NULL, NULL, ELM_GENLIST_ITEM_TREE, _gl_eap_auth_sel, NULL);
				eap_items[2] = elm_genlist_item_append(obj, &eap_id_itc,
									NULL, NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);
				eap_items[3] = elm_genlist_item_append(obj, &eap_anonyid_itc,
									NULL, NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);
				eap_items[4] = elm_genlist_item_append(obj, &eap_pw_itc,
									NULL, NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);
			} else if (prev_type == HIDDEN_AP_SECURITY_DYNAMICWEP) {
			} else {
				elm_object_item_del(elm_genlist_last_item_get(obj));

				eap_items[0] = elm_genlist_item_append(obj, &eap_itc,
									NULL, NULL, ELM_GENLIST_ITEM_TREE, _gl_eap_sel, NULL);
				eap_items[1] = elm_genlist_item_append(obj, &eap_auth_itc,
									NULL, NULL, ELM_GENLIST_ITEM_TREE, _gl_eap_auth_sel, NULL);
				eap_items[2] = elm_genlist_item_append(obj, &eap_id_itc,
									NULL, NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);
				eap_items[3] = elm_genlist_item_append(obj, &eap_anonyid_itc,
									NULL, NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);
				eap_items[4] = elm_genlist_item_append(obj, &eap_pw_itc,
									NULL, NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);
			}
			break;
		case HIDDEN_AP_SECURITY_DYNAMICWEP:
			if (prev_type == HIDDEN_AP_SECURITY_PUBLIC) {	
				eap_items[0] = elm_genlist_item_append(obj, &eap_itc,
									NULL, NULL, ELM_GENLIST_ITEM_TREE, _gl_eap_sel, NULL);
				eap_items[1] = elm_genlist_item_append(obj, &eap_auth_itc,
									NULL, NULL, ELM_GENLIST_ITEM_TREE, _gl_eap_auth_sel, NULL);
				eap_items[2] = elm_genlist_item_append(obj, &eap_id_itc,
									NULL, NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);
				eap_items[3] = elm_genlist_item_append(obj, &eap_anonyid_itc,
									NULL, NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);
				eap_items[4] = elm_genlist_item_append(obj, &eap_pw_itc,
									NULL, NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);
			} else if (prev_type == HIDDEN_AP_SECURITY_ENTERPRISE) {
			} else {
				elm_object_item_del(elm_genlist_last_item_get(obj));

				eap_items[0] = elm_genlist_item_append(obj, &eap_itc,
									NULL, NULL, ELM_GENLIST_ITEM_TREE, _gl_eap_sel, NULL);
				eap_items[1] = elm_genlist_item_append(obj, &eap_auth_itc,
									NULL, NULL, ELM_GENLIST_ITEM_TREE, _gl_eap_auth_sel, NULL);
				eap_items[2] = elm_genlist_item_append(obj, &eap_id_itc,
									NULL, NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);
				eap_items[3] = elm_genlist_item_append(obj, &eap_anonyid_itc,
									NULL, NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);
				eap_items[4] = elm_genlist_item_append(obj, &eap_pw_itc,
									NULL, NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);
			}
			break;
		default:
			break;
	}
}

static Evas_Object *_gl_content_get(void *data, Evas_Object *obj, const char *part)
{
	assertm_if(NULL == data, "NULL!!");
	assertm_if(NULL == obj, "NULL!!");
	assertm_if(NULL == part, "NULL!!");

	Evas_Object* layout = NULL;
	
	DEBUG_LOG(UG_NAME_NORMAL, "part [%s]", part);

	if (!strncmp(part, "elm.icon", strlen(part))) {
		layout = elm_layout_add(obj);
		if (layout) {
			elm_layout_theme_set(layout, "layout", "editfield", "title");
			evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
			evas_object_size_hint_align_set(layout, EVAS_HINT_FILL, EVAS_HINT_FILL);
			elm_object_part_text_set(layout, "elm.text", _("Network name"));

			_entry[0] = elm_entry_add(obj);
			elm_object_part_content_set(layout, "elm.swallow.content", _entry[0]);
			elm_entry_single_line_set(_entry[0], EINA_TRUE);
			elm_entry_scrollable_set(_entry[0], EINA_TRUE);

			evas_object_smart_callback_add(_entry[0], "changed", _network_name_entry_changed_cb, layout);
			evas_object_smart_callback_add(_entry[0], "focused", _entry_focused_cb, layout);
			evas_object_smart_callback_add(_entry[0], "unfocused", _entry_unfocused_cb, layout);
			elm_object_signal_callback_add(layout, "elm,eraser,clicked", "elm", _eraser_clicked_cb, _entry[0]);
		}
	}

	return layout;
}

static Evas_Object *_gl_pw_content_get(void *data, Evas_Object *obj, const char *part)
{
	Evas_Object* layout = NULL;
	if (!strncmp(part, "elm.icon", strlen(part))) {
		layout = elm_layout_add(obj);
		if (layout) {
			elm_layout_theme_set(layout, "layout", "editfield", "title");
			evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
			evas_object_size_hint_align_set(layout, EVAS_HINT_FILL, EVAS_HINT_FILL);
			elm_object_part_text_set(layout, "elm.text", _("Password"));

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

			_entry[3] = elm_entry_add(obj);
			elm_object_part_content_set(layout, "elm.swallow.content", _entry[3]);
			elm_entry_single_line_set(_entry[3], EINA_TRUE);
			elm_entry_scrollable_set(_entry[3], EINA_TRUE);
			elm_entry_entry_set(_entry[3], "");

			evas_object_smart_callback_add(_entry[3], "changed", _entry_changed_cb, layout);
			evas_object_smart_callback_add(_entry[3], "focused", _entry_focused_cb, layout);
			evas_object_smart_callback_add(_entry[3], "unfocused", _entry_unfocused_cb, layout);
			elm_object_signal_callback_add(layout, "elm,eraser,clicked", "elm", _eraser_clicked_cb, _entry[3]);
		}
	}

	return layout;
}

static char *_gl_text_get(void *data, Evas_Object *obj, const char *part)
{
	if (!strcmp(part, "elm.text.1")) {
		return strdup(list_sec_type[(int)data].name);
	} else if (!strcmp(part, "elm.text.2")) {
		return strdup("Security");
	}

	return NULL;
}

static char *_gl_subtext_get(void *data, Evas_Object *obj, const char *part)
{
	if (!strcmp(part, "elm.text")) {
		return strdup(list_sec_type[(int)data].name);
	}

	return NULL;
}

static char *_gl_eap_text_get(void *data, Evas_Object *obj, const char *part)
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

static void _gl_type_sel(void *data, Evas_Object *obj, void *event_info)
{
	Elm_Object_Item *item = (Elm_Object_Item *)event_info;
	Elm_Object_Item *parent_item = elm_genlist_item_parent_get(item);

	int prev_index = (int)elm_object_item_data_get(parent_item);
	int new_index = (int)data;

	INFO_LOG(UG_NAME_RESP, "prev index[%d],  new index[%d]", prev_index, new_index);

	elm_object_item_data_set(parent_item, data);
	
	Eina_Bool expanded = EINA_FALSE;
	expanded = elm_genlist_item_expanded_get(parent_item);
	elm_genlist_item_expanded_set(parent_item, !expanded);

	if (prev_index != new_index) {
		_set_list(obj, prev_index, new_index);
		elm_genlist_item_update(parent_item);
	}
}

static void _gl_sel(void *data, Evas_Object *obj, void *event_info)
{
	Elm_Object_Item *item = (Elm_Object_Item *)event_info;
	Eina_Bool expanded = EINA_FALSE;
	if (item)
		elm_genlist_item_selected_set(item, EINA_FALSE);

	expandable_list_index = 0;
	
	expanded = elm_genlist_item_expanded_get(item);
	elm_genlist_item_expanded_set(item, !expanded);
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
		while(list_sec_type[i].name != NULL) {
			elm_genlist_item_append(gl, &type_itc, (void*)i, item, list_sec_type[i].flags, _gl_type_sel, (void*)i);
			i++;
		}
		break;
	case 1:
		while(list_eap_type[i].name != NULL) {
			elm_genlist_item_append(gl, &eap_sub_itc, (void*)i, item, list_eap_type[i].flags, _gl_eap_type_sel, (void*)i);
			i++;
		}
		break;
	case 2:
		while(list_eap_auth[i].name != NULL) {
			elm_genlist_item_append(gl, &eap_auth_sub_itc, (void*)i, item, list_eap_auth[i].flags, _gl_eap_auth_type_sel, (void*)i);
			i++;
		}
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

	//evas_object_size_hint_weight_set(view_list, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	//evas_object_size_hint_align_set(view_list, EVAS_HINT_FILL, EVAS_HINT_FILL);

	seperator_itc.item_style = "dialogue/seperator";
	seperator_itc.func.text_get = NULL;
	seperator_itc.func.content_get = NULL;
	seperator_itc.func.state_get = NULL;
	seperator_itc.func.del = NULL;
	Elm_Object_Item* sep = elm_genlist_item_append(view_list, &seperator_itc, 
									NULL, NULL, ELM_GENLIST_ITEM_GROUP, NULL, NULL);

	assertm_if(NULL == sep, "NULL!!");
	elm_genlist_item_select_mode_set(sep, ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);

	itc.item_style = "1icon";
	itc.func.text_get = NULL;
	itc.func.content_get = _gl_content_get;
	itc.func.state_get = NULL;
	itc.func.del = NULL;
	elm_genlist_item_append(view_list, &itc, NULL, NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);
	
	type_sel_itc.item_style = "dialogue/2text.2/expandable";
	type_sel_itc.func.text_get = _gl_text_get;
	type_sel_itc.func.content_get = NULL;
	type_sel_itc.func.state_get = NULL;
	type_sel_itc.func.del = NULL;
	security_type_item = elm_genlist_item_append(view_list, &type_sel_itc,
									NULL, NULL, ELM_GENLIST_ITEM_TREE, _gl_sel, NULL);

	elm_object_item_data_set(security_type_item, (void*)0);

	type_itc.item_style = "dialogue/1text/expandable2";
	type_itc.func.text_get = _gl_subtext_get;
	type_itc.func.content_get = NULL;
	type_itc.func.state_get = NULL;
	type_itc.func.del = NULL;

	/* Set Password Item Class for WEP, WPA/PSK */
	password_itc.item_style = "dialogue/1icon";
	password_itc.func.text_get = NULL;
	password_itc.func.content_get = _gl_pw_content_get;
	password_itc.func.state_get = NULL;
	password_itc.func.del = NULL;

	/* Set EAP Item Class for 802.1x Enterprise, Dynamic WEP */
	eap_itc.item_style = "dialogue/2text.2/expandable";
	eap_itc.func.text_get = _gl_eap_text_get;
	eap_itc.func.content_get = NULL;
	eap_itc.func.state_get = NULL;
	eap_itc.func.del = NULL;
	eap_sub_itc.item_style = "dialogue/1text/expandable2";
	eap_sub_itc.func.text_get = _gl_eap_subtext_get;
	eap_sub_itc.func.content_get = NULL;
	eap_sub_itc.func.state_get = NULL;
	eap_sub_itc.func.del = NULL;
	
	eap_auth_itc.item_style = "dialogue/2text.2/expandable";
	eap_auth_itc.func.text_get = _gl_eap_auth_text_get;
	eap_auth_itc.func.content_get = NULL;
	eap_auth_itc.func.state_get = NULL;
	eap_auth_itc.func.del = NULL;
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
	
	eap_anonyid_itc.item_style = "dialogue/1icon";
	eap_anonyid_itc.func.text_get = NULL;
	eap_anonyid_itc.func.content_get = _gl_eap_anonyid_content_get;
	eap_anonyid_itc.func.state_get = NULL;
	eap_anonyid_itc.func.del = NULL;
	
	eap_pw_itc.item_style = "dialogue/1icon";
	eap_pw_itc.func.text_get = NULL;
	eap_pw_itc.func.content_get = _gl_eap_pw_content_get;
	eap_pw_itc.func.state_get = NULL;
	eap_pw_itc.func.del = NULL;

	evas_object_smart_callback_add(view_list, "expanded", _gl_exp, view_list);
	evas_object_smart_callback_add(view_list, "contracted", _gl_con, view_list);
	
	return view_list;
}

void view_ime_back(void *data,  Evas_Object *obj, void *event_info)
{
	__COMMON_FUNC_ENTER__;

	if(view_ime_hidden_end == TRUE) {
		return;
	}
	view_ime_hidden_end = TRUE;


	view_ime_hidden_destroy();

	viewer_manager_set_enabled_list_click(EINA_TRUE);
	viewer_manager_set_enabled_list_update(EINA_TRUE);

	elm_naviframe_item_pop(viewer_manager_get_naviframe());

	__COMMON_FUNC_EXIT__;
}

void view_ime_done(void *data,  Evas_Object *obj, void *event_info)
{
	__COMMON_FUNC_ENTER__;

	if(view_ime_hidden_end == TRUE) {
		return;
	}
	view_ime_hidden_end = TRUE;

	if(NULL == elm_entry_entry_get(_entry[0])) {
		view_ime_hidden_end = FALSE;
		return;
	}

	char* str_ssid = NULL;
	char* str_id = NULL;
	char* str_anonyid = NULL;
	char* str_pw = NULL;

	str_ssid = elm_entry_markup_to_utf8(elm_entry_entry_get(_entry[0]));
	int ssid_len = strlen(str_ssid);
	if (ssid_len == 0 || ssid_len > 32) {
		winset_popup_simple_set("SSID can be up to 32 letters.<br>Check your input.");
		view_ime_hidden_end = FALSE;
		return;
	}

	net_wifi_connection_info_t conn_info;
	strncpy(conn_info.essid, str_ssid, NET_WLAN_ESSID_LEN);
	INFO_LOG(UG_NAME_REQ, "ssid = %s", conn_info.essid);

	int sel_index = (int)elm_object_item_data_get(security_type_item);
	switch (sel_index) {
		case HIDDEN_AP_SECURITY_PUBLIC:
			conn_info.security_info.sec_mode = WLAN_SEC_MODE_NONE;

			connman_request_connection_open_hidden_ap(&conn_info);
			break;
		case HIDDEN_AP_SECURITY_WEP:
			conn_info.security_info.sec_mode = WLAN_SEC_MODE_WEP;
			str_pw = elm_entry_markup_to_utf8(elm_entry_entry_get(_entry[1]));
			strncpy(conn_info.security_info.authentication.wep.wepKey, str_pw, NETPM_WLAN_MAX_WEP_KEY_LEN);
			conn_info.security_info.authentication.wep.wepKey[NETPM_WLAN_MAX_WEP_KEY_LEN] = '\0';
			INFO_LOG(UG_NAME_REQ, "pw = %s", conn_info.security_info.authentication.wep.wepKey);

			connman_request_connection_open_hidden_ap(&conn_info);
			break;
		case HIDDEN_AP_SECURITY_WPAPSK:
			conn_info.security_info.sec_mode = WLAN_SEC_MODE_WPA_PSK; //WLAN_SEC_MODE_WPA2_PSK
			str_pw = elm_entry_markup_to_utf8(elm_entry_entry_get(_entry[1]));
			strncpy(conn_info.security_info.authentication.psk.pskKey, str_pw, NETPM_WLAN_MAX_PSK_PASSPHRASE_LEN);
			conn_info.security_info.authentication.psk.pskKey[NETPM_WLAN_MAX_PSK_PASSPHRASE_LEN] = '\0';
			INFO_LOG(UG_NAME_REQ, "pw = %s", conn_info.security_info.authentication.psk.pskKey);
			
			connman_request_connection_open_hidden_ap(&conn_info);
			break;

		/* TODO : EAP authentication. */
		case HIDDEN_AP_SECURITY_ENTERPRISE:
		case HIDDEN_AP_SECURITY_DYNAMICWEP:
			conn_info.security_info.sec_mode = WLAN_SEC_MODE_IEEE8021X;
			str_id = elm_entry_markup_to_utf8(elm_entry_entry_get(_entry[1]));
			str_anonyid = elm_entry_markup_to_utf8(elm_entry_entry_get(_entry[2]));
			str_pw = elm_entry_markup_to_utf8(elm_entry_entry_get(_entry[3]));
			INFO_LOG(UG_NAME_REQ, "id = %s", str_id);
			INFO_LOG(UG_NAME_REQ, "anonyid = %s", str_anonyid);
			INFO_LOG(UG_NAME_REQ, "pw = %s", str_pw);
			break;

		default:
			break;
	}

	view_ime_hidden_destroy();

	viewer_manager_set_enabled_list_click(EINA_TRUE);
	viewer_manager_set_enabled_list_update(EINA_TRUE);

	elm_naviframe_item_pop(viewer_manager_get_naviframe());

	__COMMON_FUNC_EXIT__;
}

void view_ime_hidden()
{
	__COMMON_FUNC_ENTER__;

	view_ime_hidden_end = FALSE;

	Evas_Object* navi_frame = viewer_manager_get_naviframe();
	assertm_if(NULL == navi_frame, "NULL!!");

	Evas_Object *layout =_create_layout(navi_frame);
	if (layout == NULL)
		return;

	/* Add Conformant */
	Evas_Object* conform = elm_conformant_add(navi_frame);
	elm_object_style_set(conform, "internal_layout");
	elm_object_part_content_set(layout, "elm.swallow.content", conform);
	evas_object_show(conform);
	
	Evas_Object* list = _create_list(navi_frame);
	assertm_if(NULL == list, "NULL!!");

	elm_object_content_set(conform, list);

	Elm_Object_Item *navi_it = elm_naviframe_item_push(navi_frame, sc(PACKAGE, I18N_TYPE_Hidden_AP), NULL, NULL, layout, NULL);

	_button_save = elm_button_add(navi_frame);
	elm_object_style_set(_button_save, "naviframe/title/default");
	elm_object_text_set(_button_save, sc(PACKAGE, I18N_TYPE_Save));
	evas_object_smart_callback_add(_button_save, "clicked", (Evas_Smart_Cb) view_ime_done, NULL);
	elm_object_item_part_content_set(navi_it, "title_left_btn", _button_save);
	elm_object_disabled_set(_button_save, EINA_TRUE);

	Evas_Object *button_cancel = elm_button_add(navi_frame);
	elm_object_style_set(button_cancel, "naviframe/title/default");
	elm_object_text_set(button_cancel, sc(PACKAGE, I18N_TYPE_Cancel));
	evas_object_smart_callback_add(button_cancel, "clicked", (Evas_Smart_Cb) view_ime_back, NULL);
	elm_object_item_part_content_set(navi_it, "title_right_btn", button_cancel);

	evas_object_show(layout);
	__COMMON_FUNC_EXIT__;
}

static void view_ime_hidden_destroy()
{
	if (_entry[0]) {
		evas_object_smart_callback_del(_entry[0], "changed", _network_name_entry_changed_cb);
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

	if (_entry[3]) {
		evas_object_smart_callback_del(_entry[3], "changed", _entry_changed_cb);
		evas_object_smart_callback_del(_entry[3], "focused", _entry_focused_cb);
		evas_object_smart_callback_del(_entry[3], "unfocused", _entry_unfocused_cb);
		_entry[3] = NULL;
	}
}
