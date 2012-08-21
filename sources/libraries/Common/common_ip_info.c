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
#include "common_ip_info.h"
#include "common_utils.h"
#include "i18nmanager.h"
#include "common_datamodel.h"

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

	view_datamodel_ip_info_t *data_object;
};

static Elm_Object_Item* _add_description(Evas_Object* genlist, char* title, char* description, Elm_Object_Item* insert_after);

static Elm_Genlist_Item_Class ip_toggle_itc ;
static Elm_Genlist_Item_Class description_itc ;

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

static char *_ip_info_detail_description_text_get(void *data, Evas_Object *obj, const char *part)
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

static void _create_static_ip_table(ip_info_list_t *ip_info_list_data)
{
	char *txt = NULL;

	/* IP Address */
	txt = view_detail_datamodel_static_ip_address_get(ip_info_list_data->data_object);
	ip_info_list_data->ip_addr_item = common_utils_add_edit_box_to_list(ip_info_list_data->genlist, ip_info_list_data->ip_toggle_item, sc(ip_info_list_data->str_pkg_name, I18N_TYPE_IP_address), txt, NULL, ELM_INPUT_PANEL_LAYOUT_IP);
	g_free(txt);

	/* Subnet Mask */
	txt = view_detail_datamodel_static_subnet_mask_get(ip_info_list_data->data_object);
	ip_info_list_data->subnet_mask_item = common_utils_add_edit_box_to_list(ip_info_list_data->genlist, ip_info_list_data->ip_addr_item, sc(ip_info_list_data->str_pkg_name, I18N_TYPE_Subnet_mask), txt, NULL, ELM_INPUT_PANEL_LAYOUT_IP);
	g_free(txt);

	/* Gateway Address */
	txt = view_detail_datamodel_static_gateway_address_get(ip_info_list_data->data_object);
	ip_info_list_data->gateway_addr_item = common_utils_add_edit_box_to_list(ip_info_list_data->genlist, ip_info_list_data->subnet_mask_item, sc(ip_info_list_data->str_pkg_name, I18N_TYPE_Gateway_address), txt, NULL, ELM_INPUT_PANEL_LAYOUT_IP);
	g_free(txt);

	/* DNS 1 */
	txt = view_detail_datamodel_static_dns1_address_get(ip_info_list_data->data_object);
	ip_info_list_data->dns_1_item = common_utils_add_edit_box_to_list(ip_info_list_data->genlist, ip_info_list_data->gateway_addr_item, sc(ip_info_list_data->str_pkg_name, I18N_TYPE_DNS_1), txt, NULL, ELM_INPUT_PANEL_LAYOUT_IP);
	g_free(txt);

	/* DNS 2 */
	txt = view_detail_datamodel_static_dns2_address_get(ip_info_list_data->data_object);
	ip_info_list_data->dns_2_item = common_utils_add_edit_box_to_list(ip_info_list_data->genlist, ip_info_list_data->dns_1_item, sc(ip_info_list_data->str_pkg_name, I18N_TYPE_DNS_2), txt, NULL, ELM_INPUT_PANEL_LAYOUT_IP);
	g_free(txt);

	return;
}

static void _delete_static_ip_table(ip_info_list_t *ip_info_list_data)
{
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

	return;
}

static int _genlist_item_disable_later(void* data)
{
	elm_genlist_item_selected_set((Elm_Object_Item*) data, FALSE);
	return FALSE;
}

static void _gl_deselect_callback(void* data, Evas_Object* obj, void* event_info)
{
	ecore_idler_add( (Ecore_Task_Cb) _genlist_item_disable_later, event_info);
}

static void _ip_info_toggle_item_sel_cb(void* data, Evas_Object* obj, void* event_info)
{
	if (!data)
		return;

	ip_info_list_t *ip_info_list_data = (ip_info_list_t *)data;
	elm_object_item_disabled_set(ip_info_list_data->ip_toggle_item, TRUE);
	if (IP_TYPE_STATIC_IP == view_detail_datamodel_ip_and_dns_type_get(ip_info_list_data->data_object)) {	/* Static IP */
		char *ip_addr = NULL;

		ip_info_save_data(ip_info_list_data, FALSE);
		_delete_static_ip_table(ip_info_list_data);

		ip_addr = view_detail_datamodel_static_ip_address_get(ip_info_list_data->data_object);
		/* Dynamic IP Address */
		ip_info_list_data->ip_addr_item = _add_description(ip_info_list_data->genlist, sc(ip_info_list_data->str_pkg_name, I18N_TYPE_IP_address), ip_addr, ip_info_list_data->ip_toggle_item);
		elm_object_item_disabled_set(ip_info_list_data->ip_addr_item, TRUE);
		g_free(ip_addr);

		view_detail_datamodel_ip_and_dns_type_set(ip_info_list_data->data_object, IP_TYPE_DHCP_IP);
	} else {	/* Dynamic IP */

		elm_object_item_del(ip_info_list_data->ip_addr_item);
		ip_info_list_data->ip_addr_item = NULL;

		/* Create the entry layouts */
		_create_static_ip_table(ip_info_list_data);
		view_detail_datamodel_ip_and_dns_type_set(ip_info_list_data->data_object, IP_TYPE_STATIC_IP);
	}
	elm_genlist_item_update(ip_info_list_data->ip_toggle_item);
	elm_object_item_disabled_set(ip_info_list_data->ip_toggle_item, FALSE);
	ecore_idler_add( (Ecore_Task_Cb) _genlist_item_disable_later, event_info);
}

