/*
  * Copyright 2012  Samsung Electronics Co., Ltd
  *
  * Licensed under the Flora License, Version 1.0 (the "License");
  * you may not use this file except in compliance with the License.
  * You may obtain a copy of the License at
  *
  *    http://www.tizenopensource.org/license
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  */



#ifndef UG_MODULE_API
#define UG_MODULE_API __attribute__ ((visibility("default")))
#endif


#include <ui-gadget-module.h>
#include "wifi.h"
#include "wlan_manager.h"
#include "popup.h"
#include "viewer_manager.h"
#include "wifi-engine-callback.h"
#include "i18nmanager.h"
#include "wifi-setting.h"
#include "view_detail.h"
#include "view_dhcpip.h"
#include "view_staticip.h"
#include "view_ime_password.h"
#include "view_ime_hidden.h"


static int wifi_exit_end = FALSE;

struct wifi_appdata *app_state = NULL;

struct ug_data 
{
	Evas_Object *base;
	struct ui_gadget *ug;
};


static void *on_create(struct ui_gadget *ug, enum ug_mode mode, bundle *data, void *priv)
{
	__COMMON_FUNC_ENTER__;

	if (!ug || !priv) {
		INFO_LOG(UG_NAME_ERR, "[Error]Data is NULL - ug or priv");
		return NULL;
	}

	app_state = (struct wifi_appdata *) g_malloc0(sizeof(struct wifi_appdata));
	assertm_if(NULL == app_state, "Err!! app_state == NULL");
	memset(app_state, 0x0, sizeof(struct wifi_appdata));

	struct ug_data *ugd;
	ugd = (struct ug_data*)priv;
	ugd->ug = ug;

	if (NULL != data) {
		INFO_LOG(UG_NAME_NORMAL, "message load from caller");

		const char* force_connected = (const char*) bundle_get_val(data,"back-button-show-force-when-connected");
		INFO_LOG(UG_NAME_NORMAL, "* bundle:back-button-show-force-when-connected [%s]", force_connected);

		if (force_connected != NULL) {
			if(strcmp(force_connected, "TRUE")==0) {
				app_state->bundle_back_button_show_force_when_connected = EINA_TRUE;
			}
		} else {
			app_state->bundle_back_button_show_force_when_connected = EINA_FALSE;
		}
	} else {
		INFO_LOG(UG_NAME_NORMAL, "I don`t know about my caller, I will just call him < Setting");
		app_state->bundle_back_button_show_force_when_connected = EINA_FALSE;
	}

	INFO_LOG(UG_NAME_NORMAL, "bundle process end");
	INFO_LOG(UG_NAME_NORMAL, "back button show when connected force set [%d]", app_state->bundle_back_button_show_force_when_connected);

	bindtextdomain(PACKAGE, LOCALEDIR);

	app_state->win_main = ug_get_parent_layout(ug);
	assertm_if( NULL == app_state->win_main, "Err!! win_main == NULL");

	elm_win_conformant_set(app_state->win_main, TRUE);
	app_state->gadget= ugd;
	app_state->ug = ug;

	if (wifi_setting_value_set(VCONFKEY_WIFI_UG_RUN_STATE, VCONFKEY_WIFI_UG_RUN_STATE_ON_FOREGROUND) < 0) {
		INFO_LOG(UG_NAME_ERR, "[Error]Failed to set vconf - VCONFKEY_WIFI_UG_RUN_STATE_ON_FOREGROUND");
	}

	evas_object_show(app_state->win_main);
	elm_win_indicator_mode_set(app_state->win_main, ELM_WIN_INDICATOR_SHOW);

	memset(&g_pending_call, 0, sizeof(wifi_pending_call_info_t));

	Evas_Object* base = viewer_manager_create(app_state->win_main);
	assertm_if(NULL == base, "Err!! main_layout == NULL");
	ugd->base = base;

	winset_popup_create(app_state->win_main);

	app_state->bAlive = EINA_TRUE;
	app_state->current_view = VIEW_MAIN;

	wlan_manager_create();
	wlan_manager_set_message_callback(wlan_engine_callback);
	wlan_manager_set_refresh_callback(wlan_engine_refresh_callback);

	switch (wlan_manager_start()) {
	case WLAN_MANAGER_ERR_NONE:
		break;
	case WLAN_MANAGER_ERR_ALREADY_REGISTERED:
		winset_popup_mode_set(NULL, POPUP_MODE_REGISTER_FAILED, POPUP_OPTION_NONE);
		return ugd->base;
	case WLAN_MANAGER_ERR_UNKNOWN:
		winset_popup_mode_set(NULL, POPUP_MODE_REGISTER_FAILED, POPUP_OPTION_REGISTER_FAILED_COMMUNICATION_FAILED);
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
		viewer_manager_hide(VIEWER_WINSET_SUB_CONTENTS);
		break;
	case WLAN_MANAGER_CONNECTING:
		ERROR_LOG(UG_NAME_NORMAL, "current state is wifi-connecting\n");
	case WLAN_MANAGER_UNCONNECTED:
		ERROR_LOG(UG_NAME_NORMAL, "current state is wifi-on\n");
		viewer_manager_header_mode_set(HEADER_MODE_SEARCHING);
		viewer_manager_hide(VIEWER_WINSET_SEARCHING);
		viewer_manager_show(VIEWER_WINSET_SUB_CONTENTS);
		if (connman_request_scan_mode_set(WIFI_BGSCAN_MODE_PERIODIC) == WLAN_MANAGER_ERR_NONE) {
			DEBUG_LOG(UG_NAME_REQ, "Set BG scan mode - PERIODIC");
		}
		break;
	case WLAN_MANAGER_CONNECTED:
		ERROR_LOG(UG_NAME_NORMAL, "current state is wifi-connected\n");
		viewer_manager_header_mode_set(HEADER_MODE_SEARCHING);
		viewer_manager_hide(VIEWER_WINSET_SEARCHING);
		viewer_manager_show(VIEWER_WINSET_SUB_CONTENTS);
		if (connman_request_scan_mode_set(WIFI_BGSCAN_MODE_PERIODIC) == WLAN_MANAGER_ERR_NONE) {
			DEBUG_LOG(UG_NAME_REQ, "Set BG scan mode - PERIODIC");
		}
		break;
	case WLAN_MANAGER_ERROR:
	default:
		winset_popup_mode_set(NULL, POPUP_MODE_ETC, POPUP_OPTION_ETC_WLAN_STATE_GET_ERROR);
		break;
	}

	__COMMON_FUNC_EXIT__;
	return ugd->base;
}

