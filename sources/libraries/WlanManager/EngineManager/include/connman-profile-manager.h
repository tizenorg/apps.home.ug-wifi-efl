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



#ifndef __WIFI_CONNMAN_PROFILE_MANAGER_H__
#define __WIFI_CONNMAN_PROFILE_MANAGER_H_

#include <network-pm-intf.h>

int connman_profile_manager_profile_cache(int count);
int connman_profile_manager_check_favourite(const char *profile_name, int *favourite);
int connman_profile_manager_connected_ssid_set(const char *profile_name);
int connman_profile_manager_disconnected_ssid_set(const char *profile_name);
int connman_profile_manager_profile_modify(net_profile_info_t *new_profile);
int connman_profile_manager_profile_modify_auth(const char *profile_name, void *authdata, int secMode);
void *connman_profile_manager_profile_table_get(void);
int connman_profile_manager_profile_info_get(const char *profile_name, net_profile_info_t *profile);
int connman_profile_manager_scanned_profile_table_size_get(void);
void connman_profile_manager_destroy(void);

#endif
