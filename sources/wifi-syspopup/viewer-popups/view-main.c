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



#include "common.h"
#include "view-main.h"
#include "wifi-syspopup.h"
#include "view-password.h"

extern wifi_object* app_state;

static Evas_Object* list = NULL;
static Elm_Genlist_Item_Class itc;

int view_main_item_connection_mode_set(genlist_data *data, ITEM_CONNECTION_MODES mode)
{
	__COMMON_FUNC_ENTER__;

	if (data == NULL) {
		WARN_LOG(SP_NAME_NORMAL, "NULL == data");
		__COMMON_FUNC_EXIT__;
		return FALSE;
	}

	if (ITEM_CONNECTION_MODE_NULL >= mode || ITEM_CONNECTION_MODE_MAX <= mode) {
		ERROR_LOG(SP_NAME_NORMAL, "mode[%d] is not valid", mode);
		__COMMON_FUNC_EXIT__;
		return FALSE;
	}

	/* mode set */
	data->connection_mode = mode;
	INFO_LOG(SP_NAME_NORMAL, "mode: [%d]", data->connection_mode);

	elm_genlist_item_update(data->it);

	__COMMON_FUNC_EXIT__;

	return TRUE;
}

static ITEM_CONNECTION_MODES view_main_state_get(Evas_Object *glist)
{
	__COMMON_FUNC_ENTER__;

	if (glist == NULL)
		return ITEM_CONNECTION_MODE_NULL;

	Elm_Object_Item* it = NULL;
	it = elm_genlist_first_item_get(glist);
	while (it) {
		genlist_data *gdata = (genlist_data *)elm_object_item_data_get(it);
		if (gdata->connection_mode == ITEM_CONNECTION_MODE_CONNECTING) {
			return ITEM_CONNECTION_MODE_CONNECTING;
		}

		it = elm_genlist_item_next_get(it);
	}

	__COMMON_FUNC_EXIT__;
	return ITEM_CONNECTION_MODE_OFF;
}

static void _gl_sel(void *data, Evas_Object *obj, void *event_info)
{
	__COMMON_FUNC_ENTER__;
	if (view_main_state_get(obj) != ITEM_CONNECTION_MODE_OFF) {
		INFO_LOG(SP_NAME_NORMAL, "In connecting state, nothing can do" );
		__COMMON_FUNC_EXIT__;
		return;
	}

	Elm_Object_Item *item = (Elm_Object_Item *)event_info;

	assertm_if(NULL == data, "data is NULL!!");
	assertm_if(NULL == obj, "obj is NULL!!");
	genlist_data *gdata = (genlist_data*) data;

	int ret = WLAN_MANAGER_ERR_UNKNOWN;

	switch (gdata->connection_mode) {
	case ITEM_CONNECTION_MODE_OFF:
		INFO_LOG( SP_NAME_NORMAL, "item state: off");
		ret = wlan_manager_request_connection(gdata->dev_info);

		break;
	case ITEM_CONNECTION_MODE_CONNECTING:
		INFO_LOG( SP_NAME_NORMAL, "item state: connecting");
		break;
	default:
		ERROR_LOG( SP_NAME_NORMAL, "item state: etc [%d]", gdata->connection_mode);
		break;
	}

	switch (ret) {
	case WLAN_MANAGER_ERR_NONE:
		INFO_LOG( SP_NAME_NORMAL, "ERROR_NONE");
		view_main_item_connection_mode_set(gdata, ITEM_CONNECTION_MODE_CONNECTING);
		break;
	case WLAN_MANAGER_ERR_CONNECT_PASSWORD_NEEDED:
		INFO_LOG( SP_NAME_NORMAL, "Password view will show up");
		view_password_show(gdata);
		break;
	default:
		view_main_item_connection_mode_set(gdata, ITEM_CONNECTION_MODE_OFF);
		view_main_refresh();
		ERROR_LOG( SP_NAME_NORMAL, "errro code [%d]", ret);
		break;
	}

	elm_genlist_item_selected_set(item, 0);
	elm_genlist_item_update(item);

	__COMMON_FUNC_EXIT__;
}

static char *_gl_text_get(void *data, Evas_Object *obj, const char *part)
{
	__COMMON_FUNC_ENTER__;
	assertm_if(NULL == data, "data param is NULL!!");
	assertm_if(NULL == obj, "obj param is NULL!!");
	assertm_if(NULL == part, "part param is NULL!!");

	genlist_data *gdata = (genlist_data *) data;
	const char* ssid_name = gdata->dev_info->ssid;

	if (ssid_name == NULL) {
		__COMMON_FUNC_EXIT__;
		ERROR_LOG(SP_NAME_NORMAL, "ssid name is NULL!!");
		return NULL;
	}
	__COMMON_FUNC_EXIT__;

	return strdup(ssid_name);
}

