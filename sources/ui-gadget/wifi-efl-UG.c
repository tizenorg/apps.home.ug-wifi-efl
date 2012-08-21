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

#ifndef UG_MODULE_API
#define UG_MODULE_API __attribute__ ((visibility("default")))
#endif


#include "wifi.h"
#include "wlan_manager.h"
#include "winset_popup.h"
#include "viewer_manager.h"
#include "wifi-engine-callback.h"
#include "i18nmanager.h"
#include "wifi-setting.h"
#include "view_detail.h"
#include "view_ime_hidden.h"
#include "motion_control.h"

static int wifi_exit_end = FALSE;

wifi_appdata *ug_app_state = NULL;

struct ug_data 
{
	Evas_Object *base;
	ui_gadget_h ug;
};

static void *on_create(ui_gadget_h ug, enum ug_mode mode, service_h service, void *priv)
{
	__COMMON_FUNC_ENTER__;

	if (!ug || !priv) {
		INFO_LOG(UG_NAME_ERR, "[Error]Data is NULL - ug or priv");
		return NULL;
	}

	ug_app_state = (wifi_appdata *) g_malloc0(sizeof(wifi_appdata));
	assertm_if(NULL == ug_app_state, "Err!! ug_app_state == NULL");
	memset(ug_app_state, 0x0, sizeof(wifi_appdata));

	struct ug_data *ugd;
	ugd = (struct ug_data*)priv;
	ugd->ug = ug;

	if (NULL != service) {
		INFO_LOG(UG_NAME_NORMAL, "message load from caller");

		char *caller = NULL;
		if (service_get_extra_data(service, UG_CALLER, &caller)) {
			ERROR_LOG(UG_NAME_NORMAL, "Failed to get service extra data");
			return NULL;
		}

		if (caller != NULL) {
			INFO_LOG(UG_NAME_NORMAL, "caller = [%s]", caller);
			if (strcmp(caller, "pwlock") == 0) {
				ug_app_state->ug_type = UG_VIEW_SETUP_WIZARD;
				service_get_extra_data(service, "lbutton", &ug_app_state->lbutton_setup_wizard);
				service_get_extra_data(service, "rbutton_skip", &ug_app_state->rbutton_setup_wizard_skip);
				service_get_extra_data(service, "rbutton_next", &ug_app_state->rbutton_setup_wizard_next);
				service_get_extra_data(service, "lbutton_icon", &ug_app_state->lbutton_setup_wizard_prev_icon);
				service_get_extra_data(service, "rbutton_skip_icon", &ug_app_state->rbutton_setup_wizard_skip_icon);
				service_get_extra_data(service, "scan_icon", &ug_app_state->rbutton_setup_wizard_scan_icon);
				service_get_extra_data(service, "rbutton_next_icon", &ug_app_state->rbutton_setup_wizard_next_icon);
			} else {
				ug_app_state->ug_type = UG_VIEW_DEFAULT;
			}

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

	ug_app_state->win_main = ug_get_parent_layout(ug);
	assertm_if( NULL == ug_app_state->win_main, "Err!! win_main == NULL");

	elm_win_conformant_set(ug_app_state->win_main, TRUE);
	ug_app_state->gadget= ugd;
	ug_app_state->ug = ug;

	if (wifi_setting_value_set(VCONFKEY_WIFI_UG_RUN_STATE, VCONFKEY_WIFI_UG_RUN_STATE_ON_FOREGROUND) < 0) {
		INFO_LOG(UG_NAME_ERR, "[Error]Failed to set vconf - VCONFKEY_WIFI_UG_RUN_STATE_ON_FOREGROUND");
	}

	evas_object_show(ug_app_state->win_main);
	elm_win_indicator_mode_set(ug_app_state->win_main, ELM_WIN_INDICATOR_SHOW);

	memset(&g_pending_call, 0, sizeof(wifi_pending_call_info_t));

	Evas_Object* base = viewer_manager_create(ug_app_state->win_main);
	assertm_if(NULL == base, "Err!! main_layout == NULL");
	ugd->base = base;

	ug_app_state->popup_manager = winset_popup_manager_create(ug_app_state->win_main, PACKAGE);

	ug_app_state->bAlive = EINA_TRUE;

	wlan_manager_create();
	wlan_manager_set_message_callback(wlan_engine_callback);
	wlan_manager_set_refresh_callback(wlan_engine_refresh_callback);

	switch (wlan_manager_start()) {
	case WLAN_MANAGER_ERR_NONE:
		break;
	case WLAN_MANAGER_ERR_ALREADY_REGISTERED:
		winset_popup_mode_set(ug_app_state->popup_manager, POPUP_OPTION_REGISTER_FAILED_UNKNOWN, NULL);
		return ugd->base;
	case WLAN_MANAGER_ERR_UNKNOWN:
		winset_popup_mode_set(ug_app_state->popup_manager, POPUP_OPTION_REGISTER_FAILED_COMMUNICATION_FAILED, NULL);
		return ugd->base;
	default:
		__COMMON_FUNC_EXIT__;
		return ugd->base;
	}

	char profile_name[NET_PROFILE_NAME_LEN_MAX+1] = "";
	switch (wlan_manager_state_get(profile_name)) {
	case WLAN_MANAGER_OFF:
		ERROR_LOG(UG_NAME_NORMAL, "current state is wifi-off\n");
		viewer_manager_header_mode_set(HEADER_MODE_OFF);
		viewer_manager_show(VIEWER_WINSET_SEARCHING);
		viewer_manager_hide(VIEWER_WINSET_SUB_CONTENTS);
		break;
	case WLAN_MANAGER_CONNECTING:
	case WLAN_MANAGER_UNCONNECTED:
	case WLAN_MANAGER_CONNECTED:
		connman_request_scan_mode_set(WIFI_BGSCAN_MODE_PERIODIC);
		viewer_manager_header_mode_set(HEADER_MODE_SEARCHING);
		viewer_manager_hide(VIEWER_WINSET_SEARCHING);
		viewer_manager_show(VIEWER_WINSET_SUB_CONTENTS);
		break;
	case WLAN_MANAGER_ERROR:
	default:
		winset_popup_mode_set(ug_app_state->popup_manager, POPUP_OPTION_ETC_WLAN_STATE_GET_ERROR, NULL);
		break;
	}

	motion_create(ug_app_state->win_main);

	__COMMON_FUNC_EXIT__;
	return ugd->base;
}

static Eina_Bool load_initial_ap_list(void *data)
{
	wlan_manager_scanned_profile_refresh_with_count((int)data);
	return ECORE_CALLBACK_CANCEL;
}

static void on_start(ui_gadget_h ug, service_h service, void *priv)
{
	__COMMON_FUNC_ENTER__;
	ecore_idler_add(load_initial_ap_list, (void *)8);

	motion_start();

	__COMMON_FUNC_EXIT__;
	return;
}

static void on_pause(ui_gadget_h ug, service_h service, void *priv)
{
	__COMMON_FUNC_ENTER__;
	motion_stop();

	if (connman_request_scan_mode_set(WIFI_BGSCAN_MODE_EXPONENTIAL) == WLAN_MANAGER_ERR_NONE) {
		DEBUG_LOG(UG_NAME_REQ, "Set BG scan mode - EXPONENTIAL");
	}

	if (wifi_setting_value_set(VCONFKEY_WIFI_UG_RUN_STATE, VCONFKEY_WIFI_UG_RUN_STATE_ON_BACKGROUND) < 0) {
		INFO_LOG(UG_NAME_NORMAL, "Failed to set vconf - VCONFKEY_WIFI_UG_RUN_STATE_ON_BACKGROUND");
	}
	__COMMON_FUNC_EXIT__;
}

static void on_resume(ui_gadget_h ug, service_h service, void *priv)
{
	__COMMON_FUNC_ENTER__;
	motion_start();

	if (connman_request_scan_mode_set(WIFI_BGSCAN_MODE_PERIODIC) == WLAN_MANAGER_ERR_NONE) {
		DEBUG_LOG(UG_NAME_REQ, "Set BG scan mode - PERIODIC");
	}

	if (wifi_setting_value_set(VCONFKEY_WIFI_UG_RUN_STATE, VCONFKEY_WIFI_UG_RUN_STATE_ON_FOREGROUND) < 0) {
		INFO_LOG(UG_NAME_NORMAL, "Failed to set vconf - VCONFKEY_WIFI_UG_RUN_STATE_ON_FOREGROUND");
	}
	__COMMON_FUNC_EXIT__;
}

static void on_destroy(ui_gadget_h ug, service_h service, void *priv)
{
	__COMMON_FUNC_ENTER__;
	if (wifi_setting_value_set(VCONFKEY_WIFI_UG_RUN_STATE, VCONFKEY_WIFI_UG_RUN_STATE_OFF) < 0) {
		INFO_LOG(UG_NAME_NORMAL, "Failed to set vconf - VCONFKEY_WIFI_UG_RUN_STATE_OFF");
	}
	if (!ug || !priv){
		__COMMON_FUNC_EXIT__;
		return;
	}

	motion_destroy();

	wifi_exit();
	struct ug_data* ugd = priv;
	if(ugd->base){
		evas_object_del(ugd->base);
		ugd->base = NULL;
	}
}

static void on_message(ui_gadget_h ug, service_h msg, service_h service, void *priv)
{
}

static void on_event(ui_gadget_h ug, enum ug_event event, service_h service, void *priv)
{
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
}

static void on_key_event(ui_gadget_h ug, enum ug_key_event event, service_h service, void *priv)
{
	__COMMON_FUNC_ENTER__;

	if (!ug) {
		return;
	}

	switch (event) {
		case UG_KEY_EVENT_END:
			INFO_LOG(UG_NAME_NORMAL, "UG_KEY_EVENT_END");

			/* popup key event determine */
			winset_popup_hide_popup(ug_app_state->popup_manager);

			Evas_Object* navi_frame = viewer_manager_get_naviframe();
			view_manager_view_type_t top_view_id = (view_manager_view_type_t)evas_object_data_get(navi_frame, SCREEN_TYPE_ID_KEY);
			if(VIEW_MANAGER_VIEW_TYPE_MAIN == top_view_id) {
				INFO_LOG(UG_NAME_NORMAL, "same");
			} else {
				INFO_LOG(UG_NAME_NORMAL, "differ");
				elm_naviframe_item_pop(viewer_manager_get_naviframe());
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

	assertm_if(NULL == ops, "Err!! ug_module_ops == NULL");

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
	assertm_if(NULL == ops, "Err!! ug_module_ops == NULL");

	struct ug_data *ugd;

	ugd = ops->priv;

	if (ugd)
		free(ugd);
	__COMMON_FUNC_EXIT__;
}

UG_MODULE_API int setting_plugin_reset(bundle *data, void *priv)
{
	__COMMON_FUNC_ENTER__;
	INFO_LOG(UG_NAME_NORMAL, "setting_plugin_reset");
	return 0;
	__COMMON_FUNC_EXIT__;
}

int wifi_exit()
{
	__COMMON_FUNC_ENTER__;
	if(wifi_exit_end == TRUE) {
		__COMMON_FUNC_EXIT__;
		return FALSE;
	}
	wifi_exit_end = TRUE;

	if (connman_request_scan_mode_set(WIFI_BGSCAN_MODE_EXPONENTIAL) == WLAN_MANAGER_ERR_NONE) {
		DEBUG_LOG(UG_NAME_REQ, "Set BG scan mode - EXPONENTIAL");
	}

	DEBUG_LOG(UG_NAME_NORMAL, "* popup manager destroying...");
	winset_popup_manager_destroy(ug_app_state->popup_manager);
	ug_app_state->popup_manager = NULL;
	DEBUG_LOG(UG_NAME_NORMAL, "* wlan manager destroying...");
	wlan_manager_destroy();
	DEBUG_LOG(UG_NAME_NORMAL, "* view_main destroying...");
	viewer_manager_destroy();
	DEBUG_LOG(UG_NAME_NORMAL, "* manager destroy complete");

	struct ug_data *ugd;
	ugd = ug_app_state->gadget;
	ug_app_state->bAlive = EINA_FALSE;

	if(g_pending_call.is_handled == FALSE)
	{
		dbus_g_proxy_cancel_call(g_pending_call.proxy, g_pending_call.pending_call);
		g_pending_call.is_handled = TRUE;
		memset(&g_pending_call, 0, sizeof(wifi_pending_call_info_t));
	}

	DEBUG_LOG(UG_NAME_NORMAL, "* ug_destroying...");
	ug_destroy_me(ugd->ug);

	__COMMON_FUNC_EXIT__;

	return TRUE;
}
