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



#include "common.h"
#include "common_utils.h"
#include "i18nmanager.h"

typedef struct {
	char *title_str;
	char *info_str;
} two_line_disp_data_t;


static void __common_utils_entry_changed_cb(void* data, Evas_Object* obj, void* event_info)
{
	if (obj == NULL)
		return;

	if (elm_object_focus_get(data)) {
		if (elm_entry_is_empty(obj))
			elm_object_signal_emit(data, "elm,state,eraser,hide", "elm");
		else
			elm_object_signal_emit(data, "elm,state,eraser,show", "elm");
	}
}

static void __common_utils_entry_focused_cb(void *data, Evas_Object *obj, void *event_info)
{
	if (!elm_entry_is_empty(obj))
		elm_object_signal_emit(data, "elm,state,eraser,show", "elm");
	elm_object_signal_emit(data, "elm,state,guidetext,hide", "elm");
}

static void __common_utils_entry_unfocused_cb(void *data, Evas_Object *obj, void *event_info)
{
	INFO_LOG(UG_NAME_NORMAL, "__common_utils_entry_unfocused_cb entered");

	if (elm_entry_is_empty(obj)) {
		const char *guide_txt = elm_object_part_text_get(data, "elm.guidetext");
		INFO_LOG(UG_NAME_NORMAL, "entry is empty");
		if (guide_txt && strlen(guide_txt)) { /* If guide text exists then show it */
			elm_object_signal_emit(data, "elm,state,guidetext,show", "elm");
		} else {	/* If default entry text exists then show it */
			char *default_entry_txt = NULL;
			default_entry_txt = evas_object_data_get(data, COMMON_UTILS_DEFAULT_ENTRY_TEXT_KEY);
			if (default_entry_txt && strlen(default_entry_txt)) {
				elm_entry_entry_set(obj, default_entry_txt);
			}
		}
	}
	elm_object_signal_emit(data, "elm,state,eraser,hide", "elm");
}

static void __common_utils_eraser_clicked_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	elm_entry_entry_set(data, "");
}

static char *__common_utils_2line_text_get(void *data, Evas_Object *obj, const char *part)
{
	two_line_disp_data_t *item_data = (two_line_disp_data_t *)data;
	if (!strcmp(part, "elm.text.1")) {
		return g_strdup(item_data->info_str);
	} else if (!strcmp(part, "elm.text.2")) {
		return g_strdup(item_data->title_str);
	}
	return NULL;
}

static void __common_utils_2line_text_del(void *data, Evas_Object *obj)
{
	two_line_disp_data_t *item_data = (two_line_disp_data_t *)data;
	if (item_data) {
		g_free(item_data->info_str);
		g_free(item_data->title_str);
		g_free(item_data);
	}
}

static Evas_Object *__common_utils_entry_content_get(void *data, Evas_Object *obj, const char *part)
{
	Evas_Object *layout = (Evas_Object *)data;
	return layout;
}

static void __common_utils_entry_del(void *data, Evas_Object *obj)
{
	Evas_Object *layout = (Evas_Object *)data;
	char *dfalt_entry_txt = evas_object_data_get(layout, COMMON_UTILS_DEFAULT_ENTRY_TEXT_KEY);
	g_free(dfalt_entry_txt);
	evas_object_unref(layout);
}

