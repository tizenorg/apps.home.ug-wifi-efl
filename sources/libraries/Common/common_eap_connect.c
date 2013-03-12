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
#include "common_eap_connect.h"
#include "i18nmanager.h"
#include "common_utils.h"
#include "common_ip_info.h"

#define EAP_CONNECT_POPUP			"popup_view"

#define MAX_EAP_PROVISION_NUMBER			3

#define EAP_METHOD_EXP_MENU_ID				0
#define EAP_PROVISION_EXP_MENU_ID			1
#define EAP_AUTH_TYPE_EXP_MENU_ID			2

static bool g_eap_id_show_keypad = FALSE;
static Elm_Genlist_Item_Class g_eap_type_itc;
static Elm_Genlist_Item_Class g_eap_type_sub_itc;
static Elm_Genlist_Item_Class g_eap_provision_itc;
static Elm_Genlist_Item_Class g_eap_provision_sub_itc;
static Elm_Genlist_Item_Class g_eap_auth_itc;
static Elm_Genlist_Item_Class g_eap_auth_sub_itc;
static Elm_Genlist_Item_Class g_eap_ca_cert_itc;
static Elm_Genlist_Item_Class g_eap_user_cert_itc;
static Elm_Genlist_Item_Class g_eap_entry_itc;

typedef enum {
	EAP_SEC_TYPE_UNKNOWN  = 0,
	EAP_SEC_TYPE_PEAP,
	EAP_SEC_TYPE_TLS,
	EAP_SEC_TYPE_TTLS,
	EAP_SEC_TYPE_SIM,
	EAP_SEC_TYPE_AKA,
	EAP_SEC_TYPE_FAST,
	EAP_SEC_TYPE_NULL
} eap_type_t;

typedef enum {
	EAP_SEC_AUTH_NONE  = 0,
	EAP_SEC_AUTH_PAP,
	EAP_SEC_AUTH_MSCHAP,
	EAP_SEC_AUTH_MSCHAPV2,
	EAP_SEC_AUTH_GTC,
	EAP_SEC_AUTH_MD5,
	EAP_SEC_AUTH_NULL
} eap_auth_t;

typedef struct {
	char depth;
	char *name;
	Elm_Genlist_Item_Type flags;
} _Expand_List_t;

struct eap_info_list {
	wifi_ap_h ap;
	Elm_Object_Item *pswd_item;
};

static const _Expand_List_t list_eap_type[] = {
	{1, "UNKNOWN", ELM_GENLIST_ITEM_NONE},
	{1, "PEAP", ELM_GENLIST_ITEM_NONE},
	{1, "TLS", ELM_GENLIST_ITEM_NONE},
	{1, "TTLS", ELM_GENLIST_ITEM_NONE},
	{1, "SIM", ELM_GENLIST_ITEM_NONE},
	{1, "AKA", ELM_GENLIST_ITEM_NONE},
#ifndef DISABLE_FAST_EAP_METHOD
	{1, "FAST", ELM_GENLIST_ITEM_NONE},
#endif
	{1, NULL, ELM_GENLIST_ITEM_NONE}
};

static const _Expand_List_t list_eap_auth[] = {
	{1, "NONE", ELM_GENLIST_ITEM_NONE},
	{1, "PAP", ELM_GENLIST_ITEM_NONE},
	{1, "MSCHAP", ELM_GENLIST_ITEM_NONE},
	{1, "MSCHAPV2", ELM_GENLIST_ITEM_NONE},
	{1, "GTC", ELM_GENLIST_ITEM_NONE},
	{1, "MD5", ELM_GENLIST_ITEM_NONE},
	{1, NULL, ELM_GENLIST_ITEM_NONE}
};

static Evas_Object *radio_main = NULL;

struct common_eap_connect_data {
	int expandable_list_index;

	Elm_Object_Item *eap_type_item;
	Elm_Object_Item *eap_provision_item;
	Elm_Object_Item *eap_auth_item;
	Elm_Object_Item *eap_ca_cert_item;
	Elm_Object_Item *eap_user_cert_item;
	Elm_Object_Item *eap_id_item;
	Elm_Object_Item *eap_anonyid_item;
	Elm_Object_Item *eap_pw_item;
	Evas_Object *popup;

	Evas_Object *genlist;
	Eina_Bool eap_done_ok;
	Evas_Object *win;
	const char *str_pkg_name;
	wifi_ap_h ap;
	ip_info_list_t *ip_info_list;
	Evas_Object* navi_frame;

	int visible_area_width;
	int visible_area_height;
};

static void _gl_eap_provision_sel(void *data, Evas_Object *obj, void *event_info);
static void _gl_eap_auth_sel(void *data, Evas_Object *obj, void *event_info);
static void _create_and_update_list_items_based_on_rules(eap_type_t new_type, common_eap_connect_data_t *eap_data);
static void _delete_eap_entry_items(common_eap_connect_data_t *eap_data);
static eap_type_t __common_eap_connect_popup_get_eap_type(wifi_ap_h ap);
static eap_auth_t __common_eap_connect_popup_get_auth_type(wifi_ap_h ap);
static wifi_eap_type_e __common_eap_connect_popup_get_wlan_eap_type(eap_type_t eap_type);
static wifi_eap_auth_type_e __common_eap_connect_popup_get_wlan_auth_type(eap_auth_t auth_type);

static void _gl_editbox_sel_cb(void *data, Evas_Object *obj, void *event_info)
{
	Elm_Object_Item *item = (Elm_Object_Item *)event_info;
	elm_genlist_item_selected_set(item, FALSE);
}

static void _gl_eap_type_sel(void *data, Evas_Object *obj, void *event_info)
{
	common_eap_connect_data_t *eap_data = (common_eap_connect_data_t *) data;
	Elm_Object_Item *item = (Elm_Object_Item *)event_info;
	Eina_Bool expanded = EINA_FALSE;
	if (item)
		elm_genlist_item_selected_set(item, EINA_FALSE);

	expanded = elm_genlist_item_expanded_get(item);
	if (expanded == FALSE) {
		eap_data->expandable_list_index = EAP_METHOD_EXP_MENU_ID;
	}

	/* Expand/Contract the sub items list */
	elm_genlist_item_expanded_set(item, !expanded);
}

static void _gl_eap_type_sub_sel(void *data, Evas_Object *obj, void *event_info)
{
	common_eap_connect_data_t *eap_data = (common_eap_connect_data_t *) data;
	Elm_Object_Item *item = (Elm_Object_Item *)event_info;
	Elm_Object_Item *parent_item = elm_genlist_item_parent_get(item);

	eap_type_t pre_index = __common_eap_connect_popup_get_eap_type(eap_data->ap);
	eap_type_t selected_item_index = elm_genlist_item_index_get(item) - elm_genlist_item_index_get(parent_item);

	DEBUG_LOG( UG_NAME_NORMAL, "previous index = %d; selected index = %d;", pre_index, selected_item_index);

	/* Contract the sub items list */
	elm_genlist_item_expanded_set(parent_item, EINA_FALSE);

	if (pre_index != selected_item_index) {
//		selected_item_index = __common_eap_connect_popup_get_eap_type(__common_eap_connect_popup_get_wlan_eap_type(selected_item_index));
		_create_and_update_list_items_based_on_rules(selected_item_index, data);
		wifi_eap_type_e type;
		wifi_ap_set_eap_type(eap_data->ap, __common_eap_connect_popup_get_wlan_eap_type(selected_item_index));
		wifi_ap_get_eap_type(eap_data->ap, &type);
		DEBUG_LOG( UG_NAME_NORMAL, "set to new index = %d", type);
		elm_genlist_item_update(parent_item);
	} else {
		DEBUG_LOG( UG_NAME_NORMAL, "pre_index == selected_item_index[%d]", selected_item_index);
	}
}

static void _gl_eap_provision_sel(void *data, Evas_Object *obj, void *event_info)
{
	common_eap_connect_data_t *eap_data = (common_eap_connect_data_t *) data;
	Elm_Object_Item *item = (Elm_Object_Item *)event_info;
	Eina_Bool expanded = EINA_FALSE;
	if (item)
		elm_genlist_item_selected_set(item, EINA_FALSE);

	expanded = elm_genlist_item_expanded_get(item);
	if (expanded == FALSE) {
		eap_data->expandable_list_index = EAP_PROVISION_EXP_MENU_ID;
	}

	/* Expand/Contract the sub items list */
	elm_genlist_item_expanded_set(item, !expanded);
}

