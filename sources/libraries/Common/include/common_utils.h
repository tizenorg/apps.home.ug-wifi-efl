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


#ifndef __COMMON_UTILS_H_
#define __COMMON_UTILS_H_
#include "wlan_manager.h"

#define COMMON_UTILS_DEFAULT_ENTRY_TEXT_KEY		"common_utils_default_entry_text_key"

typedef struct {
	char *title;
	char *entry_txt;
	char *guide_txt;
	Elm_Input_Panel_Layout panel_type;
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

Elm_Object_Item* common_utils_add_dialogue_separator(Evas_Object* genlist, const char *separator_style);
char *common_utils_get_ap_security_type_info_txt(const char *pkg_name, wifi_device_info_t *device_info);
char *common_utils_get_device_icon(const char *image_path_dir, wifi_device_info_t *device_info);
Evas_Object *common_utils_entry_layout_get_entry(Evas_Object *layout);
char *common_utils_entry_layout_get_text(Evas_Object *layout);
void common_utils_set_entry_info(common_utils_entry_info_t *entry_info, char *title, char *entry_txt, char *guide_txt, Elm_Input_Panel_Layout panel_type);
Evas_Object *common_utils_add_edit_box(Evas_Object *parent, const common_utils_entry_info_t *entry_info);
void common_utils_entry_password_set(Evas_Object *layout, Eina_Bool pswd_set);
Elm_Object_Item *common_utils_add_2_line_txt_disabled_item(Evas_Object* view_list, const char *style_name, const char *line1_txt, const char *line2_txt);
Elm_Object_Item *common_utils_add_edit_box_to_list(Evas_Object *list, Elm_Object_Item *insert_after, char *title, char *entry_txt, char *guide_txt, Elm_Input_Panel_Layout panel_type);
char *common_utils_get_list_item_entry_txt(Elm_Object_Item *entry_item);
Evas_Object *common_utils_create_radio_button(Evas_Object *parent, const int value);
Evas_Object *common_utils_create_conformant_layout(Evas_Object *navi_frame);
Evas_Object *common_utils_show_info_popup(Evas_Object *win, popup_btn_info_t *popup_data);
Evas_Object *common_utils_show_info_ok_popup(Evas_Object *win, const char *str_pkg_name, const char *info_txt);
Evas_Object *common_utils_show_info_timeout_popup(Evas_Object *win, const char* info_text, const double timeout);

#endif
