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



#ifndef __DEF_POPUP_H_
#define __DEF_POPUP_H_


typedef enum {
	POPUP_MODE_OFF=0X01,
	POPUP_MODE_PBC,
	POPUP_MODE_PIN,
	POPUP_MODE_REGISTER_FAILED,
	POPUP_MODE_POWER_ON_FAILED,
	POPUP_MODE_CONNECTING_FAILED,
	POPUP_MODE_INPUT_FAILED,
	POPUP_MODE_ETC,
	POPUP_MODE_MAX
} POPUP_MODES;

typedef enum {
	POPUP_OPTION_NONE=0X01,
	POPUP_OPTION_REGISTER_FAILED_COMMUNICATION_FAILED,
	POPUP_OPTION_POWER_ON_FAILED_MOBILE_HOTSPOT,
	POPUP_OPTION_CONNECTING_FAILED_TIMEOUT,
	POPUP_OPTION_CONNECTING_FAILED_INVALID_OPERATION,
	POPUP_OPTION_CONNECTIONG_PASSWORD_WEP_ERROR,
	POPUP_OPTION_CONNECTIONG_PASSWORD_WPAPSK_ERROR,
	POPUP_OPTION_INPUT_FAILED_PROXY_IP_MISTYPE,
	POPUP_OPTION_ETC_WLAN_STATE_GET_ERROR,
	POPUP_OPTION_ETC_BACK_ENABLE_WHEN_CONNECTED_ERROR,
	POPUP_OPTION_MAX
} POPUP_MODE_OPTIONS;

typedef struct popup_manager_object {
	POPUP_MODES mode;
	Evas_Object* content;
	Evas_Object* progressbar;
	Evas_Object* win;
} popup_manager_object;


void* winset_popup_create(Evas_Object* win);
int winset_popup_mode_set(void* object, POPUP_MODES mode, POPUP_MODE_OPTIONS option);
int winset_popup_simple_set(const char* text);
POPUP_MODES winset_popup_mode_get(void* object);
int winset_popup_destroy(void* object);
Evas_Object* winset_popup_content_get(void* object);
int winset_popup_content_set(Evas_Object* object);
int winset_popup_content_clear();
int winset_popup_stop(void* object);
int winset_popup_timer_remove(void);

#endif
