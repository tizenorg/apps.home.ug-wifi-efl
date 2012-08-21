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



#ifndef __DEF_VIEWER_LIST_H_
#define __DEF_VIEWER_LIST_H_


#include "viewer_manager.h"

//////// genlist struct data ////////////////////////////////////////////////////////////////////
typedef struct {
	wifi_device_info_t *device_info;
	VIEWER_ITEM_RADIO_MODES radio_mode;
} ug_genlist_data_t;
/////////////////////////////////////////////////////////////////////////////////////////////


Evas_Object* viewer_list_create(Evas_Object *win);
int viewer_list_destroy(void);
void viewer_list_title_item_del();
void viewer_list_title_item_update();

////////////////////////////////////////////////////////////////////////////////////////////////

//////// list item add / remove ///////////////////////////////////////////////////////////////
int viewer_list_title_item_set(Elm_Object_Item *target);
Elm_Object_Item* viewer_list_item_insert_after(Evas_Object* list,
		void* list_data,
		Elm_Object_Item *after,
		Evas_Smart_Cb callback_func,
		void *callback_data);
int viewer_list_item_clear();
////////////////////////////////////////////////////////////////////////////////////////////////

//////// item iteration /////////////////////////////////////////////////////////////////////////
int viewer_list_item_size_get(void);
void viewer_list_item_del(Elm_Object_Item *item);
Elm_Object_Item* viewer_list_item_first_get(Evas_Object* list);
Elm_Object_Item* viewer_list_item_next_get(const Elm_Object_Item* current);
Elm_Object_Item* viewer_list_item_at_index(int index);
/////////////////////////////////////////////////////////////////////////////////////////////////

//////// item control /////////////////////////////////////////////////////////////////////////
int viewer_list_item_enable_all(void);
int viewer_list_item_disable_all(void);
//////////////////////////////////////////////////////////////////////////////////////////////////

Elm_Object_Item* item_get_for_profile_name(char* profile_name);
Elm_Object_Item *item_get_for_ssid(const char* ssid, int *num_aps);
void viewer_list_item_move_connected_ap_to_top(const Elm_Object_Item *connected_item);

#endif

