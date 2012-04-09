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



#ifndef __VIEW_DETAIL_H_
#define __VIEW_DETAIL_H_


void view_detail(wifi_device_info_t *device_info);

int view_detail_current_proxy_address_set(const char* proxy_address);
const char* view_detail_current_proxy_address_get(void);
int detailview_ip_and_dns_type_set_as_static();

int detailview_modified_ip_address_set(char* data);
int detailview_modified_gateway_address_set(char* data);
int detailview_modified_subnet_mask_set(char* data);
int detailview_modified_dns1_address_set(char* data);
int detailview_modified_dns2_address_set(char* data);

const char* detailview_modified_ip_address_get(void);
const char* detailview_modified_gateway_address_get(void);
const char* detailview_modified_subnet_mask_get(void);
const char* detailview_modified_dns1_address_get(void);
const char* detailview_modified_dns2_address_get(void);

#endif
