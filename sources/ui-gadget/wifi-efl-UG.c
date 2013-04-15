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

#ifndef UG_MODULE_API
#define UG_MODULE_API __attribute__ ((visibility("default")))
#endif

#include <vconf-keys.h>

#include "ug_wifi.h"
#include "view_detail.h"
#include "i18nmanager.h"
#include "wlan_manager.h"
#include "winset_popup.h"
#include "common_utils.h"
#include "motion_control.h"
#include "viewer_manager.h"
#include "view_ime_hidden.h"
#include "wifi-engine-callback.h"

#define MAX_BSS_EXPIRY_TIME		600		/* time in seconds */

static int wifi_exit_end = FALSE;

wifi_appdata *ug_app_state = NULL;

struct ug_data {
	Evas_Object *base;
	ui_gadget_h ug;
};

UG_MODULE_API int UG_MODULE_INIT(struct ug_module_ops *ops);
UG_MODULE_API void UG_MODULE_EXIT(struct ug_module_ops *ops);
UG_MODULE_API int setting_plugin_reset(bundle *data, void *priv);

static gboolean __wifi_efl_ug_del_found_ap_noti(void *data)
{
	common_utils_send_message_to_net_popup(NULL, NULL,
										"del_found_ap_noti", NULL);

	return FALSE;
}

