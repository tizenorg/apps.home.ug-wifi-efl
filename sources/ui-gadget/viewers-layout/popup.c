/*
 * Copyright (c) 2000-2012 Samsung Electronics Co., Ltd All Rights Reserved.
 *
 * This file is part of Wi-Fi UG
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



#include "wifi.h"
#include "popup.h"
#include "wlan_manager.h"
#include "i18nmanager.h"
#include "viewer_manager.h"


#define POPUP_LAYOUT_TEXT_MAX 100

static Ecore_Timer *timer = NULL;
static popup_manager_object* manager_object = NULL;

static int checker = 0;
static double value = 0.0;


static void _timeout_cb(void *data, Evas_Object *obj, void *event_info)
{
	if (obj)
		evas_object_del(obj);
}

void* winset_popup_create(Evas_Object* win)
{
	manager_object = (popup_manager_object *)g_malloc0(sizeof(popup_manager_object));
	manager_object->win = win;
	manager_object->mode = POPUP_MODE_OFF;
	return NULL;
}

static void pbc_popup_cb(void *data, Evas_Object *obj, void *event_info)
{
	__COMMON_FUNC_ENTER__;

	if (checker == 0) {
		checker = 1;

		if (timer != NULL) {
			ecore_timer_del(timer);
			timer = NULL;
		}

		wlan_manager_request_cancel_wps_connection((char*) data);
		winset_popup_mode_set (NULL, POPUP_MODE_OFF, POPUP_OPTION_NONE);

		if (manager_object->content != NULL) {
			evas_object_hide(manager_object->content);
			evas_object_del(manager_object->content);
			manager_object->content = NULL;
		}

		checker = 0;
	}

	__COMMON_FUNC_EXIT__;
}

static void _mobilehotspot_disable_cb(DBusGProxy *proxy, DBusGProxyCall *call, gpointer user_data)
{
	__COMMON_FUNC_ENTER__;

	GError *err = NULL;
	guint type;
	guint result;
	DBusGConnection	*bus = user_data;	

	dbus_g_proxy_end_call(proxy, call, &err, G_TYPE_UINT, &type,
						G_TYPE_UINT, &result, G_TYPE_INVALID);
	if (err != NULL) {
		INFO_LOG(COMMON_NAME_LIB, "Error occured [%s]\n", err->message);
		g_error_free(err);
	} else {
		INFO_LOG(COMMON_NAME_LIB, "TYPE = %d,  Result = %d\n", type, result);
		if (type == 1 && result == 0) {
			INFO_LOG(COMMON_NAME_LIB, "OK\n");
			power_control();
		}
	}

	g_pending_call.is_handled = TRUE;

	g_object_unref(proxy);
	dbus_g_connection_unref(bus);

	__COMMON_FUNC_EXIT__;
}

int mobilehotspot_deactivate()
{
	__COMMON_FUNC_ENTER__;

	DBusGConnection *bus;
	DBusGProxy *proxy;
	GError *error= NULL;

	bus = dbus_g_bus_get(DBUS_BUS_SYSTEM, &error);
	if (error != NULL) {
		INFO_LOG(COMMON_NAME_LIB, "Couldn't connect to the system bus");
		g_error_free(error);
		return FALSE;
	}

	proxy =	dbus_g_proxy_new_for_name(bus,
					"org.tizen.mobileap",	/* name */
					"/MobileAP",			/* obj path */
					"org.tizen.mobileap");/* interface */
	if (proxy == NULL) {
		INFO_LOG(COMMON_NAME_LIB, "Couldn't create the proxy object");
		dbus_g_connection_unref(bus);
		return FALSE;
	}

	g_pending_call.pending_call = dbus_g_proxy_begin_call(proxy, "disable_wifi_tethering",
			_mobilehotspot_disable_cb, bus, NULL, G_TYPE_INVALID);

	g_pending_call.proxy = proxy;
	g_pending_call.is_handled = FALSE;

	__COMMON_FUNC_EXIT__;
	return TRUE;
}