static void _gl_eap_provision_sub_sel(void *data, Evas_Object *obj, void *event_info)
{
	Elm_Object_Item *item = (Elm_Object_Item *)event_info;
	Elm_Object_Item *parent_item = elm_genlist_item_parent_get(item);

	/* TODO: Set the EAP provision. No CAPI available now. */

	/* Contract the sub items list */
	elm_genlist_item_expanded_set(parent_item, EINA_FALSE);

	elm_genlist_item_update(parent_item);
}

static void _gl_eap_auth_sel(void *data, Evas_Object *obj, void *event_info)
{
	common_eap_connect_data_t *eap_data = (common_eap_connect_data_t *) data;
	Elm_Object_Item *item = (Elm_Object_Item *)event_info;
	Eina_Bool expanded = EINA_FALSE;
	if (item)
		elm_genlist_item_selected_set(item, EINA_FALSE);

	expanded = elm_genlist_item_expanded_get(item);
	if (expanded == FALSE) {
		eap_data->expandable_list_index = EAP_AUTH_TYPE_EXP_MENU_ID;
	}
	/* Expand/Contract the sub items list */
	elm_genlist_item_expanded_set(item, !expanded);
}

static void _gl_eap_auth_sub_sel(void *data, Evas_Object *obj, void *event_info)
{
	common_eap_connect_data_t *eap_data = (common_eap_connect_data_t *) data;
	Elm_Object_Item *item = (Elm_Object_Item *)event_info;
	Elm_Object_Item *parent_item = elm_genlist_item_parent_get(item);
	eap_auth_t selected_item_index = elm_genlist_item_index_get(item) - elm_genlist_item_index_get(parent_item) - 1;

	wifi_ap_set_eap_auth_type(eap_data->ap, __common_eap_connect_popup_get_wlan_auth_type(selected_item_index));

	/* Contract the sub items list */
	elm_genlist_item_expanded_set(parent_item, EINA_FALSE);

	elm_genlist_item_update(parent_item);
}

static char *_gl_eap_type_text_get(void *data, Evas_Object *obj, const char *part)
{
	common_eap_connect_data_t *eap_data = (common_eap_connect_data_t *)data;
	eap_type_t sel_sub_item_id = __common_eap_connect_popup_get_eap_type(eap_data->ap);
	DEBUG_LOG(UG_NAME_NORMAL, "current selected subitem = %d", sel_sub_item_id);

	if (!strcmp(part, "elm.text.1")) {
		return g_strdup(list_eap_type[sel_sub_item_id].name);
	} else if (!strcmp(part, "elm.text.2")) {
		return g_strdup(sc(eap_data->str_pkg_name, I18N_TYPE_EAP_method));
	}

	return NULL;
}

static char *_gl_eap_subtext_get(void *data, Evas_Object *obj, const char *part)
{
	wlan_eap_type_t eap_type  = (wlan_eap_type_t)elm_radio_state_value_get(data);
	if (!strcmp(part, "elm.text")) {
		return g_strdup(list_eap_type[eap_type].name);
	}

	return NULL;
}

static Evas_Object *_gl_eap_content_get(void *data, Evas_Object *obj, const char *part)
{
	if (!strcmp(part, "elm.icon") || !strcmp(part, "elm.swallow.icon")) {
		return data;
	}

	return NULL;
}

static void _gl_eap_type_sub_menu_item_del(void *data, Evas_Object *obj)
{
	evas_object_unref(data);
}

static char *_gl_eap_provision_text_get(void *data, Evas_Object *obj, const char *part)
{
	common_eap_connect_data_t *eap_data = (common_eap_connect_data_t *)data;
	int sel_sub_item_id = 0;

	/* TODO: Fetch the EAP provision. No CAPI available now. */

	DEBUG_LOG(UG_NAME_NORMAL, "current selected subitem = %d", sel_sub_item_id);

	if (!strcmp(part, "elm.text.1")) {
		return g_strdup_printf("%d", sel_sub_item_id);
	} else if (!strcmp(part, "elm.text.2")) {
		return g_strdup(sc(eap_data->str_pkg_name, I18N_TYPE_Provisioning));
	}

	return NULL;
}

static char *_gl_eap_provision_subtext_get(void *data, Evas_Object *obj, const char *part)
{
	if (!strcmp(part, "elm.text")) {
		return g_strdup_printf("%d", (int)elm_radio_state_value_get(data));
	}

	return NULL;
}

static Evas_Object *_gl_eap_provision_content_get(void *data, Evas_Object *obj, const char *part)
{
	if (!strcmp(part, "elm.icon") || !strcmp(part, "elm.swallow.icon")) {
		return data;
	}

	return NULL;
}

static void _gl_eap_provision_sub_menu_item_del(void *data, Evas_Object *obj)
{
	evas_object_unref(data);
}

static char *_gl_eap_auth_text_get(void *data, Evas_Object *obj, const char *part)
{
	common_eap_connect_data_t *eap_data = (common_eap_connect_data_t *)data;
	eap_auth_t sel_sub_item_id = __common_eap_connect_popup_get_auth_type(eap_data->ap);
	if (!strcmp(part, "elm.text.1")) {
		return g_strdup(list_eap_auth[sel_sub_item_id].name);
	} else if (!strcmp(part, "elm.text.2")) {
		return g_strdup(sc(eap_data->str_pkg_name, I18N_TYPE_Phase_2_authentication));
	}

	return NULL;
}

static char *_gl_eap_auth_subtext_get(void *data, Evas_Object *obj, const char *part)
{
	wlan_eap_auth_type_t eap_auth = (wlan_eap_auth_type_t)elm_radio_state_value_get(data);
	if (!strcmp(part, "elm.text")) {
		return g_strdup(list_eap_auth[eap_auth].name);
	}

	return NULL;
}

static Evas_Object *_gl_eap_auth_content_get(void *data, Evas_Object *obj, const char *part)
{
	if (!strcmp(part, "elm.icon") || !strcmp(part, "elm.swallow.icon")) {
		return data;
	}

	return NULL;
}

static void _gl_eap_auth_sub_menu_item_del(void *data, Evas_Object *obj)
{
	evas_object_unref(data);
}

static char *_gl_eap_ca_cert_text_get(void *data, Evas_Object *obj, const char *part)
{
	common_eap_connect_data_t *eap_data = (common_eap_connect_data_t *)data;
	if (!strcmp(part, "elm.text.2")) {
		return g_strdup(sc(eap_data->str_pkg_name, I18N_TYPE_Ca_Certificate));
	} else if (!strcmp(part, "elm.text.1")) {
		return g_strdup(sc(eap_data->str_pkg_name, I18N_TYPE_Unspecified));
	}

	return NULL;
}

static char *_gl_eap_user_cert_text_get(void *data, Evas_Object *obj, const char *part)
{
	common_eap_connect_data_t *eap_data = (common_eap_connect_data_t *)data;
	if (!strcmp(part, "elm.text.2")) {
		return g_strdup(sc(eap_data->str_pkg_name, I18N_TYPE_User_Certificate));
	} else if (!strcmp(part, "elm.text.1")) {
		return g_strdup(sc(eap_data->str_pkg_name, I18N_TYPE_Unspecified));
	}

	return NULL;
}

static void _gl_eap_entry_cursor_changed_cb(void* data, Evas_Object* obj, void* event_info)
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

static void _gl_eap_entry_changed_cb(void* data, Evas_Object* obj, void* event_info)
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

static void _gl_eap_entry_focused_cb(void *data, Evas_Object *obj, void *event_info)
{
	if (!elm_entry_is_empty(obj))
		elm_object_signal_emit(data, "elm,state,eraser,show", "elm");
	elm_object_signal_emit(data, "elm,state,guidetext,hide", "elm");
}

