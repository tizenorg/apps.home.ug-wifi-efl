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
int connman_request_connection_open_hidden_ap(net_wifi_connection_info_t* conninfo);
int connman_request_connection_close(const char* profile_name);
int connman_request_delete_profile(const char* profile_name);
int connman_request_scan(void);
int connman_request_scan_mode_set(net_wifi_background_scan_mode_t scan_mode);
int connman_request_wps_connection(const char *profile_name);
int connman_request_state_get(const char* profil_name);

#endif