void _ok_clicked_cb(void* data, Evas_Object* obj, void* event_info)
{
	if (data) {
		evas_object_del(data);
	}
}

void _retry_clicked_cb(void* data, Evas_Object* obj, void* event_info)
{
	__COMMON_FUNC_ENTER__;

	INFO_LOG(UG_NAME_NORMAL, "Response OK");

	switch (wlan_manager_start()) {
	case WLAN_MANAGER_ERR_NONE:
		break;
	case WLAN_MANAGER_ERR_ALREADY_REGISTERED:
		winset_popup_mode_set(NULL, POPUP_MODE_REGISTER_FAILED, POPUP_OPTION_NONE);
		return;
	case WLAN_MANAGER_ERR_UNKNOWN:
		winset_popup_mode_set(NULL, POPUP_MODE_REGISTER_FAILED, POPUP_OPTION_REGISTER_FAILED_COMMUNICATION_FAILED);
		return;
	default:
		__COMMON_FUNC_EXIT__;
		return;
	}
		
	char profile_name[NET_PROFILE_NAME_LEN_MAX+1] = "";
	switch (wlan_manager_state_get(profile_name)) {
	case WLAN_MANAGER_OFF:
		ERROR_LOG(UG_NAME_NORMAL, "current state is wifi-off\n");
		viewer_manager_hide(VIEWER_WINSET_SUB_CONTENTS);
		break;
	case WLAN_MANAGER_UNCONNECTED:
		ERROR_LOG(UG_NAME_NORMAL, "current state is wifi-connecting\n");
	case WLAN_MANAGER_CONNECTING:
		ERROR_LOG(UG_NAME_NORMAL, "current state is wifi-on\n");
		viewer_manager_hide(VIEWER_WINSET_SEARCHING);
		viewer_manager_show(VIEWER_WINSET_SUB_CONTENTS);
		break;
	case WLAN_MANAGER_CONNECTED:
		ERROR_LOG(UG_NAME_NORMAL, "current state is wifi-connected\n");
		viewer_manager_hide(VIEWER_WINSET_SEARCHING);
		viewer_manager_show(VIEWER_WINSET_SUB_CONTENTS);
		break;
	case WLAN_MANAGER_ERROR:
		winset_popup_mode_set(NULL, POPUP_MODE_ETC, POPUP_OPTION_ETC_WLAN_STATE_GET_ERROR);
		break;
	default:
		winset_popup_mode_set(NULL, POPUP_MODE_ETC, POPUP_OPTION_ETC_WLAN_STATE_GET_ERROR);
		break;
	}

	__COMMON_FUNC_EXIT__;
}

void _back_clicked_cb(void* data, Evas_Object* obj, void* event_info)
{
	INFO_LOG(UG_NAME_NORMAL, "Response CANCEL");
	wifi_exit();
}

void _turn_off_mobileap_yes_clicked_cb(void* data, Evas_Object* obj, void* event_info)
{
	__COMMON_FUNC_ENTER__;

	INFO_LOG(UG_NAME_NORMAL, "Response OK");
	if (mobilehotspot_deactivate()) {
		INFO_LOG(UG_NAME_NORMAL, "Mobile AP return value TRUE");
	} else {
		INFO_LOG(UG_NAME_NORMAL, "Mobile AP return value FALSE");
	}

	__COMMON_FUNC_EXIT__;
}

void _turn_off_mobileap_no_clicked_cb(void* data, Evas_Object* obj, void* event_info)
{
	__COMMON_FUNC_ENTER__;

	INFO_LOG(UG_NAME_NORMAL, "Response CANCEL");
	viewer_manager_header_mode_set(HEADER_MODE_OFF);

	__COMMON_FUNC_EXIT__;
}

/* Mobile-AP popup callback */
void _popup_out_of_wifi_yes_cb(void* data, Evas_Object* obj, void* event_info)
{
	INFO_LOG(UG_NAME_NORMAL, "Response OK");
	wifi_exit();
}