static Evas_Object* __common_utils_create_conformant(Evas_Object* parent)
{
	assertm_if(NULL == parent, "NULL!!");

	Evas_Object* conform = NULL;
	elm_win_conformant_set(parent, TRUE);
	conform = elm_conformant_add(parent);
	assertm_if(NULL == conform, "NULL!!");

	elm_object_style_set(conform, "internal_layout");

	evas_object_size_hint_weight_set(conform, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(conform, EVAS_HINT_FILL, EVAS_HINT_FILL);

	evas_object_show(conform);

	return conform;
}

static void __common_utils_separator_del(void *data, Evas_Object *obj)
{
	elm_genlist_item_class_free(data);
	return;
}

Elm_Object_Item* common_utils_add_dialogue_separator(Evas_Object* genlist, const char *separator_style)
{
	assertm_if(NULL == genlist, "NULL!!");

	static Elm_Genlist_Item_Class *separator_itc;
	separator_itc = elm_genlist_item_class_new();
	separator_itc->item_style = separator_style;
	separator_itc->func.text_get = NULL;
	separator_itc->func.content_get = NULL;
	separator_itc->func.state_get = NULL;
	separator_itc->func.del = __common_utils_separator_del;

	Elm_Object_Item* sep = elm_genlist_item_append(
					genlist,
					separator_itc,
					separator_itc,
					NULL,
					ELM_GENLIST_ITEM_GROUP,
					NULL,
					NULL);

	assertm_if(NULL == sep, "NULL!!");

	elm_genlist_item_select_mode_set(sep, ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);

	return sep;
}

char *common_utils_get_ap_security_type_info_txt(const char *pkg_name, wifi_device_info_t *device_info)
{
	char *status_txt = NULL;
	switch (device_info->security_mode)
	{
		case WLAN_SEC_MODE_NONE:		/** Security disabled */
			status_txt = g_strdup(sc(pkg_name, I18N_TYPE_Open));
			break;
		case WLAN_SEC_MODE_IEEE8021X:	/** EAP */
			status_txt = g_strdup_printf("%s (%s)", sc(pkg_name, I18N_TYPE_Secured), sc(pkg_name, I18N_TYPE_EAP));
			break;
		case WLAN_SEC_MODE_WEP:			/** WEP */
		case WLAN_SEC_MODE_WPA_PSK:		/** WPA-PSK */
		case WLAN_SEC_MODE_WPA2_PSK:	/** WPA2-PSK */
			if (TRUE == device_info->wps_mode) {
				status_txt = g_strdup_printf("%s (%s)", sc(pkg_name, I18N_TYPE_Secured), sc(pkg_name, I18N_TYPE_WPS_Available));
			} else {
				status_txt = g_strdup(sc(pkg_name, I18N_TYPE_Secured));
			}
			break;
		default:						/** Unknown */
			status_txt = g_strdup(WIFI_UNKNOWN_DEVICE_STATUS_STR);
			break;
	}
	return status_txt;
}

char *common_utils_get_device_icon(const char *image_path_dir, wifi_device_info_t *device_info)
{
	char tmp_str[MAX_DEVICE_ICON_PATH_STR_LEN] = {'\0', };
	char *ret;

	g_strlcpy(tmp_str, image_path_dir, sizeof(tmp_str));
	g_strlcat(tmp_str, "/37_wifi_icon", sizeof(tmp_str));

	if (device_info->security_mode != WLAN_SEC_MODE_NONE) {
		g_strlcat(tmp_str, "_lock", sizeof(tmp_str));
	}

	switch (wlan_manager_get_signal_strength(device_info->rssi)) {
	case SIGNAL_STRENGTH_TYPE_EXCELLENT:
		g_strlcat(tmp_str, "_03", sizeof(tmp_str));
		break;
	case SIGNAL_STRENGTH_TYPE_GOOD:
		g_strlcat(tmp_str, "_02", sizeof(tmp_str));
		break;
	case SIGNAL_STRENGTH_TYPE_WEAK:
		g_strlcat(tmp_str, "_01", sizeof(tmp_str));
		break;
	case SIGNAL_STRENGTH_TYPE_VERY_WEAK:
	case SIGNAL_STRENGTH_TYPE_NULL:
	default:
		g_strlcat(tmp_str, "_00", sizeof(tmp_str));
		break;
	}

	/* Adding .png to the end of file */
	g_strlcat(tmp_str, ".png", sizeof(tmp_str));

	ret = g_strdup(tmp_str);
	return ret;
}

Evas_Object *common_utils_entry_layout_get_entry(Evas_Object *layout)
{
	return elm_object_part_content_get(layout, "elm.swallow.content");
}

char *common_utils_entry_layout_get_text(Evas_Object *layout)
{
	Evas_Object *entry = elm_object_part_content_get(layout, "elm.swallow.content");
	return elm_entry_markup_to_utf8(elm_entry_entry_get(entry));
}

void common_utils_set_entry_info(common_utils_entry_info_t *entry_info, char *title, char *entry_txt, char *guide_txt, Elm_Input_Panel_Layout panel_type)
{
	if (NULL == entry_info) {
		return;
	}
	memset(entry_info, 0, sizeof(common_utils_entry_info_t));
	entry_info->title = title;
	entry_info->entry_txt = entry_txt;
	entry_info->guide_txt = guide_txt;
	entry_info->panel_type = panel_type;
	return;
}

void common_utils_entry_password_set(Evas_Object *layout, Eina_Bool pswd_set)
{
	Evas_Object *entry = elm_object_part_content_get(layout, "elm.swallow.content");
	elm_entry_password_set(entry, pswd_set);
}

Evas_Object *common_utils_add_edit_box(Evas_Object *parent, const common_utils_entry_info_t *entry_info)
{
	Evas_Object *layout = elm_layout_add(parent);
	Evas_Object *entry = NULL;
	elm_layout_theme_set(layout, "layout", "editfield", "title");
	evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(layout, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_object_part_text_set(layout, "elm.text", entry_info->title);
	elm_object_part_text_set(layout, "elm.guidetext", entry_info->guide_txt); // Set guidetext.

	entry = elm_entry_add(layout);
	elm_object_part_content_set(layout, "elm.swallow.content", entry);
	elm_entry_single_line_set(entry, EINA_TRUE);
	elm_entry_scrollable_set(entry, EINA_TRUE);
	elm_entry_entry_set(entry, entry_info->entry_txt);
	elm_entry_input_panel_layout_set(entry, entry_info->panel_type);
	if (!elm_entry_is_empty(entry)) {
		INFO_LOG(UG_NAME_NORMAL, "entry is not empty");
		elm_object_signal_emit(layout, "elm,state,guidetext,hide", "elm");
		if (!entry_info->guide_txt || strlen(entry_info->guide_txt) <= 0)
			evas_object_data_set(layout, COMMON_UTILS_DEFAULT_ENTRY_TEXT_KEY, g_strdup(entry_info->entry_txt));
	}

	evas_object_smart_callback_add(entry, "changed", __common_utils_entry_changed_cb, layout);
	evas_object_smart_callback_add(entry, "focused", __common_utils_entry_focused_cb, layout);
	evas_object_smart_callback_add(entry, "unfocused", __common_utils_entry_unfocused_cb, layout);
	elm_object_signal_callback_add(layout, "elm,eraser,clicked", "elm", __common_utils_eraser_clicked_cb, entry);
	evas_object_show(entry);

	return layout;
}

Elm_Object_Item *common_utils_add_2_line_txt_disabled_item(Evas_Object* view_list, const char *style_name, const char *line1_txt, const char *line2_txt)
{
	static Elm_Genlist_Item_Class two_line_display_itc;
	two_line_disp_data_t *two_line_data = NULL;
	Elm_Object_Item *item = NULL;

	two_line_display_itc.item_style = style_name;
	two_line_display_itc.func.text_get = __common_utils_2line_text_get;
	two_line_display_itc.func.content_get = NULL;
	two_line_display_itc.func.state_get = NULL;
	two_line_display_itc.func.del = __common_utils_2line_text_del;

	two_line_data = g_malloc0(sizeof(two_line_disp_data_t));
	two_line_data->title_str = g_strdup(line1_txt);
	two_line_data->info_str = g_strdup(line2_txt);
	INFO_LOG(UG_NAME_NORMAL, "title_str = %s info_str = %s", two_line_data->title_str, two_line_data->info_str);

	item = elm_genlist_item_append(view_list, &two_line_display_itc, two_line_data, NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);
	elm_object_item_disabled_set(item, TRUE);

	return item;
}

Elm_Object_Item *common_utils_add_edit_box_to_list(Evas_Object *list, Elm_Object_Item *insert_after, char *title, char *entry_txt, char *guide_txt, Elm_Input_Panel_Layout panel_type)
{
	static common_utils_entry_info_t entry_info;
	static Elm_Genlist_Item_Class entry_itc;
	Evas_Object *layout = NULL;
	common_utils_set_entry_info(&entry_info, title, entry_txt, guide_txt, panel_type);
	layout = common_utils_add_edit_box(list, &entry_info);
	evas_object_ref(layout); /* We need to ref the layout object inorder to inform EFL not to delete it */

	entry_itc.item_style = "dialogue/1icon";
	entry_itc.func.text_get = NULL;
	entry_itc.func.content_get = __common_utils_entry_content_get;
	entry_itc.func.state_get = NULL;
	entry_itc.func.del = __common_utils_entry_del;

	if (insert_after)
		return elm_genlist_item_insert_after(list, &entry_itc, layout, NULL, insert_after, ELM_GENLIST_ITEM_NONE, NULL, NULL);
	else
		return elm_genlist_item_append(list, &entry_itc, layout, NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);
}

char *common_utils_get_list_item_entry_txt(Elm_Object_Item *entry_item)
{
	Evas_Object *ly = elm_object_item_data_get(entry_item);
	return common_utils_entry_layout_get_text(ly);
}

Evas_Object *common_utils_create_radio_button(Evas_Object *parent, const int value)
{
	Evas_Object *radio = elm_radio_add(parent);
	elm_radio_state_value_set(radio, value);
//	elm_radio_group_add(radio, radio_main);
	evas_object_size_hint_weight_set(radio, EVAS_HINT_EXPAND,
		EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(radio, EVAS_HINT_FILL, EVAS_HINT_FILL);

	return radio;
}

Evas_Object *common_utils_create_conformant_layout(Evas_Object *navi_frame)
{
	Evas_Object *layout;
	layout = elm_layout_add(navi_frame);
	elm_layout_theme_set(layout, "layout", "application", "noindicator");
	evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

	Evas_Object* bg = elm_bg_add(layout);
	evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_object_style_set(bg, "group_list");
	elm_object_part_content_set(layout, "elm.swallow.bg", bg);

	Evas_Object* conform = __common_utils_create_conformant(layout);
	assertm_if(NULL == conform, "NULL!!");
	elm_object_part_content_set(layout, "elm.swallow.content", conform);
	evas_object_show(layout);

	return layout;
}

Evas_Object *common_utils_show_info_popup(Evas_Object *parent, popup_btn_info_t *popup_data)
{
	__COMMON_FUNC_ENTER__;
	Evas_Object *popup = elm_popup_add(parent);
	evas_object_size_hint_weight_set(popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	if (popup_data->title_txt)
		elm_object_part_text_set(popup, "title,text", popup_data->title_txt);
	if (popup_data->info_txt)
		elm_object_text_set(popup, popup_data->info_txt);
	if (popup_data->btn1_txt) {
		Evas_Object *btn_1 = elm_button_add(popup);
		elm_object_text_set(btn_1, popup_data->btn1_txt);
		elm_object_part_content_set(popup, "button1", btn_1);
		if (popup_data->btn1_cb) {
			evas_object_smart_callback_add(btn_1, "clicked", popup_data->btn1_cb, popup_data->btn1_data);
		} else {	// set the default callback
			evas_object_smart_callback_add(btn_1, "clicked", (Evas_Smart_Cb)evas_object_del, popup);
		}
	}
	if (popup_data->btn2_txt) {
		Evas_Object *btn_2 = elm_button_add(popup);
		elm_object_text_set(btn_2, popup_data->btn2_txt);
		elm_object_part_content_set(popup, "button2", btn_2);
		evas_object_smart_callback_add(btn_2, "clicked", popup_data->btn2_cb, NULL);
		evas_object_show(popup);
		if (popup_data->btn2_cb) {
			evas_object_smart_callback_add(btn_2, "clicked", popup_data->btn2_cb, popup_data->btn2_data);
		} else {	// set the default callback
			evas_object_smart_callback_add(btn_2, "clicked", (Evas_Smart_Cb)evas_object_del, popup);
		}
	}
	evas_object_show(popup);

	return popup;
}

Evas_Object *common_utils_show_info_ok_popup(Evas_Object *win, const char *str_pkg_name, const char *info_txt)
{
	__COMMON_FUNC_ENTER__;
	popup_btn_info_t popup_data;
	memset(&popup_data, 0, sizeof(popup_data));
	popup_data.info_txt = (char *)info_txt;
	popup_data.btn1_txt = sc(str_pkg_name, I18N_TYPE_Ok);
	return common_utils_show_info_popup(win, &popup_data);
}

Evas_Object *common_utils_show_info_timeout_popup(Evas_Object *win, const char* info_text, const double timeout)
{
	Evas_Object *popup = elm_popup_add(win);
	elm_object_text_set(popup, info_text);
	elm_popup_timeout_set(popup, timeout);
	evas_object_smart_callback_add(popup, "timeout", (Evas_Smart_Cb)evas_object_del, popup);
	evas_object_size_hint_weight_set(popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_popup_orient_set(popup, ELM_POPUP_ORIENT_CENTER);
	evas_object_show(popup);
	return popup;
}
