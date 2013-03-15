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

#include <wifi.h>
#include <syspopup.h>
#include <vconf.h>
#include <vconf-keys.h>
#include <appcore-efl.h>

#include "common.h"
#include "view-main.h"
#include "i18nmanager.h"
#include "view-alerts.h"
#include "common_utils.h"
#include "wlan_manager.h"
#include "wifi-syspopup.h"
#include "appcoreWrapper.h"
#include "wifi-syspopup-engine-callback.h"

wifi_object* syspopup_app_state = NULL;

static int wifi_syspopup_rotate_cb(enum appcore_rm rotate_mode, void *data)
{
	int rotate_angle;
	Evas_Object *box = NULL;

	rotate_angle = common_utils_get_rotate_angle(rotate_mode);
	box = elm_object_content_get(syspopup_app_state->syspopup);

	elm_win_rotation_with_resize_set(syspopup_app_state->win_main, rotate_angle);

	if (0 == rotate_angle || 180 == rotate_angle)
		evas_object_size_hint_min_set(box, -1,
				DEVICE_PICKER_POPUP_H * elm_config_scale_get());
	else
		evas_object_size_hint_min_set(box, -1,
				DEVICE_PICKER_POPUP_LN_H * elm_config_scale_get());

	if (syspopup_app_state->eap_popup)
		eap_view_rotate_popup(syspopup_app_state->eap_popup, rotate_angle);

	INFO_LOG(SP_NAME_NORMAL, "rotate_angle: %d", rotate_angle);

	return 0;
}

static void wifi_syspopup_exit(void)
{
	__COMMON_FUNC_ENTER__;

	view_main_destroy();

	if (VCONFKEY_WIFI_QS_WIFI_CONNECTED == syspopup_app_state->connection_result)
		INFO_LOG(SP_NAME_NORMAL, "Wi-Fi connected");
	else if (VCONFKEY_WIFI_QS_3G == syspopup_app_state->connection_result)
		INFO_LOG(SP_NAME_NORMAL, "Cellular connected");
	else {
		WARN_LOG(SP_NAME_NORMAL, "Result: [%d]",
				syspopup_app_state->connection_result);

		syspopup_app_state->connection_result = VCONFKEY_WIFI_QS_3G;
	}

	common_util_set_system_registry("memory/wifi/wifi_qs_exit",
								syspopup_app_state->connection_result);

	elm_exit();

	__COMMON_FUNC_EXIT__;
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
		passwd_popup_free(syspopup_app_state->passpopup);
		syspopup_app_state->passpopup = NULL;
	}

	if (syspopup_app_state->syspopup) {
		evas_object_del(syspopup_app_state->syspopup);
		syspopup_app_state->syspopup = NULL;
	}

	if (syspopup_app_state->layout_main) {
		evas_object_del(syspopup_app_state->layout_main);
		syspopup_app_state->layout_main = NULL;
	}

	if (syspopup_app_state->conformant) {
		evas_object_del(syspopup_app_state->conformant);
		syspopup_app_state->conformant = NULL;
	}

	if (syspopup_app_state->win_main) {
		evas_object_del(syspopup_app_state->win_main);
		syspopup_app_state->win_main = NULL;
	}

	connman_request_scan_mode_set(WIFI_BGSCAN_MODE_EXPONENTIAL);

	wifi_syspopup_exit();

	return 1;
}

int wifi_syspopup_create(void)
{
	__COMMON_FUNC_ENTER__;
	int rotate_angle;

	if (NULL == syspopup_app_state->syspopup) {
		syspopup_app_state->syspopup = elm_popup_add(syspopup_app_state->layout_main);
		elm_object_content_set(syspopup_app_state->layout_main, syspopup_app_state->syspopup);
		assertm_if(NULL == syspopup_app_state->syspopup, "syspopup is NULL!!");
	}

	elm_object_style_set(syspopup_app_state->syspopup,"min_menustyle");
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

	rotate_angle = common_utils_get_rotate_angle(APPCORE_RM_UNKNOWN);
	elm_win_rotation_with_resize_set(syspopup_app_state->win_main, rotate_angle);
	if (0 == rotate_angle || 180 == rotate_angle)
		evas_object_size_hint_min_set(box, -1,
				DEVICE_PICKER_POPUP_H * elm_config_scale_get());
	else
		evas_object_size_hint_min_set(box, -1,
				DEVICE_PICKER_POPUP_LN_H * elm_config_scale_get());

	elm_object_content_set(syspopup_app_state->syspopup, box);
	evas_object_show(syspopup_app_state->syspopup);
	evas_object_show(syspopup_app_state->win_main);

	memset(&g_pending_call, 0, sizeof(wifi_pending_call_info_t));

	__COMMON_FUNC_EXIT__;
	return 1;
}