static void _gl_eap_entry_unfocused_cb(void *data, Evas_Object *obj, void *event_info)
{
	INFO_LOG(UG_NAME_NORMAL, "_gl_eap_entry_unfocused_cb entered");

	if (elm_entry_is_empty(obj))
		elm_object_signal_emit(data, "elm,state,guidetext,show", "elm");
	else {
		elm_object_signal_emit(data, "elm,state,guidetext,hide", "elm");
	}

	elm_object_signal_emit(data, "elm,state,eraser,hide", "elm");
}

static void _gl_eap_entry_eraser_clicked_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	elm_entry_entry_set(data, "");
}

static Evas_Object *_gl_eap_entry_item_content_get(void *data, Evas_Object *obj, const char *part)
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
	Elm_Input_Panel_Layout panel_type = ELM_INPUT_PANEL_LAYOUT_URL;

	Elm_Entry_Filter_Limit_Size limit_filter_data;

	layout = elm_layout_add(obj);
	elm_layout_theme_set(layout, "layout", "editfield", "title");

	entry = elm_entry_add(layout);
	elm_entry_scrollable_set(entry, EINA_TRUE);
	elm_entry_single_line_set(entry, EINA_TRUE);
	elm_object_part_content_set(layout, "elm.swallow.content", entry);

	switch (entry_info->entry_id)
	{
	case ENTRY_TYPE_USER_ID:
		title = entry_info->title_txt;
		guide_txt = entry_info->guide_txt;
		break;
	case ENTRY_TYPE_ANONYMOUS_ID:
		title = entry_info->title_txt;
		guide_txt = entry_info->guide_txt;
		break;
	case ENTRY_TYPE_PASSWORD:
		title = entry_info->title_txt;
		guide_txt = entry_info->guide_txt;
		hide_entry_txt = EINA_TRUE;
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

	Ecore_IMF_Context *imf_ctxt = elm_entry_imf_context_get(entry);
	if (imf_ctxt) {
		ecore_imf_context_input_panel_event_callback_add(imf_ctxt, ECORE_IMF_INPUT_PANEL_STATE_EVENT, entry_info->input_panel_cb, entry_info->input_panel_cb_data);
		DEBUG_LOG(UG_NAME_NORMAL, "set the imf ctxt cbs");
	}

	evas_object_smart_callback_add(entry, "cursor,changed", _gl_eap_entry_cursor_changed_cb, entry_info);
	evas_object_smart_callback_add(entry, "changed", _gl_eap_entry_changed_cb, layout);
	evas_object_smart_callback_add(entry, "focused", _gl_eap_entry_focused_cb, layout);
	evas_object_smart_callback_add(entry, "unfocused", _gl_eap_entry_unfocused_cb, layout);
	elm_object_signal_callback_add(layout, "elm,eraser,clicked", "elm", _gl_eap_entry_eraser_clicked_cb, entry);
	evas_object_show(entry);

	if (ENTRY_TYPE_USER_ID == entry_info->entry_id) {
		if (TRUE == g_eap_id_show_keypad) {
			elm_object_focus_set(entry, EINA_TRUE);
			g_eap_id_show_keypad = FALSE;
		}
	}
	entry_info->layout = layout;
	return layout;
}

static void _gl_eap_entry_item_del(void *data, Evas_Object *obj)
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

static void _gl_exp(void *data, Evas_Object *obj, void *event_info)
{
	Evas_Object *radio;
	common_eap_connect_data_t *eap_data = (common_eap_connect_data_t *)data;
	Elm_Object_Item *item = (Elm_Object_Item *)event_info;
	Elm_Object_Item *sub_item = NULL;
	Evas_Object *gl = elm_object_item_widget_get(item);
	if (gl == NULL) {
		ERROR_LOG(UG_NAME_NORMAL, "gl is NULL");
		return;
	}

	evas_object_focus_set(gl, EINA_TRUE);

	int i = 0;
	eap_type_t eap_type;
	eap_auth_t auth_type;
	INFO_LOG(UG_NAME_RESP, "depth = %d", eap_data->expandable_list_index);
	switch (eap_data->expandable_list_index) {
	case EAP_METHOD_EXP_MENU_ID:
		i = EAP_SEC_TYPE_PEAP;
		eap_type = __common_eap_connect_popup_get_eap_type(eap_data->ap);
		while(list_eap_type[i].name != NULL) {
			radio = common_utils_create_radio_button(obj, i);
			elm_radio_group_add(radio, radio_main);
			evas_object_ref(radio);
			if (i == eap_type)
				elm_radio_value_set(radio, i);
			sub_item = elm_genlist_item_append(gl, &g_eap_type_sub_itc, (void*)radio, item, list_eap_type[i].flags, _gl_eap_type_sub_sel, eap_data);
#ifdef DISABLE_FAST_EAP_METHOD
			if (!g_strcmp0(list_eap_type[i].name, "FAST")) {
				elm_object_item_disabled_set(sub_item, TRUE);
			}
#endif
			i++;
		}
		break;
	case EAP_PROVISION_EXP_MENU_ID:
		while(i <= MAX_EAP_PROVISION_NUMBER) {
			radio = common_utils_create_radio_button(obj, i);
			elm_radio_group_add(radio, radio_main);
			evas_object_ref(radio);
			if (i == 0)	/* TODO: Fetch the EAP provision. CAPI not available now. */
				elm_radio_value_set(radio, i);
			elm_genlist_item_append(gl, &g_eap_provision_sub_itc, (void*)radio, item, ELM_GENLIST_ITEM_NONE, _gl_eap_provision_sub_sel, eap_data);
			i++;
		}
		break;
	case EAP_AUTH_TYPE_EXP_MENU_ID:
		auth_type = __common_eap_connect_popup_get_auth_type(eap_data->ap);
		while(list_eap_auth[i].name != NULL) {
			radio = common_utils_create_radio_button(obj, i);
			elm_radio_group_add(radio, radio_main);
			evas_object_ref(radio);
			if (i == auth_type)
				elm_radio_value_set(radio, i);
			elm_genlist_item_append(gl, &g_eap_auth_sub_itc, (void*)radio, item, list_eap_auth[i].flags, _gl_eap_auth_sub_sel, eap_data);
			i++;
		}
		break;
	default:
		break;
	}
}

static void _gl_con(void *data, Evas_Object *obj, void *event_info)
{
	Elm_Object_Item *item = (Elm_Object_Item *)event_info;

	elm_genlist_item_subitems_clear(item);
}

static void __common_eap_connect_popup_init_item_class(void *data)
{
	g_eap_type_itc.item_style = "dialogue/2text.2/expandable";
	g_eap_type_itc.func.text_get = _gl_eap_type_text_get;
	g_eap_type_itc.func.content_get = NULL;
	g_eap_type_itc.func.state_get = NULL;
	g_eap_type_itc.func.del = NULL;

	g_eap_type_sub_itc.item_style = "dialogue/1text.1icon.2/expandable2";
	g_eap_type_sub_itc.func.text_get = _gl_eap_subtext_get;
	g_eap_type_sub_itc.func.content_get = _gl_eap_content_get;
	g_eap_type_sub_itc.func.state_get = NULL;
	g_eap_type_sub_itc.func.del = _gl_eap_type_sub_menu_item_del;

	g_eap_provision_itc.item_style = "dialogue/2text.2/expandable";
	g_eap_provision_itc.func.text_get = _gl_eap_provision_text_get;
	g_eap_provision_itc.func.content_get = NULL;
	g_eap_provision_itc.func.state_get = NULL;
	g_eap_provision_itc.func.del = NULL;

	g_eap_provision_sub_itc.item_style = "dialogue/1text.1icon.2/expandable2";
	g_eap_provision_sub_itc.func.text_get = _gl_eap_provision_subtext_get;
	g_eap_provision_sub_itc.func.content_get = _gl_eap_provision_content_get;
	g_eap_provision_sub_itc.func.state_get = NULL;
	g_eap_provision_sub_itc.func.del = _gl_eap_provision_sub_menu_item_del;

	g_eap_auth_itc.item_style = "dialogue/2text.2/expandable";
	g_eap_auth_itc.func.text_get = _gl_eap_auth_text_get;
	g_eap_auth_itc.func.content_get = NULL;
	g_eap_auth_itc.func.state_get = NULL;
	g_eap_auth_itc.func.del = NULL;

	g_eap_auth_sub_itc.item_style = "dialogue/1text.1icon.2/expandable2";
	g_eap_auth_sub_itc.func.text_get = _gl_eap_auth_subtext_get;
	g_eap_auth_sub_itc.func.content_get = _gl_eap_auth_content_get;
	g_eap_auth_sub_itc.func.state_get = NULL;
	g_eap_auth_sub_itc.func.del = _gl_eap_auth_sub_menu_item_del;

	g_eap_ca_cert_itc.item_style = "dialogue/2text.2";
	g_eap_ca_cert_itc.func.text_get = _gl_eap_ca_cert_text_get;
	g_eap_ca_cert_itc.func.content_get = NULL;
	g_eap_ca_cert_itc.func.state_get = NULL;
	g_eap_ca_cert_itc.func.del = NULL;

	g_eap_user_cert_itc.item_style = "dialogue/2text.2";
	g_eap_user_cert_itc.func.text_get = _gl_eap_user_cert_text_get;
	g_eap_user_cert_itc.func.content_get = NULL;
	g_eap_user_cert_itc.func.state_get = NULL;
	g_eap_user_cert_itc.func.del = NULL;

	g_eap_entry_itc.item_style = "dialogue/1icon";
	g_eap_entry_itc.func.text_get = NULL;
	g_eap_entry_itc.func.content_get = _gl_eap_entry_item_content_get;
	g_eap_entry_itc.func.state_get = NULL;
	g_eap_entry_itc.func.del = _gl_eap_entry_item_del;
}

