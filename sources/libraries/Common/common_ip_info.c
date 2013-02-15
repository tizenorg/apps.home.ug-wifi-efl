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

#include <glib.h>

#include "common.h"
#include "common_ip_info.h"
#include "i18nmanager.h"

#define DEFAULT_PROXY_ADDR		"0.0.0.0:80"

#define MAX_PORT_NUMBER		65535

typedef struct {
	char* title;
	char* description;
} _view_detail_description_data_t;

struct ip_info_list {
	const char *str_pkg_name;
	Evas_Object *genlist;

	Elm_Object_Item* ip_toggle_item;
	Elm_Object_Item* ip_addr_item;
	Elm_Object_Item* subnet_mask_item;
	Elm_Object_Item* gateway_addr_item;
	Elm_Object_Item* dns_1_item;
	Elm_Object_Item* dns_2_item;
	Elm_Object_Item* proxy_addr_item;
	Elm_Object_Item* proxy_port_item;

	imf_ctxt_panel_cb_t input_panel_cb;
	void *input_panel_cb_data;

	wifi_ap_h ap;
};

static Elm_Object_Item* _add_description(Evas_Object* genlist, char* title,
		char* description, Elm_Object_Item* insert_after);

static Elm_Genlist_Item_Class ip_toggle_itc ;
static Elm_Genlist_Item_Class description_itc ;
static Elm_Genlist_Item_Class ip_entry_itc;

static void _gl_editbox_sel_cb(void *data, Evas_Object *obj, void *event_info)
{
	Elm_Object_Item *item = (Elm_Object_Item *)event_info;
	elm_genlist_item_selected_set(item, FALSE);
}

static void _ip_info_detail_description_del(void *data, Evas_Object *obj)
{
	__COMMON_FUNC_ENTER__;
	_view_detail_description_data_t* det = (_view_detail_description_data_t*) data;
	assertm_if(NULL == det, "NULL!!");
	assertm_if(NULL == det->title, "NULL!!");
	assertm_if(NULL == det->description, "NULL!!");
	g_free(det->description);
	g_free(det->title);
	g_free(det);
	det = NULL;
	__COMMON_FUNC_EXIT__;
}

static char *_ip_info_detail_description_text_get(void *data,
		Evas_Object *obj, const char *part)
{
	_view_detail_description_data_t* det = (_view_detail_description_data_t*) data;
	assertm_if(NULL == det, "NULL!!");
	assertm_if(NULL == det->title, "NULL!!");
	assertm_if(NULL == det->description, "NULL!!");
	assertm_if(NULL == part, "NULL!!");

	if(0 == strncmp("elm.text.1", part, strlen(part))) {
		return g_strdup(det->title);
	} else if(0 == strncmp("elm.text.2", part, strlen(part))) {
		return g_strdup(det->description);
	}

	return NULL;
}

static void _ip_info_entry_cursor_changed_cb(void* data, Evas_Object* obj, void* event_info)
{
	if (data == NULL)
		return;

	common_utils_entry_info_t *entry_info = (common_utils_entry_info_t *)data;

	if (entry_info) {
		g_free(entry_info->entry_txt);
		entry_info->entry_txt = NULL;
		char *entry_text = elm_entry_markup_to_utf8(elm_entry_entry_get(obj));

		if (entry_text != NULL && entry_text[0] != '\0')
			entry_info->entry_txt = g_strdup(elm_entry_entry_get(obj));

		g_free(entry_text);
	}
}

static void _ip_info_entry_changed_cb(void *data,
		Evas_Object *obj, void *event_info)
{
	int entry_pos = 0;
	char *entry_text = NULL;
	char **ip_text = NULL;
	int panel_type = 0;

	if (obj == NULL)
		return;

	if (elm_object_focus_get(data)) {
		if (elm_entry_is_empty(obj))
			elm_object_signal_emit(data, "elm,state,eraser,hide", "elm");
		else
			elm_object_signal_emit(data, "elm,state,eraser,show", "elm");
	}

	panel_type = elm_entry_input_panel_layout_get(obj);
	if (panel_type == ELM_INPUT_PANEL_LAYOUT_IP) {
		int i = 0;
		int ip_addr[4] = {0};
		char entry_ip_text[16] = {0,};
		gboolean fixed = FALSE;
		entry_text = elm_entry_markup_to_utf8(elm_entry_entry_get(obj));
		ip_text = g_strsplit(entry_text, ".", 5);

		for (i=0; i<5; i++) {
			if (ip_text[i] == NULL)
				break;

			if (i == 4) {
				fixed = TRUE;
				break;
			}

			ip_addr[i] = atoi(ip_text[i]);
			if (ip_addr[i] > 255) {
				ip_addr[i] = 255;
				fixed = TRUE;
			}

			if (i < 3)
				sprintf(entry_text, "%d.", ip_addr[i]);
			else
				sprintf(entry_text, "%d", ip_addr[i]);

			g_strlcat(entry_ip_text, entry_text, sizeof(entry_ip_text));
		}
		g_free(entry_text);
		g_strfreev(ip_text);

		if (fixed == TRUE) {
			entry_pos = elm_entry_cursor_pos_get(obj);
			elm_entry_entry_set(obj, entry_ip_text);
			elm_entry_cursor_pos_set(obj, entry_pos+1);
		}
	} else if (panel_type == ELM_INPUT_PANEL_LAYOUT_NUMBERONLY) {
		int port_num = 0;

		entry_text = elm_entry_markup_to_utf8(elm_entry_entry_get(obj));
		sscanf(entry_text, "%d", &port_num);

		if (port_num > MAX_PORT_NUMBER) {
			entry_pos = elm_entry_cursor_pos_get(obj);
			sprintf(entry_text, "%d", MAX_PORT_NUMBER);
			elm_entry_entry_set(obj, entry_text);
			elm_entry_cursor_pos_set(obj, entry_pos);
		}

		g_free(entry_text);
	}
}

