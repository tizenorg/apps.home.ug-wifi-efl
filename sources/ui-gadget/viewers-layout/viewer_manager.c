/*
 * Wi-Fi
 *
 * Copyright 2012 Samsung Electronics Co., Ltd
 *
 * Licensed under the Flora License, Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.tizenopensource.org/license
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <vconf.h>
#include <utilX.h>
#include <vconf-keys.h>
#include <ui-gadget.h>
#include <app_control_internal.h>
#include <efl_extension.h>

#include "common.h"
#include "ug_wifi.h"
#include "viewer_list.h"
#include "i18nmanager.h"
#include "common_utils.h"
#include "winset_popup.h"
#include "viewer_manager.h"
#include "view_ime_hidden.h"
#include "view_advanced.h"
#include "view_detail.h"

typedef struct {
	wlan_security_mode_type_t sec_mode;
	char *ssid;
	wifi_device_info_t *device_info;
	Evas_Object *confirm_popup;
} hidden_ap_data_t;

typedef struct viewer_manager_object {
	Evas_Object *nav;
	Elm_Object_Item *navi_it;
	Evas_Object *scan_button;
	Evas_Object *next_button;
	Evas_Object *prev_button;
	Evas_Object *list;

	Evas_Object *sw_hidden_btn;
	Evas_Object *sw_scan_btn;

	Elm_Object_Item *item_wifi_onoff;
	char *item_wifi_onoff_text;

	HEADER_MODES header_mode;
	Evas_Object *ctxpopup;
	bool is_first_time_no_profiles;

	int sort_type;
	hidden_ap_data_t *hidden_popup_data;
} viewer_manager_object;

typedef struct {
	Evas_Object *list;
	Elm_Object_Item *last_appended_item;
	int total_items_added;
} view_manager_list_update_info_t;

static viewer_manager_object *manager_object = NULL;

extern wifi_appdata *ug_app_state;

static Elm_Genlist_Item_Class wifi_onoff_itc;

static GList *wifi_device_list = NULL;
static Eina_Bool rotate_flag = EINA_FALSE;

static bool show_more = TRUE;
static bool is_on_color_set = FALSE;

static void _hidden_button_callback(void* data, Evas_Object* obj, void* event_info);
static void viewer_manager_hidden_confirm_cleanup(void);

#ifdef MODEL_BUILD_FEATURE_WLAN_CONCURRENT_MODE
static void _launch_wifi_direct_app(void)
{
	int ret = APP_CONTROL_ERROR_NONE;
	app_control_h app_control;

	app_control_create(&app_control);
	app_control_set_operation(app_control, APP_CONTROL_OPERATION_DEFAULT);
	app_control_set_window(app_control, elm_win_xwindow_get(ug_get_window()));
	app_control_set_app_id(app_control, "setting-wifidirect-efl");

	ret = app_control_send_launch_request(app_control, NULL, NULL);
	if(ret == APP_CONTROL_ERROR_NONE) {
		INFO_LOG(UG_NAME_NORMAL, "Launch Wi-Fi Direct successful");
	} else {
		ERROR_LOG(UG_NAME_NORMAL, "Fail to launch Wi-Fi Direct");
	}

	app_control_destroy(app_control);
}
#endif

static gboolean viewer_manager_is_passwd_popup_exists(void)
{
	if (ug_app_state->passpopup != NULL)
		return TRUE;

	return FALSE;
}

void language_changed_refresh()
{
	if (UG_VIEW_SETUP_WIZARD == ug_app_state->ug_type) {
		elm_genlist_realized_items_update(manager_object->list);
		viewer_manager_refresh();
	}
}

void viewer_manager_update_item_favorite_status(wifi_ap_h ap)
{
	if (ap == NULL)
		return;

	Elm_Object_Item *target_item = item_get_for_ap(ap);
	if (target_item == NULL)
		return;

	ug_genlist_data_t *gdata = NULL;

	gdata = (ug_genlist_data_t *)elm_object_item_data_get(target_item);
	if (gdata == NULL || gdata->device_info == NULL) {
		INFO_LOG(COMMON_NAME_ERR, "gdata or device_info is NULL");
		return;
	}

	gdata->radio_mode = VIEWER_ITEM_RADIO_MODE_OFF;
	if (gdata->device_info->ap_status_txt) {
		g_free(gdata->device_info->ap_status_txt);
		gdata->device_info->ap_status_txt =
			common_utils_get_ap_security_type_info_txt(PACKAGE,
					gdata->device_info, false);
	}

	if(target_item != NULL)
		elm_genlist_item_update(target_item);
}

void power_control(void)
{
	__COMMON_FUNC_ENTER__;

	int cur_state = -1;
	cur_state = viewer_manager_header_mode_get();

	INFO_LOG(UG_NAME_NORMAL, "current state %d\n", cur_state);

	int ret;

	switch (cur_state) {
	case HEADER_MODE_OFF:
	case HEADER_MODE_ACTIVATING:
		ug_app_state->is_first_scan = true;
		ret = wlan_manager_power_on();
		switch (ret){
		case WLAN_MANAGER_ERR_NONE:
			viewer_manager_show(VIEWER_WINSET_SEARCHING);
			viewer_manager_header_mode_set(HEADER_MODE_ACTIVATING);
			viewer_manager_create_scan_btn();
			break;

#if defined TIZEN_TETHERING_ENABLE
		case WLAN_MANAGER_ERR_WIFI_TETHERING_OCCUPIED:
			winset_popup_mode_set(ug_app_state->popup_manager,
					POPUP_OPTION_POWER_ON_FAILED_TETHERING_OCCUPIED, NULL);
			break;

		case WLAN_MANAGER_ERR_WIFI_AP_TETHERING_OCCUPIED:
			winset_popup_mode_set(ug_app_state->popup_manager,
					POPUP_OPTION_POWER_ON_FAILED_TETHERING_AP_OCCUPIED, NULL);
			break;
#endif
		case WLAN_MANAGER_ERR_IN_PROGRESS:
			/* Do nothing */
			break;

		default:
			viewer_manager_header_mode_set(HEADER_MODE_OFF);
			INFO_LOG(UG_NAME_NORMAL, "power on failed. ret = %d", ret);
			break;
		}

		break;

	case HEADER_MODE_ON:
	case HEADER_MODE_CONNECTING:
	case HEADER_MODE_CONNECTED:
	case HEADER_MODE_SEARCHING:
		ret = wlan_manager_power_off();
		switch (ret) {
		case WLAN_MANAGER_ERR_NONE:
			viewer_manager_show(VIEWER_WINSET_SEARCHING);
			viewer_manager_header_mode_set(HEADER_MODE_DEACTIVATING);

			// Lets ignore all the scan updates because we are powering off now.
			wlan_manager_disable_scan_result_update();
			break;

		case WLAN_MANAGER_ERR_IN_PROGRESS:
			/* Do nothing */
			break;

		default:
			INFO_LOG(UG_NAME_NORMAL, "power off failed. ret = %d", ret);
			break;
		}

		break;

	case HEADER_MODE_DEACTIVATING:
	default:
		INFO_LOG(UG_NAME_NORMAL, "Powering off in progress. Let it complete. \n");
		break;
	}

	__COMMON_FUNC_EXIT__;
}

static void _transition_finished_main_cb(void *data, Evas_Object *obj, void *event_info)
{
	__COMMON_FUNC_ENTER__;

	view_manager_view_type_t top_view_id;

	top_view_id = (view_manager_view_type_t)evas_object_data_get(
				obj, SCREEN_TYPE_ID_KEY);

	if (data == elm_naviframe_top_item_get(obj)) {
		/* We are now in main view */
		if (viewer_manager_is_passwd_popup_exists() == TRUE) {
			top_view_id = VIEW_MANAGER_VIEW_TYPE_PASSWD_POPUP;
		} else {
			top_view_id = VIEW_MANAGER_VIEW_TYPE_MAIN;
			if (wifi_is_scan_required() == true) {
				viewer_manager_request_scan();
			}
		}
		evas_object_data_set(obj, SCREEN_TYPE_ID_KEY,
				(void *)VIEW_MANAGER_VIEW_TYPE_MAIN);

		viewer_manager_rotate_top_setupwizard_layout();
	}

	INFO_LOG(UG_NAME_NORMAL, "top view id = %d", top_view_id);

	switch(top_view_id) {
	case VIEW_MANAGER_VIEW_TYPE_MAIN:
		/* Lets enable the scan updates */
		wlan_manager_enable_scan_result_update();
		break;

	case VIEW_MANAGER_VIEW_TYPE_DETAIL:
	case VIEW_MANAGER_VIEW_TYPE_EAP:
	case VIEW_MANAGER_VIEW_TYPE_PASSWD_POPUP:
	default:
		/* Lets disable the scan updates so that the UI is not refreshed */
		wlan_manager_disable_scan_result_update();
		if (top_view_id == VIEW_MANAGER_VIEW_TYPE_PASSWD_POPUP) {
			passwd_popup_show(ug_app_state->passpopup);
		}
		break;
	}

	__COMMON_FUNC_EXIT__;
}

static void _lbutton_click_cb(void *data, Evas_Object *obj, void *event_info)
{
	__COMMON_FUNC_ENTER__;

	app_control_h app_control;
	int ret;

	ret = app_control_create(&app_control);
	if (ret != APP_CONTROL_ERROR_NONE) {
		INFO_LOG(UG_NAME_ERR, "app_control_create failed: %d", ret);
		return;
	}

	app_control_add_extra_data(app_control, "result", "lbutton_click");
	ug_send_result(ug_app_state->ug, app_control);

	app_control_destroy(app_control);
	ug_destroy_me(ug_app_state->ug);

	__COMMON_FUNC_EXIT__;
}

static void _rbutton_click_cb(void *data, Evas_Object *obj, void *event_info)
{
	__COMMON_FUNC_ENTER__;

	app_control_h app_control;
	int ret;

	ret = app_control_create(&app_control);
	if (ret != APP_CONTROL_ERROR_NONE) {
		INFO_LOG(UG_NAME_ERR, "app_control_create failed: %d", ret);
		return;
	}

	app_control_add_extra_data(app_control, "result", "rbutton_click");
	ug_send_result(ug_app_state->ug, app_control);

	app_control_destroy(app_control);
	ug_destroy_me(ug_app_state->ug);

	__COMMON_FUNC_EXIT__;
}

