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



#ifndef __VIEW_DETAIL_DATAMODEL_H_
#define __VIEW_DETAIL_DATAMODEL_H_

#include "wlan_manager.h"

///////////////////////////////////////////////////////////////
// managing function
///////////////////////////////////////////////////////////////
int view_detail_datamodel_create(const char *profile_name);
int view_detail_datamodel_destroy(void);
///////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////
// determine it`s changeness
///////////////////////////////////////////////////////////////
void view_detail_datamodel_determine_modified(const char *profile_name);
int view_detail_datamodel_determine_staticip_modified(void);
///////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////
// getter and setter
///////////////////////////////////////////////////////////////
int view_detail_datamodel_ip_and_dns_type_set(IP_TYPES type);
IP_TYPES view_detail_datamodel_ip_and_dns_type_get(void);

int view_detail_datamodel_proxy_address_set(const char* proxy);
const char* view_detail_datamodel_proxy_address_get(void);

int view_detail_datamodel_static_ip_address_set(const char* addr);
int view_detail_datamodel_static_gateway_address_set(const char* addr);
int view_detail_datamodel_static_subnet_mask_set(const char* addr);
int view_detail_datamodel_static_dns1_address_set(const char* addr);
int view_detail_datamodel_static_dns2_address_set(const char* addr);

char* view_detail_datamodel_static_ip_address_get(void);
char* view_detail_datamodel_static_gateway_address_get(void);
char* view_detail_datamodel_static_subnet_mask_get(void);
char* view_detail_datamodel_static_dns1_address_get(void);
char* view_detail_datamodel_static_dns2_address_get(void);
///////////////////////////////////////////////////////////////

#endif
