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

#include "common.h"
#include "common_utils.h"
#include "ug_wifi.h"
#include "wlan_manager.h"
#include "view_detail.h"
#include "viewer_list.h"
#include "viewer_manager.h"
#include "appcoreWrapper.h"
#include "i18nmanager.h"

#define LIST_ITEM_CONNECTED_AP_FONT_SIZE	28
#define LIST_ITEM_CONNECTED_AP_FONT_COLOR	"#3B73B6"
#define FIRST_ITEM_NUMBER					8


static Evas_Object* viewer_list = NULL;
static Elm_Object_Item* first_item = NULL;
static Elm_Object_Item* last_item = NULL;

static Elm_Genlist_Item_Class itc;
static Elm_Genlist_Item_Class grouptitle_itc;
static Elm_Object_Item* grouptitle = NULL;

extern wifi_appdata *ug_app_state;

void list_select_cb(void *data, Evas_Object *obj, void *event_info)
{
	__COMMON_FUNC_ENTER__;

	INFO_LOG(UG_NAME_NORMAL,"=================\n");
	INFO_LOG(UG_NAME_NORMAL," %s %d\n", __func__ ,__LINE__);
	INFO_LOG(UG_NAME_NORMAL,"=================\n");

	if (data == NULL) {
		__COMMON_FUNC_EXIT__;
		return;
	}

	wifi_device_info_t *device_info = (wifi_device_info_t *)data;

	DEBUG_LOG(UG_NAME_NORMAL, "ssid [%s]", device_info->ssid);

	view_detail(device_info, ug_app_state->layout_main);

	__COMMON_FUNC_EXIT__;
}

static char* _gl_listview_text_get(void *data, Evas_Object *obj, const char *part)
{
	char* det = NULL;
	assertm_if(NULL == obj, "NULL!!");
	assertm_if(NULL == part, "NULL!!");
	assertm_if(NULL == data, "NULL!!");

	ug_genlist_data_t* gdata = (ug_genlist_data_t*) data;
	assertm_if(NULL == gdata, "NULL!!");
	assertm_if(NULL == gdata->device_info, "NULL!!");
	assertm_if(NULL == gdata->device_info->ssid, "NULL!!");
	assertm_if(NULL == gdata->device_info->ap_status_txt, "NULL!!");

	if (!strncmp(part, "elm.text.1", strlen(part))) {
		det = g_strdup(gdata->device_info->ssid);
		assertm_if(NULL == det, "NULL!!");
	} else if (!strncmp(part, "elm.text.2", strlen(part))) {
		det = g_strdup(gdata->device_info->ap_status_txt);
		assertm_if(NULL == det, "NULL!!");
	}
	return det;
}