static void _ip_info_entry_focused_cb(void *data, Evas_Object *obj, void *event_info)
{
	if (!elm_entry_is_empty(obj))
		elm_object_signal_emit(data, "elm,state,eraser,show", "elm");
	elm_object_signal_emit(data, "elm,state,guidetext,hide", "elm");
}

static void _ip_info_entry_unfocused_cb(void *data, Evas_Object *obj, void *event_info)
{
	INFO_LOG(UG_NAME_NORMAL, "_ip_info_entry_unfocused_cb entered");

	if (elm_entry_is_empty(obj))
		elm_object_signal_emit(data, "elm,state,guidetext,show", "elm");
	else {
		elm_object_signal_emit(data, "elm,state,guidetext,hide", "elm");
		int panel_type = elm_entry_input_panel_layout_get(obj);
		if (panel_type == ELM_INPUT_PANEL_LAYOUT_IP) {
			int ip_addr[4] = {0};
			char *entry_text = elm_entry_markup_to_utf8(elm_entry_entry_get(obj));
			sscanf(entry_text, "%d.%d.%d.%d", &ip_addr[0], &ip_addr[1], &ip_addr[2], &ip_addr[3]);
			sprintf(entry_text, "%d.%d.%d.%d", ip_addr[0], ip_addr[1], ip_addr[2], ip_addr[3]);
			elm_entry_entry_set(obj, entry_text);
			g_free(entry_text);
		}
	}

	elm_object_signal_emit(data, "elm,state,eraser,hide", "elm");
}

static void _ip_info_entry_eraser_clicked_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	elm_entry_entry_set(data, "");
}

