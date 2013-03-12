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



#ifndef _WIFI_SETTING_H_
#define _WIFI_SETTING_H_

#include <vconf-keys.h>

#define WIFI_SETTING_WIFI_CONNECTED_AP_NAME VCONFKEY_WIFI_CONNECTED_AP_NAME

int wifi_setting_value_set(const char *key, int value);
int wifi_setting_value_get(const char *key);

int wifi_setting_key_notify_set();

#endif //_WIFI_SETTING_H_
