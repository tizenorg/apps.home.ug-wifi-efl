/*
*  Wi-Fi syspopup
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



#include <syspopup.h>
#include "common.h"
#include "common_utils.h"
#include "wlan_manager.h"
#include "appcoreWrapper.h"
#include "wifi-syspopup.h"
#include "wifi-syspopup-engine-callback.h"
#include "view-main.h"
#include "view-alerts.h"
#include "wifi-setting.h"
#include "i18nmanager.h"

#define DEVICE_PICKER_POPUP_W		656
#define DEVICE_PICKER_POPUP_H		918
#define MAX_INITIAL_QS_POPUP_LIST_SIZE	8

wifi_object* syspopup_app_state = NULL;

/* static */
static int myterm(bundle* b, void* data);
static int mytimeout(bundle *b, void* data);
static int wifi_syspopup_exit( void );
static void _exit_cb(void *data, Evas_Object *obj, void *event_info);
static int syspopup_support_set(const char* support);
static int app_reset(bundle *b, void *data);
static int app_init(void *data);
static int app_exit(void *data);
static int app_start(void *data);
static int app_stop(void *data);
#if 0	/* This function is not called from anywhere. Unused. */
static int _power_on (void);
#endif

/* implements */
static int myterm(bundle* b, void* data)
{
	__COMMON_FUNC_ENTER__;
	wifi_syspopup_exit();

	__COMMON_FUNC_EXIT__;
	return FALSE;
}

static int mytimeout(bundle *b, void* data)
{
	__COMMON_FUNC_ENTER__;
	__COMMON_FUNC_EXIT__;

	return FALSE;
}

syspopup_handler handler = {
	.def_term_fn = myterm,
	.def_timeout_fn = mytimeout
};

static int wifi_syspopup_exit(void)
{
	__COMMON_FUNC_ENTER__;
	view_main_destroy();
	wlan_manager_destroy();

	if (g_pending_call.is_handled) {
		dbus_g_proxy_cancel_call(g_pending_call.proxy, g_pending_call.pending_call);
		g_pending_call.is_handled = FALSE;
		memset(&g_pending_call, 0, sizeof(wifi_pending_call_info_t));
	}

	if (VCONFKEY_WIFI_QS_WIFI_CONNECTED == syspopup_app_state->connection_result) {
		INFO_LOG(SP_NAME_NORMAL, "Result : WIFI");
	} else if (VCONFKEY_WIFI_QS_3G == syspopup_app_state->connection_result) {
		INFO_LOG(SP_NAME_NORMAL, "Result : 3G");
	} else {
		WARN_LOG(SP_NAME_NORMAL, "Result : ?? [%d]", syspopup_app_state->connection_result);
		syspopup_app_state->connection_result = VCONFKEY_WIFI_QS_3G;
	}

	wifi_setting_value_set("memory/wifi/wifi_qs_exit", syspopup_app_state->connection_result);

	elm_exit();

	__COMMON_FUNC_EXIT__;

	return TRUE;
}

static void _exit_cb(void *data, Evas_Object *obj, void *event_info)
{
	__COMMON_FUNC_ENTER__;
	wifi_syspopup_exit();
	__COMMON_FUNC_EXIT__;
}

int wifi_syspopup_destroy(void)
{
	if (syspopup_app_state->passpopup) {
		common_pswd_popup_destroy(syspopup_app_state->passpopup);
		syspopup_app_state->passpopup = NULL;
	}

	if (syspopup_app_state->syspopup) {
		evas_object_del(syspopup_app_state->syspopup);
		syspopup_app_state->syspopup = NULL;
	}

	if (syspopup_app_state->win_main) {
		evas_object_del(syspopup_app_state->win_main);
		syspopup_app_state->win_main = NULL;
	}

	wifi_syspopup_exit();

	return TRUE;
}

