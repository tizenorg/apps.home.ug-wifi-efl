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
#include "view_dhcpip.h"
#include "i18nmanager.h"
#include "view_detail_datamodel.h"
#include "viewer_manager.h"


typedef struct _view_dhcpip_data{
	char* title;
	char* description;
} _view_dhcpip_data;

static Elm_Genlist_Item_Class sep_itc;
static Elm_Genlist_Item_Class itc;

static char* _gl_listview_text_get(void *data, Evas_Object *obj, const char *part)
{
	assertm_if(NULL == data, "NULL!!");
	assertm_if(NULL == obj, "NULL!!");
	assertm_if(NULL == part, "NULL!!");
	
	_view_dhcpip_data* det = (_view_dhcpip_data*) data;

	DEBUG_LOG(UG_NAME_NORMAL, "part: %s", part);

	if (0 == strncmp("elm.text.1", part, strlen(part))) {
		DEBUG_LOG(UG_NAME_NORMAL, "%s", det->description);
		return strdup(det->description);
	} else if(0 == strncmp("elm.text.2", part, strlen(part))) {
		DEBUG_LOG(UG_NAME_NORMAL, "%s", det->title);
		return strdup(det->title);
	} else {
		return NULL;
	}
}

static void _gl_listview_del(void *data, Evas_Object *obj)
{
	__COMMON_FUNC_ENTER__;

	if (data == NULL)
		return;

	_view_dhcpip_data* det = (_view_dhcpip_data*) data;

	if (det->description)
		g_free(det->description);

	g_free(data);

	__COMMON_FUNC_EXIT__;
}

static int _add_list_item(Evas_Object* view_list, char* title, char* description)
{
	assertm_if(NULL == view_list, "NULL!!");

	_view_dhcpip_data* target = NULL;
	target = (_view_dhcpip_data*) malloc(sizeof(_view_dhcpip_data));
	assertm_if(NULL == target, "NULL!!");
	memset(target, 0, sizeof(_view_dhcpip_data));

	target->title = title;
	target->description = description;

	Elm_Object_Item *it = elm_genlist_item_append(
			view_list,
			&itc,
			target,
			NULL,
			ELM_GENLIST_ITEM_NONE,
			NULL,
			NULL);
	elm_genlist_item_select_mode_set(it, ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);

	return TRUE;
}

static Evas_Object* _create_list(Evas_Object* parent)
{
	__COMMON_FUNC_ENTER__;

	assertm_if(NULL == parent, "NULL!!");

	Evas_Object* view_list = NULL;
	view_list = elm_genlist_add(parent);
	assertm_if(NULL == view_list, "NULL!!");

	elm_genlist_mode_set(view_list,
			ELM_LIST_LIMIT);
	evas_object_size_hint_weight_set(view_list, 
			EVAS_HINT_EXPAND,
			EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(view_list, 
			EVAS_HINT_FILL, 
			EVAS_HINT_FILL);

	sep_itc.item_style = "dialogue/seperator";
	sep_itc.func.text_get = NULL;
	sep_itc.func.content_get = NULL;
	sep_itc.func.state_get = NULL;
	sep_itc.func.del = NULL;

	Elm_Object_Item* sep = NULL;
	sep = elm_genlist_item_append(
			view_list, 
			&sep_itc, 
			NULL, 
			NULL, 
			ELM_GENLIST_ITEM_GROUP, 
			NULL, 
			NULL);
	assertm_if(NULL == sep, "NULL!!");

	elm_genlist_item_select_mode_set(sep, ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);

	itc.item_style = "2text.3";
	itc.func.text_get = _gl_listview_text_get;
	itc.func.content_get = NULL;
	itc.func.state_get = NULL;
	itc.func.del = _gl_listview_del;

	char *ip_addr = view_detail_datamodel_static_ip_address_get();
	char *subnet_addr = view_detail_datamodel_static_subnet_mask_get();
	char *gw_addr = view_detail_datamodel_static_gateway_address_get();
	char *dns1_addr = view_detail_datamodel_static_dns1_address_get();
	char *dns2_addr = view_detail_datamodel_static_dns2_address_get();

	_add_list_item(view_list, sc(PACKAGE, I18N_TYPE_IP_address), ip_addr);
	_add_list_item(view_list, sc(PACKAGE, I18N_TYPE_Subnet_mask), subnet_addr);
	_add_list_item(view_list, sc(PACKAGE, I18N_TYPE_Gateway), gw_addr);
	_add_list_item(view_list, sc(PACKAGE, I18N_TYPE_DNS_1), dns1_addr);
	_add_list_item(view_list, sc(PACKAGE, I18N_TYPE_DNS_2), dns2_addr);

	__COMMON_FUNC_EXIT__;

	return view_list;
}

void view_dhcpip()
{
	__COMMON_FUNC_ENTER__;

	Evas_Object* navi_frame = viewer_manager_get_naviframe();
	assertm_if(NULL == navi_frame, "NULL!!");

	Evas_Object *layout = elm_layout_add(navi_frame);
	elm_layout_theme_set(layout, "layout", "application", "noindicator");
	evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

	Evas_Object *bg = elm_bg_add(layout);
	evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_object_style_set(bg, "group_list");
	elm_object_part_content_set(layout, "elm.swallow.bg", bg);

	Evas_Object* list = _create_list(layout);
	elm_object_part_content_set(layout, "elm.swallow.content", list);
	evas_object_show(layout);

	Elm_Object_Item *navi_it = elm_naviframe_item_push(navi_frame, sc(PACKAGE, I18N_TYPE_Dynamic_IP), NULL, NULL, layout, NULL);

	Evas_Object* back_btn = elm_object_item_part_content_get(navi_it, "prev_btn");
	elm_object_focus_allow_set(back_btn, EINA_TRUE);

	__COMMON_FUNC_EXIT__;
}
