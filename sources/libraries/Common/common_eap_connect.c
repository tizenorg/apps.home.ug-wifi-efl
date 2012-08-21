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
#include "common_eap_connect.h"
#include "i18nmanager.h"
#include "common_utils.h"
#include "common_ip_info.h"
#include "common_datamodel.h"

#define COMMON_EAP_CONNECT_POPUP_W			656
#define COMMON_EAP_CONNECT_POPUP_H			918

#define EAP_CONNECT_POPUP			"popup_view"

#define MAX_EAP_PROVISION_NUMBER			3

#define EAP_METHOD_EXP_MENU_ID				0
#define EAP_PROVISION_EXP_MENU_ID			1
#define EAP_AUTH_TYPE_EXP_MENU_ID			2


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
	view_datamodel_eap_info_t *data_object;
	Elm_Object_Item *pswd_item;
};

static const _Expand_List_t list_eap_type[] = {
	{1, "UNKNOWN", ELM_GENLIST_ITEM_NONE},
	{1, "PEAP", ELM_GENLIST_ITEM_NONE},
	{1, "TLS", ELM_GENLIST_ITEM_NONE},
	{1, "TTLS", ELM_GENLIST_ITEM_NONE},
	{1, "SIM", ELM_GENLIST_ITEM_NONE},
	{1, "AKA", ELM_GENLIST_ITEM_NONE},
	{1, "FAST", ELM_GENLIST_ITEM_NONE},
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

	Elm_Genlist_Item_Class *eap_type_itc;
	Elm_Genlist_Item_Class *eap_type_sub_itc;
	Elm_Genlist_Item_Class *eap_provision_itc;
	Elm_Genlist_Item_Class *eap_provision_sub_itc;
	Elm_Genlist_Item_Class *eap_auth_itc;
	Elm_Genlist_Item_Class *eap_ca_cert_itc;
	Elm_Genlist_Item_Class *eap_user_cert_itc;
	Elm_Genlist_Item_Class *eap_auth_sub_itc;

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
	view_datamodel_eap_info_t *data_object;
	ip_info_list_t *ip_info_list;
	void (*eap_closed_cb)(void);
	Elm_Object_Item* navi_it;
};

static void _gl_eap_provision_sel(void *data, Evas_Object *obj, void *event_info);
static void _gl_eap_auth_sel(void *data, Evas_Object *obj, void *event_info);
static void _create_and_update_list_items_based_on_rules(eap_type_t new_type, common_eap_connect_data_t *eap_data);
static void _delete_eap_entry_items(common_eap_connect_data_t *eap_data);
static eap_type_t __common_eap_connect_popup_get_eap_type(view_datamodel_eap_info_t *data_object);
static wlan_eap_type_t __common_eap_connect_popup_get_wlan_eap_type(eap_type_t eap_type);
static eap_auth_t __common_eap_connect_popup_get_auth_type(view_datamodel_eap_info_t *data_object);
static wlan_eap_auth_type_t __common_eap_connect_popup_get_wlan_auth_type(eap_auth_t auth_type);

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

	eap_type_t pre_index = __common_eap_connect_popup_get_eap_type(eap_data->data_object);
	eap_type_t selected_item_index = elm_genlist_item_index_get(item) - elm_genlist_item_index_get(parent_item);

	DEBUG_LOG( UG_NAME_NORMAL, "previous index = %d; selected index = %d;", pre_index, selected_item_index);

	/* Contract the sub items list */
	elm_genlist_item_expanded_set(parent_item, EINA_FALSE);

	if (pre_index != selected_item_index) {
//		selected_item_index = __common_eap_connect_popup_get_eap_type(__common_eap_connect_popup_get_wlan_eap_type(selected_item_index));
		_create_and_update_list_items_based_on_rules(selected_item_index, data);
		view_detail_datamodel_eap_method_set(eap_data->data_object, __common_eap_connect_popup_get_wlan_eap_type(selected_item_index));
		DEBUG_LOG( UG_NAME_NORMAL, "set to new index = %d", view_detail_datamodel_eap_method_get(eap_data->data_object));
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
	common_eap_connect_data_t *eap_data = (common_eap_connect_data_t *) data;
	Elm_Object_Item *item = (Elm_Object_Item *)event_info;
	Elm_Object_Item *parent_item = elm_genlist_item_parent_get(item);
	int selected_item_index = elm_genlist_item_index_get(item) - elm_genlist_item_index_get(parent_item) - 1;

	view_detail_datamodel_eap_provision_set(eap_data->data_object, selected_item_index);

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

	view_detail_datamodel_eap_auth_set(eap_data->data_object, __common_eap_connect_popup_get_wlan_auth_type(selected_item_index));

	/* Contract the sub items list */
	elm_genlist_item_expanded_set(parent_item, EINA_FALSE);

	elm_genlist_item_update(parent_item);
}

