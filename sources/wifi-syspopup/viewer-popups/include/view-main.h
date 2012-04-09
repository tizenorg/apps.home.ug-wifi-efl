/*
 * Copyright (c) 2000-2012 Samsung Electronics Co., Ltd All Rights Reserved.
 *
 * This file is part of Wi-Fi syspopup
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



#ifndef __DEF_WIFI_SYSPOPUP_VIEW_MAIN_H_
#define __DEF_WIFI_SYSPOPUP_VIEW_MAIN_H_

#include <Elementary.h>
#include "wlan_manager.h"
#include "wifi-syspopup.h"

/* create */
Evas_Object *view_main_create(Evas_Object* parent);
int view_main_destroy(void);

void *view_main_item_set(net_profile_info_t *profile_info);

int view_main_refresh(void);
int view_main_show(void);
int view_main_item_connection_mode_set(genlist_data *data, ITEM_CONNECTION_MODES mode);

#endif
