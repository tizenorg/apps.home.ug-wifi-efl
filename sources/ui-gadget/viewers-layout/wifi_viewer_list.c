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



#include "common.h"
#include "wifi.h"
#include "wlan_manager.h"
#include "wifi-ui-list-callbacks.h"
#include "viewer_list.h"
#include "viewer_manager.h"
#include "appcoreWrapper.h"
#include "i18nmanager.h"


static Evas_Object* viewer_list = NULL;
static GSList* container = NULL;

static Elm_Genlist_Item_Class itc;
static Elm_Genlist_Item_Class grouptitle_itc;
static Elm_Object_Item* grouptitle=NULL;


static char* _gl_listview_text_get(void *data, Evas_Object *obj, const char *part)
{
	assertm_if(NULL == obj, "NULL!!");
	assertm_if(NULL == part, "NULL!!");
	assertm_if(NULL == data, "NULL!!");

	genlist_data* gdata = (genlist_data*) data;
	assertm_if(NULL == gdata, "NULL!!");
	assertm_if(NULL == gdata->ssid, "NULL!!");

	char* det = (char*) strdup(gdata->ssid);
	assertm_if(NULL == det, "NULL!!");

	return det;
}

static Evas_Object *_gl_listview_content_get(void *data, Evas_Object *obj, const char *part)
{
	assertm_if(NULL == obj, "NULL!!");
	assertm_if(NULL == data, "NULL!!");
	assertm_if(NULL == part, "NULL!!");

	genlist_data* gdata = (genlist_data*) data;
	Evas_Object* icon = NULL;

	if (!strncmp(part, "elm.icon.1", strlen(part))) {
		if (gdata->ap_image_path == NULL) {
			return NULL;
		}

		icon = elm_radio_add(obj);
		assertm_if(NULL == icon, "NULL!!");

		switch (gdata->radio_mode) {
		case VIEWER_ITEM_RADIO_MODE_CONNECTED:
			elm_radio_state_value_set(icon, 0);
			break;
		case VIEWER_ITEM_RADIO_MODE_OFF:
		case VIEWER_ITEM_RADIO_MODE_NULL:
		case VIEWER_ITEM_RADIO_MODE_CONNECTING:
		case VIEWER_ITEM_RADIO_MODE_CANCEL_CONNECTING:
		case VIEWER_ITEM_RADIO_MODE_DISCONNECTING:
			elm_radio_state_value_set(icon, 1);
			break;
		default:
			assertm_if(TRUE, "Err!");
			break;
		}

		Evas_Object *fake_icon = appcore_load_edj(obj, WIFI_UG_FAKE_ICON_PATH, "fake_radio");
		elm_object_part_content_set(fake_icon, "radio", icon);

		return fake_icon;

	} else if (!strncmp(part, "elm.icon.2", strlen(part))) {
		/* if there is no ap_image_path (NO AP Found situlation) */
		if (gdata->ap_image_path == NULL) {
			return NULL;
		}

		switch (gdata->radio_mode) {
		case VIEWER_ITEM_RADIO_MODE_OFF:
		case VIEWER_ITEM_RADIO_MODE_NULL:
		case VIEWER_ITEM_RADIO_MODE_CONNECTED:
			/* for strength */
			icon = elm_icon_add(obj);
			assertm_if(NULL == icon, "NULL!!");
			if(NULL != gdata->ap_image_path) {
				elm_icon_file_set(icon, gdata->ap_image_path, NULL);
			}
			evas_object_size_hint_aspect_set(icon, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);
			return icon;

		case VIEWER_ITEM_RADIO_MODE_CONNECTING:
		case VIEWER_ITEM_RADIO_MODE_CANCEL_CONNECTING:
		case VIEWER_ITEM_RADIO_MODE_DISCONNECTING:
			/* for processing animation */
			icon = elm_progressbar_add(obj);
			assertm_if(NULL == icon, "NULL!!");
			elm_object_style_set(icon, "list_process");
			evas_object_size_hint_align_set(icon, EVAS_HINT_FILL, 0.5);
			evas_object_size_hint_weight_set(icon, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
			elm_progressbar_pulse(icon, EINA_TRUE);
			return icon;

		default:
			assertm_if(TRUE, "Err!");
			break;
		}
	} else if (!strncmp(part, "elm.icon.3", strlen(part))) {
		/* if there is no ap_image_path (NO AP Found situlation) */
		if (gdata->ap_image_path == NULL) {
			return NULL;
		}

		icon = elm_button_add(obj);
		assertm_if(NULL == icon, "NULL!!");
		elm_object_style_set(icon, "reveal");
		evas_object_smart_callback_add(icon, "clicked", (Evas_Smart_Cb)list_select_cb, gdata->device_info);
		evas_object_propagate_events_set(icon, EINA_FALSE);
		return icon;
	}

	return NULL;
}

static void _gl_listview_del(void* data, Evas_Object* obj)
{
	if (data == NULL)
		return;

	genlist_data* gdata = (genlist_data*) data;
	assertm_if(NULL == gdata->ssid, "NULL!!");

	DEBUG_LOG(UG_NAME_NORMAL, "del target ssid:[%s]", gdata->ssid);

	g_free(gdata->device_info);
	g_free(gdata);

	return;
}

static char* _gl_text_title_get(void *data, Evas_Object *obj, const char *part)
{
	assertm_if(NULL == obj, "NULL!!");
	assertm_if(NULL == part, "NULL!!");

	if (part != NULL) {
		if (!strncmp(part, "elm.text", strlen(part))) {
			return (char*) strdup(sc(PACKAGE, I18N_TYPE_Select_network));
		}
	}

	return NULL;
}

Evas_Object* viewer_list_create(Evas_Object *win)
{
	__COMMON_FUNC_ENTER__;

	assertm_if(NULL == win, "NULL!!");

	assertm_if(NULL != viewer_list, "Err!!");
	viewer_list = elm_genlist_add(win);
	assertm_if(NULL == viewer_list, "NULL!!");

	elm_genlist_mode_set(viewer_list, ELM_LIST_LIMIT);
	evas_object_size_hint_weight_set(viewer_list, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(viewer_list, EVAS_HINT_FILL, EVAS_HINT_FILL);

	container = NULL;
	container = g_slist_alloc();
	assertm_if(NULL == container, "NULL!!");

	itc.item_style = "dialogue/1text.3icon";
	itc.func.text_get = _gl_listview_text_get;
	itc.func.content_get = _gl_listview_content_get;
	itc.func.state_get = NULL;
	itc.func.del = _gl_listview_del;

	__COMMON_FUNC_EXIT__;
	return viewer_list;
}

int viewer_list_destroy(void)
{
	__COMMON_FUNC_ENTER__;

	assertm_if(NULL == viewer_list, "NULL!!");
	viewer_list_item_clear();
	evas_object_del(viewer_list);

	viewer_list = NULL;

	__COMMON_FUNC_EXIT__;
	return TRUE;
}

void viewer_list_title_item_del()
{
	if (grouptitle) {
		elm_object_item_del(grouptitle);
		grouptitle = NULL;
	}
}

int viewer_list_title_item_set(Elm_Object_Item *target)
{
	if (viewer_list_item_size_get() == 0) {

		grouptitle_itc.item_style = "grouptitle.dialogue";
		grouptitle_itc.func.text_get = _gl_text_title_get;
		grouptitle_itc.func.content_get = NULL;
		grouptitle_itc.func.state_get = NULL;
		grouptitle_itc.func.del = NULL;

		assertm_if(NULL != grouptitle, "Err!!");

		grouptitle = elm_genlist_item_insert_after(viewer_list,
				&grouptitle_itc,
				NULL,
				NULL,
				target,
				ELM_GENLIST_ITEM_NONE,
				NULL,
				NULL);

		assertm_if(NULL == grouptitle, "NULL!!");

		elm_genlist_item_select_mode_set(grouptitle, ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);

		return TRUE;
	} else {
		return FALSE;
	}
}

Elm_Object_Item* viewer_list_item_set(Evas_Object* list,
			void* list_data, 
			const char* ssid,
			const char* ap_image_path,
			VIEWER_ITEM_RADIO_MODES mode,
			void (*callback_func)(void*data,Evas_Object*obj,void*event_info), 
			void*callback_data){

	Elm_Object_Item* ret = NULL;

	assertm_if(NULL == list, "NULL!!");
	assertm_if(NULL == list_data, "NULL!!");
	assertm_if(NULL == ssid, "NULL!!");
	/* FIX ME LATER - NOT TO USE VIEWER_ITEM_RADIO_MODE_NULL */
	//assertm_if(VIEWER_ITEM_RADIO_MODE_NULL >= mode || VIEWER_ITEM_RADIO_MODE_MAX <= mode, "Wrong mode");
	/*assertm_if(NULL == ap_image_path, "NULL!!");*/ // It can be NULL when it is NO-AP
	assertm_if(NULL == callback_func, "NULL!!");
	assertm_if(NULL == callback_data, "NULL!!");

	genlist_data* gdata = NULL;
	gdata = (genlist_data*) malloc(sizeof(genlist_data));
	assertm_if(NULL == gdata, "NULL!!");
	memset(gdata, 0, sizeof(genlist_data));

	gdata->ssid = (char*) strdup( ssid );
	if(NULL != ap_image_path) {
		gdata->ap_image_path = (char*) strdup(ap_image_path);
	} else {
		gdata->ap_image_path = NULL;
	}
	gdata->device_info = (wifi_device_info_t *)list_data;

	DEBUG_LOG(UG_NAME_NORMAL, "ssid[%s] image[%s]", ssid, ap_image_path);

	Elm_Object_Item* target = NULL;

	int count = viewer_list_item_size_get();

	if (0 == count || VIEWER_ITEM_RADIO_MODE_CONNECTED == mode) {
		DEBUG_LOG(UG_NAME_NORMAL, "first added AP item!");
		target = grouptitle;
	} else {
		DEBUG_LOG(UG_NAME_NORMAL, "current size [%d]", count);
		int last_index = count - 1;
		target = viewer_list_item_at_index(last_index);
	}

	ret = elm_genlist_item_insert_after(
			viewer_list, /*obj*/
			&itc,/*itc*/
			gdata,/*data*/
			NULL,/*parent*/
			target, //grouptitle, //target, , /*after than*/
			ELM_GENLIST_ITEM_NONE, /*flags*/
			callback_func,/*func*/
			gdata);/*func_data*/

	assertm_if(NULL == ret, "NULL!!");
		
	gdata->callback_data = callback_data;
	gdata->radio_mode = mode;

	elm_object_item_data_set(ret, gdata);

	if (mode == VIEWER_ITEM_RADIO_MODE_CONNECTED) {
		g_slist_insert(container, ret, 1);
	} else {
		container = g_slist_append(container, ret);
	}

	DEBUG_LOG(UG_NAME_NORMAL,
			"* item add complete ssid:[%s] size:[%d]",
			ssid,
			viewer_list_item_size_get());

	return ret;
}

int viewer_list_item_size_get()
{
	__COMMON_FUNC_ENTER__;
	assertm_if(NULL == container, "NULL!!");

	int ret = g_slist_length(container);
	ret = ret - 1;
	if (ret < 0) {
		ret = 0;
	}

	__COMMON_FUNC_EXIT__;
	return ret;
}

int viewer_list_item_clear()
{
	__COMMON_FUNC_ENTER__;

	if (viewer_list == NULL) {	
		return TRUE;
	}

	int size = viewer_list_item_size_get();
	if(0 == size){
		DEBUG_LOG(UG_NAME_NORMAL, "header only");
		__COMMON_FUNC_EXIT__;
		return TRUE;
	}

	int index = 0;

	DEBUG_LOG(UG_NAME_NORMAL, "list item size [%d]", size);

	/* remove AP items (size) */
	Elm_Object_Item* det = NULL;

	for (index = 0; index < size; index++) {
		DEBUG_LOG(UG_NAME_NORMAL, "* remove");
		det = viewer_list_item_at_index(index);
		assertm_if(NULL == det, "NULL!!");

		genlist_data* gdata = viewer_list_item_data_get(det, "data");
		assertm_if(NULL == gdata, "NULL!!");

		DEBUG_LOG(UG_NAME_NORMAL,"remove ssid:[%s]", gdata->device_info->ssid);
		elm_object_item_del(det);
	}

	g_slist_free(container);
	container = g_slist_alloc();

	__COMMON_FUNC_EXIT__;
	return TRUE;
}

Elm_Object_Item* viewer_list_item_at_index(int index)
{
	__COMMON_FUNC_ENTER__;

	int size = viewer_list_item_size_get();

	assertm_if(0 > size, "query error! (i == zero base)");
	DEBUG_LOG(UG_NAME_NORMAL, "(index_to_get:%d total:%d)", index, size); 

	Elm_Object_Item *gli = NULL;
	gli = (Elm_Object_Item*) g_slist_nth_data(container, index+1);
	assertm_if(NULL == gli, "NULL!!");

	genlist_data* gdata = NULL;
	gdata = viewer_list_item_data_get(gli, "data");
	assertm_if(NULL == gdata, "NULL!!");

	INFO_LOG(UG_NAME_NORMAL,"selected_gli ssid:[%s]", gdata->device_info->ssid);

	__COMMON_FUNC_EXIT__;
	return gli;
}

int viewer_list_item_selected_set(Elm_Object_Item* item, int state)
{
	__COMMON_FUNC_ENTER__;
	assertm_if(NULL == item, "NULL!!");
	elm_genlist_item_selected_set(item, state);
	__COMMON_FUNC_EXIT__;

	return TRUE;
}

Elm_Object_Item* viewer_list_item_first_get(Evas_Object* list)
{
	__COMMON_FUNC_ENTER__;

	Elm_Object_Item* ret=NULL;
	ret = (Elm_Object_Item*) g_slist_nth_data(container, 1);
	assertm_if(NULL == ret, "NULL!!");

	__COMMON_FUNC_EXIT__;
	return ret;
}

Elm_Object_Item* viewer_list_item_next_get(const Elm_Object_Item* current)
{
	assertm_if(NULL == current, "NULL!!");
	Elm_Object_Item* ret=NULL;
	ret = elm_genlist_item_next_get(current);
	assertm_if(NULL == ret, "NULL!!");

	return ret;
}

int viewer_list_item_data_set(Elm_Object_Item* item, const char* key, void* data)
{
	assertm_if(NULL == item, "NULL!!");
	if(NULL == item) {
		ERROR_LOG(UG_NAME_NORMAL, "Err!");
		return FALSE;
	}
	assertm_if(NULL == key, "NULL!!");
	if(NULL == key) {
		ERROR_LOG(UG_NAME_NORMAL, "Err!");
		return FALSE;
	}

	genlist_data* gdata = NULL;
	gdata = elm_object_item_data_get(item);
	if(NULL == gdata) {
		ERROR_LOG(UG_NAME_NORMAL, "Err!");
		return FALSE;
	}

	assertm_if(NULL == gdata, "NULL!!");

	DEBUG_LOG(UG_NAME_REQ, "dataset ssid:[%s] key:[%s]", gdata->device_info->ssid, key);

	if (strncmp(key, "data", strlen(key)) == 0) {
		genlist_data* g_data = (genlist_data*)data;
		gdata->radio_mode = g_data->radio_mode;
		elm_object_item_data_set(item, gdata);
		g_data = elm_object_item_data_get(item);
		DEBUG_LOG(UG_NAME_REQ, "radio_mode:[%d], [%d]", gdata->radio_mode, g_data->radio_mode);
		return TRUE;
	} else {
		assertm_if(TRUE, "unvalid key [%s]", key);
		return FALSE;
	}
	return FALSE;
}

void* viewer_list_item_data_get(const Elm_Object_Item* item, const char* key)
{
	assertm_if(NULL == item, "NULL!!");
	assertm_if(NULL == key, "NULL!!");

	genlist_data* gdata = NULL;
	gdata = elm_object_item_data_get(item);
	assertm_if(NULL == gdata, "NULL!!");
	if(NULL == gdata) {
		return NULL;
	}

	DEBUG_LOG(UG_NAME_NORMAL, "dataget ssid:[%s] key:[%s]", gdata->device_info->ssid, key);

	if (strncmp(key, "data", strlen(key)) == 0) {
		return (void*) gdata;
	} else {
		assertm_if(TRUE, "unvalid key [%s]", key);
		return NULL;
	}

	return NULL;
}

int viewer_list_item_enable_all(void)
{
	__COMMON_FUNC_ENTER__;
	assertm_if(NULL == container, "NULL!!");

	int count = g_slist_length(container);
	int i = 1;

	for(;i<count;i++){
		Elm_Object_Item* det = NULL;
		det = (Elm_Object_Item*) g_slist_nth_data(container, i);
		assertm_if(NULL == det, "NULL!!");
		elm_object_item_disabled_set(det, EINA_FALSE);
	}

	__COMMON_FUNC_EXIT__;
	return TRUE;
}

int viewer_list_item_disable_all(void)
{
	__COMMON_FUNC_ENTER__;
	assertm_if(NULL == container, "NULL!!");

	int count = g_slist_length(container);
	int i = 1;

	for(;i<count;i++){
		Elm_Object_Item* det = NULL;
		det = (Elm_Object_Item*) g_slist_nth_data(container, i);
		assertm_if(NULL == det, "NULL!!");

		elm_object_item_disabled_set(det, EINA_TRUE);
	}

	__COMMON_FUNC_EXIT__;
	return TRUE;
}

Elm_Object_Item* item_get_for_profile_name(char* profile_name)
{
	__COMMON_FUNC_ENTER__;

	if (profile_name == NULL) {
		__COMMON_FUNC_EXIT__;
		return NULL;
	}
	
	INFO_LOG(UG_NAME_RESP, "profile_name [%s]", profile_name);

	int index = 0;
	int size = viewer_list_item_size_get();
	INFO_LOG(UG_NAME_RESP, "total size [%d]", size);

	for (index =0; index < size; index++) {
		Elm_Object_Item* item = viewer_list_item_at_index(index);
		if (item == NULL) {
			DEBUG_LOG(UG_NAME_NORMAL, "continue");
			continue;
		}

		genlist_data* gdata = (genlist_data*) viewer_list_item_data_get(item, "data");
		if (gdata == NULL) {
			__COMMON_FUNC_EXIT__;
			return NULL;
		}

		INFO_LOG(UG_NAME_RESP, "profile_name [%s]\n", gdata->device_info->profile_name);

		if (!strcmp(profile_name, gdata->device_info->profile_name)) {
			__COMMON_FUNC_EXIT__;
			return item;
		}
	}
	
	__COMMON_FUNC_EXIT__;
	return NULL;
}

