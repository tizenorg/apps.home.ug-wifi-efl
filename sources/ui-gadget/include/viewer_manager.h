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



#ifndef __DEF_VIEWER_MANAGER_H_
#define __DEF_VIEWER_MANAGER_H_

#include <Elementary.h>


//////// viewer-manager enumeration /////////////////////////////////////
typedef enum {
	HEADER_MODE_OFF=0x01,
	HEADER_MODE_ON,
	HEADER_MODE_ACTIVATING,
	HEADER_MODE_CONNECTING,
	HEADER_MODE_CONNECTED,
	HEADER_MODE_DISCONNECTING,
	HEADER_MODE_DEACTIVATING,
	HEADER_MODE_CANCEL_CONNECTING,
	HEADER_MODE_SEARCHING,
	HEADER_MODE_MAX
} HEADER_MODES;

typedef enum {
	VIEWER_ITEM_RADIO_MODE_NULL=0,
	VIEWER_ITEM_RADIO_MODE_OFF,
	VIEWER_ITEM_RADIO_MODE_CONNECTED,
	VIEWER_ITEM_RADIO_MODE_CONNECTING,
	VIEWER_ITEM_RADIO_MODE_CANCEL_CONNECTING,
	VIEWER_ITEM_RADIO_MODE_WPS_CONNECTING,
	VIEWER_ITEM_RADIO_MODE_DISCONNECTING,
	VIEWER_ITEM_RADIO_MODE_MAX

} VIEWER_ITEM_RADIO_MODES;

typedef enum {
	VIEWER_CALLBACK_TYPE_NORMAL_LIST,
	VIEWER_CALLBACK_TYPE_NONE_AP_LIST,
	VIEWER_CALLBACK_TYPE_MAX

} VIEWER_CALLBACK_TYPES;

typedef enum {
	VIEWER_WINSET_SEARCHING,
	VIEWER_WINSET_SUB_CONTENTS

} VIEWER_WINSETS;

typedef enum {
	VIEWER_MANAGER_TOUCH_RESPONSE_TYPE_NULL,
	VIEWER_MANAGER_TOUCH_RESPONSE_TYPE_CONNECTION_AND_DETAILVIEW,
	VIEWER_MANAGER_TOUCH_RESPONSE_TYPE_SELECTION,
	VIEWER_MANAGER_TOUCH_RESPONSE_TYPE_MAX

} VIEWER_MANAGER_TOUCH_RESPONSE_TYPE;


//////////////////////////////////////////////////////////////////////////////////

//////// viewer managing ///////////////////////////////////////////////////////////
Evas_Object* viewer_manager_create(Evas_Object* parent);
Eina_Bool viewer_manager_destroy();
Eina_Bool viewer_manager_show(VIEWER_WINSETS winset);
Eina_Bool viewer_manager_hide(VIEWER_WINSETS winset);
Eina_Bool viewer_manager_refresh(int is_scan);

//////////////////////////////////////////////////////////////////////////////////

//////// item control ////////////////////////////////////////////////////////////
/*
 *
 * add ap_list including "No AP" 
 */
Elm_Object_Item* viewer_manager_item_set(void*entry_data,
				const char* ssid,
				const char* ap_image_path,
				VIEWER_ITEM_RADIO_MODES mode,
				VIEWER_CALLBACK_TYPES type,
				void*callback_data);


int power_control();

Elm_Object_Item *viewer_manager_current_selected_item_get(void);
void viewer_manager_current_selected_item_set(Elm_Object_Item *item);

void viewer_manager_set_enabled_list_click(Eina_Bool enabled);

int viewer_manager_item_radio_mode_all_reset(void);
int viewer_manager_item_radio_mode_set(void* object, Elm_Object_Item* item, VIEWER_ITEM_RADIO_MODES mode);

int viewer_manager_hidden_disable_set(int mode);

Evas_Object* viewer_manager_get_naviframe();
int viewer_manager_header_mode_set(HEADER_MODES mode);
HEADER_MODES viewer_manager_header_mode_get(void);
void viewer_manager_set_enabled_list_update(Eina_Bool enabled);
void viewer_manager_scroll_to_top(void);

#endif