static char* _ip_info_iptoggle_text_get(void *data, Evas_Object *obj, const char *part)
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

static Evas_Object *_ip_info_iptoggle_content_get(void *data, Evas_Object *obj, const char *part)
{
	if (NULL == data || obj == NULL || part == NULL) {
		return NULL;
	}

	ip_info_list_t *ip_info_list_data = (ip_info_list_t *)data;

	Evas_Object *toggle_btn = elm_check_add(obj);
	assertm_if(NULL == toggle_btn, "NULL!!");
	elm_object_style_set(toggle_btn, "on&off");
	evas_object_propagate_events_set(toggle_btn, EINA_TRUE);
	if (IP_TYPE_STATIC_IP == view_detail_datamodel_ip_and_dns_type_get(ip_info_list_data->data_object)) {	/* Static IP */
		elm_check_state_set(toggle_btn, EINA_TRUE);
	} else {
		elm_check_state_set(toggle_btn, EINA_FALSE);
	}
	return toggle_btn;
}

static Elm_Object_Item* _add_description(Evas_Object* genlist, char* title, char* description, Elm_Object_Item* insert_after)
{
	assertm_if(NULL == genlist, "NULL!!");
	assertm_if(NULL == title, "NULL!!");
	assertm_if(NULL == description, "NULL!!");

	_view_detail_description_data_t* description_data = (_view_detail_description_data_t*)g_malloc0 (sizeof( _view_detail_description_data_t));
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

ip_info_list_t *ip_info_append_items(const char *profile_name, const char *pkg_name, Evas_Object *genlist)
{
	__COMMON_FUNC_ENTER__;
	char *proxy_data = NULL;
	char *proxy_addr = NULL;
	char *proxy_port = NULL;
	Elm_Object_Item* item = NULL;
	view_datamodel_ip_info_t *data_object = NULL;

	assertm_if(NULL == pkg_name, "NULL!!");
	assertm_if(NULL == genlist, "NULL!!");

	ip_info_list_t *ip_info_list_data = (ip_info_list_t *)g_malloc0(sizeof(ip_info_list_t));
	ip_info_list_data->data_object = data_object = view_detail_datamodel_ip_info_create(profile_name);
	ip_info_list_data->str_pkg_name = pkg_name;
	ip_info_list_data->genlist = genlist;

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

	common_utils_add_dialogue_separator(genlist, "dialogue/separator");

	/* Static/Dynamic switch button */
	ip_info_list_data->ip_toggle_item = elm_genlist_item_append(genlist, &ip_toggle_itc, ip_info_list_data, NULL, ELM_GENLIST_ITEM_NONE, _ip_info_toggle_item_sel_cb, ip_info_list_data);

	/* IP address */
	if (IP_TYPE_STATIC_IP == view_detail_datamodel_ip_and_dns_type_get(data_object)) {	/* Static IP */
		/* Create the entry layouts */
		_create_static_ip_table(ip_info_list_data);
	} else {	/* Dynamic IP */
		char *ip_addr = NULL;
		ip_addr = view_detail_datamodel_static_ip_address_get(data_object);
		/* Dynamic IP Address */
		ip_info_list_data->ip_addr_item = _add_description(genlist, sc(ip_info_list_data->str_pkg_name, I18N_TYPE_IP_address), ip_addr, NULL);
		elm_object_item_disabled_set(ip_info_list_data->ip_addr_item, TRUE);
		free(ip_addr);
	}

	common_utils_add_dialogue_separator(genlist, "dialogue/separator");

#if 0
	/* Channel Number */
	int channel_number = view_detail_datamodel_channel_freq_get(data_object);
	char *channel_num_str = g_strdup_printf("%u", channel_number);
	_add_description(genlist, sc(ip_info_list_data->str_pkg_name, I18N_TYPE_Channel), channel_num_str, NULL);
	g_free(channel_num_str);
#endif
	/* Mac address */
	char *mac_addr = (char *)view_detail_datamodel_MAC_addr_get(data_object);
	item = _add_description(genlist, sc(ip_info_list_data->str_pkg_name, I18N_TYPE_MAC_addr), mac_addr, NULL);
	elm_object_item_disabled_set(item, TRUE);
	g_free(mac_addr);

	proxy_data = (char *)view_detail_datamodel_proxy_address_get(data_object);
	assertm_if(NULL == proxy_data, "NULL!!");

	/* Proxy Address */
	proxy_addr = strtok(proxy_data, ":");
	DEBUG_LOG(UG_NAME_NORMAL, "* PROXY ADDR [%s]", proxy_addr);
	ip_info_list_data->proxy_addr_item = common_utils_add_edit_box_to_list(genlist, NULL, sc(ip_info_list_data->str_pkg_name, I18N_TYPE_Proxy_address), proxy_addr, NULL, ELM_INPUT_PANEL_LAYOUT_IP);

	/* Proxy port */
	proxy_port = strtok(NULL, ":");
	DEBUG_LOG(UG_NAME_NORMAL, "* PROXY PORT [%s]", proxy_port);
	ip_info_list_data->proxy_port_item = common_utils_add_edit_box_to_list(genlist, NULL, sc(ip_info_list_data->str_pkg_name, I18N_TYPE_Proxy_port), proxy_port, NULL, ELM_INPUT_PANEL_LAYOUT_NUMBERONLY);

	g_free(proxy_data);

	__COMMON_FUNC_EXIT__;
	return (void *)ip_info_list_data;
}

void ip_info_save_data(ip_info_list_t *ip_info_list_data, boolean b_save_to_profile)
{
	__COMMON_FUNC_ENTER__;
	assertm_if(NULL == ip_info_list_data, "NULL!!");
	char* txt = NULL;
	char* proxy_addr = NULL;
	char* proxy_port = NULL;
	view_datamodel_ip_info_t *data_object = ip_info_list_data->data_object;

	txt = common_utils_get_list_item_entry_txt(ip_info_list_data->ip_addr_item);
	DEBUG_LOG(UG_NAME_NORMAL, "IP [%s]", txt);
	view_detail_datamodel_static_ip_address_set(data_object, txt);
	g_free(txt);

	txt = common_utils_get_list_item_entry_txt(ip_info_list_data->subnet_mask_item);
	DEBUG_LOG(UG_NAME_NORMAL, "Subnet [%s]", txt);
	view_detail_datamodel_static_subnet_mask_set(data_object, txt);
	g_free(txt);

	txt = common_utils_get_list_item_entry_txt(ip_info_list_data->gateway_addr_item);
	DEBUG_LOG(UG_NAME_NORMAL, "Gateway [%s]", txt);
	view_detail_datamodel_static_gateway_address_set(data_object, txt);
	g_free(txt);

	txt = common_utils_get_list_item_entry_txt(ip_info_list_data->dns_1_item);
	DEBUG_LOG(UG_NAME_NORMAL, "DNS1 [%s]", txt);
	view_detail_datamodel_static_dns1_address_set(data_object, txt);
	g_free(txt);

	txt = common_utils_get_list_item_entry_txt(ip_info_list_data->dns_2_item);
	DEBUG_LOG(UG_NAME_NORMAL, "DNS2 [%s]", txt);
	view_detail_datamodel_static_dns2_address_set(data_object, txt);
	g_free(txt);

	proxy_addr = common_utils_get_list_item_entry_txt(ip_info_list_data->proxy_addr_item);
	proxy_port = common_utils_get_list_item_entry_txt(ip_info_list_data->proxy_port_item);
	txt = g_strdup_printf("%s:%s", proxy_addr, proxy_port);
	DEBUG_LOG( UG_NAME_NORMAL, "Proxy addr:port [%s]", txt);
	view_detail_datamodel_proxy_address_set(data_object, txt);
	g_free((gpointer)proxy_addr);
	g_free((gpointer)proxy_port);
	g_free((gpointer)txt);

	if (b_save_to_profile) {
		view_detail_datamodel_save_ip_info_if_modified(data_object);
	}

	__COMMON_FUNC_EXIT__;
	return;
}

void ip_info_remove(ip_info_list_t *ip_info_list)
{
	__COMMON_FUNC_ENTER__;
	assertm_if(NULL == ip_info_list, "NULL!!");
	view_detail_datamodel_ip_info_destroy(ip_info_list->data_object);
	g_free(ip_info_list);
	__COMMON_FUNC_EXIT__;
}