static void on_start(struct ui_gadget *ug, bundle *data, void *priv)
{
	__COMMON_FUNC_ENTER__;
	if(app_state->bAlive)
		ecore_idler_add((Ecore_Task_Cb)wlan_manager_scanned_profile_refresh_with_count, (void *)8);
	__COMMON_FUNC_EXIT__;
}

static void on_pause(struct ui_gadget *ug, bundle *data, void *priv)
{
	__COMMON_FUNC_ENTER__;
	if (wifi_setting_value_set(VCONFKEY_WIFI_UG_RUN_STATE, VCONFKEY_WIFI_UG_RUN_STATE_ON_BACKGROUND) < 0) {
		INFO_LOG(UG_NAME_NORMAL, "Failed to set vconf - VCONFKEY_WIFI_UG_RUN_STATE_ON_BACKGROUND");
	}
	__COMMON_FUNC_EXIT__;
}

static void on_resume(struct ui_gadget *ug, bundle *data, void *priv)
{
	__COMMON_FUNC_ENTER__;
	if (wifi_setting_value_set(VCONFKEY_WIFI_UG_RUN_STATE, VCONFKEY_WIFI_UG_RUN_STATE_ON_FOREGROUND) < 0) {
		INFO_LOG(UG_NAME_NORMAL, "Failed to set vconf - VCONFKEY_WIFI_UG_RUN_STATE_ON_FOREGROUND");
	}
	__COMMON_FUNC_EXIT__;
}

static void on_destroy(struct ui_gadget *ug, bundle *data, void *priv)
{
	__COMMON_FUNC_ENTER__;

	if (wifi_setting_value_set(VCONFKEY_WIFI_UG_RUN_STATE, VCONFKEY_WIFI_UG_RUN_STATE_OFF) < 0) {
		INFO_LOG(UG_NAME_NORMAL, "Failed to set vconf - VCONFKEY_WIFI_UG_RUN_STATE_OFF");
	}

	if (!ug || !priv){
		__COMMON_FUNC_EXIT__;
		return;
	}

	wifi_exit();

	struct ug_data* ugd = priv;

	if(ugd->base){
		evas_object_del(ugd->base);
		ugd->base = NULL;
	}

	__COMMON_FUNC_EXIT__;
}

static void on_message(struct ui_gadget *ug, bundle *msg, bundle *data, void *priv)
{
}

static void on_event(struct ui_gadget *ug, enum ug_event event, bundle *data, void *priv)
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

static void on_key_event(struct ui_gadget *ug, enum ug_key_event event, bundle *data, void *priv)
{
	__COMMON_FUNC_ENTER__;

	if (!ug) {
		return;
	}

	switch (event) {
		case UG_KEY_EVENT_END:
			INFO_LOG(UG_NAME_NORMAL, "UG_KEY_EVENT_END");

			/* popup key event determine */
			Evas_Object* det = winset_popup_content_get(NULL);
		
			if(det == NULL) {
				INFO_LOG(UG_NAME_NORMAL, "No POPUP");
			} else {
				INFO_LOG(UG_NAME_NORMAL, "POPUP Removed");
				winset_popup_timer_remove();
				winset_popup_content_clear();
				return;
			}
			
			if(app_state->current_view == VIEW_MAIN) {
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
	winset_popup_destroy(NULL);
	DEBUG_LOG(UG_NAME_NORMAL, "* wlan manager destroying...");
	wlan_manager_destroy();
	DEBUG_LOG(UG_NAME_NORMAL, "* view_main destroying...");
	viewer_manager_destroy();
	DEBUG_LOG(UG_NAME_NORMAL, "* manager destroy complete");

	struct ug_data *ugd;
	ugd = app_state->gadget;
	app_state->bAlive = EINA_FALSE;

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
