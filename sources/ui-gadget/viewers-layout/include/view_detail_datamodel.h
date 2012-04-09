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
