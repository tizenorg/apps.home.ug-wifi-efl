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
#include "view_staticip.h"
#include "view_detail.h"
#include "i18nmanager.h"
#include "view_detail_datamodel.h"
#include "popup.h"


static char* guide_text_ip = NULL;
static char* guide_text_subnet = NULL;
static char* guide_text_gateway = NULL;
static char* guide_text_dns1 = NULL;
static char* guide_text_dns2 = NULL;

static int view_staticip_end = TRUE;

static Elm_Genlist_Item_Class ip_itc;
static Elm_Genlist_Item_Class sep_itc;

typedef enum VIEW_STATICIP_TYPES
{
	VIEW_STATICIP_TYPE_IP,
	VIEW_STATICIP_TYPE_SUBNET,
	VIEW_STATICIP_TYPE_GATEWAY,
	VIEW_STATICIP_TYPE_DNS1,
	VIEW_STATICIP_TYPE_DNS2,

} VIEW_STATICIP_TYPES;

typedef struct _view_staticip_description_data
{
	char* title;
	int tag;
} _view_staticip_description_data;
/////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////
// implementations 
/////////////////////////////////////////////////////////////////
static Elm_Object_Item* _add_list_item(Evas_Object* list,
				VIEW_STATICIP_TYPES tag,
				char* title)
{
	_view_staticip_description_data* data = NULL;

	data = (_view_staticip_description_data*) malloc (
			sizeof( _view_staticip_description_data));
	assertm_if(NULL == data, "NULL!!");
	memset(data, 0, sizeof(_view_staticip_description_data));

	data->title = strdup(title);
	data->tag = tag;

	Elm_Object_Item* det = NULL;
	det = elm_genlist_item_append(
			list,
			&ip_itc,
			data,
			NULL,
			ELM_GENLIST_ITEM_NONE,
			NULL,
			NULL);
	assertm_if(NULL == det, "NULL!!");
	if (det == NULL) {
		g_free(data->title);
		g_free(data);
	}

	return det;
}

