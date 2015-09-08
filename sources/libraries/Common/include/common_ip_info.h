/*
 * Wi-Fi
 *
 * Copyright 2012 Samsung Electronics Co., Ltd
 *
 * Licensed under the Flora License, Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.tizenopensource.org/license
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifndef __COMMON_IP_INFO_H__
#define __COMMON_IP_INFO_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <Evas.h>

#include "wlan_manager.h"
#include "common_utils.h"

#define DEFAULT_GUIDE_PROXY_IP "proxy.example.com"
#define DEFAULT_GUIDE_PROXY_PORT "8080"
#define DEFAULT_GUIDE_IP "0.0.0.0"

typedef struct ip_info_list {
	const char *str_pkg_name;
	Evas_Object *genlist;
	Elm_Object_Item *ip_toggle_item;
	Elm_Object_Item *ip_addr_item;
	Elm_Object_Item *mac_addr_item;
	Elm_Object_Item *subnet_mask_item;
	Elm_Object_Item *gateway_addr_item;
	Elm_Object_Item *dns_1_item;
	Elm_Object_Item *dns_2_item;
	Elm_Object_Item *proxy_addr_item;
	Elm_Object_Item *proxy_port_item;

	imf_ctxt_panel_cb_t input_panel_cb;
	void *input_panel_cb_data;

	wifi_ap_h ap;
	wifi_ip_config_type_e ip_type;

} ip_info_list_t;

typedef struct prev_ip_info {
	char *ip_addr;
	char *subnet_mask;
	char *gateway_addr;
	char *dns_1;
	char *dns_2;
	char *proxy_data;
	wifi_ip_config_type_e ip_type;
	wifi_proxy_type_e proxy_type;
} prev_ip_info_t;

typedef struct full_ip_info {
	ip_info_list_t *ip_info_list;
	prev_ip_info_t *prev_ip_info;
	gboolean is_info_changed;
	gboolean is_first_create;
} full_ip_info_t;

full_ip_info_t *ip_info_append_items(wifi_ap_h ap, const char *pkg_name,
		Evas_Object *genlist,
		imf_ctxt_panel_cb_t input_panel_cb, void *input_panel_cb_data);
void ip_info_save_data(full_ip_info_t *ipdata);
void ip_info_remove(ip_info_list_t *ip_info_list);
void ip_info_close_all_keypads(ip_info_list_t *ip_info_list);
void ip_info_enable_all_keypads(ip_info_list_t *ip_info_list);
void ip_info_delete_prev(prev_ip_info_t *prev_ip_info);
#ifdef __cplusplus
}
#endif

#endif