static void __common_eap_connect_imf_ctxt_evnt_cb(void *data, Ecore_IMF_Context *ctx, int value)
{
	if (!data)
		return;

	if (value == ECORE_IMF_INPUT_PANEL_STATE_SHOW) {
		DEBUG_LOG(UG_NAME_NORMAL, "Key pad is now open");
		elm_object_item_signal_emit(data, "elm,state,sip,shown", "");
	} else if (value == ECORE_IMF_INPUT_PANEL_STATE_HIDE) {
		DEBUG_LOG(UG_NAME_NORMAL, "Key pad is now close");
		elm_object_item_signal_emit(data, "elm,state,sip,hidden", "");
	}
}

static void __common_eap_connect_imf_ctxt_evnt_resize_cb(void *data, Ecore_IMF_Context *ctx, int value)
{
	__COMMON_FUNC_ENTER__;

	if (!data)
		return;

	if (value == ECORE_IMF_INPUT_PANEL_STATE_WILL_SHOW)
		return;

	common_eap_connect_data_t *eap_data = (common_eap_connect_data_t *)data;
	Evas_Object *box = elm_object_content_get(eap_data->popup);

	__common_popup_size_get(ctx, &eap_data->visible_area_width, &eap_data->visible_area_height);
	evas_object_size_hint_min_set(box, eap_data->visible_area_width * elm_config_scale_get(), 
			eap_data->visible_area_height * elm_config_scale_get());

	__COMMON_FUNC_EXIT__;
}

static void __common_eap_view_set_imf_ctxt_evnt_cb(common_eap_connect_data_t *eap_data)
{
	if (!eap_data)
		return;

	Elm_Object_Item *navi_it = elm_naviframe_top_item_get(eap_data->navi_frame);

	if (!navi_it)
		return;

	if (eap_data->eap_id_item)
		common_utils_set_edit_box_imf_panel_evnt_cb(eap_data->eap_id_item, __common_eap_connect_imf_ctxt_evnt_cb, navi_it);
	if (eap_data->eap_anonyid_item)
		common_utils_set_edit_box_imf_panel_evnt_cb(eap_data->eap_anonyid_item, __common_eap_connect_imf_ctxt_evnt_cb, navi_it);
	if (eap_data->eap_pw_item)
		common_utils_set_edit_box_imf_panel_evnt_cb(eap_data->eap_pw_item, __common_eap_connect_imf_ctxt_evnt_cb, navi_it);
}

static void __common_eap_popup_set_imf_ctxt_evnt_cb(common_eap_connect_data_t *eap_data)
{
	if (!eap_data)
		return;

	if (eap_data->eap_id_item)
		common_utils_set_edit_box_imf_panel_evnt_cb(eap_data->eap_id_item, __common_eap_connect_imf_ctxt_evnt_resize_cb, eap_data);
	if (eap_data->eap_anonyid_item)
		common_utils_set_edit_box_imf_panel_evnt_cb(eap_data->eap_anonyid_item, __common_eap_connect_imf_ctxt_evnt_resize_cb, eap_data);
	if (eap_data->eap_pw_item)
		common_utils_set_edit_box_imf_panel_evnt_cb(eap_data->eap_pw_item, __common_eap_connect_imf_ctxt_evnt_resize_cb, eap_data);
}

/* 
 * This creates EAP type, Auth type, CA certificate, User certificate, User Id, Anonymous Id and Password items.
 */

