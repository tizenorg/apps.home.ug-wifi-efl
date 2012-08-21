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



#ifndef __VIEW_DETAIL_DATAMODEL_H_
#define __VIEW_DETAIL_DATAMODEL_H_

#include "wlan_manager.h"

typedef struct view_datamodel_ip_info view_datamodel_ip_info_t;
typedef struct view_datamodel_eap_info view_datamodel_eap_info_t;
typedef struct view_datamodel_basic_info view_datamodel_basic_info_t;

///////////////////////////////////////////////////////////////
// managing function
///////////////////////////////////////////////////////////////
view_datamodel_basic_info_t *view_basic_detail_datamodel_create(const char *profile_name);
view_datamodel_ip_info_t *view_detail_datamodel_ip_info_create(const char *profile_name);
view_datamodel_eap_info_t *view_detail_datamodel_eap_info_create(const char *profile_name);
void view_basic_detail_datamodel_destroy(view_datamodel_basic_info_t *data_object);
void view_detail_datamodel_ip_info_destroy(view_datamodel_ip_info_t *data_object);
void view_detail_datamodel_eap_info_destroy(view_datamodel_eap_info_t *data_object);
///////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////
// determine it`s changeness
///////////////////////////////////////////////////////////////
boolean view_detail_datamodel_save_ip_info_if_modified(const view_datamodel_ip_info_t *data_object);
boolean view_detail_datamodel_save_eap_info_if_modified(const view_datamodel_eap_info_t *data_object);
///////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////
// getter and setter
///////////////////////////////////////////////////////////////
int view_detail_datamodel_ip_and_dns_type_set(view_datamodel_ip_info_t *data_object, const IP_TYPES type);
int view_detail_datamodel_proxy_address_set(view_datamodel_ip_info_t *data_object, const char* proxy);
int view_detail_datamodel_static_ip_address_set(view_datamodel_ip_info_t *data_object, const char* addr);
int view_detail_datamodel_static_gateway_address_set(view_datamodel_ip_info_t *data_object, const char* addr);
int view_detail_datamodel_static_subnet_mask_set(view_datamodel_ip_info_t *data_object, const char* addr);
int view_detail_datamodel_static_dns1_address_set(view_datamodel_ip_info_t *data_object, const char* addr);
int view_detail_datamodel_static_dns2_address_set(view_datamodel_ip_info_t *data_object, const char* addr);
int view_detail_datamodel_eap_ap_name_set(view_datamodel_eap_info_t *data_object, const char *ssid);
int view_detail_datamodel_eap_method_set(view_datamodel_eap_info_t *data_object, const wlan_eap_type_t eap_method);
int view_detail_datamodel_eap_provision_set(view_datamodel_eap_info_t *data_object, const int provision);
int view_detail_datamodel_eap_auth_set(view_datamodel_eap_info_t *data_object, const wlan_eap_auth_type_t auth_type);
int view_detail_datamodel_eap_user_id_set(view_datamodel_eap_info_t *data_object, const char* user_id);
int view_detail_datamodel_eap_anonymous_id_set(view_datamodel_eap_info_t *data_object, const char* anonymous_id);
int view_detail_datamodel_eap_pswd_set(view_datamodel_eap_info_t *data_object, const char* pswd);
int view_detail_datamodel_eap_ca_cert_set(view_datamodel_eap_info_t *data_object, const char* ca_cert);
int view_detail_datamodel_eap_user_cert_set(view_datamodel_eap_info_t *data_object, const char* user_cert);
IP_TYPES view_detail_datamodel_ip_and_dns_type_get(view_datamodel_ip_info_t *data_object);
char *view_detail_datamodel_static_ip_address_get(view_datamodel_ip_info_t *data_object);
char *view_detail_datamodel_static_gateway_address_get(view_datamodel_ip_info_t *data_object);
char *view_detail_datamodel_static_subnet_mask_get(view_datamodel_ip_info_t *data_object);
char *view_detail_datamodel_static_dns1_address_get(view_datamodel_ip_info_t *data_object);
char *view_detail_datamodel_static_dns2_address_get(view_datamodel_ip_info_t *data_object);
char* view_detail_datamodel_proxy_address_get(view_datamodel_ip_info_t *data_object);
char* view_detail_datamodel_MAC_addr_get(view_datamodel_ip_info_t *data_object);

wlan_eap_type_t view_detail_datamodel_eap_method_get(view_datamodel_eap_info_t *data_object);
int view_detail_datamodel_eap_provision_get(view_datamodel_eap_info_t *data_object);
wlan_eap_auth_type_t view_detail_datamodel_eap_auth_get(view_datamodel_eap_info_t *data_object);
char *view_detail_datamodel_user_id_get(view_datamodel_eap_info_t *data_object);
char *view_detail_datamodel_anonymous_id_get(view_datamodel_eap_info_t *data_object);
char *view_detail_datamodel_pswd_get(view_datamodel_eap_info_t *data_object);
char *view_detail_datamodel_ca_cert_get(view_datamodel_eap_info_t *data_object);
char *view_detail_datamodel_user_cert_get(view_datamodel_eap_info_t *data_object);
char *view_detail_datamodel_eap_ap_name_get(view_datamodel_eap_info_t *data_object);

char *view_detail_datamodel_basic_info_profile_name_get(view_datamodel_basic_info_t *data_object);
char *view_detail_datamodel_ap_name_get(view_datamodel_basic_info_t *data_object);
unsigned int view_detail_datamodel_sig_strength_get(view_datamodel_basic_info_t *data_object);
unsigned int view_detail_datamodel_sec_mode_get(view_datamodel_basic_info_t *data_object);
boolean view_detail_datamodel_is_favourite_get(view_datamodel_basic_info_t *data_object);
char view_detail_datamodel_wps_support_get(view_datamodel_basic_info_t *data_object);
///////////////////////////////////////////////////////////////

#endif
