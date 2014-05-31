/*
 * Wi-Fi
 *
 * Copyright 2012-2013 Samsung Electronics Co., Ltd
 *
 * Licensed under the Flora License, Version 1.1 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://floralicense.org/license
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifndef __COMMON_EAP_CONNECT_H__
#define __COMMON_EAP_CONNECT_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <Evas.h>

#include "wlan_manager.h"
#include "common_utils.h"

#define DISABLE_FAST_EAP_METHOD

typedef struct common_eap_connect_data eap_connect_data_t;

eap_connect_data_t *create_eap_view(Evas_Object *win_main,
		Evas_Object *navi_frame, const char *pkg_name,
		wifi_device_info_t *device_info);
eap_connect_data_t *create_eap_popup(Evas_Object *win_main,
		const char *pkg_name, wifi_device_info_t *device_info);

void eap_connect_data_free(eap_connect_data_t *eap_data);

eap_info_list_t *eap_info_append_items(wifi_ap_h ap, Evas_Object* view_list,
		const char *str_pkg_name, imf_ctxt_panel_cb_t input_panel_cb,
		void *input_panel_cb_data);
void eap_info_save_data(eap_info_list_t *eap_info_list_data);
void eap_info_remove(eap_info_list_t *eap_info_list_data);
void eap_view_rotate_popup(eap_connect_data_t *eap_data, int rotate_angle);

#ifdef __cplusplus
}
#endif

#endif /* __COMMON_EAP_CONNECT_H__ */
