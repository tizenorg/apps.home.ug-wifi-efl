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
	char* det = NULL;
	assertm_if(NULL == obj, "NULL!!");
	assertm_if(NULL == part, "NULL!!");
	assertm_if(NULL == data, "NULL!!");

	ug_genlist_data_t* gdata = (ug_genlist_data_t*) data;
	assertm_if(NULL == gdata, "NULL!!");
	assertm_if(NULL == gdata->device_info, "NULL!!");
	assertm_if(NULL == gdata->device_info->ssid, "NULL!!");
	assertm_if(NULL == gdata->device_info->ap_status_txt, "NULL!!");

	if (!strncmp(part, "elm.text.1", strlen(part))) {
		det = g_strdup(gdata->device_info->ssid);
		assertm_if(NULL == det, "NULL!!");
	} else if (!strncmp(part, "elm.text.2", strlen(part))) {
		det = g_strdup(gdata->device_info->ap_status_txt);
		assertm_if(NULL == det, "NULL!!");
	}
	return det;
}

static Evas_Object *_gl_listview_content_get(void *data, Evas_Object *obj, const char *part)
{
	assertm_if(NULL == obj, "NULL!!");
	assertm_if(NULL == data, "NULL!!");
	assertm_if(NULL == part, "NULL!!");

	ug_genlist_data_t* gdata = (ug_genlist_data_t*) data;
	Evas_Object* icon = NULL;

	if (gdata->device_info->ap_image_path == NULL) {
		/* if there is no ap_image_path (NO AP Found situation) */
		DEBUG_LOG(UG_NAME_ERR, "Fatal: Image path is NULL");
	} else if (!strncmp(part, "elm.icon.1", strlen(part))) {
		/* for strength */
		icon = elm_icon_add(obj);
		assertm_if(NULL == icon, "NULL!!");
		elm_icon_file_set(icon, gdata->device_info->ap_image_path, NULL);
	} else if (!strncmp(part, "elm.icon.2", strlen(part))) {
		if (VIEWER_ITEM_RADIO_MODE_CONNECTING == gdata->radio_mode) {
			icon = elm_progressbar_add(obj);
			elm_object_style_set(icon, "list_process");
			evas_object_size_hint_align_set(icon, EVAS_HINT_FILL, 0.5);
			evas_object_size_hint_weight_set(icon, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
			elm_progressbar_pulse(icon, TRUE);
		} else {
			icon = elm_button_add(obj);
			assertm_if(NULL == icon, "NULL!!");
			elm_object_style_set(icon, "reveal");
			evas_object_smart_callback_add(icon, "clicked", (Evas_Smart_Cb)list_select_cb, gdata->device_info);
			evas_object_propagate_events_set(icon, EINA_FALSE);
		}
	} else {
		DEBUG_LOG(UG_NAME_NORMAL, "Invalid part name [%s]", part);
	}

	return icon;
}

static void _gl_listview_del(void* data, Evas_Object* obj)
{
	if (data == NULL)
		return;

	ug_genlist_data_t* gdata = (ug_genlist_data_t*) data;
	assertm_if(NULL == gdata->device_info->ssid, "NULL!!");

	DEBUG_LOG(UG_NAME_NORMAL, "del target ssid:[%s]", gdata->device_info->ssid);

	g_free(gdata->device_info->ap_image_path);
	g_free(gdata->device_info->ap_status_txt);
	g_free(gdata->device_info->ssid);
	g_free(gdata->device_info);
	g_free(gdata);

	return;
}

static char* _gl_text_title_get(void *data, Evas_Object *obj, const char *part)
{
	assertm_if(NULL == obj, "NULL!!");
	assertm_if(NULL == part, "NULL!!");

	return (char*) g_strdup(sc(PACKAGE, I18N_TYPE_WiFi_network));
}

static Evas_Object *_gl_content_title_get(void *data, Evas_Object *obj, const char *part)
{
	__COMMON_FUNC_ENTER__;
	Evas_Object *title_progressbar = NULL;
	if (HEADER_MODE_SEARCHING == viewer_manager_header_mode_get()) {
		if (!strcmp(part, "elm.icon")) {
			title_progressbar  = elm_progressbar_add(obj);
			elm_object_style_set(title_progressbar, "list_process_small");
			elm_progressbar_horizontal_set(title_progressbar, EINA_TRUE);
			elm_progressbar_pulse(title_progressbar, EINA_TRUE);
		}
	}
	__COMMON_FUNC_EXIT__;
	return title_progressbar;
}

Evas_Object* viewer_list_create(Evas_Object *win)
{
	__COMMON_FUNC_ENTER__;

	assertm_if(NULL == win, "NULL!!");

	assertm_if(NULL != viewer_list, "Err!!");
	viewer_list = elm_genlist_add(win);
	elm_object_style_set(viewer_list, "dialogue");
	assertm_if(NULL == viewer_list, "NULL!!");

	elm_genlist_mode_set(viewer_list, ELM_LIST_LIMIT);

	evas_object_size_hint_weight_set(viewer_list, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(viewer_list, EVAS_HINT_FILL, EVAS_HINT_FILL);

	container = NULL;
	container = g_slist_alloc();
	assertm_if(NULL == container, "NULL!!");

	itc.item_style = "dialogue/2text.2icon.3.tb";
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
	g_slist_free(container);
	container = NULL;
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

void viewer_list_title_item_update()
{
	elm_genlist_item_update(grouptitle);
}

int viewer_list_title_item_set(Elm_Object_Item *target)
{
	if (viewer_list_item_size_get() == 0) {
		// To use multiline textblock/entry/editfield in genlist, set height_for_width mode
		// then the item's height is calculated while the item's width fits to genlist width.
		elm_genlist_mode_set(viewer_list, ELM_LIST_COMPRESS);

		grouptitle_itc.item_style = "dialogue/title";
		grouptitle_itc.func.text_get = _gl_text_title_get;
		grouptitle_itc.func.content_get = _gl_content_title_get;
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

Elm_Object_Item* viewer_list_item_insert_after(Evas_Object* list,
			void* list_data,
			Elm_Object_Item *after,
			Evas_Smart_Cb callback_func,
			void* callback_data)
{

	Elm_Object_Item* ret = NULL;

	assertm_if(NULL == callback_data, "NULL!!");
	if (!list || !list_data) {
		assertm_if(NULL == list, "NULL!!");
		assertm_if(NULL == list_data, "NULL!!");
		return NULL;
	}

	ug_genlist_data_t* gdata = NULL;
	gdata = (ug_genlist_data_t*) g_malloc0(sizeof(ug_genlist_data_t));
	assertm_if(NULL == gdata, "NULL!!");

	gdata->device_info = (wifi_device_info_t *)list_data;
	gdata->radio_mode = VIEWER_ITEM_RADIO_MODE_OFF;

//	DEBUG_LOG(UG_NAME_NORMAL, "ssid[%s] image[%s]", gdata->device_info->ssid, gdata->device_info->ap_image_path);

	if (!after) {	/* If the after item is NULL then insert it as first item */
//		DEBUG_LOG(UG_NAME_NORMAL, "first added AP item!");
		after = grouptitle;
	}

	ret = elm_genlist_item_insert_after(
			list, /*obj*/
			&itc,/*itc*/
			gdata,/*data*/
			NULL,/*parent*/
			after, /*after than*/
			ELM_GENLIST_ITEM_NONE, /*flags*/
			callback_func,/*func*/
			callback_data);/*func_data*/

	if (!ret) {
		assertm_if(NULL == ret, "NULL!!");
		g_free(gdata->device_info->ap_image_path);
		g_free(gdata->device_info->ap_status_txt);
		g_free(gdata->device_info->ssid);
		g_free(gdata->device_info);
		g_free(gdata);
		return NULL;
	}
		
	container = g_slist_append(container, ret);
	DEBUG_LOG(UG_NAME_NORMAL,
			"* item add complete ssid:[%s] size:[%d]",
			gdata->device_info->ssid,
			viewer_list_item_size_get());

	elm_genlist_item_update(ret);
	return ret;
}

void viewer_list_item_del(Elm_Object_Item *item)
{
	if (!item)
		return;

	elm_object_item_del(item);
	container = g_slist_remove(container, item);
	return;
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

	for (index = size-1; index >= 0; index--) {
		det = viewer_list_item_at_index(index);
		assertm_if(NULL == det, "NULL!!");
		viewer_list_item_del(det);
	}

	__COMMON_FUNC_EXIT__;
	return TRUE;
}

Elm_Object_Item* viewer_list_item_at_index(int index)
{
	__COMMON_FUNC_ENTER__;

	int size = viewer_list_item_size_get();

	assertm_if(0 > size, "query error! (i == zero base)");

	Elm_Object_Item *gli = NULL;
	gli = (Elm_Object_Item*) g_slist_nth_data(container, index+1);
	assertm_if(NULL == gli, "NULL!!");

	if (gli) {
		ug_genlist_data_t* gdata = NULL;
		gdata = elm_object_item_data_get(gli);
		assertm_if(NULL == gdata, "NULL!!");
		if (!gdata)
			gli = NULL;
	}
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

static gint __viewer_list_compare_profile_names (gconstpointer a, gconstpointer b)
{
	Elm_Object_Item *item = (Elm_Object_Item *)a;
	if (item) {
		ug_genlist_data_t* gdata = elm_object_item_data_get(item);
		if (gdata && gdata->device_info) {
			return g_strcmp0((const char *)b, (const char *)gdata->device_info->profile_name);
		}
	}
	return -1;
}

static gint __viewer_list_compare_ssid (gconstpointer a, gconstpointer b)
{
	Elm_Object_Item *item = (Elm_Object_Item *)a;
	if (item) {
		ug_genlist_data_t* gdata = elm_object_item_data_get(item);
		if (gdata && gdata->device_info) {
			return g_strcmp0((const char *)b, (const char *)gdata->device_info->ssid);
		}
	}
	return -1;
}

Elm_Object_Item* item_get_for_profile_name(char* profile_name)
{
	__COMMON_FUNC_ENTER__;

	if (profile_name == NULL) {
		__COMMON_FUNC_EXIT__;
		return NULL;
	}
	
	Elm_Object_Item* item = NULL;
	GSList *found = g_slist_find_custom(container, (gconstpointer)profile_name, __viewer_list_compare_profile_names);
	if (found) {
		INFO_LOG(UG_NAME_NORMAL, "item found for profile_name [%s]", profile_name);
		item = found->data;
	}
	__COMMON_FUNC_EXIT__;
	return item;
}

/* If there is a single profile with the ssid then this function will return the item.
 * If there are no profiles with the ssid OR if there are more than one profiles with
 * ssid then this function will return NULL and the number of aps found will be update in *ap_count
 */
Elm_Object_Item *item_get_for_ssid(const char* ssid, int *ap_count)
{
	__COMMON_FUNC_ENTER__;

	if (ssid == NULL || ap_count == NULL) {
		__COMMON_FUNC_EXIT__;
		return NULL;
	}

	Elm_Object_Item* item = NULL;
	GSList *found = g_slist_find_custom(container, (gconstpointer)ssid, __viewer_list_compare_ssid);
	if (found) {
		*ap_count = 1;
		INFO_LOG(UG_NAME_NORMAL, "item found for ssid [%s]", ssid);
		item = found->data;

		/* Lets check if there are more than one profiles with same ssid */
		found = g_slist_find_custom(found->next, (gconstpointer)ssid, __viewer_list_compare_ssid);
		if (found) {
			*ap_count = 2;
			INFO_LOG(UG_NAME_NORMAL, "two items found for ssid [%s]", ssid);
			item = NULL;
		}
	} else {
		*ap_count = 0;
	}

	__COMMON_FUNC_EXIT__;
	return item;
}