static char *wifi_get_device_icon(wifi_device_info_t *device_info)
{
	__COMMON_FUNC_ENTER__;
	assertm_if(NULL == device_info, "device param is NULL!!");

	char tmp_str[128] = {0,};
	char* ret;

	sprintf(tmp_str, "%s/37_wifi_icon", WIFI_SP_ICON_PATH);

	if (device_info->security_mode != WLAN_SEC_MODE_NONE) {
		sprintf(tmp_str,"%s_lock", tmp_str);
	}

	switch (wlan_manager_get_signal_strength(device_info->rssi)) {
	case SIGNAL_STRENGTH_TYPE_EXCELLENT:
		sprintf(tmp_str,"%s_03", tmp_str);
		break;
	case SIGNAL_STRENGTH_TYPE_GOOD:
		sprintf(tmp_str,"%s_02", tmp_str);
		break;
	case SIGNAL_STRENGTH_TYPE_WEAK:
		sprintf(tmp_str,"%s_01", tmp_str);
		break;
	case SIGNAL_STRENGTH_TYPE_VERY_WEAK:
	case SIGNAL_STRENGTH_TYPE_NULL:
		sprintf(tmp_str,"%s_00", tmp_str);
		break;
	}

	sprintf(tmp_str, "%s.png", tmp_str);

	ret = strdup(tmp_str);
	__COMMON_FUNC_EXIT__;

	return ret;
}