static char *_gl_eap_type_text_get(void *data, Evas_Object *obj, const char *part)
{
	common_eap_connect_data_t *eap_data = (common_eap_connect_data_t *)data;
	eap_type_t sel_sub_item_id = __common_eap_connect_popup_get_eap_type(eap_data->data_object);
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
	return;
}

static char *_gl_eap_provision_text_get(void *data, Evas_Object *obj, const char *part)
{
	common_eap_connect_data_t *eap_data = (common_eap_connect_data_t *)data;
	int sel_sub_item_id = view_detail_datamodel_eap_provision_get(eap_data->data_object);
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
	return;
}

static char *_gl_eap_auth_text_get(void *data, Evas_Object *obj, const char *part)
{
	common_eap_connect_data_t *eap_data = (common_eap_connect_data_t *)data;
	eap_auth_t sel_sub_item_id = __common_eap_connect_popup_get_auth_type(eap_data->data_object);
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
	return;
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
	INFO_LOG(UG_NAME_RESP, "depth = %d", eap_data->expandable_list_index);
	switch (eap_data->expandable_list_index) {
	case EAP_METHOD_EXP_MENU_ID:
		i = EAP_SEC_TYPE_PEAP;
		while(list_eap_type[i].name != NULL) {
			radio = common_utils_create_radio_button(obj, i);
			elm_radio_group_add(radio, radio_main);
			evas_object_ref(radio);
			if (i == __common_eap_connect_popup_get_eap_type(eap_data->data_object))
				elm_radio_value_set(radio, i);
			sub_item = elm_genlist_item_append(gl, eap_data->eap_type_sub_itc, (void*)radio, item, list_eap_type[i].flags, _gl_eap_type_sub_sel, eap_data);
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
			if (i == view_detail_datamodel_eap_provision_get(eap_data->data_object))
				elm_radio_value_set(radio, i);
			elm_genlist_item_append(gl, eap_data->eap_provision_sub_itc, (void*)radio, item, ELM_GENLIST_ITEM_NONE, _gl_eap_provision_sub_sel, eap_data);
			i++;
		}
		break;
	case EAP_AUTH_TYPE_EXP_MENU_ID:
		while(list_eap_auth[i].name != NULL) {
			radio = common_utils_create_radio_button(obj, i);
			elm_radio_group_add(radio, radio_main);
			evas_object_ref(radio);
			if (i == __common_eap_connect_popup_get_auth_type(eap_data->data_object))
				elm_radio_value_set(radio, i);
			elm_genlist_item_append(gl, eap_data->eap_auth_sub_itc, (void*)radio, item, list_eap_auth[i].flags, _gl_eap_auth_sub_sel, eap_data);
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
	common_eap_connect_data_t *eap_data = (common_eap_connect_data_t *)data;
	eap_data->eap_type_itc = elm_genlist_item_class_new();
	eap_data->eap_type_sub_itc = elm_genlist_item_class_new();
	eap_data->eap_provision_itc = elm_genlist_item_class_new();
	eap_data->eap_provision_sub_itc = elm_genlist_item_class_new();
	eap_data->eap_auth_itc = elm_genlist_item_class_new();
	eap_data->eap_auth_sub_itc = elm_genlist_item_class_new();
	eap_data->eap_ca_cert_itc = elm_genlist_item_class_new();
	eap_data->eap_user_cert_itc = elm_genlist_item_class_new();

	eap_data->eap_type_itc->item_style = "dialogue/2text.2/expandable";
	eap_data->eap_type_itc->func.text_get = _gl_eap_type_text_get;
	eap_data->eap_type_itc->func.content_get = NULL;
	eap_data->eap_type_itc->func.state_get = NULL;
	eap_data->eap_type_itc->func.del = NULL;

	eap_data->eap_type_sub_itc->item_style = "dialogue/1text.1icon.2/expandable2";
	eap_data->eap_type_sub_itc->func.text_get = _gl_eap_subtext_get;
	eap_data->eap_type_sub_itc->func.content_get = _gl_eap_content_get;
	eap_data->eap_type_sub_itc->func.state_get = NULL;
	eap_data->eap_type_sub_itc->func.del = _gl_eap_type_sub_menu_item_del;

	eap_data->eap_provision_itc->item_style = "dialogue/2text.2/expandable";
	eap_data->eap_provision_itc->func.text_get = _gl_eap_provision_text_get;
	eap_data->eap_provision_itc->func.content_get = NULL;
	eap_data->eap_provision_itc->func.state_get = NULL;
	eap_data->eap_provision_itc->func.del = NULL;

	eap_data->eap_provision_sub_itc->item_style = "dialogue/1text.1icon.2/expandable2";
	eap_data->eap_provision_sub_itc->func.text_get = _gl_eap_provision_subtext_get;
	eap_data->eap_provision_sub_itc->func.content_get = _gl_eap_provision_content_get;
	eap_data->eap_provision_sub_itc->func.state_get = NULL;
	eap_data->eap_provision_sub_itc->func.del = _gl_eap_provision_sub_menu_item_del;

	eap_data->eap_auth_itc->item_style = "dialogue/2text.2/expandable";
	eap_data->eap_auth_itc->func.text_get = _gl_eap_auth_text_get;
	eap_data->eap_auth_itc->func.content_get = NULL;
	eap_data->eap_auth_itc->func.state_get = NULL;
	eap_data->eap_auth_itc->func.del = NULL;

	eap_data->eap_auth_sub_itc->item_style = "dialogue/1text.1icon.2/expandable2";
	eap_data->eap_auth_sub_itc->func.text_get = _gl_eap_auth_subtext_get;
	eap_data->eap_auth_sub_itc->func.content_get = _gl_eap_auth_content_get;
	eap_data->eap_auth_sub_itc->func.state_get = NULL;
	eap_data->eap_auth_sub_itc->func.del = _gl_eap_auth_sub_menu_item_del;

	eap_data->eap_ca_cert_itc->item_style = "dialogue/2text.2";
	eap_data->eap_ca_cert_itc->func.text_get = _gl_eap_ca_cert_text_get;
	eap_data->eap_ca_cert_itc->func.content_get = NULL;
	eap_data->eap_ca_cert_itc->func.state_get = NULL;
	eap_data->eap_ca_cert_itc->func.del = NULL;

	eap_data->eap_user_cert_itc->item_style = "dialogue/2text.2";
	eap_data->eap_user_cert_itc->func.text_get = _gl_eap_user_cert_text_get;
	eap_data->eap_user_cert_itc->func.content_get = NULL;
	eap_data->eap_user_cert_itc->func.state_get = NULL;
	eap_data->eap_user_cert_itc->func.del = NULL;

	return;
}

static void __common_eap_connect_popup_deinit_item_class(void *data)
{
	common_eap_connect_data_t *eap_data = (common_eap_connect_data_t *)data;

	elm_genlist_item_class_free(eap_data->eap_type_itc);
	elm_genlist_item_class_free(eap_data->eap_type_sub_itc);
	elm_genlist_item_class_free(eap_data->eap_provision_itc);
	elm_genlist_item_class_free(eap_data->eap_provision_sub_itc);
	elm_genlist_item_class_free(eap_data->eap_auth_itc);
	elm_genlist_item_class_free(eap_data->eap_auth_sub_itc);
	elm_genlist_item_class_free(eap_data->eap_ca_cert_itc);
	elm_genlist_item_class_free(eap_data->eap_user_cert_itc);

	eap_data->eap_type_itc = NULL;
	eap_data->eap_type_sub_itc = NULL;
	eap_data->eap_provision_itc = NULL;
	eap_data->eap_provision_sub_itc = NULL;
	eap_data->eap_auth_itc = NULL;
	eap_data->eap_auth_sub_itc = NULL;
	eap_data->eap_ca_cert_itc = NULL;
	eap_data->eap_user_cert_itc = NULL;
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
		eap_data->eap_type_item = elm_genlist_item_append(view_list, eap_data->eap_type_itc, eap_data, NULL, ELM_GENLIST_ITEM_TREE, _gl_eap_type_sel, eap_data);
	} else {
		pre_type = __common_eap_connect_popup_get_eap_type(eap_data->data_object);
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
		eap_data->eap_provision_item = elm_genlist_item_insert_after(view_list, eap_data->eap_provision_itc, eap_data, NULL, eap_data->eap_type_item, ELM_GENLIST_ITEM_TREE, _gl_eap_provision_sel, eap_data);
		DEBUG_LOG(UG_NAME_NORMAL, "current selected provision = %d", view_detail_datamodel_eap_provision_get(eap_data->data_object));
		if (EAP_SEC_TYPE_UNKNOWN == pre_type || EAP_SEC_TYPE_SIM == pre_type || EAP_SEC_TYPE_AKA == pre_type) {
			insert_after_item = eap_data->eap_provision_item;
		}
		break;
	default:
		break;
	}

	if (insert_after_item) {
		/* Add EAP phase2 authentication */
		eap_data->eap_auth_item = elm_genlist_item_insert_after(view_list, eap_data->eap_auth_itc, eap_data, NULL, insert_after_item, ELM_GENLIST_ITEM_TREE, _gl_eap_auth_sel, eap_data);

		/* Add CA certificate */
		eap_data->eap_ca_cert_item = elm_genlist_item_insert_after(view_list, eap_data->eap_ca_cert_itc, eap_data, NULL, eap_data->eap_auth_item, ELM_GENLIST_ITEM_NONE, NULL, NULL);
		elm_genlist_item_select_mode_set(eap_data->eap_ca_cert_item, ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);

		/* Add User certificate */
		eap_data->eap_user_cert_item = elm_genlist_item_insert_after(view_list, eap_data->eap_user_cert_itc, eap_data, NULL, eap_data->eap_ca_cert_item, ELM_GENLIST_ITEM_NONE, NULL, NULL);
		elm_genlist_item_select_mode_set(eap_data->eap_user_cert_item, ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);

		/* Add EAP ID */
		eap_data->eap_id_item = common_utils_add_edit_box_to_list(view_list, eap_data->eap_user_cert_item, sc(eap_data->str_pkg_name, I18N_TYPE_Identity), "", sc(eap_data->str_pkg_name, I18N_TYPE_Enter_Identity), ELM_INPUT_PANEL_LAYOUT_URL);

		/* Add EAP Anonymous Identity */
		eap_data->eap_anonyid_item = common_utils_add_edit_box_to_list(view_list, eap_data->eap_id_item, sc(eap_data->str_pkg_name, I18N_TYPE_Anonymous_Identity), "", sc(eap_data->str_pkg_name, I18N_TYPE_Enter_Anonymous_Identity), ELM_INPUT_PANEL_LAYOUT_URL);

		/* Add EAP Password */
		eap_data->eap_pw_item = common_utils_add_edit_box_to_list(view_list, eap_data->eap_anonyid_item, sc(eap_data->str_pkg_name, I18N_TYPE_Password), "", sc(eap_data->str_pkg_name, I18N_TYPE_Enter_password), ELM_INPUT_PANEL_LAYOUT_URL);
		common_utils_entry_password_set(elm_object_item_data_get(eap_data->eap_pw_item), TRUE);
	}
	__COMMON_FUNC_EXIT__;
	return;
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
	return;
}

static Evas_Object* _create_list(Evas_Object* parent, void *data)
{
	__COMMON_FUNC_ENTER__;
	assertm_if(NULL == parent, "NULL!!");

	const char* parent_view_name = evas_object_name_get(parent);

	Evas_Object* view_list = elm_genlist_add(parent);

	if (g_strcmp0(EAP_CONNECT_POPUP, parent_view_name) != 0)
		elm_object_style_set(view_list, "dialogue");

	common_eap_connect_data_t *eap_data = (common_eap_connect_data_t *)data;

	assertm_if(NULL == view_list, "NULL!!");
	evas_object_size_hint_weight_set(view_list, EVAS_HINT_EXPAND,
			EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(view_list, EVAS_HINT_FILL, EVAS_HINT_FILL);

	eap_data->eap_done_ok = FALSE;

	__common_eap_connect_popup_init_item_class(eap_data);

	eap_data->genlist = view_list;

	if (!radio_main) {
		radio_main = elm_radio_add(view_list);
		elm_radio_state_value_set(radio_main, 0);
		elm_radio_value_set(radio_main, 0);
	}

	if (g_strcmp0(EAP_CONNECT_POPUP, parent_view_name) != 0)
		common_utils_add_dialogue_separator(view_list, "dialogue/separator");

	/* Create the entry items */
	_create_and_update_list_items_based_on_rules(view_detail_datamodel_eap_method_get(eap_data->data_object), eap_data);

	evas_object_smart_callback_add(view_list, "expanded", _gl_exp, eap_data);
	evas_object_smart_callback_add(view_list, "contracted", _gl_con, view_list);

	__COMMON_FUNC_EXIT__;
	return view_list;
}

static void __common_eap_connect_destroy(void *data,  Evas_Object *obj, void *event_info)
{
	__COMMON_FUNC_ENTER__;

	common_eap_connect_data_t *eap_data = (common_eap_connect_data_t *) data;

	if (eap_data  != NULL) {
		evas_object_del(eap_data->genlist);
		__common_eap_connect_popup_deinit_item_class(eap_data);
		evas_object_del(eap_data->popup);
		ip_info_remove(eap_data->ip_info_list);
		eap_data->ip_info_list = NULL;
		view_detail_datamodel_eap_info_destroy(eap_data->data_object);
		eap_data->data_object = NULL;
		evas_object_del(radio_main);
		radio_main = NULL;
		if (eap_data->eap_closed_cb)
			eap_data->eap_closed_cb();
		g_free(eap_data);

		/* Lets enable the scan updates */
		wlan_manager_enable_scan_result_update();
	}

	__COMMON_FUNC_EXIT__;
}

static wlan_eap_type_t __common_eap_connect_popup_get_wlan_eap_type(eap_type_t eap_type)
{
	wlan_eap_type_t wlan_eap_type = WLAN_SEC_EAP_TYPE_PEAP;
	switch (eap_type) {
	case EAP_SEC_TYPE_PEAP:
		wlan_eap_type = WLAN_SEC_EAP_TYPE_PEAP;
		break;
	case EAP_SEC_TYPE_TLS:
		wlan_eap_type = WLAN_SEC_EAP_TYPE_TLS;
		break;
	case EAP_SEC_TYPE_TTLS:
		wlan_eap_type = WLAN_SEC_EAP_TYPE_TTLS;
		break;
	case EAP_SEC_TYPE_SIM:
		wlan_eap_type = WLAN_SEC_EAP_TYPE_SIM;
		break;
	case EAP_SEC_TYPE_AKA:
		wlan_eap_type = WLAN_SEC_EAP_TYPE_AKA;
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

static wlan_eap_auth_type_t __common_eap_connect_popup_get_wlan_auth_type(eap_auth_t auth_type)
{
	wlan_eap_auth_type_t wlan_auth_type = WLAN_SEC_EAP_AUTH_NONE;
	switch (auth_type) {
	case EAP_SEC_AUTH_NONE:
		wlan_auth_type = WLAN_SEC_EAP_AUTH_NONE;
		break;
	case EAP_SEC_AUTH_PAP:
		wlan_auth_type = WLAN_SEC_EAP_AUTH_PAP;
		break;
	case EAP_SEC_AUTH_MSCHAP:
		wlan_auth_type = WLAN_SEC_EAP_AUTH_MSCHAP;
		break;
	case EAP_SEC_AUTH_MSCHAPV2:
		wlan_auth_type = WLAN_SEC_EAP_AUTH_MSCHAPV2;
		break;
	case EAP_SEC_AUTH_GTC:
		wlan_auth_type = WLAN_SEC_EAP_AUTH_GTC;
		break;
	case EAP_SEC_AUTH_MD5:
		wlan_auth_type = WLAN_SEC_EAP_AUTH_MD5;
		break;
	default:
		/* This case should never occur */
		ERROR_LOG(UG_NAME_NORMAL, "Err!");
		break;
	}
	return wlan_auth_type;
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

	wlan_eap_type_t eap_type;
	wlan_eap_auth_type_t auth_type;
	net_wifi_connection_info_t *p_conn_info = NULL;
	p_conn_info = g_malloc0(sizeof(net_wifi_connection_info_t));
	p_conn_info->wlan_mode = NETPM_WLAN_CONNMODE_INFRA;
	p_conn_info->security_info.sec_mode = WLAN_SEC_MODE_IEEE8021X;

	eap_type = view_detail_datamodel_eap_method_get(eap_data->data_object);
	switch (eap_type) {
	case WLAN_SEC_EAP_TYPE_PEAP:
	case WLAN_SEC_EAP_TYPE_TTLS:
		str_id = common_utils_get_list_item_entry_txt(eap_data->eap_id_item);
		if (strlen(str_id) <= 0) {
			common_utils_show_info_ok_popup(eap_data->win, eap_data->str_pkg_name, EAP_CHECK_YOUR_ID_STR);
			eap_data->eap_done_ok = FALSE;
			goto eap_done_cleanup;
		}

		str_pw = common_utils_get_list_item_entry_txt(eap_data->eap_pw_item);
		if (strlen(str_pw) <= 0) {
			common_utils_show_info_ok_popup(eap_data->win, eap_data->str_pkg_name, EAP_CHECK_YOUR_PASWD_STR);
			eap_data->eap_done_ok = FALSE;
			goto eap_done_cleanup;
		}

		char *temp_str = common_utils_get_list_item_entry_txt(eap_data->eap_anonyid_item);
		view_detail_datamodel_eap_anonymous_id_set(eap_data->data_object, temp_str);
		g_free(temp_str);

		p_conn_info->security_info.authentication.eap.eap_type = eap_type;

		auth_type = view_detail_datamodel_eap_auth_get(eap_data->data_object);
		p_conn_info->security_info.authentication.eap.eap_auth = auth_type;//__common_eap_connect_popup_get_authentication_type(auth_type);
		g_strlcpy(p_conn_info->security_info.authentication.eap.username, str_id, NETPM_WLAN_USERNAME_LEN);
		g_strlcpy(p_conn_info->security_info.authentication.eap.password, str_pw, NETPM_WLAN_PASSWORD_LEN);
		view_detail_datamodel_eap_user_id_set(eap_data->data_object, str_id);
		view_detail_datamodel_eap_pswd_set(eap_data->data_object, str_pw);
		break;

	case WLAN_SEC_EAP_TYPE_TLS:
		p_conn_info->security_info.authentication.eap.eap_type = eap_type;
		auth_type = view_detail_datamodel_eap_auth_get(eap_data->data_object);
		p_conn_info->security_info.authentication.eap.eap_auth = auth_type;//__common_eap_connect_popup_get_authentication_type(auth_type);

//		g_strlcpy(p_conn_info->security_info.authentication.eap.username, str_id, NETPM_WLAN_USERNAME_LEN);
//		g_strlcpy(p_conn_info->security_info.authentication.eap.password, str_pw, NETPM_WLAN_USERNAME_LEN);
		g_strlcpy(p_conn_info->security_info.authentication.eap.ca_cert_filename, "/mnt/ums/Certification/ca2.pem", NETPM_WLAN_CA_CERT_FILENAME_LEN);
		g_strlcpy(p_conn_info->security_info.authentication.eap.client_cert_filename, "/mnt/ums/Certification/user2.pem", NETPM_WLAN_CLIENT_CERT_FILENAME_LEN);
		g_strlcpy(p_conn_info->security_info.authentication.eap.private_key_filename, "/mnt/ums/Certification/user2.prv", NETPM_WLAN_PRIVATE_KEY_FILENAME_LEN);
		g_strlcpy(p_conn_info->security_info.authentication.eap.private_key_passwd, "wifi", NETPM_WLAN_PRIVATE_KEY_PASSWD_LEN);
		break;

	case WLAN_SEC_EAP_TYPE_SIM:
		p_conn_info->security_info.sec_mode = WLAN_SEC_MODE_IEEE8021X;
		p_conn_info->security_info.authentication.eap.eap_type = eap_type;
		break;

	case WLAN_SEC_EAP_TYPE_AKA:
		p_conn_info->security_info.sec_mode = WLAN_SEC_MODE_IEEE8021X;
		p_conn_info->security_info.authentication.eap.eap_type = eap_type;
		break;

	default:
		/* This case should never occur */
		ERROR_LOG(UG_NAME_NORMAL, "Err!");
		break;
	}

	/* Before we proceed to make a connection, lets save the entered IP data */
	ip_info_save_data(eap_data->ip_info_list, TRUE);
	view_detail_datamodel_save_eap_info_if_modified(eap_data->data_object);

	char *temp_str = view_detail_datamodel_eap_ap_name_get(eap_data->data_object);
	g_strlcpy(p_conn_info->essid, temp_str, NET_WLAN_ESSID_LEN);
	g_free(temp_str);

	if (WLAN_MANAGER_ERR_NONE != connman_request_connection_open_hidden_ap(p_conn_info)) {
		ERROR_LOG(UG_NAME_NORMAL, "EAP connect request failed!!!");
	}

	if (eap_data->navi_it) {
		eap_view_close(eap_data);
	} else {
		Evas_Object *cancel_btn = elm_object_part_content_get(eap_data->popup, "button2");
		evas_object_smart_callback_call(cancel_btn, "clicked", NULL);
	}

eap_done_cleanup:
	g_free(p_conn_info);
	g_free(str_id);
	g_free(str_pw);

	__COMMON_FUNC_EXIT__;
}

common_eap_connect_data_t *create_eap_connect(Evas_Object *win_main, Evas_Object *navi_frame, const char *pkg_name, wifi_device_info_t *device_info, eap_view_close_cb_t cb)
{
	__COMMON_FUNC_ENTER__;

	Evas_Object *list = NULL;
	if (!win_main || !device_info || !pkg_name)
		return NULL;

	common_eap_connect_data_t *eap_data = (common_eap_connect_data_t *)g_malloc0(sizeof(common_eap_connect_data_t));
	eap_data->str_pkg_name = pkg_name;
	eap_data->win = win_main;

	/* Create the MVC object */
	eap_data->data_object = view_detail_datamodel_eap_info_create(device_info->profile_name);

	if (!device_info->profile_name) {
		/* This means a dummy eap data object has been created. */
		/* This situation can occur during hidden ap case. */
		/* Lets set the ssid */
		view_detail_datamodel_eap_ap_name_set(eap_data->data_object, device_info->ssid);
	}

	if (navi_frame) {	/* Create eap connect view */
		Evas_Object *layout;
		Evas_Object *conform;
		Elm_Object_Item* navi_it;
		Evas_Object* toolbar;
		Evas_Object* button_back;

		layout = common_utils_create_conformant_layout(navi_frame);
		conform = elm_object_part_content_get(layout, "elm.swallow.content");

		/* Create an EAP connect view list */
		list = _create_list(conform, eap_data);

		/* Append ip info items */
		eap_data->ip_info_list = ip_info_append_items(device_info->profile_name, pkg_name, list);

		/* Add a separator */
		common_utils_add_dialogue_separator(list, "dialogue/separator/end");

		elm_object_content_set(conform, list);

		eap_data->navi_it = navi_it = elm_naviframe_item_push(navi_frame, device_info->ssid, NULL, NULL, layout, NULL);
		evas_object_data_set(navi_frame, SCREEN_TYPE_ID_KEY, (void *)VIEW_MANAGER_VIEW_TYPE_EAP);
		eap_data->eap_closed_cb = cb;
		toolbar = elm_toolbar_add(navi_frame);
		elm_toolbar_shrink_mode_set(toolbar, ELM_TOOLBAR_SHRINK_EXPAND);

		elm_toolbar_item_append(toolbar,
								NULL,
								sc(pkg_name, I18N_TYPE_Connect),
								__common_eap_connect_done_cb,
								eap_data);
		/* Add a dummy item */
		elm_object_item_disabled_set(elm_toolbar_item_append(toolbar, NULL, NULL, NULL, NULL), EINA_TRUE);

		/* Add the control bar to the naviframe */
		elm_object_item_part_content_set(navi_it, "controlbar", toolbar);

		button_back = elm_object_item_part_content_get(navi_it, "prev_btn");
		elm_object_focus_allow_set(button_back, EINA_TRUE);
		evas_object_smart_callback_add(button_back, "clicked", __common_eap_connect_destroy, eap_data);

	} else {	/* Create eap connect popup */
		Evas_Object *popup;
		Evas_Object *box;
		Evas_Object *layout;
		Evas_Object *btn;
		Evas_Object *conformant;

		/* Lets disable the scan updates so that the UI is not refreshed un necessarily */
		wlan_manager_disable_scan_result_update();

		conformant = elm_conformant_add(win_main);
		assertm_if(NULL == conformant, "conformant is NULL!!");
		elm_win_conformant_set(win_main, EINA_TRUE);
		elm_win_resize_object_add(win_main, conformant);
		evas_object_size_hint_weight_set(conformant, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		evas_object_size_hint_align_set(conformant, EVAS_HINT_FILL, EVAS_HINT_FILL);

		layout = elm_layout_add(conformant);

		eap_data->popup = popup = elm_popup_add(layout);
		elm_object_style_set(popup, "content_expand");
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
		evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
		evas_object_size_hint_min_set(box, COMMON_EAP_CONNECT_POPUP_W * elm_config_scale_get(), COMMON_EAP_CONNECT_POPUP_H * elm_config_scale_get());

		evas_object_name_set(box, EAP_CONNECT_POPUP);

		/* Create an EAP connect view list */
		list = _create_list(box, eap_data);

		/* Append ip info items */
		eap_data->ip_info_list = ip_info_append_items(device_info->profile_name, pkg_name, list);

		/* Add a separator */
		common_utils_add_dialogue_separator(list, "dialogue/separator/end");

		/* Pack the list into the box */
		elm_box_pack_end(box, list);
		elm_object_content_set(popup, box);
		elm_object_content_set(layout, popup);
		elm_object_content_set(conformant, layout);
		evas_object_show(list);
		evas_object_show(box);
		evas_object_show(popup);
		evas_object_show(layout);
		evas_object_show(conformant);
	}

	__COMMON_FUNC_EXIT__;

	return eap_data;
}

static eap_type_t __common_eap_connect_popup_get_eap_type(view_datamodel_eap_info_t *data_object)
{
	wlan_eap_type_t wlan_eap_type;
	wlan_eap_type = view_detail_datamodel_eap_method_get(data_object);
	switch (wlan_eap_type) {
	case WLAN_SEC_EAP_TYPE_PEAP:
		return EAP_SEC_TYPE_PEAP;

	case WLAN_SEC_EAP_TYPE_TLS:
		return EAP_SEC_TYPE_TLS;

	case WLAN_SEC_EAP_TYPE_TTLS:
		return EAP_SEC_TYPE_TTLS;

	case WLAN_SEC_EAP_TYPE_SIM:
		return EAP_SEC_TYPE_SIM;

	case WLAN_SEC_EAP_TYPE_AKA:
		return EAP_SEC_TYPE_AKA;

#ifndef DISABLE_FAST_EAP_METHOD
	/*	Replace 6 with WLAN_SEC_EAP_TYPE_FAST, when libnet supports WLAN_SEC_EAP_TYPE_FAST enum */
	case 6:
		return EAP_SEC_TYPE_FAST;
#endif

	default:
		return EAP_SEC_TYPE_UNKNOWN;
	}
	return EAP_SEC_TYPE_UNKNOWN;
}

static eap_auth_t __common_eap_connect_popup_get_auth_type(view_datamodel_eap_info_t *data_object)
{
	wlan_eap_auth_type_t wlan_auth_type;
	wlan_auth_type = view_detail_datamodel_eap_auth_get(data_object);
	switch (wlan_auth_type) {
	case WLAN_SEC_EAP_AUTH_NONE:
		return EAP_SEC_AUTH_NONE;

	case WLAN_SEC_EAP_AUTH_PAP:
		return EAP_SEC_AUTH_PAP;

	case WLAN_SEC_EAP_AUTH_MSCHAP:
		return EAP_SEC_AUTH_MSCHAP;

	case WLAN_SEC_EAP_AUTH_MSCHAPV2:
		return EAP_SEC_AUTH_MSCHAPV2;

	case WLAN_SEC_EAP_AUTH_GTC:
		return EAP_SEC_AUTH_GTC;

	case WLAN_SEC_EAP_AUTH_MD5:
		return EAP_SEC_AUTH_MD5;

	default:
		return EAP_SEC_AUTH_NONE;
	}
	return EAP_SEC_AUTH_NONE;
}

/* This creates Auth type, ID, Anonymous Id and Password items
 * This function should be called after creating the EAP type item
 */
eap_info_list_t *eap_info_append_items(const char *profile_name, Evas_Object* view_list, const char *str_pkg_name)
{
	__COMMON_FUNC_ENTER__;
	eap_type_t eap_type;
	eap_auth_t auth_type;
	char *temp_str = NULL;
	Eina_Bool append_continue = TRUE;
	Elm_Object_Item *password_entry_item = NULL;
	eap_info_list_t *eap_info_list_data = NULL;
	view_datamodel_eap_info_t *data_object = NULL;
	if (!view_list || !str_pkg_name || !profile_name) {
		ERROR_LOG(UG_NAME_ERR, "Invalid params passed!");
		return NULL;
	}

	eap_info_list_data = (eap_info_list_t *)g_malloc0(sizeof(eap_info_list_t));

	eap_info_list_data->data_object = data_object = view_detail_datamodel_eap_info_create(profile_name);
	eap_type = __common_eap_connect_popup_get_eap_type(data_object);
	auth_type = __common_eap_connect_popup_get_auth_type(data_object);

	common_utils_add_dialogue_separator(view_list, "dialogue/separator");

	common_utils_add_2_line_txt_disabled_item(view_list, "dialogue/2text.2", sc(str_pkg_name, I18N_TYPE_EAP_method), list_eap_type[eap_type].name);

	switch (eap_type) {
	case EAP_SEC_TYPE_UNKNOWN:
	case EAP_SEC_TYPE_PEAP:
	case EAP_SEC_TYPE_TLS:
	case EAP_SEC_TYPE_TTLS:
		break;
	case EAP_SEC_TYPE_FAST:
		/* Add EAP provision */
		temp_str = g_strdup_printf("%d", view_detail_datamodel_eap_provision_get(data_object));
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
		/* Add EAP phase2 authentication */
		common_utils_add_2_line_txt_disabled_item(view_list, "dialogue/2text.2", sc(str_pkg_name, I18N_TYPE_Phase_2_authentication), list_eap_auth[auth_type].name);

		/* Add CA certificate */
		temp_str = view_detail_datamodel_ca_cert_get(data_object);
		temp_str = temp_str? temp_str : g_strdup(sc(str_pkg_name, I18N_TYPE_Unspecified));
		common_utils_add_2_line_txt_disabled_item(view_list, "dialogue/2text.2", sc(str_pkg_name, I18N_TYPE_Ca_Certificate), temp_str);
		g_free(temp_str);

		/* Add User certificate */
		temp_str = view_detail_datamodel_user_cert_get(data_object);
		temp_str = temp_str? temp_str : g_strdup(sc(str_pkg_name, I18N_TYPE_Unspecified));
		common_utils_add_2_line_txt_disabled_item(view_list, "dialogue/2text.2", sc(str_pkg_name, I18N_TYPE_User_Certificate),temp_str);
		g_free(temp_str);

		/* Add EAP ID */
		temp_str = view_detail_datamodel_user_id_get(data_object);
		common_utils_add_2_line_txt_disabled_item(view_list, "dialogue/2text.2", sc(str_pkg_name, I18N_TYPE_Identity), temp_str);
		g_free(temp_str);

		/* Add EAP Anonymous Identity */
		temp_str = view_detail_datamodel_anonymous_id_get(data_object);
		common_utils_add_2_line_txt_disabled_item(view_list, "dialogue/2text.2", sc(str_pkg_name, I18N_TYPE_Anonymous_Identity), temp_str);
		g_free(temp_str);

		/* Add EAP Password */
		temp_str = view_detail_datamodel_pswd_get(data_object);
		password_entry_item = common_utils_add_edit_box_to_list(view_list, NULL, sc(str_pkg_name, I18N_TYPE_Password), temp_str, sc(str_pkg_name, I18N_TYPE_Enter_password), ELM_INPUT_PANEL_LAYOUT_URL);
		g_free(temp_str);
		common_utils_entry_password_set(elm_object_item_data_get(password_entry_item), TRUE);
		eap_info_list_data->pswd_item = password_entry_item;
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
	view_detail_datamodel_eap_pswd_set(eap_info_list_data->data_object, txt);
	g_free(txt);

	view_detail_datamodel_save_eap_info_if_modified(eap_info_list_data->data_object);
	return;
}

void eap_info_remove(eap_info_list_t *eap_info_list_data)
{
	if (!eap_info_list_data) {
		ERROR_LOG(UG_NAME_ERR, "Invalid params passed!");
		return;
	}

	view_detail_datamodel_eap_info_destroy(eap_info_list_data->data_object);
	g_free(eap_info_list_data);

	return;
}

void eap_view_close(common_eap_connect_data_t *eap_data)
{
	if (NULL == eap_data) {
		return;
	}
	Evas_Object *button_back = elm_object_item_part_content_get(eap_data->navi_it, "prev_btn");
	evas_object_smart_callback_call(button_back, "clicked", NULL);
	return;
}
