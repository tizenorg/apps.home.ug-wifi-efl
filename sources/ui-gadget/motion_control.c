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

#include "sensor.h"
#include "motion_control.h"
#include "viewer_manager.h"

static int motion_handle = 0;
static Evas_Object* target = NULL;

static void __motion_shake_cb(unsigned int event_type, sensor_event_data_t *event_data, void *data)
{
	if (viewer_manager_header_mode_get() == HEADER_MODE_OFF)
		power_control();
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
	sf_register_event(motion_handle, MOTION_ENGINE_EVENT_SHAKE, NULL,__motion_shake_cb, base);
}

void motion_start(void)
{
	TARGET_VIEW_FOCUS focus_state = __motion_target_view_focus_get();
	if (focus_state == MOTION_TARGET_VIEW_FOCUS_ON)
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