static Evas_Object *_gl_content_get(void *data, Evas_Object *obj, const char *part)
{
	__COMMON_FUNC_ENTER__;
	if (data == NULL)
		return NULL;

	genlist_data *gdata = (genlist_data *) data;

	const char* ssid_name = gdata->dev_info->ssid;
	INFO_LOG(SP_NAME_NORMAL, "ssid name [%s]", ssid_name);

	char*dev_icon_file = NULL;
	Evas_Object* icon = NULL;

	if (!strncmp(part, "elm.icon.1", strlen(part))) {
		switch (gdata->connection_mode) {
		case ITEM_CONNECTION_MODE_OFF:
			if (gdata->progressbar != NULL) {
				elm_progressbar_pulse(gdata->progressbar, EINA_FALSE);
				evas_object_del(gdata->progressbar);
				gdata->progressbar = NULL;
			}

			icon = elm_icon_add(obj);
			dev_icon_file = wifi_get_device_icon(gdata->dev_info);
			INFO_LOG(SP_NAME_NORMAL, "icon name [%s]", dev_icon_file);
			elm_icon_file_set(icon, dev_icon_file, NULL);
			g_free(dev_icon_file);
			evas_object_size_hint_aspect_set(icon, EVAS_ASPECT_CONTROL_VERTICAL, 5, 5);
			return icon;

		case ITEM_CONNECTION_MODE_CONNECTING:
			gdata->progressbar = elm_progressbar_add(obj);
			if (gdata->progressbar == NULL)
				return NULL;

			elm_object_style_set(gdata->progressbar, "list_process");
			evas_object_size_hint_align_set(gdata->progressbar, EVAS_HINT_FILL, 0.5);
			evas_object_size_hint_weight_set(gdata->progressbar, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
			elm_progressbar_pulse(gdata->progressbar, TRUE);
			return gdata->progressbar;
		default:
			break;
		}
	}

	__COMMON_FUNC_EXIT__;
	return NULL;
}

Evas_Object *view_main_create(Evas_Object* parent)
{
	__COMMON_FUNC_ENTER__;
	assertm_if(NULL == parent, "parent is NULL!!");

	list = elm_genlist_add(parent); //app_state->win_main);
	assertm_if(NULL == list, "list allocation fail!!");

	elm_genlist_mode_set(list, ELM_LIST_LIMIT);
	evas_object_size_hint_weight_set(list, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(list, EVAS_HINT_FILL, EVAS_HINT_FILL);

	__COMMON_FUNC_EXIT__;

	return list;
}

int view_main_destroy(void)
{
	__COMMON_FUNC_ENTER__;
	if(NULL != list) {
		evas_object_del(list);
		list = NULL;
	}

	__COMMON_FUNC_EXIT__;
	return TRUE;
}

static void view_main_scan_ui_clear(void)
{
	__COMMON_FUNC_ENTER__;

	if (list == NULL)
		return;

	Elm_Object_Item* it = NULL;
	it = elm_genlist_first_item_get(list);
	while (it) {
		genlist_data *gdata = (genlist_data *)elm_object_item_data_get(it);
		if (gdata)
			g_free(gdata);

		it = elm_genlist_item_next_get(it);
	}

	elm_genlist_clear(list);

	__COMMON_FUNC_EXIT__;
}

void *view_main_item_set(net_profile_info_t *profile_info)
{
	__COMMON_FUNC_ENTER__;

	wifi_device_info_t *wifi_device = (wifi_device_info_t*)malloc(sizeof(wifi_device_info_t));
	memset(wifi_device, 0, sizeof(wifi_device_info_t));

	wifi_device->profile_name = strdup(profile_info->ProfileName);
	wifi_device->ssid = strdup(profile_info->ProfileInfo.Wlan.essid);
	wifi_device->rssi = (int)profile_info->ProfileInfo.Wlan.Strength;
	wifi_device->security_mode = (int)profile_info->ProfileInfo.Wlan.security_info.sec_mode;
	wifi_device->wps_mode = (int)profile_info->ProfileInfo.Wlan.security_info.wps_support;

	__COMMON_FUNC_EXIT__;
	return wifi_device;
}

int view_main_refresh()
{
	view_main_scan_ui_clear();
	view_main_show();
	return TRUE;
}

int view_main_show(void)
{
	__COMMON_FUNC_ENTER__;

	if (list == NULL) {
		ERROR_LOG( SP_NAME_NORMAL, "list is NULL!!" );
		return FALSE;
	}

	char profile_name[NET_PROFILE_NAME_LEN_MAX+1] = "";
	int state = wlan_manager_state_get(profile_name);
	switch (state) {
	case WLAN_MANAGER_ERROR:
	case WLAN_MANAGER_OFF:
		return FALSE;
	case WLAN_MANAGER_UNCONNECTED:
	case WLAN_MANAGER_CONNECTED:
	case WLAN_MANAGER_DISCONNECTING:
		profile_name[0] = '\0';
		break;
	case WLAN_MANAGER_CONNECTING:
		break;
	}

	view_main_scan_ui_clear();

	net_profile_info_t *profiles_list = wlan_manager_profile_table_get();
	if (profiles_list == NULL)
		return FALSE;

	int profiles_list_size = wlan_manager_profile_scanned_length_get();
	INFO_LOG(SP_NAME_NORMAL, "profiles list count [%d]\n", profiles_list_size);

	itc.item_style = "1text.1icon.4";
	itc.func.text_get = _gl_text_get;
	itc.func.content_get = _gl_content_get;
	itc.func.state_get = NULL;
	itc.func.del = NULL;

	int i = 0;
	wifi_device_info_t *device_info = NULL;
	if (profiles_list_size > 0) {
        if (profile_name[0] == '\0') {
		    for (i = 0; i < profiles_list_size; i++) {
				device_info = (wifi_device_info_t *)view_main_item_set(profiles_list+i);

				genlist_data *gdata = (genlist_data *) malloc(sizeof(genlist_data));
				gdata->dev_info = device_info;
				gdata->connection_mode = ITEM_CONNECTION_MODE_OFF;
				gdata->progressbar = NULL;

				gdata->it = elm_genlist_item_append(list, &itc, gdata, NULL, ELM_GENLIST_ITEM_NONE, _gl_sel, gdata);
				elm_object_item_data_set(gdata->it, gdata);
    		}
        } else {
			for (i = 0; i < profiles_list_size; i++) {
				device_info = (wifi_device_info_t *)view_main_item_set(profiles_list+i);

				genlist_data *gdata = (genlist_data *) malloc(sizeof(genlist_data));
				gdata->dev_info = device_info;
				gdata->progressbar = NULL;
				if (!strcmp((profiles_list+i)->ProfileName, profile_name)) {
					gdata->connection_mode = ITEM_CONNECTION_MODE_CONNECTING;
				} else {
					gdata->connection_mode = ITEM_CONNECTION_MODE_OFF;
				}

				gdata->it = elm_genlist_item_append(list, &itc, gdata, NULL, ELM_GENLIST_ITEM_NONE, _gl_sel, gdata);
				elm_object_item_data_set(gdata->it, gdata);
			}
        }
	} else if (profiles_list_size == 0) {
		WARN_LOG(SP_NAME_NORMAL, "scan size is ZERO");
	}

	evas_object_show(list);
	__COMMON_FUNC_EXIT__;

	return TRUE;
}