static Evas_Object *_ip_info_entry_item_content_get(void *data, Evas_Object *obj, const char *part)
{
	if (g_strcmp0(part, "elm.icon")) {
		return NULL;
	}

	common_utils_entry_info_t *entry_info = (common_utils_entry_info_t *)data;
	if (!entry_info)
		return NULL;

	Evas_Object *layout = NULL;
	Evas_Object *entry = NULL;
	char *title = NULL;
	char *guide_txt = NULL;
	char *accepted = NULL;
	Eina_Bool hide_entry_txt = EINA_FALSE;
	Elm_Input_Panel_Layout panel_type;

	Elm_Entry_Filter_Limit_Size limit_filter_data;

	layout = elm_layout_add(obj);
	elm_layout_theme_set(layout, "layout", "editfield", "title");

	entry = elm_entry_add(layout);
	elm_entry_scrollable_set(entry, EINA_TRUE);
	elm_entry_single_line_set(entry, EINA_TRUE);
	elm_object_part_content_set(layout, "elm.swallow.content", entry);

	switch (entry_info->entry_id)
	{
	case ENTRY_TYPE_IP_ADDR:
		title = entry_info->title_txt;
		guide_txt = entry_info->guide_txt;
		panel_type = ELM_INPUT_PANEL_LAYOUT_IP;
		accepted = "0123456789.";
		break;
	case ENTRY_TYPE_SUBNET_MASK:
		title = entry_info->title_txt;
		guide_txt = entry_info->guide_txt;
		panel_type = ELM_INPUT_PANEL_LAYOUT_IP;
		accepted = "0123456789.";
		break;
	case ENTRY_TYPE_GATEWAY:
		title = entry_info->title_txt;
		guide_txt = entry_info->guide_txt;
		panel_type = ELM_INPUT_PANEL_LAYOUT_IP;
		accepted = "0123456789.";
		break;
	case ENTRY_TYPE_DNS_1:
		title = entry_info->title_txt;
		guide_txt = entry_info->guide_txt;
		panel_type = ELM_INPUT_PANEL_LAYOUT_IP;
		accepted = "0123456789.";
		break;
	case ENTRY_TYPE_DNS_2:
		title = entry_info->title_txt;
		guide_txt = entry_info->guide_txt;
		panel_type = ELM_INPUT_PANEL_LAYOUT_IP;
		accepted = "0123456789.";
		break;
	case ENTRY_TYPE_PROXY_ADDR:
		title = entry_info->title_txt;
		guide_txt = DEFAULT_GUIDE_PROXY_IP;
		panel_type = ELM_INPUT_PANEL_LAYOUT_URL;
		break;
	case ENTRY_TYPE_PROXY_PORT:
		title = entry_info->title_txt;
		guide_txt = DEFAULT_GUIDE_PROXY_PORT;
		panel_type = ELM_INPUT_PANEL_LAYOUT_NUMBERONLY;
		break;
	default:
		return NULL;
	}

	elm_object_part_text_set(layout, "elm.text", title);
	elm_object_part_text_set(layout, "elm.guidetext", guide_txt);
	elm_entry_password_set(entry, hide_entry_txt);
	if (entry_info->entry_txt && (strlen(entry_info->entry_txt) > 0)) {
		elm_entry_entry_set(entry, entry_info->entry_txt);
		elm_object_signal_emit(layout, "elm,state,guidetext,hide", "elm");
	}
	elm_entry_input_panel_layout_set(entry, panel_type);
	limit_filter_data.max_char_count = 32;
	elm_entry_markup_filter_append(entry, elm_entry_filter_limit_size, &limit_filter_data);

	Elm_Entry_Filter_Accept_Set digits_filter_data;
	memset(&digits_filter_data, 0, sizeof(Elm_Entry_Filter_Accept_Set));
	digits_filter_data.accepted = accepted;
	elm_entry_markup_filter_append(entry, elm_entry_filter_accept_set, &digits_filter_data);

	if (entry_info->input_panel_cb) {
		Ecore_IMF_Context *imf_ctxt = elm_entry_imf_context_get(entry);
		if (imf_ctxt) {
			ecore_imf_context_input_panel_event_callback_add(imf_ctxt, ECORE_IMF_INPUT_PANEL_STATE_EVENT, entry_info->input_panel_cb, entry_info->input_panel_cb_data);
		}
	}
	evas_object_smart_callback_add(entry, "cursor,changed", _ip_info_entry_cursor_changed_cb, entry_info);
	evas_object_smart_callback_add(entry, "changed", _ip_info_entry_changed_cb, layout);
	evas_object_smart_callback_add(entry, "focused", _ip_info_entry_focused_cb, layout);
	evas_object_smart_callback_add(entry, "unfocused", _ip_info_entry_unfocused_cb, layout);
	elm_object_signal_callback_add(layout, "elm,eraser,clicked", "elm", _ip_info_entry_eraser_clicked_cb, entry);
	evas_object_show(entry);

	entry_info->layout = layout;
	return layout;
}

static void _ip_info_entry_item_del(void *data, Evas_Object *obj)
{
	common_utils_entry_info_t *entry_info = (common_utils_entry_info_t *)data;
	if (entry_info == NULL)
		return;

	if (entry_info->entry_txt)
		g_free(entry_info->entry_txt);

	if (entry_info->input_panel_cb) {
		Evas_Object *entry = common_utils_entry_layout_get_entry(entry_info->layout);
		Ecore_IMF_Context *imf_ctxt = elm_entry_imf_context_get(entry);
		if (imf_ctxt) {
			ecore_imf_context_input_panel_event_callback_del(imf_ctxt, ECORE_IMF_INPUT_PANEL_STATE_EVENT, entry_info->input_panel_cb);
		}
	}

	g_free(entry_info);
}

