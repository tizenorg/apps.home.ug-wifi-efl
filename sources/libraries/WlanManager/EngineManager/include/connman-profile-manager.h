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



#ifndef __WIFI_CONNMAN_PROFILE_MANAGER_H__
#define __WIFI_CONNMAN_PROFILE_MANAGER_H_

#include <network-pm-intf.h>

int connman_profile_manager_profile_cache(int count);
int connman_profile_manager_check_favourite(const char *profile_name, int *favourite);
int connman_profile_manager_connected_ssid_set(const char *profile_name);
int connman_profile_manager_disconnected_ssid_set(const char *profile_name);
int connman_profile_manager_profile_modify(net_profile_info_t new_profile);
int connman_profile_manager_profile_modify_auth(const char *profile_name, void *authdata, int secMode);
void *connman_profile_manager_profile_table_get(void);
int connman_profile_manager_profile_info_get(const char *profile_name, net_profile_info_t *profile);
int connman_profile_manager_scanned_profile_table_size_get(void);
void connman_profile_manager_destroy(void);

#endif
