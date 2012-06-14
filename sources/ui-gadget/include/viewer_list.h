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



#ifndef __DEF_VIEWER_LIST_H_
#define __DEF_VIEWER_LIST_H_


#include "viewer_manager.h"

//////// genlist struct data ////////////////////////////////////////////////////////////////////
typedef struct genlist_data {
	char* ssid;
	char* ap_image_path;
	wifi_device_info_t *device_info;
	VIEWER_ITEM_RADIO_MODES radio_mode;
	void* callback_data;
} genlist_data;
/////////////////////////////////////////////////////////////////////////////////////////////


Evas_Object* viewer_list_create(Evas_Object *win);
int viewer_list_destroy(void);
void viewer_list_title_item_del();
////////////////////////////////////////////////////////////////////////////////////////////////

//////// list item add / remove ///////////////////////////////////////////////////////////////
int viewer_list_title_item_set(Elm_Object_Item *target);
Elm_Object_Item* viewer_list_item_set(Evas_Object* list,
		void* list_data,
		const char* ssid,
		const char* ap_image_path,
		VIEWER_ITEM_RADIO_MODES mode,
		void (*callback_func)(void*data,Evas_Object*obj,void*event_info),
		void*callback_data);
int viewer_list_item_clear();
////////////////////////////////////////////////////////////////////////////////////////////////

//////// item iteration /////////////////////////////////////////////////////////////////////////
int viewer_list_item_size_get(void);
Elm_Object_Item* viewer_list_item_first_get(Evas_Object* list);
Elm_Object_Item* viewer_list_item_next_get(const Elm_Object_Item* current);
Elm_Object_Item* viewer_list_item_at_index(int index);
/////////////////////////////////////////////////////////////////////////////////////////////////

//////// data get/set ///////////////////////////////////////////////////////////////////////////
int viewer_list_item_data_set(Elm_Object_Item* item, const char* key, void* data);
void* viewer_list_item_data_get(const Elm_Object_Item* item, const char* key);
////////////////////////////////////////////////////////////////////////////////////////////////

//////// item control /////////////////////////////////////////////////////////////////////////
int viewer_list_item_enable_all(void);
int viewer_list_item_disable_all(void);
//////////////////////////////////////////////////////////////////////////////////////////////////

Elm_Object_Item* item_get_for_profile_name(char* profile_name);

#endif