static void _create_static_ip_table(ip_info_list_t *ip_info_list_data)
{
	char *txt = NULL;
	wifi_ap_h ap = ip_info_list_data->ap;
	common_utils_entry_info_t *edit_box_details;

	__COMMON_FUNC_ENTER__;

	/* IP Address */
	wifi_ap_get_ip_address(ap, WIFI_ADDRESS_FAMILY_IPV4, &txt);
	edit_box_details = g_new0(common_utils_entry_info_t, 1);
	edit_box_details->entry_id = ENTRY_TYPE_IP_ADDR;
	edit_box_details->title_txt = sc(ip_info_list_data->str_pkg_name, I18N_TYPE_IP_address);
	edit_box_details->entry_txt = txt;
	edit_box_details->input_panel_cb = ip_info_list_data->input_panel_cb;
	edit_box_details->input_panel_cb_data = ip_info_list_data->input_panel_cb_data;
	ip_info_list_data->ip_addr_item = elm_genlist_item_insert_after(ip_info_list_data->genlist, &ip_entry_itc, edit_box_details, NULL, ip_info_list_data->ip_toggle_item, ELM_GENLIST_ITEM_NONE, _gl_editbox_sel_cb, NULL);
	elm_genlist_item_select_mode_set(ip_info_list_data->ip_addr_item, ELM_OBJECT_SELECT_MODE_NONE);

	/* Subnet Mask */
	wifi_ap_get_subnet_mask(ap, WIFI_ADDRESS_FAMILY_IPV4, &txt);
	edit_box_details = g_new0(common_utils_entry_info_t, 1);
	edit_box_details->entry_id = ENTRY_TYPE_SUBNET_MASK;
	edit_box_details->title_txt = sc(ip_info_list_data->str_pkg_name, I18N_TYPE_Subnet_mask);
	edit_box_details->entry_txt = txt;
	edit_box_details->input_panel_cb = ip_info_list_data->input_panel_cb;
	edit_box_details->input_panel_cb_data = ip_info_list_data->input_panel_cb_data;
	ip_info_list_data->subnet_mask_item = elm_genlist_item_insert_after(ip_info_list_data->genlist, &ip_entry_itc, edit_box_details, NULL, ip_info_list_data->ip_addr_item, ELM_GENLIST_ITEM_NONE, _gl_editbox_sel_cb, NULL);
	elm_genlist_item_select_mode_set(ip_info_list_data->subnet_mask_item, ELM_OBJECT_SELECT_MODE_NONE);

	/* Gateway Address */
	wifi_ap_get_gateway_address(ap, WIFI_ADDRESS_FAMILY_IPV4, &txt);
	edit_box_details = g_new0(common_utils_entry_info_t, 1);
	edit_box_details->entry_id = ENTRY_TYPE_GATEWAY;
	edit_box_details->title_txt = sc(ip_info_list_data->str_pkg_name, I18N_TYPE_Gateway_address);
	edit_box_details->entry_txt = txt;
	edit_box_details->input_panel_cb = ip_info_list_data->input_panel_cb;
	edit_box_details->input_panel_cb_data = ip_info_list_data->input_panel_cb_data;
	ip_info_list_data->gateway_addr_item = elm_genlist_item_insert_after(ip_info_list_data->genlist, &ip_entry_itc, edit_box_details, NULL, ip_info_list_data->subnet_mask_item, ELM_GENLIST_ITEM_NONE, _gl_editbox_sel_cb, NULL);
	elm_genlist_item_select_mode_set(ip_info_list_data->gateway_addr_item, ELM_OBJECT_SELECT_MODE_NONE);

	/* DNS 1 */
	wifi_ap_get_dns_address(ap, 1, WIFI_ADDRESS_FAMILY_IPV4, &txt);
	edit_box_details = g_new0(common_utils_entry_info_t, 1);
	edit_box_details->entry_id = ENTRY_TYPE_DNS_1;
	edit_box_details->title_txt = sc(ip_info_list_data->str_pkg_name, I18N_TYPE_DNS_1);
	edit_box_details->entry_txt = txt;
	edit_box_details->input_panel_cb = ip_info_list_data->input_panel_cb;
	edit_box_details->input_panel_cb_data = ip_info_list_data->input_panel_cb_data;
	ip_info_list_data->dns_1_item = elm_genlist_item_insert_after(ip_info_list_data->genlist, &ip_entry_itc, edit_box_details, NULL, ip_info_list_data->gateway_addr_item, ELM_GENLIST_ITEM_NONE, _gl_editbox_sel_cb, NULL);
	elm_genlist_item_select_mode_set(ip_info_list_data->dns_1_item, ELM_OBJECT_SELECT_MODE_NONE);

	/* DNS 2 */
	wifi_ap_get_dns_address(ap, 2, WIFI_ADDRESS_FAMILY_IPV4, &txt);
	edit_box_details = g_new0(common_utils_entry_info_t, 1);
	edit_box_details->entry_id = ENTRY_TYPE_DNS_2;
	edit_box_details->title_txt = sc(ip_info_list_data->str_pkg_name, I18N_TYPE_DNS_2);
	edit_box_details->entry_txt = txt;
	edit_box_details->input_panel_cb = ip_info_list_data->input_panel_cb;
	edit_box_details->input_panel_cb_data = ip_info_list_data->input_panel_cb_data;
	ip_info_list_data->dns_2_item = elm_genlist_item_insert_after(ip_info_list_data->genlist, &ip_entry_itc, edit_box_details, NULL, ip_info_list_data->dns_1_item, ELM_GENLIST_ITEM_NONE, _gl_editbox_sel_cb, NULL);
	elm_genlist_item_select_mode_set(ip_info_list_data->dns_2_item, ELM_OBJECT_SELECT_MODE_NONE);

	__COMMON_FUNC_EXIT__;

	return;
}

static void _delete_static_ip_table(ip_info_list_t *ip_info_list_data)
{
	__COMMON_FUNC_ENTER__;

	elm_object_item_del(ip_info_list_data->ip_addr_item);
	ip_info_list_data->ip_addr_item = NULL;
	elm_object_item_del(ip_info_list_data->subnet_mask_item);
	ip_info_list_data->subnet_mask_item = NULL;
	elm_object_item_del(ip_info_list_data->gateway_addr_item);
	ip_info_list_data->gateway_addr_item = NULL;
	elm_object_item_del(ip_info_list_data->dns_1_item);
	ip_info_list_data->dns_1_item = NULL;
	elm_object_item_del(ip_info_list_data->dns_2_item);
	ip_info_list_data->dns_2_item = NULL;

	__COMMON_FUNC_EXIT__;

	return;
}

