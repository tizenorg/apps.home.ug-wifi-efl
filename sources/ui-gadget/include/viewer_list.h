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