int wifi_syspopup_create(void)
{
	__COMMON_FUNC_ENTER__;
	if (NULL == syspopup_app_state->syspopup) {
		syspopup_app_state->syspopup = elm_popup_add(syspopup_app_state->win_main);
		assertm_if(NULL == syspopup_app_state->syspopup, "syspopup is NULL!!");
	}
	elm_object_style_set(syspopup_app_state->syspopup,"content_expand");
	elm_object_part_text_set(syspopup_app_state->syspopup, "title,text", sc(PACKAGE, I18N_TYPE_WiFi_network));
	evas_object_size_hint_weight_set(syspopup_app_state->syspopup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

	Evas_Object *btn_cancel = elm_button_add(syspopup_app_state->syspopup);
	elm_object_text_set(btn_cancel, sc(PACKAGE, I18N_TYPE_Cancel));
	elm_object_part_content_set(syspopup_app_state->syspopup, "button1", btn_cancel);
	evas_object_smart_callback_add(btn_cancel, "clicked", _exit_cb, NULL);

	/* Create and add a box into the layout. */
	Evas_Object *box = elm_box_add(syspopup_app_state->syspopup);
	evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	Evas_Object *main_list = view_main_create(box);
	elm_box_pack_end(box, main_list);
	evas_object_show(main_list);
	evas_object_size_hint_min_set(box, DEVICE_PICKER_POPUP_W * elm_config_scale_get(), DEVICE_PICKER_POPUP_H * elm_config_scale_get());
	elm_object_content_set(syspopup_app_state->syspopup, box);
	evas_object_show(syspopup_app_state->syspopup);
	evas_object_show(syspopup_app_state->win_main);

	memset(&g_pending_call, 0, sizeof(wifi_pending_call_info_t));

	__COMMON_FUNC_EXIT__;

	return TRUE;
}

int wifi_syspopup_init()
{
	__COMMON_FUNC_ENTER__;

	wlan_manager_create();
	wlan_manager_set_message_callback(wlan_engine_callback);
	wlan_manager_set_refresh_callback(wlan_engine_refresh_callback);

	int wlan_ret = wlan_manager_start(NULL);
	switch (wlan_ret) {
	case WLAN_MANAGER_ERR_NONE:
		INFO_LOG(SP_NAME_NORMAL, "wlan_manager start complete" );
		break;
	case WLAN_MANAGER_ERR_ALREADY_REGISTERED:
		ERROR_LOG(SP_NAME_ERR, "fail already register." );
		break;
	case WLAN_MANAGER_ERR_UNKNOWN:
		ERROR_LOG(SP_NAME_ERR, "wlan fail communication." );
		break;
	default:
		ERROR_LOG(SP_NAME_ERR, "wlan_manager start fail ret[%d]", wlan_ret );
		break;
	}

	__COMMON_FUNC_EXIT__;
	return wlan_ret;
}

static int syspopup_support_set(const char* support) {
	__COMMON_FUNC_ENTER__;
	if(NULL == support) {
		__COMMON_FUNC_EXIT__;
		return FALSE;
	}

	if(strcmp("WIFI_SYSPOPUP_SUPPORT_QUICKPANEL",support) == 0) {
		syspopup_app_state->wifi_syspopup_support = WIFI_SYSPOPUP_SUPPORT_QUICKPANEL;
	} else {
		__COMMON_FUNC_EXIT__;
		return FALSE;
	}
	__COMMON_FUNC_EXIT__;

	return TRUE;
}

#if 0	/* This function is not called from anywhere. Unused. */
static void _mobilehotspot_disable_cb(DBusGProxy *proxy, DBusGProxyCall *call, gpointer user_data)
{
	__COMMON_FUNC_ENTER__;

	GError *err = NULL;
	guint type;
	guint result;
	DBusGConnection *bus = user_data;

	dbus_g_proxy_end_call(proxy, call, &err, G_TYPE_UINT, &type,
							G_TYPE_UINT, &result, G_TYPE_INVALID);
	if (err != NULL) {
		INFO_LOG(SP_NAME_ERR, "Error occured [%s]\n", err->message);
		g_error_free(err);
	} else {
		INFO_LOG(SP_NAME_NORMAL, "TYPE = %d,  Result = %d\n", type, result);
		if (type == 1 && result == 0) {
			INFO_LOG(SP_NAME_NORMAL, "OK\n");
			int ret = wlan_manager_request_power_on();
			INFO_LOG(SP_NAME_NORMAL, "power_on ret[%d]", ret);
		}
	}

	g_pending_call.is_handled = FALSE;

	g_object_unref(proxy);
	dbus_g_connection_unref(bus);
	wifi_syspopup_exit();

	__COMMON_FUNC_EXIT__;
}

static boolean _turn_off_mobile_hotspot(void)
{
	__COMMON_FUNC_ENTER__;

	DBusGConnection *bus;
	DBusGProxy *proxy;
	GError *error= NULL;

	bus = dbus_g_bus_get(DBUS_BUS_SYSTEM, &error);
	if (error != NULL) {
		INFO_LOG(SP_NAME_ERR, "Couldn't connect to the system bus");
		g_error_free(error);
		return FALSE;
	}

	proxy = dbus_g_proxy_new_for_name(bus,
					"com.samsung.mobileap",
					"/MobileAP",
					"com.samsung.mobileap");
	if (proxy == NULL) {
		INFO_LOG(SP_NAME_ERR, "Couldn't create the proxy object");
		dbus_g_connection_unref(bus);
		return FALSE;
	}

	g_pending_call.pending_call = dbus_g_proxy_begin_call(proxy, "disable_wifi_tethering",
				_mobilehotspot_disable_cb, bus, NULL, G_TYPE_INVALID);

	g_pending_call.proxy = proxy;
	g_pending_call.is_handled = TRUE;

	__COMMON_FUNC_EXIT__;
	return TRUE;
}
#endif

static int _power_on_check(void)
{
	char profile_name[NET_PROFILE_NAME_LEN_MAX+1] = "";
	switch (wlan_manager_state_get(profile_name)) {
	case WLAN_MANAGER_OFF:
		INFO_LOG(SP_NAME_NORMAL, "current state is wifi-off");
		int wlan_ret = wlan_manager_request_power_on();

		if (wlan_ret == WLAN_MANAGER_ERR_NONE) {
			view_alerts_powering_on_show();
			__COMMON_FUNC_EXIT__;
			return TRUE;
		} else if (wlan_ret == WLAN_MANAGER_ERR_MOBILE_HOTSPOT_OCCUPIED) {
			__COMMON_FUNC_EXIT__;
			return TRUE;
		} else {
			__COMMON_FUNC_EXIT__;
			return FALSE;
		}
		break;
	case WLAN_MANAGER_UNCONNECTED:
	case WLAN_MANAGER_CONNECTING:
		__COMMON_FUNC_EXIT__;
		return TRUE;
	case WLAN_MANAGER_CONNECTED:
		ERROR_LOG(SP_NAME_NORMAL, "current state is wifi-connected");
		__COMMON_FUNC_EXIT__;
		return FALSE;
	case WLAN_MANAGER_ERROR:
		ERROR_LOG(SP_NAME_NORMAL, "current state is wifi error");
		__COMMON_FUNC_EXIT__;
		return FALSE;
	default:
		ERROR_LOG(SP_NAME_NORMAL, "current state is wifi etc");
		__COMMON_FUNC_EXIT__;
		return FALSE;
	}

	__COMMON_FUNC_EXIT__;
	return TRUE;
}

#if 0	/* This function is not called from anywhere. Unused. */
static int _power_on(void)
{
	__COMMON_FUNC_ENTER__;

	char profile_name[NET_PROFILE_NAME_LEN_MAX+1] = "";
	switch (wlan_manager_state_get(profile_name)) {
	case WLAN_MANAGER_OFF:
		INFO_LOG(SP_NAME_NORMAL, "current state is wifi-off");
		int wlan_ret = wlan_manager_request_power_on();

		if(wlan_ret == WLAN_MANAGER_ERR_NONE) {
			view_alerts_powering_on_show();
		} else if (wlan_ret == WLAN_MANAGER_ERR_MOBILE_HOTSPOT_OCCUPIED) {
			if (_turn_off_mobile_hotspot() == FALSE) {
			    ERROR_LOG(SP_NAME_ERR, "Mobliehotspot deactivate Err!");
			    __COMMON_FUNC_EXIT__;
				wifi_syspopup_exit();
            }
		} else {
			ERROR_LOG(SP_NAME_ERR, "wifid power on Err!! [%d]", wlan_ret);
			__COMMON_FUNC_EXIT__;
			wifi_syspopup_exit();
		}
		break;
	case WLAN_MANAGER_UNCONNECTED:
		INFO_LOG(SP_NAME_NORMAL, "current state is wifi-on");
		break;
	case WLAN_MANAGER_CONNECTING:
		INFO_LOG(SP_NAME_NORMAL, "current state is wifi-connecting");
		__COMMON_FUNC_EXIT__;
		wifi_syspopup_exit();
		break;
	case WLAN_MANAGER_CONNECTED:
		INFO_LOG(SP_NAME_NORMAL, "current state is wifi-connected");
		__COMMON_FUNC_EXIT__;
		wifi_syspopup_exit();
		break;
	case WLAN_MANAGER_ERROR:
		ERROR_LOG(SP_NAME_ERR, "current state is wifi error");
		__COMMON_FUNC_EXIT__;
		wifi_syspopup_exit();
		break;
	default:
		ERROR_LOG(SP_NAME_ERR, "current state is wifi etc");
		__COMMON_FUNC_EXIT__;
		wifi_syspopup_exit();
		break;
	}

	__COMMON_FUNC_EXIT__;
	return TRUE;
}
#endif

static int app_reset(bundle *b, void *data)
{
	__COMMON_FUNC_ENTER__;

	Evas_Object *win_main = NULL;
	Evas *evas = NULL;
	const char* val = NULL;
	int ret = 0;
	int w, h = 0;

	assertm_if(NULL == data, "data param is NULL!!");
	assertm_if(NULL == b, "bundle is NULL!!");

	if (syspopup_has_popup(b)) {
		INFO_LOG(SP_NAME_NORMAL, "Wi-Fi Syspopup is already launched. So, no more.");
		syspopup_reset(b);
	} else {
		win_main = appcore_create_win(PACKAGE);
		assertm_if(NULL == win_main, "win_main is NULL!!");
		evas = evas_object_evas_get(win_main);
		assertm_if(NULL == evas, "evas is NULL!!");

		syspopup_app_state = data;
		syspopup_app_state->win_main = win_main;
		syspopup_app_state->evas = evas;
		syspopup_app_state->b = bundle_dup(b);

		elm_win_alpha_set(syspopup_app_state->win_main, EINA_TRUE); /* invisible window */
		elm_win_borderless_set(syspopup_app_state->win_main, EINA_TRUE); /* No borders */
		elm_win_indicator_mode_set(syspopup_app_state->win_main, ELM_WIN_INDICATOR_SHOW); /* indicator allow */
		elm_win_conformant_set(syspopup_app_state->win_main, TRUE); /* Popup autoscroll */

		const char* is_onoff = bundle_get_val(b, "-t");

		if (is_onoff != NULL) {
			INFO_LOG(SP_NAME_NORMAL, "is_onoff [%s]", is_onoff);

			syspopup_app_state->syspopup_type = WIFI_SYSPOPUP_WITHOUT_AP_LIST;
			int wlan_ret = wifi_syspopup_init();

			if (WLAN_MANAGER_ERR_NONE != wlan_ret) {
				INFO_LOG(SP_NAME_ERR, "wifi_syspopup_init failed. wlan_ret = %d", wlan_ret);
			} else if (strcmp(is_onoff, "on") == 0) {
				INFO_LOG(SP_NAME_NORMAL, "request power on");
				ret = wlan_manager_request_power_on();
				INFO_LOG(SP_NAME_NORMAL, "* ret [%d]", ret);
			} else if (strcmp(is_onoff, "off") == 0) {
				INFO_LOG(SP_NAME_NORMAL, "request power off");
				ret = wlan_manager_request_power_off();
				INFO_LOG(SP_NAME_NORMAL, "* ret [%d]", ret);
			}

			wifi_syspopup_destroy();
			return 0;
		} else {
			syspopup_app_state->syspopup_type = WIFI_SYSPOPUP_WITH_AP_LIST;
			int wlan_ret = wifi_syspopup_init();
			if (WLAN_MANAGER_ERR_NONE != wlan_ret || _power_on_check() == FALSE) {
				wifi_syspopup_destroy();
				__COMMON_FUNC_EXIT__;
				return 0;
			}
		}

		syspopup_app_state->syspopup = elm_popup_add(syspopup_app_state->win_main);
		ret = syspopup_create(b, &handler, syspopup_app_state->win_main, syspopup_app_state);
		if(ret != 0){
			ERROR_LOG(SP_NAME_ERR, "Syspopup create error!! return [%d]", ret );

			wlan_manager_destroy();

			__COMMON_FUNC_EXIT__;
			elm_exit();

			return 0;
		} else {
			val = bundle_get_val(b, "_INTERNAL_SYSPOPUP_NAME_");

			const char* support = bundle_get_val(b, "[Wi-Fi_syspopup wifi_syspopup_supports:support]");
			if(NULL != support) {
				syspopup_support_set(support);
			}

			ecore_x_window_size_get(ecore_x_window_root_first_get(), &w, &h);

			wifi_syspopup_create();
			wlan_manager_scanned_profile_refresh_with_count(MAX_INITIAL_QS_POPUP_LIST_SIZE);
		}
	}
	__COMMON_FUNC_EXIT__;

	return 0;
}

static int app_init(void *data)
{
	__COMMON_FUNC_ENTER__;
	bindtextdomain(PACKAGE, LOCALEDIR);
	__COMMON_FUNC_EXIT__;

	return 0;
}

static int app_exit(void *data)
{
		__COMMON_FUNC_ENTER__;
		__COMMON_FUNC_EXIT__;

		return 0;
}

static int app_start(void *data)
{
		__COMMON_FUNC_ENTER__;
		__COMMON_FUNC_EXIT__;

		return 0;
}

static int app_stop(void *data)
{
		__COMMON_FUNC_ENTER__;
		__COMMON_FUNC_EXIT__;

		return 0;
}

int main(int argc, char* argv[])
{
		__COMMON_FUNC_ENTER__;
		INFO_LOG( SP_NAME_NORMAL, "argc [%d]", argc);

		wifi_object ad;
		memset(&ad, 0x0, sizeof(wifi_object));

		ad.connection_result = VCONFKEY_WIFI_QS_3G;
		ad.win_main = NULL;
		ad.evas = NULL;
		ad.b = NULL;
		ad.syspopup = NULL;
		ad.passpopup = NULL;
		ad.alertpopup = NULL;

		struct appcore_ops ops = {
				.create = app_init,
				.terminate = app_exit,
				.pause = app_stop,
				.resume = app_start,
				.reset = app_reset,
		};

		ops.data = &ad;

		__COMMON_FUNC_EXIT__;
		return appcore_efl_main(PACKAGE, &argc, &argv, &ops);
}