static int _genlist_item_disable_later(void* data)
{
	elm_genlist_item_selected_set((Elm_Object_Item*) data, FALSE);
	return FALSE;
}

static void _gl_deselect_callback(void* data, Evas_Object* obj,
		void* event_info)
{
	ecore_idler_add( (Ecore_Task_Cb) _genlist_item_disable_later, event_info);
}

static char* _ip_info_iptoggle_text_get(void *data, Evas_Object *obj,
		const char *part)
{
	__COMMON_FUNC_ENTER__;

	assertm_if(NULL == data, "NULL!!");
	assertm_if(NULL == obj, "NULL!!");
	assertm_if(NULL == part, "NULL!!");
	if (part != NULL) {
		if (!strncmp(part, "elm.text", strlen(part))) {
			__COMMON_FUNC_EXIT__;
			ip_info_list_t *ip_info_list_data = (ip_info_list_t *)data;
			return (char*)g_strdup(sc(ip_info_list_data->str_pkg_name, I18N_TYPE_Static_IP));
		}
	}

	__COMMON_FUNC_EXIT__;
	return NULL;
}

static Elm_Object_Item* _add_description(Evas_Object* genlist, char* title,
		char* description, Elm_Object_Item* insert_after)
{
	assertm_if(NULL == genlist, "NULL!!");
	assertm_if(NULL == title, "NULL!!");
	assertm_if(NULL == description, "NULL!!");

	_view_detail_description_data_t* description_data = g_new0(_view_detail_description_data_t, 1);
	assertm_if(NULL == description_data, "NULL!!");

	description_data->title = g_strdup(title);
	description_data->description = g_strdup(description);

	Elm_Object_Item* det = NULL;
	if (insert_after) {
		det = elm_genlist_item_insert_after(
				genlist, /*obj*/
				&description_itc,/*itc*/
				description_data,/*data*/
				NULL,/*parent*/
				insert_after, /*after than*/
				ELM_GENLIST_ITEM_NONE, /*flags*/
				_gl_deselect_callback,/*func*/
				NULL);/*func_data*/
	} else {
		det = elm_genlist_item_append(
				genlist,
				&description_itc,
				description_data,
				NULL,
				ELM_GENLIST_ITEM_NONE,
				_gl_deselect_callback,
				NULL);
	}
	assertm_if(NULL == det, "NULL!!");

	elm_genlist_item_select_mode_set(det, ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);

	return det;
}

static void _ip_info_toggle_item_sel_cb(void* data, Evas_Object* obj,
		void* event_info)
{
	__COMMON_FUNC_ENTER__;

	if (!data)
		return;

	ip_info_list_t *ip_info_list_data = (ip_info_list_t *)data;
	wifi_ap_h ap = ip_info_list_data->ap;
	elm_object_item_disabled_set(ip_info_list_data->ip_toggle_item, TRUE);
	wifi_ip_config_type_e type = WIFI_IP_CONFIG_TYPE_NONE;
	wifi_ap_get_ip_config_type(ap, WIFI_ADDRESS_FAMILY_IPV4, &type);
	if (WIFI_IP_CONFIG_TYPE_STATIC == type) {	/* Static IP */
		char *ip_addr = NULL;

		ip_info_save_data(ip_info_list_data);
		_delete_static_ip_table(ip_info_list_data);

		wifi_ap_get_ip_address(ap, WIFI_ADDRESS_FAMILY_IPV4, &ip_addr);
		/* Dynamic IP Address */
		ip_info_list_data->ip_addr_item =
		_add_description(ip_info_list_data->genlist,
			sc(ip_info_list_data->str_pkg_name, I18N_TYPE_IP_address),
			ip_addr, ip_info_list_data->ip_toggle_item);
		elm_object_item_disabled_set(ip_info_list_data->ip_addr_item, TRUE);
		g_free(ip_addr);

		wifi_ap_set_ip_config_type(ap, WIFI_ADDRESS_FAMILY_IPV4,
			WIFI_IP_CONFIG_TYPE_DYNAMIC);
	} else if (WIFI_IP_CONFIG_TYPE_DYNAMIC == type) {	/* Dynamic IP */

		elm_object_item_del(ip_info_list_data->ip_addr_item);
		ip_info_list_data->ip_addr_item = NULL;

		/* Create the entry layouts */
		_create_static_ip_table(ip_info_list_data);
		wifi_ap_set_ip_config_type(ap, WIFI_ADDRESS_FAMILY_IPV4,
				WIFI_IP_CONFIG_TYPE_STATIC);
	}
	type = WIFI_IP_CONFIG_TYPE_NONE;
	wifi_ap_get_ip_config_type(ap, WIFI_ADDRESS_FAMILY_IPV4, &type);
	DEBUG_LOG(UG_NAME_NORMAL, "AP[0x%x] ip config type: %d", ap, type);
	elm_genlist_item_update(ip_info_list_data->ip_toggle_item);
	elm_object_item_disabled_set(ip_info_list_data->ip_toggle_item, FALSE);
	ecore_idler_add( (Ecore_Task_Cb) _genlist_item_disable_later, event_info);

	__COMMON_FUNC_EXIT__;
}

