/*
*  Wi-Fi syspopup
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



#ifndef __COMMON_EAP_CONNECT_POPUP_H_
#define __COMMON_EAP_CONNECT_POPUP_H_
#include "wlan_manager.h"

#define DISABLE_FAST_EAP_METHOD

typedef struct eap_info_list eap_info_list_t;
typedef struct common_eap_connect_data common_eap_connect_data_t;
typedef void (*eap_view_close_cb_t)(void);

common_eap_connect_data_t *create_eap_connect(Evas_Object *win_main, Evas_Object *navi_frame, const char *pkg_name, wifi_device_info_t *device_info, eap_view_close_cb_t cb);
eap_info_list_t *eap_info_append_items(const char *profile_name, Evas_Object* view_list, const char *str_pkg_name);
void eap_info_save_data(eap_info_list_t *eap_info_list_data);
void eap_info_remove(eap_info_list_t *eap_info_list_data);
void eap_view_close(common_eap_connect_data_t *eap_data);

#endif
