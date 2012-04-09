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



#ifndef _WIFI_SETTING_H_
#define _WIFI_SETTING_H_

#include <vconf-keys.h>

#define WIFI_SETTING_WIFI_CONNECTED_AP_NAME VCONFKEY_WIFI_CONNECTED_AP_NAME

int wifi_setting_value_set(const char *key, int value);
int wifi_setting_value_get(const char *key);

int wifi_setting_key_notify_set();

#endif //_WIFI_SETTING_H_