static Evas_Object *_ip_info_iptoggle_content_get(void *data,
		Evas_Object *obj, const char *part)
{
	if (NULL == data || obj == NULL || part == NULL) {
		return NULL;
	}

	ip_info_list_t *ip_info_list_data = (ip_info_list_t *)data;
	wifi_ap_h ap = ip_info_list_data->ap;

	Evas_Object *toggle_btn = elm_check_add(obj);
	assertm_if(NULL == toggle_btn, "NULL!!");
	elm_object_style_set(toggle_btn, "on&off");
	evas_object_propagate_events_set(toggle_btn, EINA_TRUE);
	wifi_ip_config_type_e type;
	wifi_ap_get_ip_config_type(ap, WIFI_ADDRESS_FAMILY_IPV4, &type);
	if (WIFI_IP_CONFIG_TYPE_STATIC == type) {	/* Static IP */
		elm_check_state_set(toggle_btn, EINA_TRUE);
	} else {
		elm_check_state_set(toggle_btn, EINA_FALSE);
	}
	return toggle_btn;
}

static void ip_info_print_values(wifi_ap_h ap)
{
	char *txt;
	wifi_ip_config_type_e type = WIFI_IP_CONFIG_TYPE_NONE;
	wifi_ap_get_ip_config_type(ap, WIFI_ADDRESS_FAMILY_IPV4, &type);
	if (WIFI_IP_CONFIG_TYPE_STATIC == type) {	/* Static IP */

		DEBUG_LOG(UG_NAME_NORMAL, "* STATIC CONFIGURATION *");

		/* IP Address */
		wifi_ap_get_ip_address(ap, WIFI_ADDRESS_FAMILY_IPV4, &txt);
		DEBUG_LOG(UG_NAME_NORMAL, "* IP address [%s]", txt);
		g_free(txt);

		/* Subnet Mask */
		wifi_ap_get_subnet_mask(ap, WIFI_ADDRESS_FAMILY_IPV4, &txt);
		DEBUG_LOG(UG_NAME_NORMAL, "* Subnet Mask [%s]", txt);
		g_free(txt);

		/* Gateway Address */
		wifi_ap_get_gateway_address(ap, WIFI_ADDRESS_FAMILY_IPV4, &txt);
		DEBUG_LOG(UG_NAME_NORMAL, "* Gateway address [%s]", txt);
		g_free(txt);

		/* DNS 1 */
		wifi_ap_get_dns_address(ap, 1, WIFI_ADDRESS_FAMILY_IPV4, &txt);
		DEBUG_LOG(UG_NAME_NORMAL, "* DNS-1 address [%s]", txt);
		g_free(txt);

		/* DNS 2 */
		wifi_ap_get_dns_address(ap, 2, WIFI_ADDRESS_FAMILY_IPV4, &txt);
		DEBUG_LOG(UG_NAME_NORMAL, "* DNS-2 address [%s]", txt);
		g_free(txt);

	} else if (WIFI_IP_CONFIG_TYPE_DYNAMIC == type) {	/* Dynamic IP */

		DEBUG_LOG(UG_NAME_NORMAL, "* DYNAMIC CONFIGURATION *");

		/* Dynamic IP Address */
		wifi_ap_get_ip_address(ap, WIFI_ADDRESS_FAMILY_IPV4, &txt);
		DEBUG_LOG(UG_NAME_NORMAL, "* IP address [%s]", txt);
		g_free(txt);
	}

	/* Mac address */
	wifi_get_mac_address(&txt);
	DEBUG_LOG(UG_NAME_NORMAL, "* MAC address [%s]", txt);
	g_free(txt);

	txt = NULL;
	wifi_ap_get_proxy_address(ap, WIFI_ADDRESS_FAMILY_IPV4, &txt);
	assertm_if(NULL == txt, "NULL!!");

	if (!txt || !strlen(txt)) {
		if (txt)
			g_free(txt);
		txt = g_strdup(DEFAULT_PROXY_ADDR);
	}
	/* Proxy Address */
	char *proxy_addr = strtok(txt, ":");
	DEBUG_LOG(UG_NAME_NORMAL, "* PROXY ADDR [%s]", proxy_addr);

	/* Proxy port */
	char *proxy_port = strtok(NULL, ":");
	DEBUG_LOG(UG_NAME_NORMAL, "* PROXY PORT [%s]", proxy_port);
	g_free(txt);

}

