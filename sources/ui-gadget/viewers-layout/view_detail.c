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

#include "wifi.h"
#include "view_detail.h"
#include "i18nmanager.h"
#include "viewer_manager.h"
#include "connman-profile-manager.h"
#include "winset_popup.h"
#include "common_utils.h"
#include "common_ip_info.h"
#include "common_eap_connect.h"
#include "common_datamodel.h"

typedef struct _view_detail_data {
	Evas_Object *layout;
	char *ap_image_path;
	view_datamodel_basic_info_t *data_object;
	eap_info_list_t *eap_info_list;
	ip_info_list_t *ip_info_list;
	Evas_Object *forget_confirm_popup;
	Evas_Object *view_detail_list;
} view_detail_data;

static int view_detail_end = TRUE;

/* function declaration */
static void detailview_sk_cb(void *data, Evas_Object *obj, void *event_info);
static void forget_sk_cb(void *data, Evas_Object *obj, void *event_info);

///////////////////////////////////////////////////////////////
// implementation
///////////////////////////////////////////////////////////////

static char* _view_detail_grouptitle_text_get(void *data, Evas_Object *obj, const char *part)
{
	__COMMON_FUNC_ENTER__;
	char *ret = NULL;
	assertm_if(NULL == obj, "NULL!!");
	assertm_if(NULL == part, "NULL!!");
	assertm_if(NULL == data, "NULL!!");

	view_detail_data *detail_data = (view_detail_data *)data;
	if (!strncmp(part, "elm.text.2", strlen(part))) {
		ret = (char*) g_strdup(sc(PACKAGE, I18N_TYPE_Name));
	} else if (!strncmp(part, "elm.text.1", strlen(part))) {
		ret = view_detail_datamodel_ap_name_get(detail_data->data_object);
	}

	__COMMON_FUNC_EXIT__;
	return ret;
}

static Evas_Object *_view_detail_grouptitle_content_get(void *data, Evas_Object *obj, const char *part)
{
	view_detail_data *detail_data = (view_detail_data *)data;
	Evas_Object* icon = NULL;
	assertm_if(NULL == obj, "NULL!!");
	assertm_if(NULL == data, "NULL!!");
	assertm_if(NULL == part, "NULL!!");


	if (detail_data->ap_image_path == NULL) {
		/* if there is no ap_image_path (NO AP Found situation) */
		DEBUG_LOG(UG_NAME_ERR, "Fatal: Image path is NULL");
	} else if (!strncmp(part, "elm.icon", strlen(part))) {
		/* for strength */
		icon = elm_icon_add(obj);
		assertm_if(NULL == icon, "NULL!!");
		elm_icon_file_set(icon, detail_data->ap_image_path, NULL);
	} else {
		DEBUG_LOG(UG_NAME_NORMAL, "Invalid part name [%s]", part);
	}

	return icon;
}

static void _remove_all(view_detail_data *_detail_data)
{
	__COMMON_FUNC_ENTER__;
	if(_detail_data) {
		evas_object_del(_detail_data->view_detail_list);
		_detail_data->view_detail_list = NULL;
		if (_detail_data->eap_info_list) {
			eap_info_remove(_detail_data->eap_info_list);
			_detail_data->eap_info_list = NULL;
		}

		ip_info_remove(_detail_data->ip_info_list);
		_detail_data->ip_info_list = NULL;
		view_basic_detail_datamodel_destroy(_detail_data->data_object);
		_detail_data->data_object = NULL;
		g_free(_detail_data->ap_image_path);
		g_free(_detail_data);
		_detail_data = NULL;
	}

	__COMMON_FUNC_EXIT__;
}

static void ok_cb(void *data, Evas_Object *obj, void *event_info)
{
	__COMMON_FUNC_ENTER__;
	if(view_detail_end == TRUE) {
		return;
	}
	view_detail_end = TRUE;
	view_detail_data *_detail_data = (view_detail_data *)data;
	assertm_if(NULL == _detail_data, "NULL!!");

	evas_object_del(_detail_data->forget_confirm_popup);
	_detail_data->forget_confirm_popup = NULL;
	char *temp_str = view_detail_datamodel_basic_info_profile_name_get(_detail_data->data_object);
	wlan_manager_forget(temp_str);
	g_free(temp_str);
	_remove_all(_detail_data);
	elm_naviframe_item_pop(viewer_manager_get_naviframe());

	__COMMON_FUNC_EXIT__;
	return;
}

static void cancel_cb(void *data, Evas_Object *obj, void *event_info)
{
	__COMMON_FUNC_ENTER__;
	view_detail_data *_detail_data = (view_detail_data *)data;
	assertm_if(NULL == _detail_data, "NULL!!");
	evas_object_del(_detail_data->forget_confirm_popup);
	_detail_data->forget_confirm_popup = NULL;
	__COMMON_FUNC_EXIT__;
}

