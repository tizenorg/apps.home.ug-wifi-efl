/*
 * Wi-Fi
 *
 * Copyright 2012-2013 Samsung Electronics Co., Ltd
 *
 * Licensed under the Flora License, Version 1.1 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://floralicense.org/license
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifndef __COMMON_UTILS_H__
#define __COMMON_UTILS_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <Elementary.h>

/* Fix build warning (redefine '_()' in appcore-common.h) */
#ifdef _
#undef _
#endif
#include <appcore-common.h>

#include "wlan_manager.h"

typedef void (*imf_ctxt_panel_cb_t)(void *data, Ecore_IMF_Context *ctx, int value);

typedef enum {
	ENTRY_TYPE_USER_ID,
	ENTRY_TYPE_ANONYMOUS_ID,
	ENTRY_TYPE_PASSWORD,
	ENTRY_TYPE_IP_ADDR,
	ENTRY_TYPE_SUBNET_MASK,
	ENTRY_TYPE_GATEWAY,
	ENTRY_TYPE_DNS_1,
	ENTRY_TYPE_DNS_2,
	ENTRY_TYPE_PROXY_ADDR,
	ENTRY_TYPE_PROXY_PORT,
} entry_id_type_t;

typedef struct {
	entry_id_type_t entry_id;
	char *title_txt;
	char *guide_txt;
	char *entry_txt;
	Elm_Object_Item *item;
	imf_ctxt_panel_cb_t input_panel_cb;
	void *input_panel_cb_data;
} common_utils_entry_info_t;

typedef struct {
	char *title_txt;
	char *info_txt;
	char *btn1_txt;
	char *btn2_txt;
	Evas_Smart_Cb btn1_cb;
	Evas_Smart_Cb btn2_cb;
	const void *btn1_data;
	const void *btn2_data;
} popup_btn_info_t;

Elm_Object_Item *common_utils_add_dialogue_separator(
		Evas_Object* genlist, const char *separator_style);
char *common_utils_get_ap_security_type_info_txt(
		const char *pkg_name, wifi_device_info_t *device_info);
void common_utils_get_device_icon(const char *image_path_dir,
		wifi_device_info_t *device_info, char **icon_path);
char *common_utils_get_rssi_text(
		const char *str_pkg_name, int rssi);
Evas_Object *common_utils_entry_layout_get_entry(Evas_Object *layout);
char *common_utils_entry_layout_get_text(Evas_Object *layout);
Evas_Object *common_utils_add_edit_box(Evas_Object *parent,
		common_utils_entry_info_t *entry_info);
void common_utils_set_edit_box_imf_panel_evnt_cb(Elm_Object_Item *item,
		imf_ctxt_panel_cb_t input_panel_cb,	void *user_data);
void common_utils_edit_box_focus_set(Elm_Object_Item *item, Eina_Bool focus_set);

void common_utils_entry_password_set(Evas_Object *layout, Eina_Bool pswd_set);
Elm_Object_Item *common_utils_add_2_line_txt_disabled_item(
		Evas_Object* view_list, const char *style_name,
		const char *line1_txt, const char *line2_txt);
char *common_utils_get_list_item_entry_txt(Elm_Object_Item *entry_item);
Evas_Object *common_utils_create_radio_button(Evas_Object *parent,
		const int value);
Evas_Object *common_utils_create_layout(Evas_Object *navi_frame);
Evas_Object *common_utils_show_info_popup(Evas_Object *win,
		popup_btn_info_t *popup_data);
Evas_Object *common_utils_show_info_ok_popup(Evas_Object *win,
		const char *str_pkg_name, const char *info_txt);
Evas_Object *common_utils_show_info_timeout_popup(Evas_Object *win,
		const char* info_text, const double timeout);
int common_utils_get_rotate_angle(enum appcore_rm rotate_mode);
wlan_security_mode_type_t common_utils_get_sec_mode(
		wifi_security_type_e sec_type);
int common_utils_send_message_to_net_popup(const char *title,
		const char *content, const char *type, const char *ssid);

int common_util_set_system_registry(const char *key, int value);
int common_util_get_system_registry(const char *key);

guint common_util_managed_idle_add(GSourceFunc func, gpointer user_data);
void common_util_managed_idle_cleanup(void);
void common_popup_size_get(Ecore_IMF_Context *target_imf, int *width, int *height);

#ifdef __cplusplus
}
#endif

#endif