ip_info_list_t *ip_info_append_items(wifi_ap_h ap, const char *pkg_name,
		Evas_Object *genlist, imf_ctxt_panel_cb_t input_panel_cb, void *input_panel_cb_data)
{
	__COMMON_FUNC_ENTER__;
	int ret = WIFI_ERROR_NONE;
	char *proxy_data = NULL;
	char *proxy_addr = NULL;
	char *proxy_port = NULL;
	Elm_Object_Item* item = NULL;
	common_utils_entry_info_t *edit_box_details;

	assertm_if(NULL == ap, "NULL!!");
	assertm_if(NULL == pkg_name, "NULL!!");
	assertm_if(NULL == genlist, "NULL!!");

	ip_info_list_t *ip_info_list_data = g_new0(ip_info_list_t, 1);
	ip_info_list_data->ap = ap;
	ip_info_list_data->str_pkg_name = pkg_name;
	ip_info_list_data->genlist = genlist;
	ip_info_list_data->input_panel_cb = input_panel_cb;
	ip_info_list_data->input_panel_cb_data = input_panel_cb_data;

	ip_toggle_itc.item_style = "dialogue/1text.1icon";
	ip_toggle_itc.func.text_get = _ip_info_iptoggle_text_get;
	ip_toggle_itc.func.content_get = _ip_info_iptoggle_content_get;
	ip_toggle_itc.func.state_get = NULL;
	ip_toggle_itc.func.del = NULL;

	description_itc.item_style = "dialogue/2text.3";
	description_itc.func.text_get = _ip_info_detail_description_text_get;
	description_itc.func.content_get = NULL;
	description_itc.func.state_get = NULL;
	description_itc.func.del = _ip_info_detail_description_del;

	ip_entry_itc.item_style = "dialogue/1icon";
	ip_entry_itc.func.text_get = NULL;
	ip_entry_itc.func.content_get = _ip_info_entry_item_content_get;
	ip_entry_itc.func.state_get = NULL;
	ip_entry_itc.func.del = _ip_info_entry_item_del;

	common_utils_add_dialogue_separator(genlist, "dialogue/separator.2");

	/* Static/Dynamic switch button */
	ip_info_list_data->ip_toggle_item =
	elm_genlist_item_append(genlist,
		&ip_toggle_itc, ip_info_list_data, NULL, ELM_GENLIST_ITEM_NONE,
		_ip_info_toggle_item_sel_cb, ip_info_list_data);

	/* IP address */
	wifi_ip_config_type_e type = WIFI_IP_CONFIG_TYPE_NONE;
	wifi_ap_get_ip_config_type(ap, WIFI_ADDRESS_FAMILY_IPV4, &type);
	if (WIFI_IP_CONFIG_TYPE_STATIC == type) {	/* Static IP */
		/* Create the entry layouts */
		_create_static_ip_table(ip_info_list_data);
	} else if (WIFI_IP_CONFIG_TYPE_DYNAMIC == type) {	/* Dynamic IP */
		char *ip_addr = NULL;
		/* Dynamic IP Address */
		wifi_ap_get_ip_address(ap, WIFI_ADDRESS_FAMILY_IPV4, &ip_addr);
		ip_info_list_data->ip_addr_item =
		_add_description(genlist,
			sc(ip_info_list_data->str_pkg_name, I18N_TYPE_IP_address),
			ip_addr, NULL);
		elm_object_item_disabled_set(ip_info_list_data->ip_addr_item, TRUE);
		g_free(ip_addr);
	}

	common_utils_add_dialogue_separator(genlist, "dialogue/separator.2");

#if 0
	/* Channel Number */
	int channel_number;
	wifi_ap_get_frequency(ap, &channel_number);
	char *channel_num_str = g_strdup_printf("%u", channel_number);
	_add_description(genlist,
		sc(ip_info_list_data->str_pkg_name, I18N_TYPE_Channel), channel_num_str,
		NULL);
	g_free(channel_num_str);
#endif
	/* Mac address */
	char *mac_addr = NULL;
	wifi_get_mac_address(&mac_addr);
	item =
	_add_description(genlist,
		sc(ip_info_list_data->str_pkg_name, I18N_TYPE_MAC_addr), mac_addr,
		NULL);
	elm_object_item_disabled_set(item, TRUE);
	g_free(mac_addr);

	ret = wifi_ap_get_proxy_address(ap, WIFI_ADDRESS_FAMILY_IPV4, &proxy_data);
	assertm_if(NULL == proxy_data, "NULL!!");

	if (WIFI_ERROR_NONE == ret && proxy_data && strlen(proxy_data)) {
		DEBUG_LOG(UG_NAME_NORMAL, "* PROXY DATA [%s]", proxy_data);
		/* Proxy Address */
		proxy_addr = g_strdup(strtok(proxy_data, ":"));
		DEBUG_LOG(UG_NAME_NORMAL, "* PROXY ADDR [%s]", proxy_addr);

		/* Proxy port */
		proxy_port = g_strdup(strtok(NULL, ":"));
		DEBUG_LOG(UG_NAME_NORMAL, "* PROXY PORT [%s]", proxy_port);
	} else {
		ERROR_LOG(UG_NAME_ERR, "Error = %d", ret);
	}

	edit_box_details = g_new0(common_utils_entry_info_t, 1);
	edit_box_details->entry_id = ENTRY_TYPE_PROXY_ADDR;
	edit_box_details->title_txt = sc(pkg_name, I18N_TYPE_Proxy_address);
	edit_box_details->entry_txt = proxy_addr;
	edit_box_details->input_panel_cb = input_panel_cb;
	edit_box_details->input_panel_cb_data = input_panel_cb_data;
	ip_info_list_data->proxy_addr_item = elm_genlist_item_append(genlist, &ip_entry_itc, edit_box_details, NULL, ELM_GENLIST_ITEM_NONE, _gl_editbox_sel_cb, NULL);
	elm_genlist_item_select_mode_set(ip_info_list_data->proxy_addr_item, ELM_OBJECT_SELECT_MODE_NONE);

	edit_box_details = g_new0(common_utils_entry_info_t, 1);
	edit_box_details->entry_id = ENTRY_TYPE_PROXY_PORT;
	edit_box_details->title_txt = sc(pkg_name, I18N_TYPE_Proxy_port);
	edit_box_details->entry_txt = proxy_port;
	edit_box_details->input_panel_cb = input_panel_cb;
	edit_box_details->input_panel_cb_data = input_panel_cb_data;
	ip_info_list_data->proxy_port_item = elm_genlist_item_append(genlist, &ip_entry_itc, edit_box_details, NULL, ELM_GENLIST_ITEM_NONE, _gl_editbox_sel_cb, NULL);
	elm_genlist_item_select_mode_set(ip_info_list_data->proxy_port_item, ELM_OBJECT_SELECT_MODE_NONE);

	g_free(proxy_data);

	__COMMON_FUNC_EXIT__;
	return ip_info_list_data;
}