static void __back_cb(void *data, Evas_Object *obj, void *event_info)
{
	__COMMON_FUNC_ENTER__;

	if ((view_manager_view_type_t)evas_object_data_get(obj, SCREEN_TYPE_ID_KEY) !=
			VIEW_MANAGER_VIEW_TYPE_MAIN) {
		eext_naviframe_back_cb(data, obj, event_info);
		__COMMON_FUNC_EXIT__;
		return;
	}

	if (viewer_manager_is_passwd_popup_exists() == TRUE ||
			ug_app_state->bAlive == EINA_FALSE) {
		__COMMON_FUNC_EXIT__;
		return;
	}

	wifi_exit();

	__COMMON_FUNC_EXIT__;
}

static Eina_Bool _back_sk_cb(void *data, Elm_Object_Item *it)
{
	__COMMON_FUNC_ENTER__;

	if (ug_app_state->bAlive == EINA_FALSE) {
		__COMMON_FUNC_EXIT__;
		return EINA_TRUE;
	}

	wifi_exit();

	__COMMON_FUNC_EXIT__;
	return EINA_FALSE;
}

#ifdef MODEL_BUILD_FEATURE_WLAN_CONCURRENT_MODE
static gboolean __activate_more_btn(gpointer pdata)
{
	show_more = TRUE;

	if (ug_app_state->timeout) {
		g_source_remove(ug_app_state->timeout);
		ug_app_state->timeout = 0;
	}

	return FALSE;
}
#endif

static void _ctxpopup_item_select_cb(void *data, Evas_Object *obj,
		void *event_info)
{
	const char *label = elm_object_item_text_get((Elm_Object_Item *) event_info);
	if (label) {
		SECURE_INFO_LOG(UG_NAME_NORMAL, "text(%s) is clicked\n", label);
	}

	if (g_strcmp0(label, sc(PACKAGE, I18N_TYPE_Advanced)) == 0) {
		view_advanced();
	} else if (g_strcmp0(label, sc(PACKAGE, I18N_TYPE_Find_Hidden_Network)) == 0) {
		_hidden_button_callback(data, obj, event_info);
	}
#ifdef MODEL_BUILD_FEATURE_WLAN_CONCURRENT_MODE
	else if (g_strcmp0(label, sc(PACKAGE, I18N_TYPE_WiFi_direct)) == 0) {
		show_more = FALSE;
		ug_app_state->timeout = g_timeout_add(300, __activate_more_btn, NULL);
		_launch_wifi_direct_app();
	}
#endif

	if (manager_object->ctxpopup) {
		evas_object_del(manager_object->ctxpopup);
		manager_object->ctxpopup = NULL;
	}
}

static void _ctxpopup_del_cb(void *data,
		 Evas_Object *obj, void *event_info)
{
	__COMMON_FUNC_ENTER__;

	if (!manager_object)
		return;

	evas_object_del(manager_object->ctxpopup);
	manager_object->ctxpopup = NULL;

	__COMMON_FUNC_EXIT__;
}

static void _ctxpopup_move(Evas_Object *parent)
{
	__COMMON_FUNC_ENTER__;

	if (!manager_object)
		return;

	Evas_Coord y = 0, w = 0, h = 0;
	int rotate_angle;

	elm_win_screen_size_get(parent, NULL, &y, &w, &h);
	rotate_angle = elm_win_rotation_get(parent);

	if (0 == rotate_angle || 180 == rotate_angle)
		evas_object_move(manager_object->ctxpopup, w/2, h);
	else if (90 == rotate_angle)
		evas_object_move(manager_object->ctxpopup, h/2, w);
	else
		evas_object_move(manager_object->ctxpopup, h/2, w);

	__COMMON_FUNC_EXIT__;
}

static void _ctxpopup_dismissed_cb(void *data, Evas_Object *obj,
		void *event_info)
{
	__COMMON_FUNC_ENTER__;

	if (!manager_object)
		return;

	Evas_Object *_win_main = data;

	if (!rotate_flag) {
		evas_object_del(manager_object->ctxpopup);
		manager_object->ctxpopup = NULL;
	} else {
		_ctxpopup_move(_win_main);
		evas_object_show(manager_object->ctxpopup);
		rotate_flag = EINA_FALSE;
	}

	__COMMON_FUNC_EXIT__;
}

static void _ctxpopup_rotate_cb(void *data, Evas_Object *obj, void *event_info)
{

	__COMMON_FUNC_ENTER__;

	if (!manager_object)
		return;

	Evas_Object *_win_main = data;

	_ctxpopup_move(_win_main);
	evas_object_show(manager_object->ctxpopup);

	__COMMON_FUNC_EXIT__;
}

static void _ctxpopup_resize_cb(void *data, Evas *e, Evas_Object *obj,
		void *event_info)
{
	__COMMON_FUNC_ENTER__;

	if (manager_object->ctxpopup)
		rotate_flag = EINA_TRUE;
	else
		rotate_flag = EINA_FALSE;

	__COMMON_FUNC_EXIT__;
}

static void _ctxpopup_delete_cb(void *data, Evas *e, Evas_Object *obj,
		void *event_info)
{
	__COMMON_FUNC_ENTER__;

	Evas_Object *navi = (Evas_Object *)data;
	Evas_Object *ctx = obj;

	if (navi == NULL)
		return;

	if (ctx == NULL)
		return;

	evas_object_smart_callback_del(ctx, "dismissed",
			_ctxpopup_dismissed_cb);
	evas_object_event_callback_del(navi, EVAS_CALLBACK_RESIZE,
			_ctxpopup_resize_cb);
	evas_object_smart_callback_del(elm_object_top_widget_get(ctx),
			"rotation,changed", _ctxpopup_rotate_cb);
	evas_object_event_callback_del_full(ctx, EVAS_CALLBACK_DEL,
			_ctxpopup_delete_cb, navi);

	__COMMON_FUNC_EXIT__;

}

static void _more_button_cb(void *data, Evas_Object *obj, void *event_info)
{
	Evas_Object *_win_main = (Evas_Object*)data;
	HEADER_MODES current_state;
	struct ug_data *ugd = NULL;
	Evas_Object *parent = NULL;
	Elm_Object_Item *item = NULL;
	int ps_mode = 0;

	ugd = (struct ug_data *)ug_app_state->gadget;
	retm_if(ugd == NULL || show_more == FALSE);

	parent = ugd->win_main;
	if (!parent || !manager_object) {
		return;
	}

	current_state = viewer_manager_header_mode_get();

	if (manager_object->ctxpopup) {
		evas_object_del(manager_object->ctxpopup);
	}

	ps_mode = common_util_get_system_registry(VCONFKEY_SETAPPL_PSMODE);
	INFO_LOG(UG_NAME_NORMAL, "PS mode - [%d]", ps_mode);

	manager_object->ctxpopup = elm_ctxpopup_add(parent);
	elm_ctxpopup_auto_hide_disabled_set(manager_object->ctxpopup, EINA_TRUE);
	elm_object_style_set(manager_object->ctxpopup, "more/default");
	eext_object_event_callback_add(manager_object->ctxpopup, EEXT_CALLBACK_BACK,
			_ctxpopup_del_cb, NULL);
	eext_object_event_callback_add(manager_object->ctxpopup, EEXT_CALLBACK_MORE,
			_ctxpopup_del_cb, NULL);
	evas_object_smart_callback_add(manager_object->ctxpopup, "dismissed",
			_ctxpopup_dismissed_cb, _win_main);
	evas_object_event_callback_add(manager_object->ctxpopup, EVAS_CALLBACK_DEL,
			_ctxpopup_delete_cb, parent);
	evas_object_event_callback_add(parent, EVAS_CALLBACK_RESIZE,
			_ctxpopup_resize_cb, _win_main);
	evas_object_smart_callback_add(elm_object_top_widget_get(manager_object->ctxpopup),
			"rotation,changed", _ctxpopup_rotate_cb, _win_main);

	if (current_state != HEADER_MODE_OFF &&
			current_state != HEADER_MODE_DEACTIVATING) {
		item = elm_ctxpopup_item_append(manager_object->ctxpopup,
				"IDS_WIFI_BUTTON_FIND_HIDDEN_NETWORK", NULL,
				_ctxpopup_item_select_cb, parent);
		elm_object_item_domain_text_translatable_set(item, PACKAGE, EINA_TRUE);
	}

	item = elm_ctxpopup_item_append(manager_object->ctxpopup,
			"IDS_ST_BODY_ADVANCED", NULL,
			_ctxpopup_item_select_cb, parent);
	elm_object_item_domain_text_translatable_set(item, PACKAGE, EINA_TRUE);

#ifdef MODEL_BUILD_FEATURE_WLAN_CONCURRENT_MODE
	if (ps_mode <= SETTING_PSMODE_POWERFUL &&
			current_state != HEADER_MODE_OFF &&
			current_state != HEADER_MODE_DEACTIVATING) {
		item = elm_ctxpopup_item_append(manager_object->ctxpopup,
				"IDS_WIFI_BODY_WI_FI_DIRECT_ABB", NULL,
				_ctxpopup_item_select_cb, parent);
		elm_object_item_domain_text_translatable_set(item, PACKAGE, EINA_TRUE);
	}
#endif

	elm_ctxpopup_direction_priority_set(manager_object->ctxpopup,
			ELM_CTXPOPUP_DIRECTION_UP, ELM_CTXPOPUP_DIRECTION_UNKNOWN,
			ELM_CTXPOPUP_DIRECTION_UNKNOWN, ELM_CTXPOPUP_DIRECTION_UNKNOWN);

	_ctxpopup_move(_win_main);
	evas_object_show(manager_object->ctxpopup);
}