void _popup_out_of_wifi_no_cb(void* data, Evas_Object* obj, void* event_info)
{
	INFO_LOG(UG_NAME_NORMAL, "Response CANCEL");
	if (data) {
		evas_object_del(data);
	}
}

int winset_popup_timer_remove(void)
{
	if(timer != NULL) {
		ecore_timer_del(timer);
		timer = NULL;
		return TRUE;
	}
	return FALSE;
}

int winset_popup_stop(void* object)
{
	__COMMON_FUNC_ENTER__;
	value = 1.0;
	__COMMON_FUNC_EXIT__;
	return TRUE;
}

static int _fn_pb_timer_bar(void *data)
{
	if (timer == NULL || manager_object->progressbar == NULL) {
		return 0;
	}

	value = 0.0;
	value = elm_progressbar_value_get(manager_object->progressbar);
	if (value >= 1) {
		if (checker == 0) {
			checker = 1;

			__COMMON_FUNC_ENTER__;

			wlan_manager_request_cancel_wps_connection((char*) data);

			char message[128] = "one click connection failed";

			winset_popup_mode_set (manager_object, POPUP_MODE_OFF, POPUP_OPTION_NONE);
			evas_object_hide(manager_object->content);
			evas_object_del(manager_object->content);
			manager_object->content = NULL;

			checker = 0;

			Evas_Object*instance_popup = elm_popup_add(manager_object->win);
			elm_object_text_set(instance_popup, message);
			elm_popup_timeout_set(instance_popup, 3.0f);
			evas_object_smart_callback_add(instance_popup, "timeout", _timeout_cb, NULL);
			evas_object_size_hint_weight_set(instance_popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
			elm_popup_orient_set(instance_popup, ELM_POPUP_ORIENT_CENTER);
			evas_object_show(instance_popup);
			INFO_LOG(UG_NAME_NORMAL, "Back button");

			__COMMON_FUNC_EXIT__;
			return 0;
		}
	} else {
		value += 0.001;
	}

	elm_progressbar_value_set(manager_object->progressbar, value);

	return 1;
}

int winset_popup_simple_set(const char* text)
{
	__COMMON_FUNC_ENTER__;
	manager_object->content = elm_popup_add(manager_object->win);
	evas_object_size_hint_weight_set(manager_object->content, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_object_text_set(manager_object->content, text);
	Evas_Object *btn_ok = elm_button_add(manager_object->content);
	elm_object_text_set(btn_ok, sc(PACKAGE, I18N_TYPE_Ok));
	elm_object_part_content_set(manager_object->content, "button1", btn_ok);
	evas_object_smart_callback_add(btn_ok, "clicked", _ok_clicked_cb, manager_object->content);
	evas_object_show(manager_object->content);

	return TRUE;
}

static POPUP_MODES _popup_mode;
static POPUP_MODE_OPTIONS _popup_option;

static int _winset_popup_mode_set(void* data)
{
	__COMMON_FUNC_ENTER__;

	POPUP_MODES mode = _popup_mode;
	POPUP_MODE_OPTIONS option = _popup_option;

	if(NULL != manager_object->content) {
		evas_object_del(manager_object->content);
		manager_object->content = NULL;
	}
	switch (manager_object->mode) {
		case POPUP_MODE_OFF:
			if (POPUP_MODE_PBC == mode) {
				manager_object->content = elm_popup_add(manager_object->win);
				evas_object_size_hint_weight_set(manager_object->content, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
				elm_object_text_set(manager_object->content, _("Push the wps button"));
				Evas_Object *btn_cancel = elm_button_add(manager_object->content);
				elm_object_text_set(btn_cancel,  sc(PACKAGE, I18N_TYPE_Cancel));
				elm_object_part_content_set(manager_object->content, "button1", btn_cancel);
				evas_object_smart_callback_add(btn_cancel, "clicked", pbc_popup_cb, data);
				evas_object_show(manager_object->content);

				manager_object->progressbar = elm_progressbar_add(manager_object->content);
				elm_progressbar_horizontal_set(manager_object->progressbar, EINA_TRUE);

				elm_object_style_set(manager_object->progressbar, "list_progress");
				evas_object_size_hint_align_set(manager_object->progressbar, EVAS_HINT_FILL, 0.5);
				evas_object_size_hint_weight_set(manager_object->progressbar, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
				elm_object_content_set(manager_object->content, manager_object->progressbar);
				elm_progressbar_span_size_set(manager_object->progressbar, 250);
				elm_progressbar_value_set(manager_object->progressbar, 0.0);
				timer = ecore_timer_add(0.1, (Ecore_Task_Cb) _fn_pb_timer_bar, data);
				elm_progressbar_unit_format_set(manager_object->progressbar, NULL);

			} else if (POPUP_MODE_REGISTER_FAILED == mode){
				switch(option) {
					case POPUP_OPTION_REGISTER_FAILED_COMMUNICATION_FAILED:
						manager_object->content = elm_popup_add(manager_object->win);
						evas_object_size_hint_weight_set(manager_object->content, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
						elm_object_text_set(manager_object->content, "connman is not working now");
						Evas_Object *btn_retry = elm_button_add(manager_object->content);
						elm_object_text_set(btn_retry, "Retry");
						elm_object_part_content_set(manager_object->content, "button1", btn_retry);
						evas_object_smart_callback_add(btn_retry, "clicked", _retry_clicked_cb, NULL);
						Evas_Object *btn_back = elm_button_add(manager_object->content);
						elm_object_text_set(btn_retry, "Back");
						elm_object_part_content_set(manager_object->content, "button2", btn_back);
						evas_object_smart_callback_add(btn_retry, "clicked", _back_clicked_cb, NULL);
						evas_object_show(manager_object->content);
						break;

					default:
						manager_object->content = elm_popup_add(manager_object->win);
						elm_object_text_set(manager_object->content, _("[ERROR] wlan_client func<br>wlan_client_register"));
						Evas_Object *btn_ok = elm_button_add(manager_object->content);
						elm_object_text_set(btn_ok, sc(PACKAGE, I18N_TYPE_Ok));
						elm_object_part_content_set(manager_object->content, "button1", btn_ok);
						evas_object_smart_callback_add(btn_ok, "clicked", _ok_clicked_cb, manager_object->content);
						evas_object_show(manager_object->content);
						break;
				}

			} else if (POPUP_MODE_POWER_ON_FAILED == mode) {
				switch (option) {
				case POPUP_OPTION_POWER_ON_FAILED_MOBILE_HOTSPOT:
					manager_object->content = elm_popup_add(manager_object->win);
					evas_object_size_hint_weight_set(manager_object->content, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
					elm_object_text_set(manager_object->content, "Connecting Wi-Fi will turn off Mobile hotspot. Continue?");
					Evas_Object *btn_yes = elm_button_add(manager_object->content);
					elm_object_text_set(btn_yes, sc(PACKAGE, I18N_TYPE_Yes));
					elm_object_part_content_set(manager_object->content, "button1", btn_yes);
					evas_object_smart_callback_add(btn_yes, "clicked", _turn_off_mobileap_yes_clicked_cb, NULL);
					Evas_Object *btn_no = elm_button_add(manager_object->content);
					elm_object_text_set(btn_no, sc(PACKAGE, I18N_TYPE_No));
					elm_object_part_content_set(manager_object->content, "button2", btn_no);
					evas_object_smart_callback_add(btn_no, "clicked", _turn_off_mobileap_no_clicked_cb, NULL);
					evas_object_show(manager_object->content);
					break;
				default:
					manager_object->content = elm_popup_add(manager_object->win);
					evas_object_size_hint_weight_set(manager_object->content, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
					elm_object_text_set(manager_object->content, "wlan_client_power_on fail");
					Evas_Object *btn_ok = elm_button_add(manager_object->content);
					elm_object_text_set(btn_ok, sc(PACKAGE, I18N_TYPE_Ok));
					elm_object_part_content_set(manager_object->content, "button1", btn_ok);
					evas_object_smart_callback_add(btn_ok, "clicked", _ok_clicked_cb, manager_object->content);
					break;
				}
				evas_object_show(manager_object->content);

			} else if (POPUP_MODE_CONNECTING_FAILED == mode) {
				manager_object->content = elm_popup_add(manager_object->win);
				evas_object_size_hint_weight_set(manager_object->content, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
				Evas_Object *btn_ok = elm_button_add(manager_object->content);
				elm_object_text_set(btn_ok, sc(PACKAGE, I18N_TYPE_Ok));
				elm_object_part_content_set(manager_object->content, "button1", btn_ok);
				evas_object_smart_callback_add(btn_ok, "clicked", _ok_clicked_cb, manager_object->content);

				char buffer[POPUP_LAYOUT_TEXT_MAX+1];
/*
 * FIX ME LATER
 * It will be fixed to use i18n function when elm_popup bug fixing work done
 * ex) str = i18n_manager_get_text(I18N_TYPE_Connection_attempt...);
 */
				char* str = NULL;

				switch (option) {
					case POPUP_OPTION_CONNECTING_FAILED_TIMEOUT:
						str = "No response from<br>Wi-Fi Access Point";
						break;
					case POPUP_OPTION_CONNECTING_FAILED_INVALID_OPERATION:
						str = "[ERROR]<br>wlan_client_open_connection<br>return INVALID OPERATION";
						break;
					case POPUP_OPTION_CONNECTIONG_PASSWORD_WEP_ERROR:
						str = "WEP requires 5, 10, 13, 26 letters for a password.<br>Please, check your input.";
						break;
					case POPUP_OPTION_CONNECTIONG_PASSWORD_WPAPSK_ERROR:
						str = "WPA2 requires 8 - 63 letters for a password.<br>Please, check your input.";
						break;
					case POPUP_OPTION_NONE:
						str = "NONE";
						break;
					default:
						str = "Connection attempt failed";
						break;
				}

				INFO_LOG(UG_NAME_NORMAL,"popup str [%s] by option [%d]", str, option);
				if(NULL != str) {
					int len = strlen(str);
					if(len+1 <= POPUP_LAYOUT_TEXT_MAX+1) {
						snprintf(buffer, len+1, "%s", str);
					}
				}

				elm_object_text_set(manager_object->content, buffer);
				evas_object_show(manager_object->content);

			} else if (POPUP_MODE_INPUT_FAILED == mode) {
				char* text = NULL;
				switch(option){
					case POPUP_OPTION_INPUT_FAILED_PROXY_IP_MISTYPE :
						text = _("Please, input a valid address.");
						break;
					default:
						text = _("Type error");
						break;
				}

				manager_object->content = elm_popup_add(manager_object->win);
				evas_object_size_hint_weight_set(manager_object->content, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
				elm_object_text_set(manager_object->content, text);
				Evas_Object *btn_ok = elm_button_add(manager_object->content);
				elm_object_text_set(btn_ok, sc(PACKAGE, I18N_TYPE_Ok));
				elm_object_part_content_set(manager_object->content, "button1", btn_ok);
				evas_object_smart_callback_add(btn_ok, "clicked", _ok_clicked_cb, manager_object->content);

				evas_object_show(manager_object->content);

			} else if (POPUP_MODE_PIN == mode) {
				manager_object->content = elm_popup_add(manager_object->win);
				evas_object_size_hint_weight_set(manager_object->content, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
				elm_object_text_set(manager_object->content, _("Enter PIN code"));
				Evas_Object *btn_cancel = elm_button_add(manager_object->content);
				elm_object_text_set(btn_cancel, sc(PACKAGE, I18N_TYPE_Cancel));
				elm_object_part_content_set(manager_object->content, "button1", btn_cancel);
				evas_object_smart_callback_add(btn_cancel, "clicked", _ok_clicked_cb, manager_object->content);
				evas_object_show(manager_object->content);

				manager_object->progressbar = elm_progressbar_add(manager_object->content);
				elm_object_style_set(manager_object->progressbar, "list_progress");
				evas_object_size_hint_align_set(manager_object->progressbar, EVAS_HINT_FILL, 0.5);
				evas_object_size_hint_weight_set(manager_object->progressbar, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
				elm_progressbar_unit_format_set(manager_object->progressbar, NULL);
				elm_object_content_set(manager_object->content, manager_object->progressbar);
				timer = ecore_timer_add(0.1, (Ecore_Task_Cb) _fn_pb_timer_bar, manager_object->progressbar);

			} else if (POPUP_MODE_ETC) {
				switch (option) {
				case POPUP_OPTION_ETC_WLAN_STATE_GET_ERROR:
					manager_object->content = elm_popup_add(manager_object->win);
					elm_object_text_set(manager_object->content, _("[ERROR] wlan_client func<br>wlan_client_get_state"));
					Evas_Object *btn_ok = elm_button_add(manager_object->content);
					elm_object_text_set(btn_ok, sc(PACKAGE, I18N_TYPE_Ok));
					elm_object_part_content_set(manager_object->content, "button1", btn_ok);
					evas_object_smart_callback_add(btn_ok, "clicked", _ok_clicked_cb, manager_object->content);
					evas_object_show(manager_object->content);
					break;
				case POPUP_OPTION_ETC_BACK_ENABLE_WHEN_CONNECTED_ERROR:
					manager_object->content = elm_popup_add(manager_object->win);
					evas_object_size_hint_weight_set(manager_object->content, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
					elm_object_text_set(manager_object->content, "Wi-Fi is not connected yet<br>Will you out of Wi-Fi?");
					Evas_Object *btn_yes = elm_button_add(manager_object->content);
					elm_object_text_set(btn_yes, sc(PACKAGE, I18N_TYPE_Yes));
					elm_object_part_content_set(manager_object->content, "button1", btn_yes);
					evas_object_smart_callback_add(btn_yes, "clicked", _popup_out_of_wifi_yes_cb, manager_object->content);
					Evas_Object *btn_no = elm_button_add(manager_object->content);
					elm_object_text_set(btn_no, sc(PACKAGE, I18N_TYPE_No));
					elm_object_part_content_set(manager_object->content, "button1", btn_no);
					evas_object_smart_callback_add(btn_no, "clicked", _popup_out_of_wifi_no_cb, manager_object->content);
					evas_object_show(manager_object->content);
					break;
				case POPUP_OPTION_NONE:
					break;
				default:
					break;
				}
			}
			break;
		default:
			if (POPUP_MODE_OFF == mode) {
				if (manager_object->content) {
					evas_object_hide(manager_object->content);
					evas_object_del(manager_object->content);
					manager_object->content = NULL;
				}
			}
			break;
	}
	__COMMON_FUNC_EXIT__;
	return FALSE;
}

/**
 * FIX ME LATER
 *
 * This function had re-factored as elm_popup's bug
 *
 */
int winset_popup_mode_set(void* object, POPUP_MODES mode, POPUP_MODE_OPTIONS option)
{
	_popup_mode = mode;
	_popup_option = option;
	ecore_idler_add( (Ecore_Task_Cb) _winset_popup_mode_set, object);
	return TRUE;
}

POPUP_MODES winset_popup_mode_get(void* object)
{
	return manager_object->mode;
}

int winset_popup_destroy(void* object)
{
	if (timer != NULL) {
		ecore_timer_del(timer);
		timer = NULL;
	}

	if (NULL != manager_object->content) {
		evas_object_del(manager_object->content);
		manager_object->content = NULL;
	}

	g_free(manager_object);
	return TRUE;
}

Evas_Object* winset_popup_content_get(void* object)
{
	return manager_object->content;
}

int winset_popup_content_set(Evas_Object* object)
{
	manager_object->content = object;
	return TRUE;
}

int winset_popup_content_clear()
{
	evas_object_hide(manager_object->content);
	evas_object_del(manager_object->content);
	manager_object->content = NULL;
	return TRUE;
}
