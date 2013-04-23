/*
 * Wi-Fi
 *
 * Copyright 2012 Samsung Electronics Co., Ltd
 *
 * Licensed under the Flora License, Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://floralicense.org/license
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <vconf.h>
#include <sensor.h>
#include <vconf-keys.h>

#include "common.h"
#include "wlan_manager.h"
#include "motion_control.h"
#include "viewer_manager.h"
#include "wifi-engine-callback.h"

static int motion_handle = -1;
static Evas_Object* target = NULL;

static void __motion_shake_cb(unsigned int event_type, sensor_event_data_t *event_data, void *data)
{
	int scan_result;
	int motion_activated = 0;
	HEADER_MODES current_state;

	vconf_get_bool(VCONFKEY_SETAPPL_MOTION_ACTIVATION, &motion_activated);
	if(motion_activated != 1)
		return;

	vconf_get_bool(VCONFKEY_SETAPPL_USE_SHAKE, &motion_activated);
	if(motion_activated != 1)
		return;

	current_state = viewer_manager_header_mode_get();

	switch(current_state) {
	case HEADER_MODE_OFF:
		power_control();
		break;

	case HEADER_MODE_ON:
	case HEADER_MODE_CONNECTED:
		viewer_manager_show(VIEWER_WINSET_SEARCHING);
		viewer_manager_header_mode_set(HEADER_MODE_SEARCHING);

		scan_result = wlan_manager_scan();
		if (scan_result != WLAN_MANAGER_ERR_NONE) {
			viewer_manager_hide(VIEWER_WINSET_SEARCHING);
			viewer_manager_header_mode_set(current_state);
		}
		break;

	default:
		break;
	}
}

static TARGET_VIEW_FOCUS __motion_target_view_focus_get(void)
{
	if (target == NULL)
		return MOTION_TARGET_VIEW_FOCUS_OFF;

	if (elm_object_focus_get(target))
		return MOTION_TARGET_VIEW_FOCUS_ON;
	else
		return MOTION_TARGET_VIEW_FOCUS_OFF;
}

void motion_create(Evas_Object* base)
{
	target = base;

	motion_handle = sf_connect(MOTION_SENSOR);
	if (motion_handle < 0)
		return;

	sf_register_event(motion_handle, MOTION_ENGINE_EVENT_SHAKE, NULL, __motion_shake_cb, base);
}

void motion_start(void)
{
	TARGET_VIEW_FOCUS focus_state = __motion_target_view_focus_get();

	if ((focus_state == MOTION_TARGET_VIEW_FOCUS_ON) && (motion_handle >= 0))
		sf_start(motion_handle, 0);
}

void motion_stop(void)
{
	sf_stop(motion_handle);
}

void motion_destroy(void)
{
	sf_stop(motion_handle);

	sf_unregister_event(motion_handle, MOTION_ENGINE_EVENT_SHAKE);

	sf_disconnect(motion_handle);
}