static void __refresh_scan_callback(void *data,
		Evas_Object *obj, void *event_info)
{
	__COMMON_FUNC_ENTER__;

	int scan_result;
	HEADER_MODES current_state;
	current_state = viewer_manager_header_mode_get();

	switch (current_state) {
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

	__COMMON_FUNC_EXIT__;
}

static void __power_onoff_cb(void *data, Evas_Object *obj, void *event_info)
{
	__COMMON_FUNC_ENTER__;

	Elm_Object_Item *item = NULL;
	const char *object_type;
	HEADER_MODES current_mode;

	item = (Elm_Object_Item *)event_info;
	object_type = evas_object_type_get(obj);

	current_mode = viewer_manager_header_mode_get();

	if (current_mode == HEADER_MODE_ACTIVATING ||
			current_mode == HEADER_MODE_DEACTIVATING) {
		if (g_strcmp0(object_type, "elm_genlist") == 0) {
			elm_genlist_item_selected_set(item, EINA_FALSE);
		}

		__COMMON_FUNC_EXIT__;
		return;
	}

	if (g_strcmp0(object_type, "elm_check") == 0) {
		Eina_Bool check_mode = elm_check_state_get(obj);

		if (check_mode == TRUE && current_mode == HEADER_MODE_OFF) {
			elm_check_state_set(obj, EINA_FALSE);
			power_control();
		} else if (check_mode != TRUE && current_mode != HEADER_MODE_OFF) {
			power_control();
		}
	} else if (g_strcmp0(object_type, "elm_genlist") == 0) {
		power_control();
	}

	if (item != NULL) {
		elm_genlist_item_update(item);
		elm_genlist_item_selected_set(item, EINA_FALSE);
	}

	__COMMON_FUNC_EXIT__;
}

static char *_gl_wifi_onoff_text_get(void *data, Evas_Object *obj, const char *part)
{
	char* det = NULL;

	retvm_if(NULL == manager_object, NULL);

	if (!strcmp("elm.text", part)) {
		det = g_strdup(manager_object->item_wifi_onoff_text);
		assertm_if(NULL == det, "NULL!!");
	}

	return det;
}

static Evas_Object *_gl_wifi_onoff_content_get(void *data,
		Evas_Object *obj, const char *part)
{
	if (manager_object == NULL)
		return NULL;

	Evas_Object *c = NULL;

	Evas_Object *icon = NULL;
#ifdef ACCESSIBLITY_FEATURE
	Evas_Object *ao = NULL;
#endif

	if (!strcmp("elm.swallow.end", part)) {
		icon = elm_layout_add(obj);
		elm_layout_theme_set(icon, "layout", "list/C/type.3", "default");

		switch (manager_object -> header_mode) {
		case HEADER_MODE_OFF:
			/* Wi-Fi off indication button */
			c = elm_check_add(icon);
			elm_object_style_set(c, "on&off");
			evas_object_propagate_events_set(c, EINA_FALSE);
			elm_check_state_set(c, EINA_FALSE);
			evas_object_smart_callback_add(c, "changed",
					__power_onoff_cb, NULL);
#ifdef ACCESSIBLITY_FEATURE
			ao = elm_object_item_access_object_get(manager_object->item_wifi_onoff);
			elm_access_info_set(ao, ELM_ACCESS_TYPE, "on/off button");
			elm_access_info_set(ao, ELM_ACCESS_STATE, "off");
#endif
			break;

		case HEADER_MODE_ACTIVATING:
		case HEADER_MODE_DEACTIVATING:
			/* Progress animation */
			c = elm_progressbar_add(icon);
			elm_object_style_set(c, "process_medium");
			evas_object_size_hint_align_set(c, EVAS_HINT_FILL, 0.5);
			evas_object_size_hint_weight_set(c, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
			elm_progressbar_pulse(c, TRUE);
			break;

		default:
			/* Wi-Fi on indication button */
			c = elm_check_add(icon);
			elm_object_style_set(c, "on&off");
			evas_object_propagate_events_set(c, EINA_FALSE);
			elm_check_state_set(c, EINA_TRUE);
			evas_object_smart_callback_add(c, "changed",
					__power_onoff_cb, NULL);
#ifdef ACCESSIBLITY_FEATURE
			ao = elm_object_item_access_object_get(manager_object->item_wifi_onoff);
			elm_access_info_set(ao, ELM_ACCESS_TYPE, "on/off button");
			elm_access_info_set(ao, ELM_ACCESS_STATE, "on");
#endif
			break;
		}
		evas_object_size_hint_align_set(c, EVAS_HINT_FILL, EVAS_HINT_FILL);
		evas_object_size_hint_weight_set(c, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		elm_layout_content_set(icon, "elm.swallow.content", c);
	}

	return icon;
}

static void __viewer_manager_wifi_onoff_item_create(Evas_Object* genlist)
{
	__COMMON_FUNC_ENTER__;

	manager_object->item_wifi_onoff_text = g_strdup(sc(PACKAGE, I18N_TYPE_Wi_Fi));

	wifi_onoff_itc.item_style = WIFI_GENLIST_1LINE_TEXT_ICON_STYLE;
	wifi_onoff_itc.func.text_get = _gl_wifi_onoff_text_get;
	wifi_onoff_itc.func.content_get = _gl_wifi_onoff_content_get;
	wifi_onoff_itc.func.state_get = NULL;
	wifi_onoff_itc.func.del = NULL;

	manager_object->item_wifi_onoff = elm_genlist_item_append(genlist,
			&wifi_onoff_itc, NULL, NULL, ELM_GENLIST_ITEM_NONE,
			__power_onoff_cb, NULL);

	__COMMON_FUNC_EXIT__;
}

static void _hidden_button_callback(void* data, Evas_Object* obj, void* event_info)
{
	__COMMON_FUNC_ENTER__;

	view_hidden_ap_popup_create(ug_app_state->layout_main, PACKAGE);

	__COMMON_FUNC_EXIT__;
}

void viewer_manager_setup_wizard_btns_color_set(bool state)
{
	Evas_Object *layout = NULL;
	Evas_Object *ly = NULL;
	Elm_Object_Item *navi_it = NULL;

	navi_it = elm_naviframe_top_item_get(manager_object->nav);
	layout = elm_object_item_content_get(navi_it);
	if (layout == NULL) {
		ERROR_LOG(UG_NAME_NORMAL, "layout is NULL");
		return;
	}

	ly = (Evas_Object *)elm_layout_edje_get(layout);
	if (ly == NULL) {
		ERROR_LOG(UG_NAME_NORMAL, "ly is NULL");
		return;
	}
	int item_r, item_g, item_b, item_a = 0;
	int bg_r, bg_g, bg_b, bg_a = 0;
	int item_banded_a, bg_banded_a = 0;
	int i = 0;
	int item_color[3] = {0,};
	Elm_Object_Item *item = NULL;

	if (state == TRUE && is_on_color_set == TRUE) {
		return;
	}

	item = elm_genlist_first_item_get(manager_object->list);
	if (item == NULL) {
		INFO_LOG(UG_NAME_NORMAL, "first item is NULL");
		return;
	}

	const char *item_bg = edje_object_data_get(
			(Evas_Object *)elm_object_item_edje_get(item),
			"bg_color");
	const char *pers_bg = edje_object_data_get(
			(Evas_Object *)elm_layout_edje_get(manager_object->list),
			"personalized_color");

	if (item_bg == NULL || pers_bg == NULL) {
		ERROR_LOG(UG_NAME_NORMAL, "color is NULL");
		return;
	} else {
		INFO_LOG(UG_NAME_NORMAL, "item_bg-[%s] pers_bg-[%s]",
				item_bg, pers_bg);
	}

	edje_object_color_class_get(
			(Evas_Object *)elm_object_item_edje_get(item),
			item_bg, &item_r, &item_g, &item_b, &item_a,
			NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
	edje_object_color_class_get(
			(Evas_Object *)elm_layout_edje_get(manager_object->list),
			pers_bg, &bg_r, &bg_g, &bg_b, &bg_a,
			NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

	if (state == TRUE && manager_object->sw_hidden_btn != NULL) {
		edje_object_signal_emit(ly, "rect_bg_color_on", "elm");

		for (i = 0; i < 5; i++) {
			item_banded_a = 255 - (i+1)*10;
			bg_banded_a = 255 - item_banded_a;

			if (i == 4) {
				item_color[0] = ((item_r * item_banded_a) >> 8) +
						((bg_r * bg_banded_a) >> 8);
				item_color[1] = ((item_g * item_banded_a) >> 8) +
						((bg_g * bg_banded_a) >> 8);
				item_color[2] = ((item_b * item_banded_a) >> 8) +
						((bg_b * bg_banded_a) >> 8);
			}
		}
		edje_object_color_class_set(ly,
				"on_color", item_color[0], item_color[1], item_color[2],
				255, 255, 255, 255, 255, 255, 255, 255, 255);

		is_on_color_set = TRUE;
	} else {
		edje_object_signal_emit(ly, "rect_bg_color_off", "elm");

		item_banded_a = 255 - (i+1)*10;
		bg_banded_a = 255 - item_banded_a;

		item_color[0] = ((item_r * item_banded_a) >> 8) +
				((bg_r * bg_banded_a) >> 8);
		item_color[1] = ((item_g * item_banded_a) >> 8) +
				((bg_g * bg_banded_a) >> 8);
		item_color[2] = ((item_b * item_banded_a) >> 8) +
				((bg_b * bg_banded_a) >> 8);

		edje_object_color_class_set(ly, "off_color",
				item_color[0], item_color[1], item_color[2],
				255, 255, 255, 255, 255, 255, 255, 255, 255);
		is_on_color_set = FALSE;
	}
	INFO_LOG(UG_NAME_NORMAL, "Set button bg color [%d]", state);
}

static void __viewer_manager_hidden_button_create(Evas_Object *genlist)
{
	__COMMON_FUNC_ENTER__;

	if (manager_object->sw_hidden_btn != NULL ||
			ug_app_state->ug_type != UG_VIEW_SETUP_WIZARD) {
		__COMMON_FUNC_EXIT__;
		return;
	}

	Evas_Object *btn = NULL;
	Evas_Object *layout = NULL;
	Elm_Object_Item *navi_it = NULL;

	navi_it = elm_naviframe_top_item_get(manager_object->nav);
	layout = elm_object_item_content_get(navi_it);

	btn = elm_button_add(layout);
	elm_object_style_set(btn, "default");
	evas_object_propagate_events_set(btn, EINA_FALSE);

	evas_object_smart_callback_add(btn, "clicked", _hidden_button_callback,
			NULL);
	elm_object_domain_translatable_text_set(btn, PACKAGE,
			"IDS_WIFI_BUTTON_FIND_HIDDEN_NETWORK");
	elm_object_part_content_set(layout, "button1", btn);

	switch (viewer_manager_header_mode_get()) {
	case HEADER_MODE_ACTIVATING:
	case HEADER_MODE_DEACTIVATING:
	case HEADER_MODE_SEARCHING:
		elm_object_disabled_set(btn, EINA_TRUE);
		break;
	default:
		elm_object_disabled_set(btn, EINA_FALSE);
		break;
	}

	evas_object_show(btn);
	manager_object->sw_hidden_btn = btn;

	__COMMON_FUNC_EXIT__;
}


static void __viewer_manager_setup_wizard_scan_btn_create(Evas_Object *genlist)
{
	__COMMON_FUNC_ENTER__;

	if (manager_object->sw_scan_btn != NULL ||
			ug_app_state->ug_type != UG_VIEW_SETUP_WIZARD) {
		__COMMON_FUNC_EXIT__;
		return;
	}

	Evas_Object *btn = NULL;
	Evas_Object *layout = NULL;
	Elm_Object_Item *navi_it = NULL;

	navi_it = elm_naviframe_top_item_get(manager_object->nav);
	layout = elm_object_item_content_get(navi_it);

	btn = elm_button_add(layout);
	elm_object_style_set(btn, "default");
	evas_object_propagate_events_set(btn, EINA_FALSE);

	evas_object_smart_callback_add(btn, "clicked", __refresh_scan_callback,
			NULL);
	elm_object_domain_translatable_text_set(btn, PACKAGE,
			"IDS_WIFI_BUTTON_SCAN");
	elm_object_part_content_set(layout, "button2", btn);

	switch (viewer_manager_header_mode_get()) {
	case HEADER_MODE_ACTIVATING:
	case HEADER_MODE_DEACTIVATING:
	case HEADER_MODE_SEARCHING:
		elm_object_disabled_set(btn, EINA_TRUE);
		break;
	default:
		elm_object_disabled_set(btn, EINA_FALSE);
		break;
	}

	evas_object_show(btn);
	manager_object->sw_scan_btn = btn;

	__COMMON_FUNC_EXIT__;
}

static void viewer_manager_setup_wizard_scan_btn_set(Eina_Bool show_state)
{
	__COMMON_FUNC_ENTER__;

	if (ug_app_state->ug_type != UG_VIEW_SETUP_WIZARD ||
			manager_object == NULL ||
			manager_object->sw_scan_btn == NULL) {
		__COMMON_FUNC_EXIT__;
		return;
	}

	elm_object_disabled_set(manager_object->sw_scan_btn, !show_state);

	__COMMON_FUNC_EXIT__;
}

static Eina_Bool viewer_manager_scan_button_set(Eina_Bool show_state)
{
	__COMMON_FUNC_ENTER__;

	if (manager_object == NULL) {
		__COMMON_FUNC_EXIT__;
		return EINA_FALSE;
	}

	if (manager_object->scan_button == NULL) {
		viewer_manager_create_scan_btn();
	}
	/* TODO: need to check VIEW_MANAGER_VIEW_TYPE_MAIN ?
	 * Evas_Object* navi_frame = viewer_manager_get_naviframe();
	 * view_manager_view_type_t top_view_id =
	 *			(view_manager_view_type_t)evas_object_data_get(
	 *				navi_frame, SCREEN_TYPE_ID_KEY);
	 */

	if (show_state == EINA_TRUE && manager_object->scan_button != NULL) {
		elm_object_disabled_set(manager_object->scan_button, EINA_FALSE);
	} else if (show_state == EINA_FALSE && manager_object->scan_button != NULL) {
		elm_object_disabled_set(manager_object->scan_button, EINA_TRUE);
	}

	viewer_manager_setup_wizard_scan_btn_set(show_state);

	__COMMON_FUNC_EXIT__;
	return EINA_TRUE;
}

static void __ea_setup_wizard_back_cb(void *data, Evas_Object *obj,
		void *event_info)
{
	if ((view_manager_view_type_t)evas_object_data_get(obj, SCREEN_TYPE_ID_KEY) !=
			VIEW_MANAGER_VIEW_TYPE_MAIN) {
		eext_naviframe_back_cb(data, obj, event_info);
		return;
	}

	if (viewer_manager_is_passwd_popup_exists() == TRUE ||
			ug_app_state->bAlive == EINA_FALSE) {
		return;
	}

	_lbutton_click_cb(data, obj, event_info);
}

static void __viewer_manager_create_setup_wizard_content(Evas_Object *layout)
{
	__COMMON_FUNC_ENTER__;

	Elm_Object_Item *navi_it = NULL;

	elm_layout_file_set(layout, SETUP_WIZARD_EDJ_PATH, "main_pwlock");
	elm_object_domain_translatable_part_text_set(layout, "text.title",
		PACKAGE, sc(PACKAGE, I18N_TYPE_Wi_Fi));

	eext_object_event_callback_add(manager_object->nav, EEXT_CALLBACK_BACK,
			__ea_setup_wizard_back_cb, NULL);

	__viewer_manager_wifi_onoff_item_create(manager_object->list);

	elm_object_part_content_set(layout, "elm.swallow.content",
			manager_object->list);

	navi_it = elm_naviframe_item_push(manager_object->nav, NULL, NULL, NULL, layout, NULL);
	elm_naviframe_item_title_enabled_set(navi_it, EINA_FALSE, EINA_FALSE);

	evas_object_data_set(manager_object->nav, SCREEN_TYPE_ID_KEY,
			(void *)VIEW_MANAGER_VIEW_TYPE_MAIN);
	evas_object_smart_callback_add(manager_object->nav,
			"transition,finished", _transition_finished_main_cb, navi_it);

	Evas_Object *prev_btn = elm_button_add(layout);
	elm_object_style_set(prev_btn, "bottom");
	elm_object_part_content_set(layout, "button.prev", prev_btn);
	elm_object_text_set(prev_btn, sc(PACKAGE, I18N_TYPE_Prev));
	evas_object_smart_callback_add(prev_btn, "clicked", _lbutton_click_cb, manager_object);
	manager_object->prev_button = prev_btn;

	Evas_Object *next_btn = elm_button_add(layout);
	elm_object_style_set(next_btn, "bottom");
	elm_object_part_content_set(layout, "button.next", next_btn);
	elm_object_text_set(next_btn, sc(PACKAGE, I18N_TYPE_Next));
	evas_object_smart_callback_add(next_btn, "clicked", _rbutton_click_cb, manager_object);
	manager_object->next_button = next_btn;

	__COMMON_FUNC_EXIT__;
}

static void __viewer_manager_create_wifi_ug_content(Evas_Object *layout,
		Evas_Object *_win_main)
{
	__COMMON_FUNC_ENTER__;

	Elm_Object_Item *navi_it = NULL;
	Evas_Object *back_btn = NULL;
	Evas_Object *more_btn = NULL;

	elm_layout_theme_set(layout, "layout", "application", "default");
	edje_object_signal_emit(elm_layout_edje_get(layout), "elm,state,show,content", "elm");
	edje_object_signal_emit(elm_layout_edje_get(layout), "elm,bg,show,group_list", "elm");

	__viewer_manager_wifi_onoff_item_create(manager_object->list);

	elm_object_part_content_set(layout, "elm.swallow.content", manager_object->list);

	eext_object_event_callback_add(manager_object->nav, EEXT_CALLBACK_BACK, eext_naviframe_back_cb, NULL);

	back_btn = elm_button_add(manager_object->nav);
	elm_object_style_set(back_btn, "naviframe/back_btn/default");
	evas_object_smart_callback_add(back_btn, "clicked",__back_cb, _win_main);
	elm_object_focus_allow_set(back_btn, EINA_FALSE);

	manager_object->navi_it = navi_it = elm_naviframe_item_push(manager_object->nav,
			sc(PACKAGE, I18N_TYPE_Wi_Fi), back_btn, NULL,
			layout, NULL);
	evas_object_data_set(manager_object->nav, SCREEN_TYPE_ID_KEY,
			(void *)VIEW_MANAGER_VIEW_TYPE_MAIN);
	evas_object_smart_callback_add(manager_object->nav,
			"transition,finished", _transition_finished_main_cb, navi_it);

	more_btn = elm_button_add(manager_object->nav);
	elm_object_style_set(more_btn, "naviframe/more/default");
	evas_object_smart_callback_add(more_btn, "clicked",
			_more_button_cb, _win_main);
	elm_object_item_part_content_set(navi_it, "toolbar_more_btn", more_btn);

	elm_naviframe_item_pop_cb_set(navi_it, _back_sk_cb, NULL);

	__COMMON_FUNC_EXIT__;
}

#if 0 /* not used */
Evas_Object *viewer_manager_create_bg(Evas_Object *parent, char *style)
{
	Evas_Object *bg;

	bg = elm_bg_add(parent);
	evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND,
		EVAS_HINT_EXPAND);

	if (style)
		elm_object_style_set(bg, style);

	elm_win_resize_object_add(parent, bg);
	evas_object_show(bg);

	return bg;
}
#endif

Evas_Object *viewer_manager_create(Evas_Object *_parent, Evas_Object *_win_main)
{
	__COMMON_FUNC_ENTER__;

	retvm_if(NULL != manager_object || NULL == _parent, NULL);

	Evas_Object *layout = NULL;
	Evas_Object *view_content = NULL;

	manager_object = g_new0(viewer_manager_object, 1);
	retvm_if(NULL == manager_object, NULL);

	/* Add Full Layout */
	layout = elm_layout_add(_parent);
	retvm_if(NULL == layout, NULL);

	elm_layout_theme_set(layout, "layout", "application", "default");
	evas_object_size_hint_weight_set(layout,
			EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	edje_object_signal_emit(elm_layout_edje_get(layout),
			"elm,state,show,content", "elm");
	edje_object_signal_emit(elm_layout_edje_get(layout),
			"elm,bg,show,group_list", "elm");
	edje_object_signal_emit(elm_layout_edje_get(layout),
			"elm,state,show,indicator", "elm");

	/* Add Naviframe */
	manager_object->nav = elm_naviframe_add(layout);
	assertm_if(NULL == manager_object->nav, "manager_object->nav is NULL");
	elm_object_part_content_set(layout,
			"elm.swallow.content", manager_object->nav);
	elm_naviframe_prev_btn_auto_pushed_set(manager_object->nav, EINA_FALSE);
	eext_object_event_callback_add(manager_object->nav, EEXT_CALLBACK_MORE,
			eext_naviframe_more_cb, NULL);

	/* Add layout for custom styles */
	elm_theme_extension_add(NULL, CUSTOM_EDITFIELD_PATH);

	/* Add MainView Layout */
	view_content = elm_layout_add(manager_object->nav);

	/* Add genlist */
	manager_object->list = viewer_list_create(view_content);
	assertm_if(NULL == manager_object->list, "manager_object->list is NULL");

	/* Add app setup-wizard/wifi-ug specific contents */
	if (ug_app_state->ug_type == UG_VIEW_SETUP_WIZARD) {
		__viewer_manager_create_setup_wizard_content(view_content);
	} else {
		__viewer_manager_create_wifi_ug_content(view_content, _win_main);
	}

	evas_object_show(layout);
	elm_object_focus_set(layout, EINA_TRUE);
	if (ug_app_state->ug_type == UG_VIEW_SETUP_WIZARD) {
		viewer_manager_setup_wizard_btns_color_set(FALSE);
	}

	__COMMON_FUNC_EXIT__;
	return layout;
}

void viewer_manager_destroy(void)
{
	__COMMON_FUNC_ENTER__;

	if (wifi_device_list != NULL) {
		g_list_free(wifi_device_list);
		wifi_device_list = NULL;
	}

	if (manager_object != NULL) {
		viewer_manager_cleanup_views();

		if (manager_object->item_wifi_onoff_text) {
			g_free(manager_object->item_wifi_onoff_text);
			manager_object->item_wifi_onoff_text = NULL;
		}

		g_free(manager_object);
		manager_object = NULL;
	}

	__COMMON_FUNC_EXIT__;
}

Eina_Bool viewer_manager_show(VIEWER_WINSETS winset)
{
	__COMMON_FUNC_ENTER__;

	assertm_if(NULL == manager_object, "Manager object is NULL");

	switch (winset) {
	case VIEWER_WINSET_SEARCHING:
		viewer_manager_scan_button_set(EINA_FALSE);
		viewer_list_item_disable_all();
		break;

	case VIEWER_WINSET_SUB_CONTENTS:
		assertm_if(NULL == manager_object->list, "List is NULL");

		viewer_list_title_item_set(manager_object->item_wifi_onoff);
		if (ug_app_state->ug_type == UG_VIEW_SETUP_WIZARD) {
			__viewer_manager_hidden_button_create(manager_object->list);
			__viewer_manager_setup_wizard_scan_btn_create(
				manager_object->list);
			viewer_manager_setup_wizard_btns_color_set(TRUE);
		}
		break;

	case VIEWER_WINSET_SEARCHING_GRP_TITLE:
		viewer_manager_scan_button_set(EINA_FALSE);
		break;

	default:
		break;
	}

	__COMMON_FUNC_EXIT__;
	return EINA_TRUE;
}

static void __viewer_manager_hidden_btn_del(void)
{
	__COMMON_FUNC_ENTER__;

	if (manager_object->sw_hidden_btn) {
		evas_object_del(manager_object->sw_hidden_btn);
		manager_object->sw_hidden_btn = NULL;
	}

	__COMMON_FUNC_EXIT__;
}

static void __viewer_manager_setup_wizard_scan_btn_del(void)
{
	if (ug_app_state->ug_type != UG_VIEW_SETUP_WIZARD) {
		return;
	}

	__COMMON_FUNC_ENTER__;

	if (manager_object->sw_scan_btn) {
		evas_object_del(manager_object->sw_scan_btn);
		manager_object->sw_scan_btn = NULL;
	}

	__COMMON_FUNC_EXIT__;
}

Eina_Bool viewer_manager_hide(VIEWER_WINSETS winset)
{
	__COMMON_FUNC_ENTER__;

	switch (winset) {
	case VIEWER_WINSET_SEARCHING:
		/* searching view */
		viewer_manager_scan_button_set(EINA_TRUE);
		viewer_list_item_enable_all();
		break;

	case VIEWER_WINSET_SUB_CONTENTS:
		/* hidden AP and WPS PBC */
		if (ug_app_state->passpopup) {
			passwd_popup_free(ug_app_state->passpopup);
			ug_app_state->passpopup = NULL;
		}

		if (ug_app_state->eap_view) {
			eap_connect_data_free(ug_app_state->eap_view);
			ug_app_state->eap_view = NULL;
		}

		if (ug_app_state->ug_type == UG_VIEW_SETUP_WIZARD) {
			viewer_manager_setup_wizard_btns_color_set(FALSE);
			__viewer_manager_hidden_btn_del();
			__viewer_manager_setup_wizard_scan_btn_del();
		}
		viewer_list_title_item_del();
		break;

	default:
		break;
	}

	__COMMON_FUNC_EXIT__;
	return EINA_TRUE;
}

#if 0
/* Unused function */
Eina_Bool viewer_manager_genlist_item_update(Elm_Object_Item* item)
{
	__COMMON_FUNC_ENTER__;
	if (item == NULL) {
		__COMMON_FUNC_EXIT__;
		return EINA_FALSE;
	}

	if(item != NULL)
		elm_genlist_item_update(item);

	__COMMON_FUNC_EXIT__;
	return EINA_FALSE;
}
#endif

void viewer_manager_update_hidden_btn(void)
{
	if (ug_app_state->ug_type != UG_VIEW_SETUP_WIZARD) {
		return;
	}

	__COMMON_FUNC_ENTER__;
	retm_if(NULL == manager_object->sw_hidden_btn);

	switch (viewer_manager_header_mode_get()) {
	case HEADER_MODE_ACTIVATING:
	case HEADER_MODE_DEACTIVATING:
	case HEADER_MODE_SEARCHING:
		elm_object_disabled_set(manager_object->sw_hidden_btn,
				EINA_TRUE);
		break;
	default:
		elm_object_disabled_set(manager_object->sw_hidden_btn,
				EINA_FALSE);
		break;
	}

	__COMMON_FUNC_EXIT__;
}

void viewer_manager_update_setup_wizard_scan_btn(void)
{
	if (ug_app_state->ug_type != UG_VIEW_SETUP_WIZARD) {
		return;
	}

	__COMMON_FUNC_ENTER__;
	retm_if(NULL == manager_object->sw_scan_btn);

	switch (viewer_manager_header_mode_get()) {
	case HEADER_MODE_ACTIVATING:
	case HEADER_MODE_DEACTIVATING:
	case HEADER_MODE_SEARCHING:
		elm_object_disabled_set(manager_object->sw_scan_btn,
				EINA_TRUE);
		break;
	default:
		elm_object_disabled_set(manager_object->sw_scan_btn,
				EINA_FALSE);
		break;
	}

	__COMMON_FUNC_EXIT__;
}

void viewer_manager_setup_wizard_button_controller()
{
	__COMMON_FUNC_ENTER__;

	int ret;
	wifi_connection_state_e connection_state;

	ret = wifi_get_connection_state(&connection_state);
	if (ret != WIFI_ERROR_NONE) {
		INFO_LOG(UG_NAME_NORMAL, "Failed to retrieve connection state ret [%d]", ret);
	}

	if (manager_object->prev_button != NULL &&
		ug_app_state->lbutton_setup_wizard_prev != NULL) {
		elm_object_text_set(manager_object->prev_button,
			sc(PACKAGE, I18N_TYPE_Prev));
	}

	if (connection_state == WIFI_CONNECTION_STATE_CONNECTED) {
		if (manager_object->next_button != NULL &&
			ug_app_state->rbutton_setup_wizard_next != NULL) {
			elm_object_text_set(manager_object->next_button,
				sc(PACKAGE, I18N_TYPE_Next));
		}
	} else {
		if (manager_object->next_button != NULL &&
			ug_app_state->rbutton_setup_wizard_skip != NULL) {
			elm_object_text_set(manager_object->next_button,
				sc(PACKAGE, I18N_TYPE_Skip));
		}
	}

	if (manager_object->sw_hidden_btn != NULL) {
		elm_object_domain_translatable_text_set(
				manager_object->sw_hidden_btn, PACKAGE,
				"IDS_WIFI_BUTTON_FIND_HIDDEN_NETWORK");
	}

	if (manager_object->sw_scan_btn != NULL) {
		elm_object_domain_translatable_text_set(
				manager_object->sw_scan_btn, PACKAGE,
				"IDS_WIFI_BUTTON_SCAN");
	}
}

static void viewer_manager_pop_naviframe_item(void)
{
	__COMMON_FUNC_ENTER__;

	view_manager_view_type_t top_viewID;

	top_viewID = viewer_manager_view_type_get();
	if (top_viewID == VIEW_MANAGER_VIEW_TYPE_DETAIL) {
		elm_naviframe_item_pop(viewer_manager_get_naviframe());
	}

	__COMMON_FUNC_EXIT__;
}

Evas_Object *viewer_manager_naviframe_power_item_get(void)
{
	__COMMON_FUNC_ENTER__;

	Elm_Object_Item *navi_it = NULL;
	Evas_Object *layout = NULL;

	if (manager_object == NULL)
		return NULL;

	navi_it = elm_naviframe_top_item_get(manager_object->nav);

	if (navi_it == NULL) {
		ERROR_LOG(UG_NAME_ERR, "navi_it is NULL");
		return NULL;
	}

	if (ug_app_state->ug_type == UG_VIEW_SETUP_WIZARD) {
		layout = elm_object_item_content_get(navi_it);
		if (layout) {
			__COMMON_FUNC_EXIT__;
			return elm_object_part_content_get(layout, "title_right_btn");
		}
	}

	__COMMON_FUNC_EXIT__;
	return elm_object_item_part_content_get(navi_it, "title_right_btn");
}

int viewer_manager_create_scan_btn(void) {
	__COMMON_FUNC_ENTER__;
	if (ug_app_state->ug_type == UG_VIEW_SETUP_WIZARD || manager_object->scan_button) {
		__COMMON_FUNC_EXIT__;
		return 0;
	}
	Evas_Object *btn = elm_button_add(manager_object->nav);
		/* Use "bottom" style button */
	if (!btn) {
		ERROR_LOG(UG_NAME_NORMAL, "Error creating toolbar");
		return -1;
	}
	elm_object_style_set(btn, "bottom");
	elm_object_domain_translatable_text_set(btn, PACKAGE,
			"IDS_WIFI_BUTTON_SCAN");
	evas_object_smart_callback_add(btn, "clicked", __refresh_scan_callback, NULL);

		/* Set button into "toolbar" swallow part */
	elm_object_item_part_content_set(manager_object->navi_it, "toolbar", btn);
	manager_object->scan_button = btn;
	evas_object_show(manager_object->scan_button);
	__COMMON_FUNC_EXIT__;
	return 0;
}

void viewer_manager_header_mode_set(HEADER_MODES new_mode)
{
	__COMMON_FUNC_ENTER__;

	HEADER_MODES old_mode;
	if (manager_object == NULL)
		return;

	if (HEADER_MODE_OFF > new_mode || HEADER_MODE_MAX <= new_mode) {
		ERROR_LOG(UG_NAME_ERR, "Invalid mode %d", new_mode);
		return;
	}

	old_mode = manager_object->header_mode;
	if (old_mode == new_mode)
		return;

	DEBUG_LOG(UG_NAME_NORMAL, "Header mode %d --> %d", old_mode, new_mode);

	manager_object->header_mode = new_mode;

	switch (new_mode) {
	case HEADER_MODE_OFF:
		if (manager_object->scan_button) {
			evas_object_del(manager_object->scan_button);
			manager_object->scan_button = NULL;
		}
		if (ug_app_state->ug_type == UG_VIEW_SETUP_WIZARD) {
			viewer_manager_setup_wizard_btns_color_set(FALSE);
			__viewer_manager_hidden_btn_del();
			__viewer_manager_setup_wizard_scan_btn_del();
		}
		break;
	case HEADER_MODE_ON:
	case HEADER_MODE_CONNECTED:
		viewer_manager_scan_button_set(EINA_TRUE);
		break;
	default:
		break;
	}

	if(manager_object->item_wifi_onoff != NULL)
		elm_genlist_item_update(manager_object->item_wifi_onoff);
	viewer_list_title_item_update();
	if (ug_app_state->ug_type == UG_VIEW_SETUP_WIZARD) {
		viewer_manager_update_hidden_btn();
		viewer_manager_update_setup_wizard_scan_btn();
		viewer_manager_setup_wizard_button_controller();
	}

	__COMMON_FUNC_EXIT__;
}

HEADER_MODES viewer_manager_header_mode_get(void)
{
	return manager_object->header_mode;
}

Evas_Object* viewer_manager_get_naviframe()
{
	return manager_object->nav;
}

void viewer_manager_refresh_ap_info(Elm_Object_Item *item)
{
	if (!item) {
		return;
	}

	ug_genlist_data_t *gdata = elm_object_item_data_get(item);
	if (!gdata) {
		return;
	}
	wifi_device_info_t *wifi_device = gdata->device_info;
	if (!wifi_device) {
		return;
	}

	wifi_ap_refresh(wifi_device->ap);

	return;
}

static gboolean __viewer_manager_list_scroll_to_top(void *data)
{
	if (data)
		elm_genlist_item_bring_in((Elm_Object_Item *)data, ELM_GENLIST_ITEM_SCROLLTO_TOP);

	return FALSE;
}

void viewer_manager_move_to_top(void)
{
	__COMMON_FUNC_ENTER__;

	Elm_Object_Item * first_item = NULL;

	if (manager_object == NULL || manager_object->list == NULL)
		return;

	first_item = elm_genlist_first_item_get(manager_object->list);
	if (first_item == NULL)
		return;

	common_util_managed_idle_add(__viewer_manager_list_scroll_to_top, first_item);

	__COMMON_FUNC_EXIT__;
}

Elm_Object_Item *viewer_manager_move_item_to_top(Elm_Object_Item *old_item)
{
	__COMMON_FUNC_ENTER__;

	Elm_Object_Item *first_item = viewer_list_get_first_item();
	ug_genlist_data_t *gdata = NULL, *first_it_gdata = NULL;

	if (!old_item || !first_item) {
		__COMMON_FUNC_EXIT__;
		return NULL;
	}

	gdata = elm_object_item_data_get(old_item);
	if (!gdata || !gdata->device_info) {
		__COMMON_FUNC_EXIT__;
		return NULL;
	}

	if (old_item != first_item) {
		first_it_gdata = elm_object_item_data_get(first_item);
		elm_object_item_data_set(first_item, gdata);
		elm_object_item_data_set(old_item, first_it_gdata);

		if(first_item != NULL)
			elm_genlist_item_update(first_item);
		if(old_item != NULL)
			elm_genlist_item_update(old_item);
	}

	__COMMON_FUNC_EXIT__;
	return first_item;
}

void viewer_manager_update_rssi(void)
{
	int ret;
	wifi_ap_h ap;

	ret = wifi_get_connected_ap(&ap);
	if (WIFI_ERROR_NONE != ret) {
		return;
	}

	Elm_Object_Item *item = item_get_for_ap(ap);
	if (!item) {
		wifi_ap_destroy(ap);
		return;
	}

	ug_genlist_data_t* gdata = elm_object_item_data_get(item);
	if (gdata && gdata->device_info) {
		int rssi = 0;

		if (WIFI_ERROR_NONE != wifi_ap_get_rssi(ap, &rssi)) {
			wifi_ap_destroy(ap);
			return;
		} else if (gdata->device_info->rssi != rssi) {
			wifi_ap_refresh(gdata->device_info->ap);
			gdata->device_info->rssi = rssi;

			g_free(gdata->device_info->ap_image_path);

			common_utils_get_device_icon(gdata->device_info,
					&gdata->device_info->ap_image_path);

			if(item != NULL)
				elm_genlist_item_update(item);
		}
	}

	wifi_ap_destroy(ap);
}

static gint compare(gconstpointer a, gconstpointer b)
{
	bool favorite1 = false, favorite2 = false;
	wifi_connection_state_e state1 = 0, state2 = 0;

	wifi_device_info_t *wifi_device1 = (wifi_device_info_t*)a;
	wifi_device_info_t *wifi_device2 = (wifi_device_info_t*)b;

	wifi_ap_get_connection_state(wifi_device1->ap, &state1);
	wifi_ap_get_connection_state(wifi_device2->ap, &state2);

	if (state1 != state2) {
		if (state1 == WIFI_CONNECTION_STATE_CONNECTED)
			return -1;
		if (state2 == WIFI_CONNECTION_STATE_CONNECTED)
			return 1;

		if (state1 == WIFI_CONNECTION_STATE_CONFIGURATION)
			return -1;
		if (state2 == WIFI_CONNECTION_STATE_CONFIGURATION)
			return 1;

		if (state1 == WIFI_CONNECTION_STATE_ASSOCIATION)
			return -1;
		if (state2 == WIFI_CONNECTION_STATE_ASSOCIATION)
			return 1;
	}

	wifi_ap_is_favorite(wifi_device1->ap, &favorite1);
	wifi_ap_is_favorite(wifi_device2->ap, &favorite2);

	if (favorite1 != favorite2) {
		if (favorite1 == true)
			return -1;
		if (favorite2 == true)
			return 1;
	}

	if (manager_object->sort_type == I18N_TYPE_Alphabetical) {
		return strcasecmp((const char *)wifi_device1->ssid,
				(const char *)wifi_device2->ssid);
	} else {
		return ((wifi_device1->rssi >= wifi_device2->rssi) ? -1 : 1);
	}
}

static bool wifi_update_list_for_each_ap(wifi_ap_h ap, void *user_data)
{
	int *profile_size = (int *)user_data;
	wifi_device_info_t *wifi_device = NULL;

	wifi_device = view_list_item_device_info_create(ap);
	if (wifi_device == NULL)
		return true;

	wifi_device_list = g_list_insert_sorted(wifi_device_list, wifi_device, compare);
	(*profile_size)++;

	return true;
}

static int viewer_manager_update_list_all(char *ssid)
{
	int i, profiles_list_size = 0;
	Elm_Object_Item *item = NULL;
	GList* list_of_device ;
	wifi_device_info_t *wifi_device = NULL;
	int ssid_count = 0;

	__COMMON_FUNC_ENTER__;

	view_manager_list_update_info_t update_info;
	memset(&update_info, 0, sizeof(update_info));

	manager_object->sort_type = _convert_vconf_to_sort_by_value(
			common_util_get_system_registry(VCONF_SORT_BY));

	wifi_foreach_found_aps(wifi_update_list_for_each_ap, &profiles_list_size);

	list_of_device = wifi_device_list;
	for (i = 0; i < profiles_list_size && list_of_device != NULL; i++) {
		wifi_device = (wifi_device_info_t*)list_of_device->data;
		if (ssid == NULL || g_strcmp0(ssid, wifi_device->ssid) == 0) {
			item = viewer_list_item_insert_after(wifi_device, update_info.last_appended_item);
			if (item) {
				update_info.last_appended_item = item;
				update_info.total_items_added++;
				ssid_count++;
			}
		}

		list_of_device = list_of_device->next;
	}

	if (wifi_device_list != NULL) {
		g_list_free(wifi_device_list);
		wifi_device_list = NULL;
	}

	INFO_LOG(UG_NAME_NORMAL, "total items added = %d", update_info.total_items_added);

	if (0 == update_info.total_items_added) {
		/* Check if whether it's the first time we get no profiles,
		 * if so, send one more scan request. This avoids the issue of
		 * showing 'No Wi-Fi AP found' during launch time.
		 */
		if (manager_object->is_first_time_no_profiles == false) {
			HEADER_MODES current_state;
			int scan_result = WLAN_MANAGER_ERR_NONE;
			current_state = viewer_manager_header_mode_get();

			viewer_manager_show(VIEWER_WINSET_SEARCHING);
			viewer_manager_header_mode_set(HEADER_MODE_SEARCHING);

			scan_result = wlan_manager_scan();
			if (scan_result != WLAN_MANAGER_ERR_NONE) {
				viewer_manager_hide(VIEWER_WINSET_SEARCHING);
				viewer_manager_header_mode_set(current_state);
			}

			manager_object->is_first_time_no_profiles = true;
		} else {
			/* if there is no scan_data, generate No-AP item */
			item = viewer_list_item_insert_after(NULL, NULL);
			if (item)
				elm_genlist_item_select_mode_set(item,
						ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);

		}
	}

	__COMMON_FUNC_EXIT__;
	return ssid_count;
}

Eina_Bool viewer_manager_refresh(void)
{
	int state;
	int header_state;

	INFO_LOG(UG_NAME_SCAN, "viewer manager refresh");

	if (manager_object == NULL)
		return EINA_FALSE;

	header_state = viewer_manager_header_mode_get();
	if (header_state == HEADER_MODE_CONNECTING ||
			header_state == HEADER_MODE_ACTIVATING ||
			header_state == HEADER_MODE_DEACTIVATING) {
		return EINA_FALSE;
	}

	viewer_list_item_disable_all();
	viewer_list_item_clear();

	state = wlan_manager_state_get();
	if (WLAN_MANAGER_ERROR == state || WLAN_MANAGER_OFF == state) {
		/* Some body requested to refresh the list
		 * while the WLAN manager is OFF or Unable to get the profile state
		 */
		INFO_LOG(UG_NAME_ERR, "WLAN Manager state: %d", state);

		viewer_manager_header_mode_set(HEADER_MODE_OFF);

		return EINA_FALSE;
	}

	wifi_ap_h ap = wlan_manager_get_ap_with_state(state);
	viewer_manager_update_list_all(NULL);
	if (ug_app_state->ug_type == UG_VIEW_SETUP_WIZARD) {
		__viewer_manager_hidden_button_create(manager_object->list);
		__viewer_manager_setup_wizard_scan_btn_create(
			manager_object->list);
		viewer_manager_setup_wizard_btns_color_set(TRUE);
	}

	if (WLAN_MANAGER_CONNECTING == state) {
		INFO_LOG(UG_NAME_NORMAL, "Wi-Fi is connecting");

		Elm_Object_Item* target_item = item_get_for_ap(ap);

		viewer_manager_header_mode_set(HEADER_MODE_CONNECTING);
		viewer_list_item_radio_mode_set(target_item,
				VIEWER_ITEM_RADIO_MODE_CONNECTING);
	} else if (WLAN_MANAGER_CONNECTED == state) {
		INFO_LOG(UG_NAME_NORMAL, "Wi-Fi is connected");

		Elm_Object_Item* target_item = item_get_for_ap(ap);

		target_item = viewer_manager_move_item_to_top(target_item);
		viewer_manager_header_mode_set(HEADER_MODE_CONNECTED);
		viewer_list_item_radio_mode_set(target_item,
				VIEWER_ITEM_RADIO_MODE_CONNECTED);
	} else {
		INFO_LOG(UG_NAME_NORMAL, "Wi-Fi state: %d", state);

		viewer_manager_header_mode_set(HEADER_MODE_ON);
	}
	wifi_ap_destroy(ap);

	INFO_LOG(UG_NAME_SCAN, "viewer manager refresh finished");

	return EINA_TRUE;
}

static void hidden_ap_connect_ok_cb (void *data,
		Evas_Object *obj, void *event_info)
{
	__COMMON_FUNC_ENTER__;

	hidden_ap_data_t *hidden_ap_data = (hidden_ap_data_t *)data;
	if (hidden_ap_data == NULL) {
		__COMMON_FUNC_EXIT__;
		return;
	}

	char* szPassword = NULL;
	wifi_ap_h ap;

	int ret = wifi_ap_hidden_create(hidden_ap_data->ssid, &ap);
	if (WIFI_ERROR_NONE != ret) {
		ERROR_LOG(UG_NAME_ERR, "Failed to create an AP handle. Err = %d", ret);

		goto exit;
	}

	SECURE_INFO_LOG(UG_NAME_NORMAL, "Hidden AP[%s]. Sec mode = %d. Connect ok cb",
			hidden_ap_data->ssid, hidden_ap_data->sec_mode);

	switch (hidden_ap_data->sec_mode) {
	case WLAN_SEC_MODE_NONE:
		INFO_LOG(UG_NAME_NORMAL, "OPEN: event %x; passpopup %x",
				event_info, ug_app_state->passpopup);

		wifi_ap_set_security_type(ap, WIFI_SECURITY_TYPE_NONE);
		break;

	case WLAN_SEC_MODE_WEP:
	case WLAN_SEC_MODE_WPA_PSK:
	case WLAN_SEC_MODE_WPA2_PSK:
		szPassword = passwd_popup_get_txt(ug_app_state->passpopup);

		if (WLAN_SEC_MODE_WEP == hidden_ap_data->sec_mode) {
			wifi_ap_set_security_type(ap, WIFI_SECURITY_TYPE_WEP);
		} else if (WLAN_SEC_MODE_WPA_PSK == hidden_ap_data->sec_mode) {
			wifi_ap_set_security_type(ap, WIFI_SECURITY_TYPE_WPA_PSK);
		} else {
			wifi_ap_set_security_type(ap, WIFI_SECURITY_TYPE_WPA2_PSK);
		}

		int ret = wifi_ap_set_passphrase(ap, szPassword);
		if (ret != WIFI_ERROR_NONE)
			INFO_LOG(UG_NAME_ERR, "Failed to set passpharse [%d]" ,ret);

		g_free(szPassword);

		passwd_popup_free(ug_app_state->passpopup);
		ug_app_state->passpopup = NULL;
		break;

	default:
		INFO_LOG(UG_NAME_NORMAL, "Fatal: Unknown Sec mode: %d",
				hidden_ap_data->sec_mode);

		goto hidden_ap_connect_end;
	}

	wlan_manager_connect(ap);

hidden_ap_connect_end:
	wifi_ap_destroy(ap);

exit:
	viewer_manager_hidden_confirm_cleanup();

	__COMMON_FUNC_EXIT__;
}

static void hidden_ap_connect_cancel_cb(void *data, Evas_Object *obj,
		void *event_info)
{
	__COMMON_FUNC_ENTER__;

	hidden_ap_data_t *hidden_ap_data = (hidden_ap_data_t *)data;
	if (hidden_ap_data == NULL) {
		__COMMON_FUNC_EXIT__;
		return;
	}

	switch (hidden_ap_data->sec_mode) {
	case WLAN_SEC_MODE_NONE:
		INFO_LOG(UG_NAME_NORMAL, "This hidden AP is Open");
		break;

	case WLAN_SEC_MODE_WEP:
	case WLAN_SEC_MODE_WPA_PSK:
	case WLAN_SEC_MODE_WPA2_PSK:
		INFO_LOG(UG_NAME_NORMAL, "Hidden AP Secured");

		passwd_popup_free(ug_app_state->passpopup);
		ug_app_state->passpopup = NULL;
		break;

	default:
		INFO_LOG(UG_NAME_NORMAL, "Fatal: Unknown Sec mode: %d",
				hidden_ap_data->sec_mode);
		break;
	}

	viewer_manager_request_scan();
	viewer_manager_hidden_confirm_cleanup();

	__COMMON_FUNC_EXIT__;
}

static void __hidden_ap_cancel_mouseup_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	__COMMON_FUNC_ENTER__;

	Evas_Event_Mouse_Up *event = event_info;

	if (event->button == 3) {
		hidden_ap_connect_cancel_cb(data, obj, event_info);
	}

	__COMMON_FUNC_EXIT__;
}

static void __hidden_ap_cancel_keydown_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	__COMMON_FUNC_ENTER__;

	Evas_Event_Key_Down *event = event_info;

	if (g_strcmp0(event->keyname, KEY_BACK) == 0) {
		hidden_ap_connect_cancel_cb(data, obj, event_info);
	}

	__COMMON_FUNC_EXIT__;
}

void viewer_manager_eap_view_deref(void)
{
	__COMMON_FUNC_ENTER__;

	ug_app_state->eap_view = NULL;

	__COMMON_FUNC_EXIT__;
}

void viewer_manager_specific_scan_response_hlr(
		GSList *bss_info_list, void *user_data)
{
	hidden_ap_data_t *hidden_ap_data = NULL;
	const char *ssid = (const char *)user_data;
	wlan_security_mode_type_t sec_mode = 0;
	bool favorite = false;
	ug_genlist_data_t* gdata = NULL;
	Evas_Object *popup = NULL;
	int hidden_ap_count = (int)g_slist_length(bss_info_list);
	Elm_Object_Item *item = NULL;

	viewer_manager_hide(VIEWER_WINSET_SEARCHING);
	viewer_manager_header_mode_set(HEADER_MODE_ON);

	if (view_hidden_ap_popup_data_get() == NULL) {
		ERROR_LOG(UG_NAME_RESP, "Popup is already destroyed");

		g_free(user_data);
		return;
	}
	/**
	 * We will delete the popup anyway
	 */
	view_hidden_ap_popup_destroy();

	if (ssid == NULL) {
		ERROR_LOG(UG_NAME_RESP, "SSID is empty");
		return;
	}

	INFO_LOG(UG_NAME_RESP, "Recieved %s(%d) hidden Wi-Fi networks",
			ssid, hidden_ap_count);
	viewer_list_item_clear();
	hidden_ap_count = viewer_manager_update_list_all((char *)ssid);
	INFO_LOG(UG_NAME_NORMAL, "Found %s(%d) Wi-Fi networks", ssid,
			hidden_ap_count);

	if (hidden_ap_count > 1)
		goto exit;

	viewer_manager_refresh();
	if (hidden_ap_count == 0) {
		INFO_LOG(UG_NAME_ERR, "no ap found");
		common_utils_send_message_to_net_popup("Network connection popup",
				"no ap found", "toast_popup", NULL);
		/*Recreate the popup*/
		view_hidden_ap_popup_create(ug_app_state->layout_main, PACKAGE);
		goto exit;
	}

	item = item_get_for_ssid(ssid);
	if (item != NULL) {
		gdata = elm_object_item_data_get(item);
		if (gdata == NULL || gdata->device_info == NULL ||
				gdata->radio_mode == VIEWER_ITEM_RADIO_MODE_CONNECTED)
			goto exit;
		sec_mode = gdata->device_info->security_mode;
		gdata->device_info->is_hidden = true;
		wifi_ap_is_favorite(gdata->device_info->ap, &favorite);
		if (favorite == true) {
			wlan_manager_connect(gdata->device_info->ap);
			goto exit;
		}

		/* Only if there is one AP found then we need Users further action */
		switch (sec_mode) {
		case WLAN_SEC_MODE_NONE:
			SECURE_INFO_LOG(UG_NAME_NORMAL, "%s open network found", ssid);

			hidden_ap_data = g_new0(hidden_ap_data_t, 1);
			hidden_ap_data->sec_mode = WLAN_SEC_MODE_NONE;
			hidden_ap_data->ssid = g_strdup(ssid);

			popup_btn_info_t popup_btn_data;
			memset(&popup_btn_data, 0, sizeof(popup_btn_data));
			popup_btn_data.title_txt = g_strdup(ssid);
			popup_btn_data.info_txt = "IDS_WIFI_BODY_A_WI_FI_NETWORK_HAS_BEEN_DETECTED_YOU_WILL_BE_CONNECTED";
			popup_btn_data.btn1_txt = "IDS_WIFI_SK_CANCEL";
			popup_btn_data.btn1_cb = hidden_ap_connect_cancel_cb;
			popup_btn_data.btn1_data = popup_btn_data.btn2_data = hidden_ap_data;
			popup_btn_data.btn2_txt = "IDS_WIFI_BODY_CONNECT";
			popup_btn_data.btn2_cb = hidden_ap_connect_ok_cb;

			popup = common_utils_show_info_popup(ug_app_state->layout_main,
							&popup_btn_data);

			hidden_ap_data->confirm_popup  = popup;

			evas_object_event_callback_add(popup,
					EVAS_CALLBACK_MOUSE_UP,
					__hidden_ap_cancel_mouseup_cb,
					hidden_ap_data);
			evas_object_event_callback_add(popup,
					EVAS_CALLBACK_KEY_DOWN,
					__hidden_ap_cancel_keydown_cb,
					hidden_ap_data);

			manager_object->hidden_popup_data = hidden_ap_data;
			break;

		case WLAN_SEC_MODE_IEEE8021X:
			SECURE_INFO_LOG(UG_NAME_NORMAL, "%s IEEE8021X found", ssid);

			wifi_device_info_t device_info;
			wifi_ap_h ap;

			wifi_ap_hidden_create(ssid, &ap);
			wifi_ap_set_security_type(ap, WIFI_SECURITY_TYPE_EAP);

			struct ug_data *ugd = (struct ug_data *)ug_app_state->gadget;

			memset(&device_info, 0, sizeof(device_info));
			device_info.security_mode = WIFI_SECURITY_TYPE_EAP;
			device_info.ssid = (char *)ssid;
			device_info.ap = ap;
			ug_app_state->eap_view =
					create_eap_view(ug_app_state->layout_main,
							ugd->win_main,
							ug_app_state->conformant,
							PACKAGE, &device_info,
							viewer_manager_eap_view_deref);

			wifi_ap_destroy(ap);

			break;

		case WLAN_SEC_MODE_WEP:
		case WLAN_SEC_MODE_WPA_PSK:
		case WLAN_SEC_MODE_WPA2_PSK:
			SECURE_INFO_LOG(UG_NAME_NORMAL,
					"Secured(%d) %s found", sec_mode, ssid);

			pswd_popup_create_req_data_t popup_info;

			hidden_ap_data = g_try_new0(hidden_ap_data_t, 1);
			if (hidden_ap_data == NULL) {
				ERROR_LOG(UG_NAME_RESP, "Memory allocation error.");
				goto exit;
			}
			hidden_ap_data->sec_mode = sec_mode;
			hidden_ap_data->ssid = g_strdup(ssid);
			if (gdata && gdata->device_info) {
				hidden_ap_data->device_info = view_list_item_device_info_create(
						gdata->device_info->ap);
			}
			memset(&popup_info, 0, sizeof(pswd_popup_create_req_data_t));
			/* Wi-Fi hidden network cannot be connected by WPS method.
			 * In addition, Wi-Fi network info is also impossible to be set.
			 * It's ConnMan agent limitation.
			 * Do not use WPS method in here.
			 */
			popup_info.title = (char *)ssid;
			popup_info.ok_cb = hidden_ap_connect_ok_cb;
			popup_info.cancel_cb = hidden_ap_connect_cancel_cb;
			popup_info.cb_data = hidden_ap_data;

			if (sec_mode == WLAN_SEC_MODE_WEP)
				popup_info.sec_type = WIFI_SECURITY_TYPE_WEP;
			else if (sec_mode == WLAN_SEC_MODE_WPA_PSK)
				popup_info.sec_type = WIFI_SECURITY_TYPE_WPA_PSK;
			else if (sec_mode == WLAN_SEC_MODE_WPA2_PSK)
				popup_info.sec_type = WIFI_SECURITY_TYPE_WPA2_PSK;

			ug_app_state->passpopup =
					create_passwd_popup(
							ug_app_state->conformant,
							ug_app_state->layout_main,
							PACKAGE, &popup_info);
			if (ug_app_state->passpopup == NULL)
				INFO_LOG(UG_NAME_ERR, "Fail to create password popup");

			manager_object->hidden_popup_data = hidden_ap_data;
			break;

		default:
			INFO_LOG(UG_NAME_NORMAL, "Unknown security mode: %d", sec_mode);
			break;
		}
	}

exit:
	g_free(user_data);
}

wifi_device_info_t *view_list_item_device_info_create(wifi_ap_h ap)
{
	wifi_security_type_e sec_type;
	int ret = false;
	wifi_device_info_t* wifi_device = NULL;

	if (ap != NULL) {
		wifi_device = g_try_new0(wifi_device_info_t, 1);
		retvm_if(NULL == wifi_device, NULL);

		if (WIFI_ERROR_NONE != wifi_ap_clone(&(wifi_device->ap), ap)) {
			goto FREE_DEVICE_INFO;
		} else if (WIFI_ERROR_NONE != wifi_ap_get_essid(ap, &(wifi_device->ssid))) {
			goto FREE_DEVICE_INFO;
		} else if (WIFI_ERROR_NONE != wifi_ap_get_rssi(ap, &(wifi_device->rssi))) {
			goto FREE_DEVICE_INFO;
		} else if (WIFI_ERROR_NONE != wifi_ap_get_security_type(ap, &sec_type)) {
			goto FREE_DEVICE_INFO;
		} else if (WIFI_ERROR_NONE != wifi_ap_is_wps_supported(ap, &(wifi_device->wps_mode))) {
			goto FREE_DEVICE_INFO;
		}

		wifi_device->security_mode = common_utils_get_sec_mode(sec_type);
		common_utils_get_device_icon(wifi_device, &wifi_device->ap_image_path);
	}
	ret = true;

FREE_DEVICE_INFO:
	if (ret == false && wifi_device) {
		wifi_ap_destroy(wifi_device->ap);
		g_free(wifi_device->ap_image_path);
		g_free(wifi_device->ap_status_txt);
		g_free(wifi_device->ssid);
		g_free(wifi_device);
	}

	return wifi_device;
}

view_manager_view_type_t viewer_manager_view_type_get(void)
{
	__COMMON_FUNC_ENTER__;

	Evas_Object* navi_frame = NULL;
	view_manager_view_type_t top_view_id;

	navi_frame = viewer_manager_get_naviframe();
	top_view_id = (view_manager_view_type_t)evas_object_data_get(
							navi_frame,
							SCREEN_TYPE_ID_KEY);

	__COMMON_FUNC_EXIT__;
	return top_view_id;
}

void viewer_manager_request_scan(void)
{
	__COMMON_FUNC_ENTER__;

	HEADER_MODES current_state;

	current_state = viewer_manager_header_mode_get();

	if (current_state == HEADER_MODE_ON ||
			current_state == HEADER_MODE_CONNECTED) {
		int scan_result;

		INFO_LOG(UG_NAME_NORMAL, "Time to make a scan..");

		viewer_manager_show(VIEWER_WINSET_SEARCHING_GRP_TITLE);
		viewer_manager_header_mode_set(HEADER_MODE_SEARCHING);

		scan_result = wlan_manager_scan();
		if (scan_result != WLAN_MANAGER_ERR_NONE) {
			viewer_manager_header_mode_set(current_state);
		}
	}

	__COMMON_FUNC_EXIT__;
}

void viewer_manager_ctxpopup_cleanup(void)
{
	if (manager_object->ctxpopup) {
		evas_object_del(manager_object->ctxpopup);
		manager_object->ctxpopup = NULL;
	}
}

void viewer_manager_hidden_confirm_cleanup(void)
{
	if (manager_object == NULL)
		return;

	hidden_ap_data_t *hidden_data = manager_object->hidden_popup_data;

	if (hidden_data != NULL) {
		if (hidden_data->confirm_popup != NULL) {
			evas_object_del(hidden_data->confirm_popup);
			hidden_data->confirm_popup = NULL;
		}

		if (hidden_data->device_info) {
			g_free(hidden_data->device_info);
		}
		g_free(hidden_data->ssid);
		g_free(hidden_data);

		manager_object->hidden_popup_data = NULL;
	}
}

void viewer_manager_cleanup_views(void)
{
	viewer_manager_ctxpopup_cleanup();

	viewer_manager_pop_naviframe_item();
	view_hidden_ap_popup_delete();
	viewer_manager_hidden_confirm_cleanup();
}

void viewer_manager_rotate_top_setupwizard_layout(void)
{
	Evas_Object *layout = NULL;
	struct ug_data *ugd;
	int change_ang = 0;
	Elm_Object_Item *navi_it = NULL;

	navi_it = elm_naviframe_top_item_get(manager_object->nav);
	layout = elm_object_item_content_get(navi_it);

	ugd = (struct ug_data *)ug_app_state->gadget;
	change_ang = elm_win_rotation_get(ugd->win_main);
	if (change_ang == 0 || change_ang == 180) {
		common_utils_contents_rotation_adjust(UG_EVENT_ROTATE_PORTRAIT);
		edje_object_signal_emit((Evas_Object *)elm_layout_edje_get(layout)
				,"location,vertical", "elm");
	} else {
		common_utils_contents_rotation_adjust(UG_EVENT_ROTATE_LANDSCAPE);
		edje_object_signal_emit((Evas_Object *)elm_layout_edje_get(layout),
				"location,horizontal", "elm");
	}
}
