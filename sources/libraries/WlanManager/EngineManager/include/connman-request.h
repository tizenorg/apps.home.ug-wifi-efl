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



#ifndef __CONNMAN_REQUEST_H_
#define __CONNMAN_REQUEST_H_

#include <network-cm-error.h>
#include <network-cm-intf.h>
#include <network-pm-config.h>
#include <network-pm-intf.h>
#include <network-pm-wlan.h>
#include <network-wifi-intf.h>


int connman_request_register(void);
int connman_request_deregister(void);
int connman_request_power_on(void);
int connman_request_power_off(void);
int connman_request_connection_open(const char* profile_name);
int connman_request_specific_scan(const char *ssid);
int connman_request_connection_open_hidden_ap(net_wifi_connection_info_t* conninfo);
int connman_request_connection_close(const char* profile_name);
int connman_request_delete_profile(const char* profile_name);
int connman_request_scan(void);
int connman_request_scan_mode_set(net_wifi_background_scan_mode_t scan_mode);
int connman_request_wps_connection(const char *profile_name);
int connman_request_state_get(const char* profil_name);

#endif
