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
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <Ecore_X.h>
#include <syspopup.h>
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

#define POPUP_HEAD_AREA 134
#define POPUP_BUTTON_AREA 200
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

static int __get_window_property(Display *dpy, Window win, Atom atom,
								 Atom type, unsigned int *val,
								 unsigned int len)
{
	__COMMON_FUNC_ENTER__;
	unsigned char *prop_ret = NULL;
	Atom type_ret = -1;
	unsigned long bytes_after = 0;
	unsigned long  num_ret = -1;
	int format_ret = -1;
	unsigned int i = 0;
	int num = 0;

	prop_ret = NULL;
	if (XGetWindowProperty(dpy, win, atom, 0, 0x7fffffff, False,
						   type, &type_ret, &format_ret, &num_ret,
						   &bytes_after, &prop_ret) != Success) {
		return -1;
	}

	if (type_ret != type || format_ret != 32) {
		num = -1;
	} else if (num_ret == 0 || !prop_ret) {
		num = 0;
	} else {
		if (num_ret < len) {
			len = num_ret;
		}
		for (i = 0; i < len; i++) {
			val[i] = ((unsigned long *)prop_ret)[i];
		}
		num = len;
	}

	if (prop_ret) {
		XFree(prop_ret);
	}

	__COMMON_FUNC_EXIT__;
	return num;
}

static int __x_rotation_get(Display *dpy, Window win)
{
	__COMMON_FUNC_ENTER__;
	Window active_win = 0;
	Window root_win = 0;
	int rotation = -1;
	int ret = -1;

	Atom atom_active_win;
	Atom atom_win_rotate_angle;

	root_win = XDefaultRootWindow(dpy);

	atom_active_win = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", False);
	ret = __get_window_property(dpy, root_win, atom_active_win,
								XA_WINDOW,
								(unsigned int *)&active_win, 1);

	if (ret < 0)
		return ret;

	atom_win_rotate_angle =
		XInternAtom(dpy, "_E_ILLUME_ROTATE_WINDOW_ANGLE", False);
	ret = __get_window_property(dpy, active_win	,
								atom_win_rotate_angle, XA_CARDINAL,
								(unsigned int *)&rotation, 1);

	__COMMON_FUNC_EXIT__;

	if (ret != -1) {
		return rotation;
	}

	return -1;
}

static Eina_Bool __rotate(void *data, int type, void *event)
{
	__COMMON_FUNC_ENTER__;
	struct wifi_object *ad = data;
	Ecore_X_Event_Client_Message *ev = event;
	int visible_area_width, visible_area_height;
	int rotate_angle;

	Evas_Object *box = NULL;

	if (!event)
		return ECORE_CALLBACK_RENEW;

	if (ev->message_type == ECORE_X_ATOM_E_ILLUME_ROTATE_ROOT_ANGLE) {
		box = elm_object_content_get(syspopup_app_state->syspopup);

		if (box) {
			rotate_angle = __x_rotation_get(ecore_x_display_get(), elm_win_xwindow_get(syspopup_app_state->win_main));
			__common_popup_size_set(NULL ,&visible_area_width, &visible_area_height, rotate_angle);
			elm_win_rotation_with_resize_set(syspopup_app_state->win_main, rotate_angle);
			evas_object_size_hint_min_set(box, visible_area_width * elm_config_scale_get(), visible_area_height * elm_config_scale_get());
		}

		if (syspopup_app_state->eap_popup)
			eap_view_rotate_popup(syspopup_app_state->eap_popup, rotate_angle);
	}

	__COMMON_FUNC_EXIT__;
	return 0;
}

void __common_popup_size_set(Ecore_IMF_Context *target_imf, int *width, int *height, int rotate_angle)
{
	__COMMON_FUNC_ENTER__;

	int window_width, window_height;
	int start_x, start_y, imf_width, imf_height;
	float resize_scale = 0.7f;

	ecore_x_window_size_get(ecore_x_window_root_first_get(), &window_width, &window_height);

	*width = window_width;

	if (rotate_angle == 0 || rotate_angle == 180)
	{
		*height = window_height * resize_scale;
	}else
		*height = window_width;

	if (target_imf != NULL) {
		ecore_imf_context_input_panel_geometry_get(target_imf, &start_x, &start_y, &imf_width, &imf_height);
		*height = start_y * resize_scale;
	}else
		*height = *height-POPUP_HEAD_AREA-POPUP_BUTTON_AREA;

	__COMMON_FUNC_EXIT__;
}

static int wifi_syspopup_rotate_cb(enum appcore_rm rotate_mode, void *data)
{
	__COMMON_FUNC_ENTER__;

	int rotate_angle;
	int visible_area_width, visible_area_height;

	Evas_Object *box = NULL;

	rotate_angle = common_utils_get_rotate_angle(rotate_mode);
	box = elm_object_content_get(syspopup_app_state->syspopup);
	elm_win_rotation_with_resize_set(syspopup_app_state->win_main, rotate_angle);

	__common_popup_size_get(NULL ,&visible_area_width, &visible_area_height);
	evas_object_size_hint_min_set(box, visible_area_width * elm_config_scale_get(), visible_area_height * elm_config_scale_get());

	if (syspopup_app_state->eap_popup)
		eap_view_rotate_popup(syspopup_app_state->eap_popup, rotate_angle);

	INFO_LOG(SP_NAME_NORMAL, "rotate_angle : %d", rotate_angle);

	__COMMON_FUNC_EXIT__;
	return 0;
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

	if (VCONFKEY_WIFI_QS_WIFI_CONNECTED == syspopup_app_state->connection_result)
		INFO_LOG(SP_NAME_NORMAL, "Result : WIFI");
	else if (VCONFKEY_WIFI_QS_3G == syspopup_app_state->connection_result)
		INFO_LOG(SP_NAME_NORMAL, "Result : 3G");
	else {
		WARN_LOG(SP_NAME_NORMAL, "Result : ?? [%d]", syspopup_app_state->connection_result);
		syspopup_app_state->connection_result = VCONFKEY_WIFI_QS_3G;
	}

	common_util_set_system_registry("memory/wifi/wifi_qs_exit",
								syspopup_app_state->connection_result);

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

	wifi_syspopup_exit();

	return TRUE;
}