int wifi_syspopup_init()
{
	__COMMON_FUNC_ENTER__;

	int wlan_ret;
	bool activated = FALSE;

	wlan_manager_create();
	wlan_manager_set_message_callback(wlan_engine_callback);
	wlan_manager_set_refresh_callback(wlan_engine_refresh_callback);

	wlan_ret = wlan_manager_start();
	switch (wlan_ret) {
	case WLAN_MANAGER_ERR_ALREADY_REGISTERED:
		ERROR_LOG(SP_NAME_ERR, "Already registered.");
		/* fall through */
	case WLAN_MANAGER_ERR_NONE:
		wlan_ret = wifi_is_activated(&activated);
		if (WIFI_ERROR_NONE == wlan_ret)
			INFO_LOG(SP_NAME_NORMAL, "Wi-Fi activated: %d", activated);

		INFO_LOG(SP_NAME_NORMAL, "wlan_manager start complete" );
		break;

	default:
		ERROR_LOG(SP_NAME_ERR, "Failed to start wlan_manager (%d)", wlan_ret);
		break;
	}

	connman_request_scan_mode_set(WIFI_BGSCAN_MODE_PERIODIC);

	__COMMON_FUNC_EXIT__;
	return wlan_ret;
}

static int syspopup_support_set(const char* support)
{
	__COMMON_FUNC_ENTER__;

	if (NULL == support) {
		__COMMON_FUNC_EXIT__;
		return 0;
	}

	if (g_strcmp0("WIFI_SYSPOPUP_SUPPORT_QUICKPANEL",support) == 0)
		syspopup_app_state->wifi_syspopup_support =
									WIFI_SYSPOPUP_SUPPORT_QUICKPANEL;
	else {
		__COMMON_FUNC_EXIT__;
		return 0;
	}

	__COMMON_FUNC_EXIT__;
	return 1;
}