static void _create_and_update_list_items_based_on_rules(eap_type_t new_type, common_eap_connect_data_t *eap_data)
{
	__COMMON_FUNC_ENTER__;
	Evas_Object* view_list = eap_data->genlist;
	Elm_Object_Item *insert_after_item = NULL;
	eap_type_t pre_type;

	if (NULL == eap_data->eap_type_item) {
		/* Create EAP method/type */
		pre_type = EAP_SEC_TYPE_SIM;
		eap_data->eap_type_item = elm_genlist_item_append(view_list, &g_eap_type_itc, eap_data, NULL, ELM_GENLIST_ITEM_TREE, _gl_eap_type_sel, eap_data);
	} else {
		pre_type = __common_eap_connect_popup_get_eap_type(eap_data->ap);
	}

	switch (new_type) {
	case EAP_SEC_TYPE_PEAP:
	case EAP_SEC_TYPE_TLS:
	case EAP_SEC_TYPE_TTLS:
		if (EAP_SEC_TYPE_UNKNOWN == pre_type || EAP_SEC_TYPE_SIM == pre_type || EAP_SEC_TYPE_AKA == pre_type) {
			insert_after_item = eap_data->eap_type_item;
		} else if (EAP_SEC_TYPE_FAST == pre_type) {
			elm_object_item_del(eap_data->eap_provision_item);
			eap_data->eap_provision_item = NULL;
		}
		break;
	case EAP_SEC_TYPE_SIM:
	case EAP_SEC_TYPE_AKA:
		if (EAP_SEC_TYPE_PEAP == pre_type || EAP_SEC_TYPE_TLS == pre_type || EAP_SEC_TYPE_TTLS == pre_type) {
			_delete_eap_entry_items(eap_data);
		} else if (EAP_SEC_TYPE_FAST == pre_type) {
			elm_object_item_del(eap_data->eap_provision_item);
			eap_data->eap_provision_item = NULL;
			_delete_eap_entry_items(eap_data);
		}
		break;
	case EAP_SEC_TYPE_FAST:
		/* Add EAP provision */
		eap_data->eap_provision_item = elm_genlist_item_insert_after(view_list, &g_eap_provision_itc, eap_data, NULL, eap_data->eap_type_item, ELM_GENLIST_ITEM_TREE, _gl_eap_provision_sel, eap_data);
		DEBUG_LOG(UG_NAME_NORMAL, "current selected provision = %d", 0); // TODO: Fetch the EAP provision. CAPI not yet available.
		if (EAP_SEC_TYPE_UNKNOWN == pre_type || EAP_SEC_TYPE_SIM == pre_type || EAP_SEC_TYPE_AKA == pre_type) {
			insert_after_item = eap_data->eap_provision_item;
		}
		break;
	default:
		break;
	}

	if (insert_after_item) {
		common_utils_entry_info_t *edit_box_details;

		/* Add EAP phase2 authentication */
		eap_data->eap_auth_item = elm_genlist_item_insert_after(view_list, &g_eap_auth_itc, eap_data, NULL, insert_after_item, ELM_GENLIST_ITEM_TREE, _gl_eap_auth_sel, eap_data);

		/* Add CA certificate */
		eap_data->eap_ca_cert_item = elm_genlist_item_insert_after(view_list, &g_eap_ca_cert_itc, eap_data, NULL, eap_data->eap_auth_item, ELM_GENLIST_ITEM_NONE, NULL, NULL);
		elm_genlist_item_select_mode_set(eap_data->eap_ca_cert_item, ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);

		/* Add User certificate */
		eap_data->eap_user_cert_item = elm_genlist_item_insert_after(view_list, &g_eap_user_cert_itc, eap_data, NULL, eap_data->eap_ca_cert_item, ELM_GENLIST_ITEM_NONE, NULL, NULL);
		elm_genlist_item_select_mode_set(eap_data->eap_user_cert_item, ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);

		/* Add EAP ID */
		edit_box_details = g_new0(common_utils_entry_info_t, 1);
		edit_box_details->entry_id = ENTRY_TYPE_USER_ID;
		edit_box_details->title_txt = sc(eap_data->str_pkg_name, I18N_TYPE_Identity);
		edit_box_details->guide_txt = sc(eap_data->str_pkg_name, I18N_TYPE_Enter_Identity);
		eap_data->eap_id_item = elm_genlist_item_insert_after(view_list, &g_eap_entry_itc, edit_box_details, NULL, eap_data->eap_user_cert_item, ELM_GENLIST_ITEM_NONE, _gl_editbox_sel_cb, NULL);
		elm_genlist_item_select_mode_set(eap_data->eap_id_item, ELM_OBJECT_SELECT_MODE_NONE);
		g_eap_id_show_keypad = FALSE;

		/* Add EAP Anonymous Identity */
		edit_box_details = g_new0(common_utils_entry_info_t, 1);
		edit_box_details->entry_id = ENTRY_TYPE_ANONYMOUS_ID;
		edit_box_details->title_txt = sc(eap_data->str_pkg_name, I18N_TYPE_Anonymous_Identity);
		edit_box_details->guide_txt = sc(eap_data->str_pkg_name, I18N_TYPE_Enter_Anonymous_Identity);
		eap_data->eap_anonyid_item = elm_genlist_item_insert_after(view_list, &g_eap_entry_itc, edit_box_details, NULL, eap_data->eap_id_item, ELM_GENLIST_ITEM_NONE, _gl_editbox_sel_cb, NULL);
		elm_genlist_item_select_mode_set(eap_data->eap_anonyid_item, ELM_OBJECT_SELECT_MODE_NONE);

		/* Add EAP Password */
		edit_box_details = g_new0(common_utils_entry_info_t, 1);
		edit_box_details->entry_id = ENTRY_TYPE_PASSWORD;
		edit_box_details->title_txt = sc(eap_data->str_pkg_name, I18N_TYPE_Password);
		edit_box_details->guide_txt = sc(eap_data->str_pkg_name, I18N_TYPE_Enter_password);
		eap_data->eap_pw_item = elm_genlist_item_insert_after(view_list, &g_eap_entry_itc, edit_box_details, NULL, eap_data->eap_anonyid_item, ELM_GENLIST_ITEM_NONE, _gl_editbox_sel_cb, NULL);
		elm_genlist_item_select_mode_set(eap_data->eap_pw_item, ELM_OBJECT_SELECT_MODE_NONE);

		if (eap_data->popup) {	/* Popup */
			__common_eap_popup_set_imf_ctxt_evnt_cb(eap_data);
		} else {			 	/* View */
			__common_eap_view_set_imf_ctxt_evnt_cb(eap_data);
		}
	}
	__COMMON_FUNC_EXIT__;
}

void _delete_eap_entry_items(common_eap_connect_data_t *eap_data)
{
	__COMMON_FUNC_ENTER__;
	elm_object_item_del(eap_data->eap_auth_item);
	eap_data->eap_auth_item = NULL;
	elm_object_item_del(eap_data->eap_ca_cert_item);
	eap_data->eap_ca_cert_item = NULL;
	elm_object_item_del(eap_data->eap_user_cert_item);
	eap_data->eap_user_cert_item = NULL;
	elm_object_item_del(eap_data->eap_id_item);
	eap_data->eap_id_item = NULL;
	elm_object_item_del(eap_data->eap_anonyid_item);
	eap_data->eap_anonyid_item = NULL;
	elm_object_item_del(eap_data->eap_pw_item);
	eap_data->eap_pw_item = NULL;
	__COMMON_FUNC_EXIT__;
}