void ip_info_save_data(ip_info_list_t *ip_info_list_data)
{
	__COMMON_FUNC_ENTER__;
	assertm_if(NULL == ip_info_list_data, "NULL!!");
	char* txt = NULL;
	char* proxy_addr = NULL;
	char* proxy_port = NULL;
	wifi_ap_h ap = ip_info_list_data->ap;

	wifi_ip_config_type_e type = WIFI_IP_CONFIG_TYPE_NONE;
	wifi_ap_get_ip_config_type(ap, WIFI_ADDRESS_FAMILY_IPV4, &type);
	if (WIFI_IP_CONFIG_TYPE_STATIC == type) {	/* Static IP */
		txt = common_utils_get_list_item_entry_txt(ip_info_list_data->ip_addr_item);
		DEBUG_LOG(UG_NAME_NORMAL, "IP [%s]", txt);
		wifi_ap_set_ip_address(ap, WIFI_ADDRESS_FAMILY_IPV4, txt);
		g_free(txt);

		txt =
		common_utils_get_list_item_entry_txt(ip_info_list_data->subnet_mask_item);
		DEBUG_LOG(UG_NAME_NORMAL, "Subnet [%s]", txt);
		wifi_ap_set_subnet_mask(ap, WIFI_ADDRESS_FAMILY_IPV4, txt);
		g_free(txt);

		txt =
		common_utils_get_list_item_entry_txt(ip_info_list_data->gateway_addr_item);
		DEBUG_LOG(UG_NAME_NORMAL, "Gateway [%s]", txt);
		wifi_ap_set_gateway_address(ap, WIFI_ADDRESS_FAMILY_IPV4, txt);
		g_free(txt);

		txt =
		common_utils_get_list_item_entry_txt(ip_info_list_data->dns_1_item);
		DEBUG_LOG(UG_NAME_NORMAL, "DNS1 [%s]", txt);
		wifi_ap_set_dns_address(ap, 1, WIFI_ADDRESS_FAMILY_IPV4, txt);
		g_free(txt);

		txt =
		common_utils_get_list_item_entry_txt(ip_info_list_data->dns_2_item);
		DEBUG_LOG(UG_NAME_NORMAL, "DNS2 [%s]", txt);
		wifi_ap_set_dns_address(ap, 2, WIFI_ADDRESS_FAMILY_IPV4, txt);
		g_free(txt);
	}
	proxy_addr =
	common_utils_get_list_item_entry_txt(ip_info_list_data->proxy_addr_item);
	proxy_port =
	common_utils_get_list_item_entry_txt(ip_info_list_data->proxy_port_item);
	txt = g_strdup_printf("%s:%s", proxy_addr?proxy_addr:"", proxy_port?proxy_port:"");
	DEBUG_LOG( UG_NAME_NORMAL, "Proxy addr:port [%s]", txt);
	wifi_ap_set_proxy_type(ap, WIFI_PROXY_TYPE_MANUAL);
	wifi_ap_set_proxy_address(ap, WIFI_ADDRESS_FAMILY_IPV4, txt);
	g_free((gpointer)proxy_addr);
	g_free((gpointer)proxy_port);
	g_free((gpointer)txt);

	ip_info_print_values(ap);

	__COMMON_FUNC_EXIT__;
	return;
}

void ip_info_remove(ip_info_list_t *ip_info_list)
{
	__COMMON_FUNC_ENTER__;
	assertm_if(NULL == ip_info_list, "NULL!!");
	_delete_static_ip_table(ip_info_list);
	g_free(ip_info_list);
	__COMMON_FUNC_EXIT__;
}