int wifi_syspopup_create(void)
{
	__COMMON_FUNC_ENTER__;
	int rotate_angle;
	int visible_area_height;
	int visible_area_width;

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

	rotate_angle = __x_rotation_get(ecore_x_display_get(), elm_win_xwindow_get(syspopup_app_state->win_main));
	if (rotate_angle < 0)
		rotate_angle = 0;

	ecore_x_icccm_hints_set(elm_win_xwindow_get(syspopup_app_state->win_main), 1, 0, 0, 0, 0, 0, 0);

	elm_win_rotation_with_resize_set(syspopup_app_state->win_main, rotate_angle);

	__common_popup_size_set(NULL ,&visible_area_width, &visible_area_height, rotate_angle);
	evas_object_size_hint_min_set(box, visible_area_width * elm_config_scale_get(), visible_area_height * elm_config_scale_get());

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

	int wlan_ret;
	bool activated = FALSE;

	wlan_manager_create();
	wlan_manager_set_message_callback(wlan_engine_callback);
	wlan_manager_set_refresh_callback(wlan_engine_refresh_callback);

	wlan_ret = wlan_manager_start(NULL);
	switch (wlan_ret) {
	case WLAN_MANAGER_ERR_NONE:
		wlan_ret = wifi_is_activated (&activated);
		if (WIFI_ERROR_NONE != wlan_ret) {
			ERROR_LOG(UG_NAME_ERR, "Failed to get the Wi-Fi State. Return error = %d", wlan_ret);
			return WLAN_MANAGER_ERR_UNKNOWN;
		}

		if (activated) {
			INFO_LOG(UG_NAME_NORMAL, "WiFi is activated");
			if (connman_request_scan_mode_set(WIFI_BGSCAN_MODE_PERIODIC) == WLAN_MANAGER_ERR_NONE) {
				INFO_LOG(SP_NAME_NORMAL, "Set BG scan mode - PERIODIC");
			}
		}
		INFO_LOG(SP_NAME_NORMAL, "wlan_manager start complete" );
		break;

	case WLAN_MANAGER_ERR_ALREADY_REGISTERED:
		ERROR_LOG(SP_NAME_ERR, "Already registered.");
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

static int _power_on_check(void)
{
	int connection_state = wlan_manager_state_get();
	switch (connection_state) {
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

static Eina_Bool __wifi_syspopup_del_found_ap_noti(void *data)
{
	common_utils_send_message_to_net_popup(NULL, NULL, "del_found_ap_noti", NULL);
	return ECORE_CALLBACK_CANCEL;
}

static int app_reset(bundle *b, void *data)
{
	__COMMON_FUNC_ENTER__;

	Evas_Object *win_main = NULL;
	Evas *evas = NULL;
	int ret = 0;
	int w, h = 0;

	assertm_if(NULL == data, "data param is NULL!!");
	assertm_if(NULL == b, "bundle is NULL!!");

	/* Remove the "WiFi networks found" from the notification tray.*/
	ecore_idler_add(__wifi_syspopup_del_found_ap_noti, NULL);

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
			const char* support = bundle_get_val(b, "[Wi-Fi_syspopup wifi_syspopup_supports:support]");
			if(NULL != support) {
				syspopup_support_set(support);
			}

			ecore_x_window_size_get(ecore_x_window_root_first_get(), &w, &h);

			wifi_syspopup_create();
			wlan_manager_scanned_profile_refresh_with_count(MAX_INITIAL_QS_POPUP_LIST_SIZE);
			ecore_event_handler_add(ECORE_X_EVENT_CLIENT_MESSAGE, __rotate, (void *)syspopup_app_state);

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
	wlan_manager_destroy();
	__COMMON_FUNC_EXIT__;

	return 0;
}

static int app_start(void *data)
{
	__COMMON_FUNC_ENTER__;
	if (connman_request_scan_mode_set(WIFI_BGSCAN_MODE_PERIODIC) == WLAN_MANAGER_ERR_NONE) {
		INFO_LOG(SP_NAME_NORMAL, "Set BG scan mode - PERIODIC");
	}
	__COMMON_FUNC_EXIT__;

	return 0;
}

static int app_stop(void *data)
{
	__COMMON_FUNC_ENTER__;
	if (connman_request_scan_mode_set(WIFI_BGSCAN_MODE_EXPONENTIAL) == WLAN_MANAGER_ERR_NONE) {
		INFO_LOG(SP_NAME_NORMAL, "Set BG scan mode - EXPONENTIAL");
	}
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