static Evas_Object* _create_list(Evas_Object* parent, void *data)
{
	__COMMON_FUNC_ENTER__;
	assertm_if(NULL == parent, "NULL!!");

	common_eap_connect_data_t *eap_data = (common_eap_connect_data_t *)data;
	const char* parent_view_name = evas_object_name_get(parent);
	Evas_Object* view_list = NULL;

	__common_eap_connect_popup_init_item_class(eap_data);
	eap_data->eap_done_ok = FALSE;
	eap_data->genlist = view_list = elm_genlist_add(parent);

	if (g_strcmp0(EAP_CONNECT_POPUP, parent_view_name) != 0) {
		elm_object_style_set(view_list, "dialogue");
		common_utils_add_dialogue_separator(view_list, "dialogue/separator.2");
	}

	evas_object_size_hint_weight_set(view_list, EVAS_HINT_EXPAND,
			EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(view_list, EVAS_HINT_FILL, EVAS_HINT_FILL);

	if (!radio_main) {
		radio_main = elm_radio_add(view_list);
		elm_radio_state_value_set(radio_main, 0);
		elm_radio_value_set(radio_main, 0);
	}

	/* Set default values. eap type = PEAP, auth type = MSCHAPv2 */
	wifi_ap_set_eap_type(eap_data->ap, WIFI_EAP_TYPE_PEAP);
	wifi_ap_set_eap_auth_type(eap_data->ap, WIFI_EAP_AUTH_TYPE_MSCHAPV2);

	/* Create the entry items */
	_create_and_update_list_items_based_on_rules(EAP_SEC_TYPE_PEAP, eap_data);

	evas_object_smart_callback_add(view_list, "expanded", _gl_exp, eap_data);
	evas_object_smart_callback_add(view_list, "contracted", _gl_con, view_list);

	__COMMON_FUNC_EXIT__;
	return view_list;
}

static void __common_eap_connect_cleanup(common_eap_connect_data_t *eap_data)
{
	if (eap_data  != NULL) {
		ip_info_remove(eap_data->ip_info_list);
		eap_data->ip_info_list = NULL;
		evas_object_del(eap_data->genlist);
		wifi_ap_destroy(eap_data->ap);
		eap_data->ap = NULL;
		evas_object_del(radio_main);
		radio_main = NULL;
		if (NULL == eap_data->navi_frame) {
			evas_object_del(eap_data->popup);
			/* Lets enable the scan updates */
			wlan_manager_enable_scan_result_update();
		}
		g_free(eap_data);

	}
}

static void __common_eap_connect_destroy(void *data,  Evas_Object *obj, void *event_info)
{
	__COMMON_FUNC_ENTER__;
	common_eap_connect_data_t *eap_data = (common_eap_connect_data_t *) data;
	__common_eap_connect_cleanup(eap_data);
	__COMMON_FUNC_EXIT__;
}

static void __common_eap_connect_done_cb(void *data,  Evas_Object *obj, void *event_info)
{
	__COMMON_FUNC_ENTER__;
	common_eap_connect_data_t *eap_data = (common_eap_connect_data_t *)data;

	if(eap_data->eap_done_ok == TRUE) {
		return;
	}
	eap_data->eap_done_ok = TRUE;

	char* str_id = NULL;
	char* str_pw = NULL;

	wifi_eap_type_e eap_type;

	wifi_ap_set_eap_ca_cert_file(eap_data->ap, "");
	wifi_ap_set_eap_client_cert_file(eap_data->ap, "");
	wifi_ap_set_eap_private_key_info(eap_data->ap, "", "");

	wifi_ap_get_eap_type(eap_data->ap, &eap_type);
	switch (eap_type) {
	case WIFI_EAP_TYPE_PEAP:
	case WIFI_EAP_TYPE_TTLS:
		str_id = common_utils_get_list_item_entry_txt(eap_data->eap_id_item);
		if (!str_id || strlen(str_id) <= 0) {
			common_utils_show_info_ok_popup(eap_data->win, eap_data->str_pkg_name, EAP_CHECK_YOUR_ID_STR);
			eap_data->eap_done_ok = FALSE;
			__COMMON_FUNC_EXIT__;
			return;
		}

		str_pw = common_utils_get_list_item_entry_txt(eap_data->eap_pw_item);
		if (!str_pw || strlen(str_pw) <= 0) {
			common_utils_show_info_ok_popup(eap_data->win, eap_data->str_pkg_name, EAP_CHECK_YOUR_PASWD_STR);
			eap_data->eap_done_ok = FALSE;
			__COMMON_FUNC_EXIT__;
			return;
		}

		char *temp_str = common_utils_get_list_item_entry_txt(eap_data->eap_anonyid_item);
		/* TODO: Set the anonymous id. CAPI not yet available.  */
		g_free(temp_str);

		wifi_ap_set_eap_passphrase(eap_data->ap, str_id, str_pw);
		break;

	case WIFI_EAP_TYPE_TLS:
//		g_strlcpy(p_conn_info->security_info.authentication.eap.username, str_id, NETPM_WLAN_USERNAME_LEN);
//		g_strlcpy(p_conn_info->security_info.authentication.eap.password, str_pw, NETPM_WLAN_USERNAME_LEN);
		wifi_ap_set_eap_ca_cert_file(eap_data->ap, "/mnt/ums/Certification/ca2.pem");
		wifi_ap_set_eap_client_cert_file(eap_data->ap, "/mnt/ums/Certification/user2.pem");
		wifi_ap_set_eap_private_key_info(eap_data->ap, "/mnt/ums/Certification/user2.prv", "wifi");
		break;

	case WIFI_EAP_TYPE_SIM:
	case WIFI_EAP_TYPE_AKA:
		break;

	default:
		/* This case should never occur */
		ERROR_LOG(UG_NAME_NORMAL, "Err!");
		break;
	}

	/* Before we proceed to make a connection, lets save the entered IP data */
	ip_info_save_data(eap_data->ip_info_list);

	int ret = wlan_manager_connect_with_wifi_info(eap_data->ap);
	if (WLAN_MANAGER_ERR_NONE != ret) {
		ERROR_LOG(UG_NAME_NORMAL, "EAP connect request failed!!! Err = %d", ret);
	}

	if (eap_data->navi_frame) {
		eap_view_close(eap_data);
	} else {
		__common_eap_connect_destroy(eap_data, NULL, NULL);
	}

	__COMMON_FUNC_EXIT__;
}

static Eina_Bool __common_eap_connect_show_ime(void *data)
{
	Elm_Object_Item *list_entry_item = (Elm_Object_Item *)data;
	if (!list_entry_item)
		return ECORE_CALLBACK_CANCEL;

	common_utils_entry_info_t *entry_info = elm_object_item_data_get(list_entry_item);
	Evas_Object *lyt = entry_info->layout;
	Evas_Object *entry = common_utils_entry_layout_get_entry(lyt);
	if (!lyt || !entry)
		return ECORE_CALLBACK_CANCEL;

	g_eap_id_show_keypad = TRUE;
	elm_genlist_item_update(list_entry_item);
	return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool __common_eap_connect_load_ip_info_list_cb(void *data)
{
	common_eap_connect_data_t *eap_data = (common_eap_connect_data_t *)data;
	Elm_Object_Item *navi_it = NULL;
	Evas_Object *list = NULL;

	if (!eap_data)
		return ECORE_CALLBACK_CANCEL;

	if (eap_data->navi_frame) {
		Evas_Object *layout = NULL;
		navi_it = elm_naviframe_top_item_get(eap_data->navi_frame);
		layout = elm_object_item_part_content_get(navi_it, "elm.swallow.content");
		list = elm_object_part_content_get(layout, "elm.swallow.content");
		eap_data->ip_info_list = ip_info_append_items(eap_data->ap, eap_data->str_pkg_name, list, __common_eap_connect_imf_ctxt_evnt_cb, navi_it);
	} else {
		Evas_Object *box = elm_object_content_get(eap_data->popup);
		Eina_List *box_childs = elm_box_children_get(box);
		list = eina_list_nth(box_childs, 0);
		eap_data->ip_info_list = ip_info_append_items(eap_data->ap, eap_data->str_pkg_name, list, __common_eap_connect_imf_ctxt_evnt_resize_cb, eap_data);
	}

	/* Add a separator */
	common_utils_add_dialogue_separator(list, "dialogue/separator");

	ecore_idler_add(__common_eap_connect_show_ime, eap_data->eap_id_item);

	return ECORE_CALLBACK_CANCEL;
}

common_eap_connect_data_t *create_eap_connect_view(Evas_Object *win_main, Evas_Object *navi_frame, const char *pkg_name, wifi_device_info_t *device_info)
{
	__COMMON_FUNC_ENTER__;

	if (!win_main || !device_info || !pkg_name)
		return NULL;

	common_eap_connect_data_t *eap_data = g_new0(common_eap_connect_data_t, 1);
	eap_data->str_pkg_name = pkg_name;
	eap_data->win = win_main;

	/* Clone the WiFi AP handle */
	wifi_ap_clone(&(eap_data->ap), device_info->ap);

	/* Create eap connect view */
	Elm_Object_Item* navi_it;
	Evas_Object* button_back;
	Evas_Object *connect_button;
	Evas_Object *layout;
	Evas_Object *list = NULL;

	eap_data->navi_frame = navi_frame;

	layout = common_utils_create_layout(navi_frame);
	/* Create an EAP connect view list */
	list = _create_list(layout, eap_data);
	elm_object_part_content_set(layout, "elm.swallow.content", list);
	navi_it = elm_naviframe_item_push(navi_frame, device_info->ssid, NULL, NULL, layout, NULL);
	evas_object_data_set(navi_frame, SCREEN_TYPE_ID_KEY, (void *)VIEW_MANAGER_VIEW_TYPE_EAP);

	/* Tool bar Connect button */
	connect_button = elm_button_add(navi_frame);
	elm_object_style_set(connect_button, "naviframe/toolbar/default");
	elm_object_text_set(connect_button, sc(pkg_name, I18N_TYPE_Connect));
	evas_object_smart_callback_add(connect_button, "clicked", __common_eap_connect_done_cb, eap_data);
	elm_object_item_part_content_set(navi_it, "toolbar_button1", connect_button);

	/* Tool bar Back button */
	button_back = elm_object_item_part_content_get(navi_it, "prev_btn");
	evas_object_smart_callback_add(button_back, "clicked", __common_eap_connect_destroy, eap_data);

	/* Append ip info items and add a seperator */
	ecore_idler_add(__common_eap_connect_load_ip_info_list_cb, eap_data);

	/* Title Connect button */
	connect_button = elm_button_add(navi_frame);
	elm_object_style_set(connect_button, "naviframe/toolbar/default");
	elm_object_text_set(connect_button, sc(pkg_name, I18N_TYPE_Connect));
	evas_object_smart_callback_add(connect_button, "clicked", __common_eap_connect_done_cb, eap_data);
	elm_object_item_part_content_set(navi_it, "title_toolbar_button1", connect_button);

	/* Title Back button */
	button_back = elm_button_add(navi_frame);
	elm_object_style_set(button_back, "naviframe/back_btn/default");
	evas_object_smart_callback_add(button_back, "clicked", (Evas_Smart_Cb)eap_view_close, eap_data);
	elm_object_item_part_content_set(navi_it, "title_prev_btn", button_back);

	/* Register imf event cbs */
	__common_eap_view_set_imf_ctxt_evnt_cb(eap_data);

	__COMMON_FUNC_EXIT__;

	return eap_data;
}

common_eap_connect_data_t *create_eap_connect_popup(Evas_Object *win_main, const char *pkg_name, wifi_device_info_t *device_info)
{
	__COMMON_FUNC_ENTER__;

	Evas_Object *list = NULL;
	if (!win_main || !device_info || !pkg_name)
		return NULL;

	common_eap_connect_data_t *eap_data = g_new0(common_eap_connect_data_t, 1);
	eap_data->str_pkg_name = pkg_name;
	eap_data->win = win_main;

	/* Clone the WiFi AP handle */
	wifi_ap_clone(&(eap_data->ap), device_info->ap);

	/* Create eap connect popup */
	Evas_Object *popup;
	Evas_Object *box;
	Evas_Object *btn;
	int rotate_angle;
	int visible_area_height;
	int visible_area_width;

	/* Lets disable the scan updates so that the UI is not refreshed un necessarily */
	wlan_manager_disable_scan_result_update();

	eap_data->popup = popup = elm_popup_add(win_main);
	elm_object_style_set(popup, "min_menustyle");
	elm_object_part_text_set(popup, "title,text", device_info->ssid);
	evas_object_size_hint_weight_set(popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(popup, EVAS_HINT_FILL, EVAS_HINT_FILL);

	btn = elm_button_add(popup);
	elm_object_text_set(btn, sc(pkg_name, I18N_TYPE_Connect));
	elm_object_part_content_set(popup, "button1", btn);
	evas_object_smart_callback_add(btn, "clicked", __common_eap_connect_done_cb, eap_data);

	btn = elm_button_add(popup);
	elm_object_text_set(btn, sc(pkg_name, I18N_TYPE_Cancel));
	elm_object_part_content_set(popup, "button2", btn);
	evas_object_smart_callback_add(btn, "clicked", __common_eap_connect_destroy, eap_data);

	/* Create and add a box into the layout. */
	box = elm_box_add(popup);
	evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, 0.0);
	evas_object_size_hint_align_set(box, EVAS_HINT_FILL, 0.0);

	rotate_angle = common_utils_get_rotate_angle(APPCORE_RM_UNKNOWN);

	__common_popup_size_get(NULL ,&visible_area_width, &visible_area_height);
	evas_object_size_hint_min_set(box, visible_area_width * elm_config_scale_get(), visible_area_height * elm_config_scale_get());
	evas_object_name_set(box, EAP_CONNECT_POPUP);

	/* Create an EAP connect view list */
	list = _create_list(box, eap_data);

	/* Append ip info items and add a seperator */
	ecore_idler_add(__common_eap_connect_load_ip_info_list_cb, eap_data);


	/* Pack the list into the box */
	elm_box_pack_end(box, list);
	elm_object_content_set(popup, box);
	evas_object_show(list);
	evas_object_show(box);
	evas_object_show(popup);

	__COMMON_FUNC_EXIT__;

	return eap_data;
}

static wifi_eap_type_e __common_eap_connect_popup_get_wlan_eap_type(eap_type_t eap_type)
{
	wifi_eap_type_e wlan_eap_type = WLAN_SEC_EAP_TYPE_PEAP;
	switch (eap_type) {
	case EAP_SEC_TYPE_PEAP:
		wlan_eap_type = WIFI_EAP_TYPE_PEAP;
		break;
	case EAP_SEC_TYPE_TLS:
		wlan_eap_type = WIFI_EAP_TYPE_TLS;
		break;
	case EAP_SEC_TYPE_TTLS:
		wlan_eap_type = WIFI_EAP_TYPE_TTLS;
		break;
	case EAP_SEC_TYPE_SIM:
		wlan_eap_type = WIFI_EAP_TYPE_SIM;
		break;
	case EAP_SEC_TYPE_AKA:
		wlan_eap_type = WIFI_EAP_TYPE_AKA;
		break;
#ifndef DISABLE_FAST_EAP_METHOD
	/*	Replace 6 with WLAN_SEC_EAP_TYPE_FAST, when libnet supports WLAN_SEC_EAP_TYPE_FAST enum */
	case EAP_SEC_TYPE_FAST:
		wlan_eap_type = 6;
#endif
	default:
		/* This case should never occur */
		ERROR_LOG(UG_NAME_NORMAL, "Err!");
		break;
	}
	return wlan_eap_type;
}

static wifi_eap_auth_type_e __common_eap_connect_popup_get_wlan_auth_type(eap_auth_t auth_type)
{
	wifi_eap_auth_type_e wlan_auth_type = WIFI_EAP_AUTH_TYPE_NONE;
	switch (auth_type) {
	case EAP_SEC_AUTH_NONE:
		wlan_auth_type = WIFI_EAP_AUTH_TYPE_NONE;
		break;
	case EAP_SEC_AUTH_PAP:
		wlan_auth_type = WIFI_EAP_AUTH_TYPE_PAP;
		break;
	case EAP_SEC_AUTH_MSCHAP:
		wlan_auth_type = WIFI_EAP_AUTH_TYPE_MSCHAP;
		break;
	case EAP_SEC_AUTH_MSCHAPV2:
		wlan_auth_type = WIFI_EAP_AUTH_TYPE_MSCHAPV2;
		break;
	case EAP_SEC_AUTH_GTC:
		wlan_auth_type = WIFI_EAP_AUTH_TYPE_GTC;
		break;
	case EAP_SEC_AUTH_MD5:
		wlan_auth_type = WIFI_EAP_AUTH_TYPE_MD5;
		break;
	default:
		/* This case should never occur */
		ERROR_LOG(UG_NAME_NORMAL, "Err!");
		break;
	}
	return wlan_auth_type;
}

static eap_type_t __common_eap_connect_popup_get_eap_type(wifi_ap_h ap)
{
	wifi_eap_type_e wlan_eap_type = 0;
	int ret = wifi_ap_get_eap_type(ap, &wlan_eap_type);
	if (WIFI_ERROR_OPERATION_FAILED == ret) {
		ret = wifi_ap_set_eap_type(ap, WIFI_EAP_TYPE_PEAP);	// Set to default
	}

	if (WIFI_ERROR_NONE != ret) {
		ERROR_LOG(UG_NAME_ERR, "Unable to get the eap type. err = %d", ret);
		return EAP_SEC_TYPE_UNKNOWN;
	}
	INFO_LOG(UG_NAME_NORMAL, "WiFi EAP type = %d", wlan_eap_type);
	switch (wlan_eap_type) {
	case WIFI_EAP_TYPE_PEAP:  /**< EAP PEAP type */
		return EAP_SEC_TYPE_PEAP;

	case WIFI_EAP_TYPE_TLS:  /**< EAP TLS type */
		return EAP_SEC_TYPE_TLS;

	case WIFI_EAP_TYPE_TTLS:  /**< EAP TTLS type */
		return EAP_SEC_TYPE_TTLS;

	case WIFI_EAP_TYPE_SIM:  /**< EAP SIM type */
		return EAP_SEC_TYPE_SIM;

	case WIFI_EAP_TYPE_AKA:  /**< EAP AKA type */
		return EAP_SEC_TYPE_AKA;

#ifndef DISABLE_FAST_EAP_METHOD
	/*	Replace 6 with WLAN_SEC_EAP_TYPE_FAST, when libnet supports WLAN_SEC_EAP_TYPE_FAST enum */
	case 6:
		return EAP_SEC_TYPE_FAST;
#endif

	default:
		return EAP_SEC_TYPE_PEAP;
	}
	return EAP_SEC_TYPE_PEAP;
}

static eap_auth_t __common_eap_connect_popup_get_auth_type(wifi_ap_h ap)
{
	wifi_eap_auth_type_e wlan_auth_type = 0;
	int ret = wifi_ap_get_eap_auth_type(ap, &wlan_auth_type);
	if (WIFI_ERROR_OPERATION_FAILED == ret) {
		ret = wifi_ap_set_eap_auth_type(ap, EAP_SEC_AUTH_NONE);	// Set to default
	}

	if (WIFI_ERROR_NONE != ret) {
		ERROR_LOG(UG_NAME_ERR, "Unable to get the eap auth type. err = %d", ret);
		return EAP_SEC_AUTH_NONE;
	}
	INFO_LOG(UG_NAME_NORMAL, "WiFi EAP auth type = %d", wlan_auth_type);

    switch (wlan_auth_type) {
	case WIFI_EAP_AUTH_TYPE_NONE:  /**< EAP phase2 authentication none */
		return EAP_SEC_AUTH_NONE;

	case WIFI_EAP_AUTH_TYPE_PAP:  /**< EAP phase2 authentication PAP */
		return EAP_SEC_AUTH_PAP;

	case WIFI_EAP_AUTH_TYPE_MSCHAP:  /**< EAP phase2 authentication MSCHAP */
		return EAP_SEC_AUTH_MSCHAP;

	case WIFI_EAP_AUTH_TYPE_MSCHAPV2:  /**< EAP phase2 authentication MSCHAPv2 */
		return EAP_SEC_AUTH_MSCHAPV2;

	case WIFI_EAP_AUTH_TYPE_GTC:  /**< EAP phase2 authentication GTC */
		return EAP_SEC_AUTH_GTC;

	case WIFI_EAP_AUTH_TYPE_MD5:  /**< EAP phase2 authentication MD5 */
		return EAP_SEC_AUTH_MD5;

	default:
		return EAP_SEC_AUTH_NONE;
	}
	return EAP_SEC_AUTH_NONE;
}

/* This creates Auth type, ID, Anonymous Id and Password items
 * This function should be called after creating the EAP type item
 */
eap_info_list_t *eap_info_append_items(wifi_ap_h ap, Evas_Object* view_list, const char *str_pkg_name, imf_ctxt_panel_cb_t input_panel_cb,	void *input_panel_cb_data)
{
	__COMMON_FUNC_ENTER__;
	eap_type_t eap_type;
	eap_auth_t auth_type;
	char *temp_str = NULL;
	Eina_Bool append_continue = TRUE;
	eap_info_list_t *eap_info_list_data = NULL;
	if (!view_list || !str_pkg_name || !ap) {
		ERROR_LOG(UG_NAME_ERR, "Invalid params passed!");
		return NULL;
	}

	eap_info_list_data = g_new0(eap_info_list_t, 1);

	eap_info_list_data->ap = ap;
	eap_type = __common_eap_connect_popup_get_eap_type(ap);
	auth_type = __common_eap_connect_popup_get_auth_type(ap);
	common_utils_add_dialogue_separator(view_list, "dialogue/separator.2");

	common_utils_add_2_line_txt_disabled_item(view_list, "dialogue/2text.2", sc(str_pkg_name, I18N_TYPE_EAP_method), list_eap_type[eap_type].name);

	switch (eap_type) {
	case EAP_SEC_TYPE_UNKNOWN:
	case EAP_SEC_TYPE_PEAP:
	case EAP_SEC_TYPE_TLS:
	case EAP_SEC_TYPE_TTLS:
		break;
	case EAP_SEC_TYPE_FAST:
		/* Add EAP provision */
		/* TODO: Fetch EAP provisioning. CAPI not available. */
		temp_str = g_strdup("");
		common_utils_add_2_line_txt_disabled_item(view_list, "dialogue/2text.2", sc(str_pkg_name, I18N_TYPE_Provisioning), temp_str);
		g_free(temp_str);
		temp_str = NULL;
		break;
	case EAP_SEC_TYPE_SIM:
	case EAP_SEC_TYPE_AKA:
	default:
		append_continue = FALSE;
		break;
	}

	if (append_continue) {
		common_utils_entry_info_t *edit_box_details;

		/* Add EAP phase2 authentication */
		common_utils_add_2_line_txt_disabled_item(view_list, "dialogue/2text.2", sc(str_pkg_name, I18N_TYPE_Phase_2_authentication), list_eap_auth[auth_type].name);

		/* Add CA certificate */
		temp_str = NULL;
		wifi_ap_get_eap_ca_cert_file(ap, &temp_str);
		temp_str = temp_str? temp_str : g_strdup(sc(str_pkg_name, I18N_TYPE_Unspecified));
		common_utils_add_2_line_txt_disabled_item(view_list, "dialogue/2text.2", sc(str_pkg_name, I18N_TYPE_Ca_Certificate), temp_str);
		g_free(temp_str);

		/* Add User certificate */
		temp_str = NULL;
		wifi_ap_get_eap_client_cert_file(ap, &temp_str);
		temp_str = temp_str? temp_str : g_strdup(sc(str_pkg_name, I18N_TYPE_Unspecified));
		common_utils_add_2_line_txt_disabled_item(view_list, "dialogue/2text.2", sc(str_pkg_name, I18N_TYPE_User_Certificate),temp_str);
		g_free(temp_str);

		/* Add EAP ID */
		bool is_paswd_set;
		temp_str = NULL;
		wifi_ap_get_eap_passphrase(ap, &temp_str, &is_paswd_set);
		common_utils_add_2_line_txt_disabled_item(view_list, "dialogue/2text.2", sc(str_pkg_name, I18N_TYPE_Identity), temp_str);
		g_free(temp_str);

		/* Add EAP Anonymous Identity */
		/* TODO: Fetch the anonymous user id. CAPI not available. */
		temp_str = g_strdup("");
		common_utils_add_2_line_txt_disabled_item(view_list, "dialogue/2text.2", sc(str_pkg_name, I18N_TYPE_Anonymous_Identity), temp_str);
		g_free(temp_str);

		/* Add EAP Password */
		g_eap_entry_itc.item_style = "dialogue/1icon";
		g_eap_entry_itc.func.text_get = NULL;
		g_eap_entry_itc.func.content_get = _gl_eap_entry_item_content_get;
		g_eap_entry_itc.func.state_get = NULL;
		g_eap_entry_itc.func.del = _gl_eap_entry_item_del;

		/* TODO: Fetch the password. CAPI not available. */
		if (is_paswd_set)
			temp_str = g_strdup("XXXXX");
		else
			temp_str = g_strdup("");

		edit_box_details = g_new0(common_utils_entry_info_t, 1);
		edit_box_details->entry_id = ENTRY_TYPE_PASSWORD;
		edit_box_details->title_txt = sc(str_pkg_name, I18N_TYPE_Password);
		edit_box_details->entry_txt = temp_str;
		edit_box_details->input_panel_cb = input_panel_cb;
		edit_box_details->input_panel_cb_data = input_panel_cb_data;
		eap_info_list_data->pswd_item = elm_genlist_item_append(view_list, &g_eap_entry_itc, edit_box_details, NULL, ELM_GENLIST_ITEM_NONE, _gl_editbox_sel_cb, NULL);
		elm_genlist_item_select_mode_set(eap_info_list_data->pswd_item, ELM_OBJECT_SELECT_MODE_NONE);
	}

	__COMMON_FUNC_EXIT__;
	return eap_info_list_data;
}

void eap_info_save_data(eap_info_list_t *eap_info_list_data)
{
	if (!eap_info_list_data) {
		ERROR_LOG(UG_NAME_ERR, "Invalid params passed!");
		return;
	}
	char *txt = common_utils_get_list_item_entry_txt(eap_info_list_data->pswd_item);
	DEBUG_LOG(UG_NAME_NORMAL, "Password [%s]", txt);

	wifi_ap_set_eap_passphrase(eap_info_list_data->ap, NULL, txt);
	g_free(txt);
}

void eap_info_remove(eap_info_list_t *eap_info_list_data)
{
	if (!eap_info_list_data) {
		ERROR_LOG(UG_NAME_ERR, "Invalid params passed!");
		return;
	}

	elm_object_item_del(eap_info_list_data->pswd_item);
	eap_info_list_data->pswd_item = NULL;
	g_free(eap_info_list_data);
}

void eap_view_close(common_eap_connect_data_t *eap_data)
{
	if (NULL == eap_data)
		return;

	Evas_Object *nf = eap_data->navi_frame;
	__common_eap_connect_cleanup(eap_data);
	elm_naviframe_item_pop(nf);
}

void eap_view_rotate_popup(common_eap_connect_data_t *eap_data, int rotate_angle)
{
	__COMMON_FUNC_ENTER__;

	if (NULL == eap_data || NULL == eap_data->popup)
		return;

	Evas_Object *box = elm_object_content_get(eap_data->popup);
	__common_popup_size_get(NULL, &eap_data->visible_area_width, &eap_data->visible_area_height);
	evas_object_size_hint_min_set(box, eap_data->visible_area_width * elm_config_scale_get(), eap_data->visible_area_height* elm_config_scale_get());

	__COMMON_FUNC_EXIT__;
}