static void forget_sk_cb(void *data, Evas_Object *obj, void *event_info)
{
	__COMMON_FUNC_ENTER__;
	view_detail_data *_detail_data = (view_detail_data *)data;
	assertm_if(NULL == _detail_data, "NULL!!");
	if (!_detail_data->forget_confirm_popup) {
		Evas_Object* popup = elm_popup_add(_detail_data->layout);
		evas_object_size_hint_weight_set(popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		char *temp_str = view_detail_datamodel_ap_name_get(_detail_data->data_object);
		char *buffer = g_strdup_printf(sc(PACKAGE, I18N_TYPE_Autonomous_connection_to_s_will_be_turned_off_Continue), temp_str);
		elm_object_text_set(popup, buffer);
		g_free(buffer);
		Evas_Object *btn_ok = elm_button_add(popup);
		elm_object_text_set(btn_ok, sc(PACKAGE, I18N_TYPE_Ok));
		elm_object_part_content_set(popup, "button1", btn_ok);
		evas_object_smart_callback_add(btn_ok, "clicked", ok_cb, _detail_data);
		Evas_Object *btn_cancel = elm_button_add(popup);
		elm_object_text_set(btn_cancel, sc(PACKAGE, I18N_TYPE_Cancel));
		elm_object_part_content_set(popup, "button2", btn_cancel);
		evas_object_smart_callback_add(btn_cancel, "clicked", cancel_cb, _detail_data);
		evas_object_show(popup);
		_detail_data->forget_confirm_popup = popup;
	}

	__COMMON_FUNC_EXIT__;
}

static void detailview_sk_cb(void *data, Evas_Object *obj, void *event_info)
{
	__COMMON_FUNC_ENTER__;

	if (view_detail_end == TRUE)
		return;
	view_detail_end = TRUE;

	view_detail_data *_detail_data = (view_detail_data *)data;
	assertm_if(NULL == _detail_data, "NULL!!");

	if (_detail_data->eap_info_list)
		eap_info_save_data(_detail_data->eap_info_list);

	ip_info_save_data(_detail_data->ip_info_list, TRUE);
	_remove_all(_detail_data);

	__COMMON_FUNC_EXIT__;
}

void view_detail(wifi_device_info_t *device_info)
{
	__COMMON_FUNC_ENTER__;

	int favourite = 0;
	view_datamodel_basic_info_t *data_object = NULL;
	static Elm_Genlist_Item_Class grouptitle_itc;
	if (device_info == NULL) {
		ERROR_LOG(UG_NAME_NORMAL, "Failed : device_info is NULL");
		return;
	}
	Evas_Object *layout = NULL;
	Evas_Object* navi_frame = viewer_manager_get_naviframe();
	if (navi_frame == NULL) {
		ERROR_LOG(UG_NAME_NORMAL, "Failed : get naviframe");
		return;
	}

	view_detail_end = FALSE;

	view_detail_data *_detail_data = (view_detail_data *)g_malloc0(sizeof(view_detail_data));
	assertm_if(NULL == _detail_data, "NULL!!");

	_detail_data->ap_image_path = g_strdup(device_info->ap_image_path);
	_detail_data->data_object = data_object = view_basic_detail_datamodel_create(device_info->profile_name);

	favourite = view_detail_datamodel_is_favourite_get(data_object);

	_detail_data->layout = layout = common_utils_create_conformant_layout(navi_frame);
	evas_object_show(layout);
	Evas_Object *conform = elm_object_part_content_get(layout, "elm.swallow.content");
	Evas_Object* detailview_list = elm_genlist_add(conform);
	elm_object_style_set(detailview_list, "dialogue");
	assertm_if(NULL == detailview_list, "NULL!!");
	_detail_data->view_detail_list = detailview_list;

	grouptitle_itc.item_style = "dialogue/2text.1icon.5";
	grouptitle_itc.func.text_get = _view_detail_grouptitle_text_get;
	grouptitle_itc.func.content_get = _view_detail_grouptitle_content_get;
	grouptitle_itc.func.state_get = NULL;
	grouptitle_itc.func.del = NULL;

	common_utils_add_dialogue_separator(detailview_list, "dialogue/separator");

	/* AP name and signal strength icon */
	Elm_Object_Item* title = elm_genlist_item_append(detailview_list, &grouptitle_itc, _detail_data, NULL, ELM_GENLIST_ITEM_GROUP, NULL, NULL);
	elm_object_item_disabled_set(title, TRUE);

	if (WLAN_SEC_MODE_IEEE8021X == view_detail_datamodel_sec_mode_get(data_object)) {
		if (view_detail_datamodel_is_favourite_get(data_object) ||
		!g_strcmp0(wlan_manager_get_connected_ssid(), view_detail_datamodel_ap_name_get(data_object))) {
			_detail_data->eap_info_list = eap_info_append_items(device_info->profile_name, detailview_list, PACKAGE);
		}
	}

	/* Append ip info list */
	_detail_data->ip_info_list = ip_info_append_items(device_info->profile_name, PACKAGE, detailview_list);

	common_utils_add_dialogue_separator(detailview_list, "dialogue/separator/end");

	elm_object_content_set(conform, detailview_list);

	Elm_Object_Item* navi_it = elm_naviframe_item_push(navi_frame, sc(PACKAGE, I18N_TYPE_Details), NULL, NULL, layout, NULL);
	evas_object_data_set(navi_frame, SCREEN_TYPE_ID_KEY, (void *)VIEW_MANAGER_VIEW_TYPE_DETAIL);

	if (favourite) {
		Evas_Object* toolbar = elm_toolbar_add(navi_frame);
		elm_toolbar_shrink_mode_set(toolbar, ELM_TOOLBAR_SHRINK_EXPAND);

		elm_toolbar_item_append(toolbar,
								NULL,
								sc(PACKAGE, I18N_TYPE_Forget),
								forget_sk_cb,
								_detail_data);
		elm_object_item_disabled_set(elm_toolbar_item_append(toolbar, NULL, NULL, NULL, NULL), EINA_TRUE);
		elm_object_item_part_content_set(navi_it, "controlbar", toolbar);
	}

	Evas_Object* button_back = elm_object_item_part_content_get(navi_it, "prev_btn");
	elm_object_focus_allow_set(button_back, EINA_TRUE);
	evas_object_smart_callback_add(button_back, "clicked", detailview_sk_cb, _detail_data);

	__COMMON_FUNC_EXIT__;
}