static gboolean _power_on_check(void)
{
	int connection_state;

	connection_state = wlan_manager_state_get();
	switch (connection_state) {
	case WLAN_MANAGER_OFF:
		INFO_LOG(SP_NAME_NORMAL, "current state is wifi-off");

		int wlan_ret = wlan_manager_power_on();
		if (wlan_ret == WLAN_MANAGER_ERR_NONE) {
			view_alerts_powering_on_show();

			__COMMON_FUNC_EXIT__;
			return TRUE;
		} else if (wlan_ret == WLAN_MANAGER_ERR_WIFI_TETHERING_OCCUPIED) {
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

/* implements */
static int myterm(bundle* b, void* data)
{
	__COMMON_FUNC_ENTER__;

	wifi_syspopup_exit();

	__COMMON_FUNC_EXIT__;
	return 0;
}

static int mytimeout(bundle *b, void* data)
{
	__COMMON_FUNC_ENTER__;

	__COMMON_FUNC_EXIT__;
	return 0;
}

syspopup_handler handler = {
		.def_term_fn = myterm,
		.def_timeout_fn = mytimeout
};

static Eina_Bool __wifi_syspopup_del_found_ap_noti(void *data)
{
	common_utils_send_message_to_net_popup(NULL, NULL,
										"del_found_ap_noti", NULL);

	return ECORE_CALLBACK_CANCEL;
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

static void __pw_lock_state_change_cb(keynode_t *node, void *user_data)
{
	__COMMON_FUNC_ENTER__;

	int pw_lock_state = 0;
	if (0 != vconf_get_int(VCONFKEY_PWLOCK_STATE, &pw_lock_state) ||
			pw_lock_state != VCONFKEY_PWLOCK_BOOTING_LOCK) {
		vconf_ignore_key_changed(VCONFKEY_PWLOCK_STATE, __pw_lock_state_change_cb);
		wifi_syspopup_create();
		appcore_set_rotation_cb(wifi_syspopup_rotate_cb, NULL);
		g_idle_add(load_initial_ap_list, NULL);
	}

	INFO_LOG(SP_NAME_NORMAL, "pwlock state = %d", pw_lock_state);

	__COMMON_FUNC_EXIT__;
	return;
}

static int app_reset(bundle *b, void *data)
{
	__COMMON_FUNC_ENTER__;

	Evas_Object *win_main = NULL;
	Evas *evas = NULL;
	int ret = 0;
	int pw_lock_state = 0;

	assertm_if(NULL == data, "data param is NULL!!");
	assertm_if(NULL == b, "bundle is NULL!!");

	/* Remove the "WiFi networks found" from the notification tray.*/
	ecore_idler_add(__wifi_syspopup_del_found_ap_noti, NULL);

	if (syspopup_has_popup(b)) {
		INFO_LOG(SP_NAME_NORMAL, "Wi-Fi device picker is already launched");

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
		elm_win_conformant_set(syspopup_app_state->win_main, TRUE); /* Popup autoscroll */

		Evas_Object *conformant = elm_conformant_add(syspopup_app_state->win_main);
		elm_win_conformant_set(syspopup_app_state->win_main, EINA_TRUE);
		elm_win_resize_object_add(syspopup_app_state->win_main, conformant);
		evas_object_size_hint_weight_set(conformant, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		evas_object_size_hint_align_set(conformant, EVAS_HINT_FILL, EVAS_HINT_FILL);
		evas_object_show(conformant);
		syspopup_app_state->conformant = conformant;

		Evas_Object *layout = elm_layout_add(conformant);
		elm_object_content_set(conformant, layout);
		syspopup_app_state->layout_main = layout;

		const char* is_onoff = bundle_get_val(b, "-t");

		if (is_onoff != NULL) {
			INFO_LOG(SP_NAME_NORMAL, "is_onoff [%s]", is_onoff);

			syspopup_app_state->syspopup_type = WIFI_SYSPOPUP_WITHOUT_AP_LIST;
			int wlan_ret = wifi_syspopup_init();

			if (WLAN_MANAGER_ERR_NONE != wlan_ret)
				INFO_LOG(SP_NAME_ERR, "wifi_syspopup_init failed. wlan_ret = %d", wlan_ret);
			else if (strcmp(is_onoff, "on") == 0) {
				ret = wlan_manager_power_on();

				INFO_LOG(SP_NAME_NORMAL, "Wi-Fi power on: [%d]", ret);
			} else if (strcmp(is_onoff, "off") == 0) {
				ret = wlan_manager_power_off();

				INFO_LOG(SP_NAME_NORMAL, "Wi-Fi power off: [%d]", ret);
			}

			wifi_syspopup_destroy();

			__COMMON_FUNC_EXIT__;
			return 0;
		} else {
			syspopup_app_state->syspopup_type = WIFI_SYSPOPUP_WITH_AP_LIST;

			int wlan_ret = wifi_syspopup_init();
			if (WLAN_MANAGER_ERR_NONE != wlan_ret ||
								_power_on_check() == FALSE) {
				wifi_syspopup_destroy();

				__COMMON_FUNC_EXIT__;
				return 0;
			}
		}

		syspopup_app_state->syspopup =
				elm_popup_add(syspopup_app_state->win_main);

		ret = syspopup_create(b, &handler,
				syspopup_app_state->win_main, syspopup_app_state);
		if (ret != 0){
			ERROR_LOG(SP_NAME_ERR, "Fail to create popup [%d]", ret );

			wlan_manager_destroy();

			elm_exit();

			__COMMON_FUNC_EXIT__;
			return 0;
		} else {
			const char* support = bundle_get_val(b,
					"[Wi-Fi_syspopup wifi_syspopup_supports:support]");
			if(NULL != support)
				syspopup_support_set(support);

			if (0 != vconf_get_int(VCONFKEY_PWLOCK_STATE, &pw_lock_state) ||
					pw_lock_state == VCONFKEY_PWLOCK_BOOTING_LOCK) {
				/* The PW lock is in lock mode during the booting stage.
				 * Lets wait for the lock state to change.
				 * After the lock state is changed we can create the popup
				 */
				vconf_notify_key_changed(VCONFKEY_PWLOCK_STATE, __pw_lock_state_change_cb, NULL);
			} else {
				wifi_syspopup_create();
				appcore_set_rotation_cb(wifi_syspopup_rotate_cb, NULL);
				g_idle_add(load_initial_ap_list, NULL);
			}

			INFO_LOG(SP_NAME_NORMAL, "pwlock state = %d", pw_lock_state);
		}
	}

	__COMMON_FUNC_EXIT__;
	return 0;
}

static int app_create(void *data)
{
	__COMMON_FUNC_ENTER__;

	bindtextdomain(PACKAGE, LOCALEDIR);

	__COMMON_FUNC_EXIT__;
	return 0;
}

static int app_exit(void *data)
{
	__COMMON_FUNC_ENTER__;

	wlan_manager_destroy();

	__COMMON_FUNC_EXIT__;
	return 0;
}

static int app_pause(void *data)
{
	__COMMON_FUNC_ENTER__;

	connman_request_scan_mode_set(WIFI_BGSCAN_MODE_EXPONENTIAL);

	__COMMON_FUNC_EXIT__;
	return 0;
}

static int app_resume(void *data)
{
	__COMMON_FUNC_ENTER__;

	connman_request_scan_mode_set(WIFI_BGSCAN_MODE_PERIODIC);

	__COMMON_FUNC_EXIT__;
	return 0;
}

int main(int argc, char* argv[])
{
	__COMMON_FUNC_ENTER__;

	INFO_LOG(SP_NAME_NORMAL, "argc [%d]", argc);

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
			.create = app_create,
			.terminate = app_exit,
			.pause = app_pause,
			.resume = app_resume,
			.reset = app_reset,
	};

	ops.data = &ad;

	__COMMON_FUNC_EXIT__;
	return appcore_efl_main(PACKAGE, &argc, &argv, &ops);
}