static void __make_scan_if_bss_expired(void)
{
	__COMMON_FUNC_ENTER__;

	time_t time_since_last_scan = 0;

	/* Trigger a scan, if last scan was more than 600 secs */
	time_since_last_scan = time(NULL) - wlan_manager_get_last_scan_time();
	INFO_LOG(SP_NAME_NORMAL, "time since last scan = %d secs", time_since_last_scan);
	if (time_since_last_scan >= MAX_BSS_EXPIRY_TIME) {
		HEADER_MODES current_state;
		int scan_result;
		current_state = viewer_manager_header_mode_get();

		switch (current_state) {
		case HEADER_MODE_DEACTIVATING:
		case HEADER_MODE_OFF:
			break;

		case HEADER_MODE_ON:
		case HEADER_MODE_CONNECTED:
			INFO_LOG(SP_NAME_NORMAL, "Time to make a scan..");

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

	__COMMON_FUNC_EXIT__;
}

static void *on_create(ui_gadget_h ug, enum ug_mode mode,
		service_h service, void *priv)
{
	__COMMON_FUNC_ENTER__;

	if (!ug || !priv) {
		INFO_LOG(UG_NAME_ERR, "UG and PRIV should not be NULL");

		__COMMON_FUNC_EXIT__;
		return NULL;
	}

	ug_app_state = g_new0(wifi_appdata, 1);
	retvm_if(NULL == ug_app_state, NULL);

	struct ug_data *ugd;
	ugd = (struct ug_data*)priv;
	ugd->ug = ug;

	if (NULL != service) {
		INFO_LOG(UG_NAME_NORMAL, "message load from caller");

		char *caller = NULL;
		if (service_get_extra_data(service, UG_CALLER, &caller)) {
			ERROR_LOG(UG_NAME_NORMAL, "Fail to get service extra data");

			__COMMON_FUNC_EXIT__;
			return NULL;
		}

		if (caller != NULL) {
			INFO_LOG(UG_NAME_NORMAL, "caller: %s", caller);

			if (strcmp(caller, "pwlock") == 0) {
				ug_app_state->ug_type = UG_VIEW_SETUP_WIZARD;
				service_get_extra_data(service, "lbutton",
								&ug_app_state->lbutton_setup_wizard_prev);
				service_get_extra_data(service, "rbutton_skip",
								&ug_app_state->rbutton_setup_wizard_skip);
				service_get_extra_data(service, "rbutton_next",
						&ug_app_state->rbutton_setup_wizard_next);
			} else
				ug_app_state->ug_type = UG_VIEW_DEFAULT;

			free(caller);
		} else {
			INFO_LOG(UG_NAME_NORMAL, "caller is not defined");
			ug_app_state->ug_type = UG_VIEW_DEFAULT;
		}
	} else {
		INFO_LOG(UG_NAME_NORMAL, "caller is not defined");
		ug_app_state->ug_type = UG_VIEW_DEFAULT;
	}

	bindtextdomain(PACKAGE, LOCALEDIR);

	Evas_Object *parent_layout = ug_get_parent_layout(ug);
	if (parent_layout == NULL) {
		ERROR_LOG(UG_NAME_NORMAL, "Failed to get parent layout");

		__COMMON_FUNC_EXIT__;
		return NULL;
	}

	ug_app_state->gadget= ugd;
	ug_app_state->ug = ug;

	common_util_set_system_registry(VCONFKEY_WIFI_UG_RUN_STATE,
								VCONFKEY_WIFI_UG_RUN_STATE_ON_FOREGROUND);

	/* Remove the "WiFi networks found" from the notification tray.*/
	common_util_managed_idle_add(__wifi_efl_ug_del_found_ap_noti, NULL);

	memset(&g_pending_call, 0, sizeof(wifi_pending_call_info_t));

	Evas_Object *layout_main = viewer_manager_create(parent_layout);
	if (layout_main == NULL) {
		INFO_LOG(UG_NAME_ERR, "Failed to create viewer_manager");

		__COMMON_FUNC_EXIT__;
		return NULL;
	}

	ug_app_state->popup_manager = winset_popup_manager_create(layout_main, PACKAGE);

	ugd->base = layout_main;
	ug_app_state->layout_main = layout_main;
	ug_app_state->bAlive = EINA_TRUE;

	wlan_manager_create();
	wlan_manager_set_message_callback(wlan_engine_callback);
	wlan_manager_set_refresh_callback(wlan_engine_refresh_callback);

	switch (wlan_manager_start()) {
	case WLAN_MANAGER_ERR_NONE:
		break;

	case WLAN_MANAGER_ERR_ALREADY_REGISTERED:
		break;

	default:
		ERROR_LOG(UG_NAME_NORMAL, "Cannot start wlan_manager");

		__COMMON_FUNC_EXIT__;
		return ugd->base;
	}

	switch (wlan_manager_state_get()) {
	case WLAN_MANAGER_OFF:
		viewer_manager_header_mode_set(HEADER_MODE_OFF);
		viewer_manager_hide(VIEWER_WINSET_SUB_CONTENTS);
		break;

	case WLAN_MANAGER_CONNECTING:
	case WLAN_MANAGER_UNCONNECTED:
	case WLAN_MANAGER_CONNECTED:
		viewer_manager_header_mode_set(HEADER_MODE_SEARCHING);
		viewer_manager_hide(VIEWER_WINSET_SEARCHING);
		viewer_manager_show(VIEWER_WINSET_SUB_CONTENTS);
		break;

	case WLAN_MANAGER_ERROR:
	default:
		return ugd->base;
	}

	evas_object_show(layout_main);

	motion_create(layout_main);

	__COMMON_FUNC_EXIT__;
	return ugd->base;
}

static gboolean load_initial_ap_list(gpointer data)
{
	__COMMON_FUNC_ENTER__;

	/* Because of transition effect performance,
	 * Wi-Fi lists might be better to be updated at maximum delayed
	 */
	wlan_manager_scanned_profile_refresh();

	__COMMON_FUNC_EXIT__;
	return FALSE;
}

static void on_start(ui_gadget_h ug, service_h service, void *priv)
{
	__COMMON_FUNC_ENTER__;

	common_util_managed_idle_add(load_initial_ap_list, NULL);

	connman_request_scan_mode_set(WIFI_BGSCAN_MODE_PERIODIC);

	motion_start();

	__COMMON_FUNC_EXIT__;
}

static void on_pause(ui_gadget_h ug, service_h service, void *priv)
{
	__COMMON_FUNC_ENTER__;

	motion_stop();

	connman_request_scan_mode_set(WIFI_BGSCAN_MODE_EXPONENTIAL);

	common_util_set_system_registry(VCONFKEY_WIFI_UG_RUN_STATE,
								VCONFKEY_WIFI_UG_RUN_STATE_ON_BACKGROUND);

	__COMMON_FUNC_EXIT__;
}

static void on_resume(ui_gadget_h ug, service_h service, void *priv)
{
	__COMMON_FUNC_ENTER__;

	motion_start();

	connman_request_scan_mode_set(WIFI_BGSCAN_MODE_PERIODIC);

	common_util_set_system_registry(VCONFKEY_WIFI_UG_RUN_STATE,
								VCONFKEY_WIFI_UG_RUN_STATE_ON_FOREGROUND);

	__make_scan_if_bss_expired();

	__COMMON_FUNC_EXIT__;
}

static void on_destroy(ui_gadget_h ug, service_h service, void *priv)
{
	__COMMON_FUNC_ENTER__;

	int ret;
	common_util_set_system_registry(VCONFKEY_WIFI_UG_RUN_STATE,
									VCONFKEY_WIFI_UG_RUN_STATE_OFF);

	if (!ug || !priv){
		__COMMON_FUNC_EXIT__;
		return;
	}

	motion_destroy();

	connman_request_scan_mode_set(WIFI_BGSCAN_MODE_EXPONENTIAL);

	DEBUG_LOG(UG_NAME_NORMAL, "* popup manager destroying...");
	winset_popup_manager_destroy(ug_app_state->popup_manager);
	ug_app_state->popup_manager = NULL;
	DEBUG_LOG(UG_NAME_NORMAL, "* view_main destroying...");
	viewer_manager_destroy();
	DEBUG_LOG(UG_NAME_NORMAL, "* manager destroy complete");
	DEBUG_LOG(UG_NAME_NORMAL, "* wlan manager destroying...");

	ret = wlan_manager_destroy();
	if (ret != WLAN_MANAGER_ERR_NONE)
		ERROR_LOG(UG_NAME_NORMAL, "Failed to destroy : %d",ret);

	if (g_pending_call.is_handled == FALSE) {
		dbus_g_proxy_cancel_call(g_pending_call.proxy, g_pending_call.pending_call);
		g_pending_call.is_handled = TRUE;

		memset(&g_pending_call, 0, sizeof(wifi_pending_call_info_t));

		DEBUG_LOG(UG_NAME_NORMAL, "* pending dbus call cleared");
	}

	struct ug_data* ugd = priv;
	if (ugd->base){
		evas_object_del(ugd->base);
		ugd->base = NULL;
	}

	common_util_managed_idle_cleanup();

	__COMMON_FUNC_EXIT__;
}

static void on_message(ui_gadget_h ug, service_h msg, service_h service, void *priv)
{
}

static void on_event(ui_gadget_h ug, enum ug_event event, service_h service, void *priv)
{
	__COMMON_FUNC_ENTER__;

	switch (event) {
	case UG_EVENT_LOW_MEMORY:
		break;
	case UG_EVENT_LOW_BATTERY:
		break;
	case UG_EVENT_LANG_CHANGE:
		break;
	case UG_EVENT_ROTATE_PORTRAIT:
		break;
	case UG_EVENT_ROTATE_PORTRAIT_UPSIDEDOWN:
		break;
	case UG_EVENT_ROTATE_LANDSCAPE:
		break;
	case UG_EVENT_ROTATE_LANDSCAPE_UPSIDEDOWN:
		break;
	default:
		break;
	}

	__COMMON_FUNC_EXIT__;
}

static void on_key_event(ui_gadget_h ug, enum ug_key_event event, service_h service, void *priv)
{
	__COMMON_FUNC_ENTER__;

	if (!ug) {
		__COMMON_FUNC_EXIT__;
		return;
	}

	switch (event) {
	case UG_KEY_EVENT_END:
		INFO_LOG(UG_NAME_NORMAL, "UG_KEY_EVENT_END");

		/* popup key event determine */
		winset_popup_hide_popup(ug_app_state->popup_manager);

		Evas_Object* navi_frame = viewer_manager_get_naviframe();
		view_manager_view_type_t top_view_id = (view_manager_view_type_t)evas_object_data_get(navi_frame, SCREEN_TYPE_ID_KEY);
		if(VIEW_MANAGER_VIEW_TYPE_MAIN == top_view_id)
			INFO_LOG(UG_NAME_NORMAL, "same");
		else {
			INFO_LOG(UG_NAME_NORMAL, "differ");
			elm_naviframe_item_pop(viewer_manager_get_naviframe());

			__COMMON_FUNC_EXIT__;
			return;
		}

		wifi_exit();
		break;
	default:
		INFO_LOG(UG_NAME_NORMAL, "UG_KEY_EVENT [%d]", event);
		break;
	}

	__COMMON_FUNC_EXIT__;
}

UG_MODULE_API int UG_MODULE_INIT(struct ug_module_ops *ops)
{
	__COMMON_FUNC_ENTER__;

	wifi_exit_end = FALSE;

	retvm_if(NULL == ops, 0);

	struct ug_data *ugd;
	ugd = calloc(1, sizeof(struct ug_data));

	assertm_if(NULL == ugd, "Err!! calloc fail");

	ops->create = on_create;
	ops->start = on_start;
	ops->pause = on_pause;
	ops->resume = on_resume;
	ops->destroy = on_destroy;
	ops->message = on_message;
	ops->event = on_event;
	ops->key_event = on_key_event;
	ops->priv = ugd;
	ops->opt = UG_OPT_INDICATOR_ENABLE;

	__COMMON_FUNC_EXIT__;
	return 0;
}

UG_MODULE_API void UG_MODULE_EXIT(struct ug_module_ops *ops)
{
	__COMMON_FUNC_ENTER__;

	retm_if(NULL == ops);

	struct ug_data *ugd;

	ugd = ops->priv;

	if (ugd)
		free(ugd);

	__COMMON_FUNC_EXIT__;
}

static bool setting_plugin_wifi_found_ap_cb(wifi_ap_h ap, void* user_data)
{
	bool favorite = false;

	wifi_ap_is_favorite(ap, &favorite);

	if (true == favorite)
		wlan_manager_forget(ap);

	return true;
}

UG_MODULE_API int setting_plugin_reset(bundle *data, void *priv)
{
	__COMMON_FUNC_ENTER__;

	int return_value = 0;
	bool activated = false;

	return_value = wlan_manager_start();
	if (return_value != WLAN_MANAGER_ERR_NONE) {
		ERROR_LOG(UG_NAME_NORMAL, "Failed to register : %d",return_value);
		return_value = -1;
		goto error;
	}

	wifi_foreach_found_aps(setting_plugin_wifi_found_ap_cb, NULL);

	return_value = wifi_is_activated(&activated);
	if (WIFI_ERROR_NONE == return_value)
		INFO_LOG(UG_NAME_NORMAL, "Wi-Fi activated: %d", activated);
	else {
		ERROR_LOG(UG_NAME_NORMAL, "Failed to check state : %d",return_value);
		return_value = -1;
		goto error;
	}

	if (activated != 0) {
		return_value = wlan_manager_power_off();
		if (return_value != WLAN_MANAGER_ERR_NONE) {
			ERROR_LOG(UG_NAME_NORMAL, "Failed to power_off: %d",return_value);
			return_value = -1;
			goto error;
		}
	}

	common_util_set_system_registry(VCONFKEY_WIFI_ENABLE_QS,
									VCONFKEY_WIFI_QS_ENABLE);

error:
	wlan_manager_destroy();

	__COMMON_FUNC_EXIT__;
	return return_value;
}

int wifi_exit(void)
{
	__COMMON_FUNC_ENTER__;
	if(wifi_exit_end == TRUE) {
		__COMMON_FUNC_EXIT__;
		return FALSE;
	}
	wifi_exit_end = TRUE;

	struct ug_data *ugd;
	ugd = ug_app_state->gadget;
	ug_app_state->bAlive = EINA_FALSE;

	DEBUG_LOG(UG_NAME_NORMAL, "* ug_destroying...");
	ug_destroy_me(ugd->ug);

	__COMMON_FUNC_EXIT__;

	return TRUE;
}
