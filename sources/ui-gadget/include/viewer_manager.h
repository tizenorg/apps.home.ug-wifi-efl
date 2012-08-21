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



#ifndef __DEF_VIEWER_MANAGER_H_
#define __DEF_VIEWER_MANAGER_H_

#include <Elementary.h>
#include <glib.h>

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
	VIEWER_ITEM_RADIO_MODE_OFF = 0,
	VIEWER_ITEM_RADIO_MODE_CONNECTED,
	VIEWER_ITEM_RADIO_MODE_CONNECTING,
	VIEWER_ITEM_RADIO_MODE_CANCEL_CONNECTING,
	VIEWER_ITEM_RADIO_MODE_WPS_CONNECTING,
	VIEWER_ITEM_RADIO_MODE_DISCONNECTING,
	VIEWER_ITEM_RADIO_MODE_MAX

} VIEWER_ITEM_RADIO_MODES;

typedef enum {
	VIEWER_WINSET_SEARCHING,
	VIEWER_WINSET_SUB_CONTENTS
} VIEWER_WINSETS;

//////////////////////////////////////////////////////////////////////////////////

//////// viewer managing ///////////////////////////////////////////////////////////
Evas_Object* viewer_manager_create(Evas_Object* parent);
Eina_Bool viewer_manager_destroy();
Eina_Bool viewer_manager_show(VIEWER_WINSETS winset);
Eina_Bool viewer_manager_hide(VIEWER_WINSETS winset);
Eina_Bool viewer_manager_refresh(void);
void viewer_manager_specific_scan_response_hlr(GSList *bss_info_list);

//////////////////////////////////////////////////////////////////////////////////

//////// item control ////////////////////////////////////////////////////////////
/*
 *
 * add ap_list including "No AP" 
 */
int power_control();

int viewer_manager_item_radio_mode_set(void* object, Elm_Object_Item* item, VIEWER_ITEM_RADIO_MODES mode);
int viewer_manager_hidden_disable_set(int mode);
Evas_Object* viewer_manager_get_naviframe();
int viewer_manager_header_mode_set(HEADER_MODES mode);
HEADER_MODES viewer_manager_header_mode_get(void);
void viewer_manager_scroll_to_top(void);
Elm_Object_Item *viewer_manager_move_item_to_top(Elm_Object_Item *item);
Elm_Object_Item *viewer_manager_add_new_item(const char *profile_name);

#endif