static void _entry_changed_cb(void* data, Evas_Object* obj, void* event_info)
{
	if (obj == NULL || view_staticip_end == TRUE)
		return;

	if (elm_object_focus_get(data)) {
		if (elm_entry_is_empty(obj))
			elm_object_signal_emit(data, "elm,state,eraser,hide", "elm");
		else
			elm_object_signal_emit(data, "elm,state,eraser,show", "elm");
	}

	const char* txt = elm_entry_markup_to_utf8(elm_entry_entry_get(obj));
	assertm_if(NULL == txt, "NULL!!");
	DEBUG_LOG( UG_NAME_NORMAL, "text [%s]", txt);

	_view_staticip_description_data* det = 
		(_view_staticip_description_data*) data;

	switch(det->tag) {
		case VIEW_STATICIP_TYPE_IP:
			view_detail_datamodel_static_ip_address_set(txt);
			break;
		case VIEW_STATICIP_TYPE_SUBNET:
			view_detail_datamodel_static_subnet_mask_set(txt);
			break;
		case VIEW_STATICIP_TYPE_GATEWAY:
			view_detail_datamodel_static_gateway_address_set(txt);
			break;
		case VIEW_STATICIP_TYPE_DNS1:
			view_detail_datamodel_static_dns1_address_set(txt);
			break;
		case VIEW_STATICIP_TYPE_DNS2:
			view_detail_datamodel_static_dns2_address_set(txt);
			break;
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

static Evas_Object *_view_staticip_content_get(void *data, Evas_Object *obj, const char *part)
{
	__COMMON_FUNC_ENTER__;

	Evas_Object* layout = NULL;

	assertm_if(NULL == part, "NULL!!");
	DEBUG_LOG(UG_NAME_NORMAL, "part [%s]", part);

	if(!strncmp(part, "elm.icon", strlen(part))) {
		_view_staticip_description_data* det = 
			(_view_staticip_description_data*) data;

		layout = elm_layout_add(obj);
		if (layout) {
			elm_layout_theme_set(layout, "layout", "editfield", "title");
			evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
			evas_object_size_hint_align_set(layout, EVAS_HINT_FILL, EVAS_HINT_FILL);
			Evas_Object *entry = elm_entry_add(obj);
			elm_object_part_content_set(layout, "elm.swallow.content", entry);
			elm_object_part_text_set(layout, "elm.text", det->title);
			elm_entry_single_line_set(entry, EINA_TRUE);
			elm_entry_scrollable_set(entry, EINA_TRUE);

			const char* txt = NULL;
			switch(det->tag) {
			case VIEW_STATICIP_TYPE_IP:
				txt = view_detail_datamodel_static_ip_address_get();
				DEBUG_LOG(UG_NAME_NORMAL, "* IP [%s]", txt);
				g_free(guide_text_ip);
				guide_text_ip = g_strdup(txt);
				break;
			case VIEW_STATICIP_TYPE_SUBNET:
				txt = view_detail_datamodel_static_subnet_mask_get();
				DEBUG_LOG(UG_NAME_NORMAL, "* SUBNET [%s]", txt);
				g_free(guide_text_subnet);
				guide_text_subnet = g_strdup(txt);
				break;
			case VIEW_STATICIP_TYPE_GATEWAY:
				txt = view_detail_datamodel_static_gateway_address_get();
				DEBUG_LOG(UG_NAME_NORMAL, "* GATEWAY [%s]", txt);
				g_free(guide_text_gateway);
				guide_text_gateway = g_strdup(txt);
				break;
			case VIEW_STATICIP_TYPE_DNS1:
				txt = view_detail_datamodel_static_dns1_address_get();
				DEBUG_LOG(UG_NAME_NORMAL, "* DNS1 [%s]", txt);
				g_free(guide_text_dns1);
				guide_text_dns1 = g_strdup(txt);
				break;
			case VIEW_STATICIP_TYPE_DNS2:
				txt = view_detail_datamodel_static_dns2_address_get();
				DEBUG_LOG(UG_NAME_NORMAL, "* DNS2 [%s]", txt);
				g_free(guide_text_dns2);
				guide_text_dns2 = g_strdup(txt);
				break;
			}

			elm_object_part_text_set(layout, "elm.guidetext", txt);
			elm_entry_input_panel_layout_set(entry, ELM_INPUT_PANEL_LAYOUT_IP);
			elm_entry_cnp_mode_set(entry, ELM_CNP_MODE_NO_IMAGE);

			evas_object_smart_callback_add(entry, "changed", _entry_changed_cb, layout);
			evas_object_smart_callback_add(entry, "focused", _entry_focused_cb, layout);
			evas_object_smart_callback_add(entry, "unfocused", _entry_unfocused_cb, layout);
			elm_object_signal_callback_add(layout, "elm,eraser,clicked", "elm", _eraser_clicked_cb, entry);

			evas_object_show(entry);
		}

	}

	__COMMON_FUNC_EXIT__;
	return layout;
}

static void _view_staticip_list_del(void *data, Evas_Object *obj)
{
	__COMMON_FUNC_ENTER__;

	if (data == NULL)
		return;

	_view_staticip_description_data* det = (_view_staticip_description_data *) data;

	if (det->title)
		g_free(det->title);

	g_free(data);

	__COMMON_FUNC_EXIT__;
}

static Evas_Object* _create_list(Evas_Object* parent)
{
	__COMMON_FUNC_ENTER__;

	ip_itc.item_style = "1icon";
	ip_itc.func.text_get = NULL;
	ip_itc.func.content_get = _view_staticip_content_get;
	ip_itc.func.state_get = NULL;
	ip_itc.func.del = _view_staticip_list_del;

	Evas_Object *list = elm_genlist_add(parent);
	elm_genlist_mode_set(list, ELM_LIST_LIMIT);
	evas_object_size_hint_weight_set(list, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(list, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(list);

	sep_itc.item_style = "dialogue/seperator";
	sep_itc.func.text_get = NULL;
	sep_itc.func.content_get = NULL;
	sep_itc.func.state_get = NULL;
	sep_itc.func.del = NULL;

	Elm_Object_Item *sep = NULL;
	sep = elm_genlist_item_append(list, &sep_itc, NULL, NULL, ELM_GENLIST_ITEM_GROUP, NULL, NULL);
	elm_genlist_item_select_mode_set(sep, ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);

	/* add list */
	_add_list_item(list, VIEW_STATICIP_TYPE_IP, sc(PACKAGE, I18N_TYPE_IP_address));
	_add_list_item(list, VIEW_STATICIP_TYPE_SUBNET, sc(PACKAGE, I18N_TYPE_Subnet_mask));
	_add_list_item(list, VIEW_STATICIP_TYPE_GATEWAY, sc(PACKAGE, I18N_TYPE_Gateway));
	_add_list_item(list, VIEW_STATICIP_TYPE_DNS1, sc(PACKAGE, I18N_TYPE_DNS_1));
	_add_list_item(list, VIEW_STATICIP_TYPE_DNS2, sc(PACKAGE, I18N_TYPE_DNS_2));

	__COMMON_FUNC_EXIT__;

	return list;
}

static boolean isValidIP(const char *szIP)
{
	__COMMON_FUNC_ENTER__;

	unsigned int n1,n2,n3,n4;
	if (NULL == szIP) {
		__COMMON_FUNC_EXIT__;
		return FALSE;
	}

	INFO_LOG(UG_NAME_NORMAL, "* str [%s]", szIP);

	int i;
	int nDotCnt = 0;
	for(i=0; i<strlen(szIP); i++) {
		if(szIP[i] == '.') {
			++nDotCnt;
		} else if(szIP[i] == ':') {
			__COMMON_FUNC_EXIT__;
			return FALSE;
		} else if(szIP[i] == '\0') {
			break;
		} else if(0 == isdigit((int)szIP[i])) {
			__COMMON_FUNC_EXIT__;
			return FALSE;	
		}
	}

	if (nDotCnt > 3){
		__COMMON_FUNC_EXIT__;
		return FALSE;
	}

	/* check dotted decial addressing format */
	if(sscanf(szIP, "%u.%u.%u.%u", &n1, &n2, &n3, &n4) != 4) {
		__COMMON_FUNC_EXIT__;
		return FALSE;
	}

	/* check IPv4 address class invariants */
	//if(!n1) return FALSE;

	/* check address range */
	if ((n1 < 0x100) &&
			(n2 < 0x100) &&
			(n3 < 0x100) &&
			(n4 < 0x100)){
		__COMMON_FUNC_EXIT__;
		return TRUE;
	} else {
		__COMMON_FUNC_EXIT__;
		return FALSE;
	}
}

static boolean _staticip_check(void)
{
	__COMMON_FUNC_ENTER__;

	boolean ret = FALSE;

	/** current addresses get */
	char* str1= view_detail_datamodel_static_ip_address_get();
	char* str2= view_detail_datamodel_static_subnet_mask_get();
	char* str3= view_detail_datamodel_static_gateway_address_get();
	char* str4= view_detail_datamodel_static_dns1_address_get();
	char* str5= view_detail_datamodel_static_dns2_address_get();

	assertm_if(NULL == str1, "NULL!!");
	assertm_if(NULL == str2, "NULL!!");
	assertm_if(NULL == str3, "NULL!!");
	assertm_if(NULL == str4, "NULL!!");
	assertm_if(NULL == str5, "NULL!!");

	/** reset target value from "" to guide-text when user input is zero */
	if(0 == strlen(str1)) {
		g_free(str1);
		DEBUG_LOG(UG_NAME_NORMAL, "reset address to guide text [%s]", guide_text_ip);
		view_detail_datamodel_static_ip_address_set(guide_text_ip);
		str1 = view_detail_datamodel_static_ip_address_get();
	}
	if(0 == strlen(str2)) {
		g_free(str2);
		DEBUG_LOG(UG_NAME_NORMAL, "reset address to guide text [%s]", guide_text_subnet);
		view_detail_datamodel_static_subnet_mask_set(guide_text_subnet);
		str2 = view_detail_datamodel_static_subnet_mask_get();
	}
	if(0 == strlen(str3)) {
		g_free(str3);
		DEBUG_LOG(UG_NAME_NORMAL, "reset address to guide text [%s]", guide_text_gateway);
		view_detail_datamodel_static_gateway_address_set(guide_text_gateway);
		str3 = view_detail_datamodel_static_gateway_address_get();
	}
	if(0 == strlen(str4)) {
		g_free(str4);
		DEBUG_LOG(UG_NAME_NORMAL, "reset address to guide text [%s]", guide_text_dns1);
		view_detail_datamodel_static_dns1_address_set(guide_text_dns1);
		str4 = view_detail_datamodel_static_dns1_address_get();
	}
	if(0 == strlen(str5)) {
		g_free(str5);
		DEBUG_LOG(UG_NAME_NORMAL, "reset address to guide text [%s]", guide_text_dns2);
		view_detail_datamodel_static_dns2_address_set(guide_text_dns2);
		str5 = view_detail_datamodel_static_dns2_address_get();
	}

	/** Judge valid or not */
	if(isValidIP(str1) == TRUE && 
		isValidIP(str2) == TRUE && 
		isValidIP(str3) == TRUE && 
		isValidIP(str4) == TRUE && 
		isValidIP(str5) == TRUE) {

		ret = TRUE;
	} else {
		ret = FALSE;
	}

	g_free(str1);
	g_free(str2);
	g_free(str3);
	g_free(str4);
	g_free(str5);

	__COMMON_FUNC_EXIT__;
	return ret;
}

static void static_done_cb(void *data, Evas_Object *obj, void *event_info)
{
	__COMMON_FUNC_ENTER__;

	if(view_staticip_end == TRUE) {
		__COMMON_FUNC_EXIT__;
		return;
	}
	view_staticip_end = TRUE;

	boolean det = _staticip_check();
	if (det) {
		detailview_ip_and_dns_type_set_as_static();
		elm_naviframe_item_pop(viewer_manager_get_naviframe());

		g_free((gpointer) guide_text_ip);
		g_free((gpointer) guide_text_subnet);
		g_free((gpointer) guide_text_gateway);
		g_free((gpointer) guide_text_dns1);
		g_free((gpointer) guide_text_dns2);

		guide_text_ip = NULL;
		guide_text_subnet = NULL;
		guide_text_gateway = NULL;
		guide_text_dns1 = NULL;
		guide_text_dns2 = NULL;
	} else {
		winset_popup_mode_set(NULL, POPUP_MODE_INPUT_FAILED,
				POPUP_OPTION_INPUT_FAILED_PROXY_IP_MISTYPE);
		view_staticip_end = FALSE;
	}

	__COMMON_FUNC_EXIT__;
}

static void static_back_cb(void *data, Evas_Object *obj, void *event_info)
{
	__COMMON_FUNC_ENTER__;

	if(view_staticip_end == TRUE) {
		__COMMON_FUNC_EXIT__;
		return;
	}
	view_staticip_end = TRUE;

	__COMMON_FUNC_EXIT__;
}

static Evas_Object* _create_conformant(Evas_Object* parent)
{
	assertm_if(NULL == parent, "NULL!!");

	Evas_Object* conform = NULL;
	elm_win_conformant_set(parent, TRUE);
	conform = elm_conformant_add(parent);
	assertm_if(NULL == conform, "NULL!!");

	elm_object_style_set(conform, "internal_layout");
	evas_object_show(conform);

	return conform;
}

void view_staticip()
{
	__COMMON_FUNC_ENTER__;

	view_staticip_end = FALSE;

	Evas_Object* navi_frame = NULL;
	navi_frame = viewer_manager_get_naviframe();
	assertm_if(NULL == navi_frame, "NULL!!");

	Evas_Object *layout = elm_layout_add(navi_frame);
	elm_layout_theme_set(layout, "layout", "application", "noindicator");
	evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

	Evas_Object *bg = elm_bg_add(layout);
	evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_object_style_set(bg, "group_list");
	elm_object_part_content_set(layout, "elm.swallow.bg", bg);

	Evas_Object* conform = NULL;
	conform = _create_conformant(layout);
	assertm_if(NULL == conform, "NULL!!");
	elm_object_part_content_set(layout, "elm.swallow.content", conform);

	Evas_Object* bx = NULL;
	bx = _create_list(conform);
	assertm_if(NULL == bx, "NULL!!");

	evas_object_show(layout);
	elm_object_content_set(conform, bx);

	Elm_Object_Item *navi_it = elm_naviframe_item_push(navi_frame, sc(PACKAGE, I18N_TYPE_Static_IP), NULL, NULL, layout, NULL);

	Evas_Object *button_done = elm_button_add(navi_frame);
	elm_object_style_set(button_done, "naviframe/title/default");
	elm_object_text_set(button_done, sc(PACKAGE, I18N_TYPE_Done));
	evas_object_smart_callback_add(button_done, "clicked", (Evas_Smart_Cb) static_done_cb, NULL);
	elm_object_item_part_content_set(navi_it, "title_right_btn", button_done);

	Evas_Object* button_back = elm_object_item_part_content_get(navi_it, "prev_btn");
	elm_object_focus_allow_set(button_back, EINA_TRUE);
	evas_object_smart_callback_add(button_back, "clicked", (Evas_Smart_Cb)static_back_cb, NULL);

	__COMMON_FUNC_EXIT__;
}