static Evas_Object *_gl_listview_content_get(void *data, Evas_Object *obj, const char *part)
{
	assertm_if(NULL == obj, "NULL!!");
	assertm_if(NULL == data, "NULL!!");
	assertm_if(NULL == part, "NULL!!");

	ug_genlist_data_t* gdata = (ug_genlist_data_t*) data;
	Evas_Object* icon = NULL;

	if (gdata->device_info->ap_image_path == NULL) {
		/* if there is no ap_image_path (NO AP Found situation) */
		DEBUG_LOG(UG_NAME_ERR, "Fatal: Image path is NULL");
	} else if (!strncmp(part, "elm.icon.1", strlen(part))) {
		/* for strength */
		icon = elm_image_add(obj);
		assertm_if(NULL == icon, "NULL!!");
		elm_image_file_set(icon, gdata->device_info->ap_image_path, NULL);
	} else if (!strncmp(part, "elm.icon.2", strlen(part))) {
		if (VIEWER_ITEM_RADIO_MODE_CONNECTING == gdata->radio_mode) {
			icon = elm_progressbar_add(obj);
			elm_object_style_set(icon, "list_process");
			evas_object_size_hint_align_set(icon, EVAS_HINT_FILL, 0.5);
			evas_object_size_hint_weight_set(icon, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
			elm_progressbar_pulse(icon, TRUE);
		} else {
			icon = elm_button_add(obj);
			assertm_if(NULL == icon, "NULL!!");
			elm_object_style_set(icon, "reveal");
			evas_object_smart_callback_add(icon, "clicked", (Evas_Smart_Cb)list_select_cb, gdata->device_info);
			evas_object_propagate_events_set(icon, EINA_FALSE);
		}
	}

	return icon;
}

static void _gl_listview_del(void* data, Evas_Object* obj)
{
	if (data == NULL)
		return;

	ug_genlist_data_t* gdata = (ug_genlist_data_t*) data;
	assertm_if(NULL == gdata->device_info->ssid, "NULL!!");

	DEBUG_LOG(UG_NAME_NORMAL, "del target ssid:[%s]", gdata->device_info->ssid);

	g_free(gdata->device_info->ap_image_path);
	g_free(gdata->device_info->ap_status_txt);
	g_free(gdata->device_info->ssid);
	wifi_ap_destroy(gdata->device_info->ap);
	g_free(gdata->device_info);
	g_free(gdata);

	return;
}

static char* _gl_text_title_get(void *data, Evas_Object *obj, const char *part)
{
	assertm_if(NULL == obj, "NULL!!");
	assertm_if(NULL == part, "NULL!!");

	return (char*) g_strdup(sc(PACKAGE, I18N_TYPE_WiFi_network));
}

static Evas_Object *_gl_content_title_get(void *data, Evas_Object *obj, const char *part)
{
	__COMMON_FUNC_ENTER__;
	Evas_Object *title_progressbar = NULL;
	if (HEADER_MODE_SEARCHING == viewer_manager_header_mode_get()) {
		if (!strcmp(part, "elm.icon")) {
			title_progressbar  = elm_progressbar_add(obj);
			elm_object_style_set(title_progressbar, "list_process_small");
			elm_progressbar_horizontal_set(title_progressbar, EINA_TRUE);
			elm_progressbar_pulse(title_progressbar, EINA_TRUE);
		}
	}
	__COMMON_FUNC_EXIT__;
	return title_progressbar;
}


Elm_Object_Item* viewer_list_get_first_item(void)
{
	return first_item;
}

static Elm_Object_Item* viewer_list_get_last_item(void)
{
	return last_item;
}

static void _gl_realized(void *data, Evas_Object *obj, void *event_info)
{
	HEADER_MODES header_mode = viewer_manager_header_mode_get();

	if (header_mode == HEADER_MODE_ACTIVATING ||
		header_mode == HEADER_MODE_DEACTIVATING ||
		header_mode == HEADER_MODE_OFF)
		return ;

	Elm_Object_Item *item = (Elm_Object_Item *)event_info;
	int index = (int)elm_genlist_item_index_get(item);
	int first_item_index = (int)elm_genlist_item_index_get(viewer_list_get_first_item());
	int last_item_index = (int)elm_genlist_item_index_get(viewer_list_get_last_item());

	if (last_item_index == FIRST_ITEM_NUMBER)
		return ;

	if (first_item_index == -1) {
		int group_index = (int)elm_genlist_item_index_get(grouptitle);
		first_item_index = group_index+1;
	}

	if (first_item_index <= index) {
		if(index == first_item_index)
			elm_object_item_signal_emit(item, "elm,state,top", "");
		else if (index == last_item_index)
			elm_object_item_signal_emit(item, "elm,state,bottom", "");
		else
			elm_object_item_signal_emit(item, "elm,state,center", "");
	}

	elm_genlist_item_update(item);

	return;
}

static void _popup_cancel_cb(void *data,  Evas_Object *obj, void *event_info)
{
	__COMMON_FUNC_ENTER__;
	if (!ug_app_state->passpopup) {
		return;
	}

	common_pswd_popup_destroy(ug_app_state->passpopup);
	ug_app_state->passpopup = NULL;
	__COMMON_FUNC_EXIT__;
}

static void _popup_ok_cb(void *data, Evas_Object *obj, void *event_info)
{
	__COMMON_FUNC_ENTER__;
	if (!ug_app_state->passpopup) {
		return;
	}

	wifi_security_type_e sec_mode = 0;
	wifi_ap_h ap = common_pswd_popup_get_ap(ug_app_state->passpopup);
	int ret = WLAN_MANAGER_ERR_NONE;
	int nLen = 0;
	const char* szPassword = common_pswd_popup_get_txt(ug_app_state->passpopup);
	nLen = strlen(szPassword);
	INFO_LOG(UG_NAME_NORMAL, "password = [%s]", szPassword);

	wifi_ap_get_security_type(ap, &sec_mode);
	switch (sec_mode) {
	case WIFI_SECURITY_TYPE_WEP:

		if (nLen != 5 && nLen != 13 && nLen != 26 && nLen != 10) {
			winset_popup_mode_set(ug_app_state->popup_manager, POPUP_OPTION_WEP_PSWD_LEN_ERROR, NULL);
			goto popup_ok_cb_exit;
		}
		break;

	case WIFI_SECURITY_TYPE_WPA_PSK:
	case WIFI_SECURITY_TYPE_WPA2_PSK:

		if (nLen < 8 || nLen > 63) {
			winset_popup_mode_set(ug_app_state->popup_manager, POPUP_OPTION_WPA_PSWD_LEN_ERROR, NULL);
			goto popup_ok_cb_exit;
		}
		break;

	default:
		ERROR_LOG(UG_NAME_SCAN, "Fatal: Wrong security mode : %d", sec_mode);
		common_pswd_popup_destroy(ug_app_state->passpopup);
		ug_app_state->passpopup = NULL;
		goto popup_ok_cb_exit;
	}

	INFO_LOG(UG_NAME_SCAN, "connect with password comp");
	ret = wlan_manager_connect_with_password(ap, szPassword);
	if (WLAN_MANAGER_ERR_NONE == ret) {
		viewer_manager_header_mode_set(HEADER_MODE_CONNECTING);
	} else {
		ERROR_LOG(UG_NAME_SCAN, "wlan error %d", ret);
		viewer_manager_header_mode_set(HEADER_MODE_ON);
	}

	common_pswd_popup_destroy(ug_app_state->passpopup);
	ug_app_state->passpopup = NULL;


popup_ok_cb_exit:
	g_free((gpointer)szPassword);

	__COMMON_FUNC_EXIT__;
}

static void _wps_pbc_popup_cancel_connecting(void* data, Evas_Object* obj, void* event_info)
{
	if (!ug_app_state->passpopup) {
		return;
	}

	wifi_ap_h ap = common_pswd_popup_get_ap(ug_app_state->passpopup);;
	int ret = wlan_manager_request_disconnection(ap);
	if (ret == WLAN_MANAGER_ERR_NONE) {
		INFO_LOG(UG_NAME_NORMAL, "WPS conection cancelled successfully for AP[0x%x]", ap);
	} else {
		ERROR_LOG(UG_NAME_NORMAL, "Error!!! wlan_manager_request_disconnection failed for AP[0x%x]", ap);
	}
	common_pswd_popup_destroy(ug_app_state->passpopup);
	ug_app_state->passpopup = NULL;
	viewer_manager_header_mode_set(HEADER_MODE_ON);
	return;
}

static void _wps_btn_cb(void* data, Evas_Object* obj, void* event_info)
{
	__COMMON_FUNC_ENTER__;
	if (!ug_app_state->passpopup) {
		return;
	}

	wifi_ap_h ap = common_pswd_popup_get_ap(ug_app_state->passpopup);
	int ret = wlan_manager_request_wps_connection(ap);
	if (ret == WLAN_MANAGER_ERR_NONE) {
		viewer_manager_header_mode_set(HEADER_MODE_CONNECTING);
		common_pswd_popup_pbc_popup_create(ug_app_state->passpopup, _wps_pbc_popup_cancel_connecting, NULL);
	} else {
		ERROR_LOG(UG_NAME_NORMAL, "Error!!! wlan_manager_request_wps_connection failed");
		common_pswd_popup_destroy(ug_app_state->passpopup);
		ug_app_state->passpopup = NULL;
	}
	__COMMON_FUNC_EXIT__;
}

#if 0
static gint __viewer_list_compare_mode(gconstpointer a, gconstpointer b)
{
	Elm_Object_Item *item = (Elm_Object_Item *)a;
	if (item) {
		ug_genlist_data_t* gdata = elm_object_item_data_get(item);
		if (gdata) {
			VIEWER_ITEM_RADIO_MODES mode = (VIEWER_ITEM_RADIO_MODES)b;
			if (gdata->radio_mode == mode)
				return 0;
		}
	}
	return -1;
}

static Elm_Object_Item *__wifi_viewer_list_get_item_in_mode(VIEWER_ITEM_RADIO_MODES mode)
{
	Elm_Object_Item *item = NULL;
	GSList *found = g_slist_find_custom(container, (gconstpointer)mode, __viewer_list_compare_mode);
	if (found) {
		INFO_LOG(UG_NAME_NORMAL, "item found in mode [%d]", mode);
		item = found->data;
	}
	return item;
}
#endif

static void __wifi_viewer_list_request_connection(wifi_device_info_t *device_info)
{
	if (!device_info)
		return;

	pswd_popup_create_req_data_t popup_info;
	Evas_Object* navi_frame = NULL;
	int ret = wlan_manager_request_connection(device_info->ap);;
	switch (ret) {
	case WLAN_MANAGER_ERR_NONE:
		INFO_LOG(UG_NAME_NORMAL, "ERROR_NONE");
		viewer_manager_header_mode_set(HEADER_MODE_CONNECTING);
		break;
	case WLAN_MANAGER_ERR_CONNECT_PASSWORD_NEEDED:
		memset(&popup_info, 0, sizeof(pswd_popup_create_req_data_t));
		popup_info.title = device_info->ssid;
		popup_info.ok_cb = _popup_ok_cb;
		popup_info.cancel_cb = _popup_cancel_cb;
		popup_info.show_wps_btn = device_info->wps_mode;
		popup_info.wps_btn_cb = _wps_btn_cb;
		popup_info.ap = device_info->ap;
		popup_info.cb_data = NULL;
		INFO_LOG(UG_NAME_NORMAL, "Going to create a popup. ug_app_state = 0x%x", ug_app_state);
		ug_app_state->passpopup = common_pswd_popup_create(ug_app_state->layout_main, PACKAGE, &popup_info);
		INFO_LOG(UG_NAME_NORMAL, "After create a popup");
		if (ug_app_state->passpopup == NULL) {
			INFO_LOG(UG_NAME_ERR, "pass popup create failed !");
		}
		break;

	case WLAN_MANAGER_ERR_CONNECT_EAP_SEC_TYPE:
		navi_frame = viewer_manager_get_naviframe();
		ug_app_state->eap_view = create_eap_connect_view(ug_app_state->layout_main, navi_frame, PACKAGE, device_info);
		break;

	default:
		ERROR_LOG(UG_NAME_NORMAL, "errro code [%d]", ret);
		break;
	}
}

static void viewer_list_item_clicked_cb(void* data, Evas_Object* obj, void* event_info)
{
	__COMMON_FUNC_ENTER__;
	assertm_if(NULL == event_info, "event_info is NULL!!");
	assertm_if(NULL == data, "data is NULL!!");
	assertm_if(NULL == obj, "obj is NULL!!");

	int ret = WLAN_MANAGER_ERR_UNKNOWN;
	Elm_Object_Item *it = (Elm_Object_Item *)event_info;
//	Elm_Object_Item *connecting_item = NULL;
	ug_genlist_data_t *gdata = (ug_genlist_data_t *)elm_object_item_data_get(it);
	wifi_device_info_t *device_info = (wifi_device_info_t *)data;

	if (!gdata || !device_info || !device_info->ssid) {
		ERROR_LOG(UG_NAME_NORMAL, "Error!!! Invalid inout params");
		__COMMON_FUNC_EXIT__;
		return;
	}

	int item_state = gdata->radio_mode;
	int current_state = 0;

	INFO_LOG(UG_NAME_NORMAL, "ssid --- %s", device_info->ssid);
	INFO_LOG(UG_NAME_NORMAL, "ap --- 0x%x", device_info->ap);
	INFO_LOG(UG_NAME_NORMAL, "current item_state state is --- %d\n", item_state);

	switch (item_state) {
		case VIEWER_ITEM_RADIO_MODE_OFF:
			current_state = viewer_manager_header_mode_get();

			INFO_LOG(UG_NAME_NORMAL, "Clicked AP`s information\n");
			INFO_LOG(UG_NAME_NORMAL, "header mode [%d]", current_state);

			switch (current_state) {
				case HEADER_MODE_CONNECTED:
				case HEADER_MODE_ON:
					__wifi_viewer_list_request_connection(device_info);
					break;

				case HEADER_MODE_CONNECTING:
#if 0	// Enable this later when two connect requests is supported by libnet.
					/* Try connecting to the selected AP */
					connecting_item = __wifi_viewer_list_get_item_in_mode(VIEWER_ITEM_RADIO_MODE_CONNECTING);
					viewer_manager_header_mode_set(HEADER_MODE_ON);
					wlan_manager_request_connection(device_info->ap);

					/* Disconnect the connecting AP */
					if (connecting_item) {
						ug_genlist_data_t *connecting_gdata = elm_object_item_data_get(connecting_item);
						if (connecting_gdata) {
							wlan_manager_request_disconnection(connecting_gdata->device_info->ap);
						}
					}
					break;
#endif
				case HEADER_MODE_OFF:
				case HEADER_MODE_SEARCHING:
				case HEADER_MODE_ACTIVATING:
				case HEADER_MODE_DISCONNECTING:
				case HEADER_MODE_DEACTIVATING:
				default:
					INFO_LOG(UG_NAME_NORMAL, "Ignore the item click");
					break;
			}
			break;

		case VIEWER_ITEM_RADIO_MODE_CONNECTED:
			INFO_LOG(UG_NAME_NORMAL, "want to disconnect for connected item");
			ret = wlan_manager_request_disconnection(device_info->ap);
			if(ret == WLAN_MANAGER_ERR_NONE){
				viewer_manager_header_mode_set(HEADER_MODE_DISCONNECTING);
			}
			break;

		case VIEWER_ITEM_RADIO_MODE_CONNECTING:
			INFO_LOG(UG_NAME_NORMAL, "want to cancel connecting for connected item");
			ret = wlan_manager_request_disconnection(device_info->ap);
			if(ret == WLAN_MANAGER_ERR_NONE){
				viewer_manager_header_mode_set(HEADER_MODE_CANCEL_CONNECTING);
			}
			break;

		default:
			ret = WLAN_MANAGER_ERR_UNKNOWN;
			break;
	}

	elm_genlist_item_selected_set((Elm_Object_Item *)event_info, EINA_FALSE);

	__COMMON_FUNC_EXIT__;
	return;
}

static char *viewer_list_get_device_status_txt(wifi_device_info_t *wifi_device, VIEWER_ITEM_RADIO_MODES mode)
{
	char *status_txt = NULL;
	char *ret_txt = NULL;
	/* The strings are currently hard coded. It will be replaced with string ids later */
	if (VIEWER_ITEM_RADIO_MODE_CONNECTING == mode || VIEWER_ITEM_RADIO_MODE_WPS_CONNECTING == mode) {
		status_txt = g_strdup(sc(PACKAGE, I18N_TYPE_Connecting));
	} else if (VIEWER_ITEM_RADIO_MODE_CONNECTED == mode) {
		status_txt = g_strdup_printf("<color=%s><b>%s</b></color>", LIST_ITEM_CONNECTED_AP_FONT_COLOR, sc(PACKAGE, I18N_TYPE_Connected));
	} else if (VIEWER_ITEM_RADIO_MODE_DISCONNECTING == mode) {
		status_txt = g_strdup(sc(PACKAGE, I18N_TYPE_Disconnecting));
	} else if (VIEWER_ITEM_RADIO_MODE_OFF == mode) {
		status_txt = common_utils_get_ap_security_type_info_txt(PACKAGE, wifi_device);
	} else {
		status_txt = g_strdup(WIFI_UNKNOWN_DEVICE_STATUS_STR);
		INFO_LOG(UG_NAME_NORMAL, "Invalid mode: %d", mode);
	}
	ret_txt = g_strdup_printf("%s", status_txt);
	g_free(status_txt);
	return ret_txt;
}

Evas_Object* viewer_list_create(Evas_Object *win)
{
	__COMMON_FUNC_ENTER__;

	assertm_if(NULL == win, "NULL!!");

	assertm_if(NULL != viewer_list, "Err!!");
	viewer_list = elm_genlist_add(win);


	elm_object_style_set(viewer_list, "dialogue");
	assertm_if(NULL == viewer_list, "NULL!!");

	elm_genlist_mode_set(viewer_list, ELM_LIST_LIMIT);

	evas_object_size_hint_weight_set(viewer_list, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(viewer_list, EVAS_HINT_FILL, EVAS_HINT_FILL);

	itc.item_style = "dialogue/2text.2icon.3.tb";
	itc.func.text_get = _gl_listview_text_get;
	itc.func.content_get = _gl_listview_content_get;
	itc.func.state_get = NULL;
	itc.func.del = _gl_listview_del;

	first_item = last_item = NULL;

	evas_object_smart_callback_add(viewer_list, "realized", _gl_realized, NULL);
	__COMMON_FUNC_EXIT__;
	return viewer_list;
}

int viewer_list_destroy(void)
{
	__COMMON_FUNC_ENTER__;

	assertm_if(NULL == viewer_list, "NULL!!");
	viewer_list_item_clear();
	evas_object_del(viewer_list);
	viewer_list = NULL;

	__COMMON_FUNC_EXIT__;
	return TRUE;
}

void viewer_list_title_item_del()
{
	if (grouptitle) {
		elm_object_item_del(grouptitle);
		grouptitle = NULL;
	}
}

void viewer_list_title_item_update()
{
	elm_genlist_item_update(grouptitle);
}

int viewer_list_title_item_set(void)
{
	if (viewer_list_item_size_get() == 0 && !grouptitle) {
		// To use multiline textblock/entry/editfield in genlist, set height_for_width mode
		// then the item's height is calculated while the item's width fits to genlist width.
		elm_genlist_mode_set(viewer_list, ELM_LIST_COMPRESS);

		grouptitle_itc.item_style = "dialogue/title";
		grouptitle_itc.func.text_get = _gl_text_title_get;
		grouptitle_itc.func.content_get = _gl_content_title_get;
		grouptitle_itc.func.state_get = NULL;
		grouptitle_itc.func.del = NULL;

		assertm_if(NULL != grouptitle, "Err!!");

		grouptitle = elm_genlist_item_append(viewer_list,
				&grouptitle_itc,
				NULL,
				NULL,
				ELM_GENLIST_ITEM_NONE,
				NULL,
				NULL);

		assertm_if(NULL == grouptitle, "NULL!!");

		elm_genlist_item_select_mode_set(grouptitle, ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);

		return TRUE;
	} else {
		return FALSE;
	}
}

int viewer_list_item_radio_mode_set(Elm_Object_Item* item, VIEWER_ITEM_RADIO_MODES mode)
{
	__COMMON_FUNC_ENTER__;
	if (NULL == item) {
		INFO_LOG(COMMON_NAME_ERR, "item is NULL");
		return FALSE;
	}

	ug_genlist_data_t* gdata = (ug_genlist_data_t *) elm_object_item_data_get(item);
	if (NULL == gdata || NULL == gdata->device_info) {
		INFO_LOG(COMMON_NAME_ERR, "gdata or device_info is NULL");
		return FALSE;
	}

	if (gdata->radio_mode == mode) {
		INFO_LOG(UG_NAME_NORMAL, "[%s] is already in requested state", gdata->device_info->ssid);
		return FALSE;
	}

	INFO_LOG(UG_NAME_NORMAL, "[%s] AP Item State Transition from [%d] --> [%d]", gdata->device_info->ssid, gdata->radio_mode, mode);
	gdata->radio_mode = mode;
	if (gdata->device_info->ap_status_txt) {
		g_free(gdata->device_info->ap_status_txt);
		gdata->device_info->ap_status_txt = viewer_list_get_device_status_txt(gdata->device_info, mode);
	}

	elm_genlist_item_update(item);

	__COMMON_FUNC_EXIT__;
	return TRUE;
}

Elm_Object_Item* viewer_list_item_insert_after(wifi_ap_h ap, Elm_Object_Item *after)
{
	Elm_Object_Item* ret = NULL;
	wifi_security_type_e sec_type;

	if (!viewer_list) {
		assertm_if(NULL == viewer_list, "NULL!!");
		return NULL;
	}

	wifi_device_info_t *wifi_device = NULL;

	if (!ap) {
		wifi_device = wlan_manager_profile_device_info_blank_create();
		if (!wifi_device)
			return NULL;
	} else {
		wifi_device = g_new0(wifi_device_info_t, 1);
		if (WIFI_ERROR_NONE != wifi_ap_clone(&(wifi_device->ap), ap)) {
			goto FREE_DEVICE_INFO;
		} else if (WIFI_ERROR_NONE != wifi_ap_get_essid(ap, &(wifi_device->ssid))) {
			goto FREE_DEVICE_INFO;
		} else if (WIFI_ERROR_NONE != wifi_ap_get_rssi(ap, &(wifi_device->rssi))) {
			goto FREE_DEVICE_INFO;
		} else if (WIFI_ERROR_NONE != wifi_ap_get_security_type (ap, &sec_type)) {
			goto FREE_DEVICE_INFO;
		} else if (WIFI_ERROR_NONE != wifi_ap_is_wps_supported (ap, &(wifi_device->wps_mode))) {
			goto FREE_DEVICE_INFO;
		}
		wifi_device->security_mode = common_utils_get_sec_mode(sec_type);
		wifi_device->ap_image_path = common_utils_get_device_icon(WIFI_APP_IMAGE_DIR, wifi_device);
		wifi_device->ap_status_txt = viewer_list_get_device_status_txt(wifi_device, VIEWER_ITEM_RADIO_MODE_OFF);
	}
	ug_genlist_data_t* gdata = g_new0(ug_genlist_data_t, 1);
	gdata->device_info = wifi_device;
	gdata->radio_mode = VIEWER_ITEM_RADIO_MODE_OFF;

	if (!after) {	/* If the after item is NULL then insert it as first item */
		after = grouptitle;
	}

	ret = elm_genlist_item_insert_after(
			viewer_list, /*obj*/
			&itc,/*itc*/
			gdata,/*data*/
			NULL,/*parent*/
			after, /*after than*/
			ELM_GENLIST_ITEM_NONE, /*flags*/
			viewer_list_item_clicked_cb,/*func*/
			wifi_device);/*func_data*/

	if (!ret) {
		assertm_if(NULL == ret, "NULL!!");
		g_free(gdata);
	} else {
		DEBUG_LOG(UG_NAME_NORMAL,
				"* item add complete item [0x%x] ssid:[%s] security[%d] size:[%d]",
				ret,
				wifi_device->ssid,
				wifi_device->security_mode,
				viewer_list_item_size_get());

		if (after == grouptitle) {
			first_item = ret;
			if (!last_item)
				last_item = ret;
		} else {
			last_item = ret;
			if (!first_item)
				first_item = ret;
		}

		elm_genlist_item_update(ret);
	}

FREE_DEVICE_INFO:
	if (!ret && wifi_device) {
		wifi_ap_destroy(wifi_device->ap);
		g_free(wifi_device->ap_image_path);
		g_free(wifi_device->ap_status_txt);
		g_free(wifi_device->ssid);
		g_free(wifi_device);
	}

	return ret;
}

void viewer_list_item_del(Elm_Object_Item *it)
{
	if (it == NULL)
		return;

	if (it == first_item) {
		first_item = elm_genlist_item_next_get(first_item);
	} else if (it == last_item) {
		last_item = elm_genlist_item_prev_get(last_item);
	}
	elm_object_item_del(it);
}

int viewer_list_item_size_get()
{
	__COMMON_FUNC_ENTER__;
	int ret = 0;
	Elm_Object_Item *it = first_item;

	while(it) {
		ret++;
		if (it == last_item)
			break;
		it = elm_genlist_item_next_get(it);
	}

	__COMMON_FUNC_EXIT__;
	return ret;
}

void viewer_list_item_clear(void)
{
	__COMMON_FUNC_ENTER__;

	Elm_Object_Item *it = first_item;
	Elm_Object_Item *nxt = NULL;

	while(it) {
		nxt = elm_genlist_item_next_get(it);
		elm_object_item_del(it);
		if (it == last_item)
			break;
		it = nxt;
	}

	first_item = last_item = NULL;

	__COMMON_FUNC_EXIT__;
}

void viewer_list_item_enable_all(void)
{
	__COMMON_FUNC_ENTER__;

	Elm_Object_Item *it = first_item;

	while(it) {
		elm_object_item_disabled_set(it, EINA_FALSE);
		if (it == last_item)
			break;
		it = elm_genlist_item_next_get(it);
	}

	__COMMON_FUNC_EXIT__;
}

void viewer_list_item_disable_all(void)
{
	__COMMON_FUNC_ENTER__;

	Elm_Object_Item *it = first_item;

	while(it) {
		elm_object_item_disabled_set(it, EINA_TRUE);
		if (it == last_item)
			break;
		it = elm_genlist_item_next_get(it);
	}

	__COMMON_FUNC_EXIT__;
}

Elm_Object_Item* item_get_for_ap(wifi_ap_h ap)
{
	__COMMON_FUNC_ENTER__;
	if (!ap) {
		__COMMON_FUNC_EXIT__;
		return NULL;
	}
	
	char *essid = NULL;
	wifi_security_type_e type = WIFI_SECURITY_TYPE_NONE;

	if (WIFI_ERROR_NONE != wifi_ap_get_essid(ap, &essid)) {
		__COMMON_FUNC_EXIT__;
		return NULL;
	} else if (WIFI_ERROR_NONE != wifi_ap_get_security_type(ap, &type)) {
		__COMMON_FUNC_EXIT__;
		return NULL;
	}

	Elm_Object_Item *it = first_item;
	wlan_security_mode_type_t sec_mode = common_utils_get_sec_mode(type);
	while(it) {
		ug_genlist_data_t* gdata = elm_object_item_data_get(it);
		wifi_device_info_t *device_info = NULL;
		if (gdata && (device_info = gdata->device_info)) {
			if (!g_strcmp0(device_info->ssid, essid) && device_info->security_mode == sec_mode)
				break;
		}
		if (it == last_item) {
			it = NULL;
			break;
		}
		it = elm_genlist_item_next_get(it);
	}

	g_free(essid);
	__COMMON_FUNC_EXIT__;
	return it;
}
