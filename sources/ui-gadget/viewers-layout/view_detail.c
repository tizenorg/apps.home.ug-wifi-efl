/*
 * Copyright (c) 2000-2012 Samsung Electronics Co., Ltd All Rights Reserved.
 *
 * This file is part of Wi-Fi UG
 * Written by Sanghoon Cho <sanghoon80.cho@samsung.com>
 *
 * PROPRIETARY/CONFIDENTIAL
 *
 * This software is the confidential and proprietary information of
 * SAMSUNG ELECTRONICS ("Confidential Information").
 * You shall not disclose such Confidential Information and shall
 * use it only in accordance with the terms of the license agreement
 * you entered into with SAMSUNG ELECTRONICS.
 *
 * SAMSUNG make no representations or warranties about the suitability
 * of the software, either express or implied, including but not limited
 * to the implied warranties of merchantability, fitness for a particular
 * purpose, or non-infringement. SAMSUNG shall not be liable for
 * any damages suffered by licensee as a result of using, modifying or
 * distributing this software or its derivatives.
 *
 */



#include "wifi.h"
#include "view_detail.h"
#include "view_detail_datamodel.h"
#include "view_staticip.h"
#include "view_dhcpip.h"
#include "i18nmanager.h"
#include "viewer_manager.h"
#include "connman-profile-manager.h"
#include "popup.h"


static Elm_Genlist_Item_Class grouptitle_itc ;
static Elm_Genlist_Item_Class staticip_itc ;
static Elm_Genlist_Item_Class dhcpip_itc ;
static Elm_Genlist_Item_Class seperator_itc ;
static Elm_Genlist_Item_Class description_itc ;
static Elm_Genlist_Item_Class proxy_itc ;

typedef struct _view_detail_data {
	char *profile_name;
	char *ssid;
	int rssi;
	int security_mode;
} view_detail_data;
view_detail_data *_detail_data = NULL;

typedef struct _view_detail_description_data {
	char* title;
	char* description;
} _view_detail_description_data;


static Evas_Object* _list = NULL;
static Elm_Object_Item* _button_forget_item = NULL;
static Elm_Object_Item* dhcpip_item = NULL;
static Elm_Object_Item* staticip_item = NULL;

static int view_detail_end = TRUE;

/* function declaration */
static void detailview_sk_cb(void *data, Evas_Object *obj, void *event_info);
static void forget_sk_cb(void *data, Evas_Object *obj, void *event_info);
static void resource_remove_and_pop(void);
static void _detailview_staticip_button_cb(void* data, Evas_Object* obj, void* event_info);
static void _detailview_dhcpip_button_cb(void* data, Evas_Object* obj, void* event_info);


///////////////////////////////////////////////////////////////
// implementation
///////////////////////////////////////////////////////////////
int view_detail_current_proxy_address_set(const char* proxy_address)
{
	return view_detail_datamodel_proxy_address_set(proxy_address);
}

const char* view_detail_current_proxy_address_get(void){
	return view_detail_datamodel_proxy_address_get();
}

static void _remove_all(void)
{
	__COMMON_FUNC_ENTER__;

	if(_list) {
		evas_object_del(_list);
	}

	if (_detail_data) {
		g_free(_detail_data);
		_detail_data = NULL;
	}

	view_detail_datamodel_destroy();

	__COMMON_FUNC_EXIT__;
}

int detailview_ip_and_dns_type_set_as_static()
{
	view_detail_datamodel_ip_and_dns_type_set(IP_TYPE_STATIC_IP);

	if(NULL != dhcpip_item) {
		elm_genlist_item_update(dhcpip_item);
	}
	if(NULL != staticip_item) {
		elm_genlist_item_update(staticip_item);
	}

	return TRUE;
}

int detailview_modified_ip_address_set(char* data){
	/* data can be NULL */
	view_detail_datamodel_static_ip_address_set((const char*) data);
	return TRUE;
}
int detailview_modified_gateway_address_set(char* data){
	/* data can be NULL */
	view_detail_datamodel_static_gateway_address_set((const char*) data);
	return TRUE;
}
int detailview_modified_subnet_mask_set(char* data){
	/* data can be NULL */
	view_detail_datamodel_static_subnet_mask_set((const char*) data);
	return TRUE;
}
int detailview_modified_dns1_address_set(char* data){
	/* data can be NULL */
	view_detail_datamodel_static_dns1_address_set((const char*) data);
	return TRUE;
}
int detailview_modified_dns2_address_set(char* data){
	/* data can be NULL */
	view_detail_datamodel_static_dns2_address_set((const char*) data);
	return TRUE;
}
const char* detailview_modified_ip_address_get(){
	return view_detail_datamodel_static_ip_address_get();
}
const char* detailview_modified_gateway_address_get(){
	return view_detail_datamodel_static_gateway_address_get();
}
const char* detailview_modified_subnet_mask_get(){
	return view_detail_datamodel_static_subnet_mask_get();
}
const char* detailview_modified_dns1_address_get(){
	return view_detail_datamodel_static_dns1_address_get();
}
const char* detailview_modified_dns2_address_get(){
	return view_detail_datamodel_static_dns2_address_get();
}

static void yes_cb(void *data, Evas_Object *obj, void *event_info)
{
	__COMMON_FUNC_ENTER__;

	if(view_detail_end == TRUE) {
		return;
	}
	view_detail_end = TRUE;

	wlan_manager_forget(_detail_data->profile_name);
	elm_naviframe_item_pop(viewer_manager_get_naviframe());
	resource_remove_and_pop();

	__COMMON_FUNC_EXIT__;
	return;
}

static void no_cb(void *data, Evas_Object *obj, void *event_info)
{
	__COMMON_FUNC_ENTER__;
	if (data) {
		evas_object_del(data);
	}
	__COMMON_FUNC_EXIT__;
}

static void forget_sk_cb(void *data, Evas_Object *obj, void *event_info)
{
	__COMMON_FUNC_ENTER__;

	Evas_Object* popup = elm_popup_add((Evas_Object *)data);
	evas_object_size_hint_weight_set(popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	char buffer[100];
	sprintf(buffer, sc(PACKAGE, I18N_TYPE_Autonomous_connection_to_s_will_be_turned_off_Continue), _detail_data->ssid);
	elm_object_text_set(popup, buffer);
	Evas_Object *btn_yes = elm_button_add(popup);
	elm_object_text_set(btn_yes, sc(PACKAGE, I18N_TYPE_Yes));
	elm_object_part_content_set(popup, "button1", btn_yes);
	evas_object_smart_callback_add(btn_yes, "clicked", yes_cb, NULL);
	Evas_Object *btn_no = elm_button_add(popup);
	elm_object_text_set(btn_no, sc(PACKAGE, I18N_TYPE_No));
	elm_object_part_content_set(popup, "button2", btn_no);
	evas_object_smart_callback_add(btn_no, "clicked", no_cb, popup);
	winset_popup_content_set(popup);

	evas_object_show(popup);

	__COMMON_FUNC_EXIT__;
}

static void resource_remove_and_pop()
{
	_remove_all();
}

static void detailview_sk_cb(void *data, Evas_Object *obj, void *event_info)
{
	__COMMON_FUNC_ENTER__;

	assertm_if(NULL == data, "NULL!!");

	if (view_detail_end == TRUE)
		return;

	view_detail_end = TRUE;

	view_detail_datamodel_determine_modified(_detail_data->profile_name);

	resource_remove_and_pop();

	__COMMON_FUNC_EXIT__;
}

static char* _view_detail_grouptitle_text_get(void *data, Evas_Object *obj, const char *part)
{
	__COMMON_FUNC_ENTER__;

	assertm_if(NULL == obj, "NULL!!");
	assertm_if(NULL == part, "NULL!!");

	if (part != NULL) {
		if (!strncmp(part, "elm.text", strlen(part))) {
			__COMMON_FUNC_EXIT__;
			return (char*) strdup(sc(PACKAGE, I18N_TYPE_IP_address));
		}
	}

	__COMMON_FUNC_EXIT__;
	return NULL;
}

static char* _view_detail_staticip_text_get(void *data, Evas_Object *obj, const char *part)
{
	__COMMON_FUNC_ENTER__;

	assertm_if(NULL == obj, "NULL!!");
	assertm_if(NULL == part, "NULL!!");

	if (part != NULL) {
		if (!strncmp(part, "elm.text", strlen(part))) {
			__COMMON_FUNC_EXIT__;
			return (char*) strdup(sc(PACKAGE, I18N_TYPE_Static_IP));
		}
	}

	__COMMON_FUNC_EXIT__;
	return NULL;
}

static char* _view_detail_dhcpip_text_get(void *data, Evas_Object *obj, const char *part)
{
	__COMMON_FUNC_ENTER__;

	assertm_if(NULL == obj, "NULL!!");
	assertm_if(NULL == part, "NULL!!");

	if (part != NULL) {
		if (!strncmp(part, "elm.text", strlen(part))) {
			__COMMON_FUNC_EXIT__;
			return (char*) strdup(sc(PACKAGE, I18N_TYPE_Dynamic_IP));
		}
	}

	__COMMON_FUNC_EXIT__;
	return NULL;
}

static Evas_Object *_view_detail_staticip_content_get(void *data, Evas_Object *obj, const char *part)
{
	__COMMON_FUNC_ENTER__;

	if (obj == NULL || part == NULL) {
		return NULL;
	}

	int current_value = view_detail_datamodel_ip_and_dns_type_get();
	Evas_Object* icon = NULL;
	if (!strncmp(part, "elm.icon.1", strlen(part))) {
		icon = elm_radio_add(obj);
		DEBUG_LOG(UG_NAME_NORMAL, "current_value [%d]", current_value);

		if (current_value == IP_TYPE_STATIC_IP) {
			elm_radio_state_value_set(icon, FALSE); /* ON */
		} else {
			elm_radio_state_value_set(icon, TRUE); /* OFF */
		}
	} else if (!strncmp(part, "elm.icon.2", strlen(part))) {
		if (current_value == IP_TYPE_STATIC_IP) {
			icon = elm_button_add(obj);
			assertm_if(NULL == icon, "NULL!!");
			elm_object_style_set(icon, "reveal");
			evas_object_smart_callback_add(icon, "clicked", (Evas_Smart_Cb)_detailview_staticip_button_cb, NULL);
			evas_object_propagate_events_set(icon, EINA_FALSE);
		}
		return icon;
	}

	__COMMON_FUNC_EXIT__;
	return icon;
}

static Evas_Object *_view_detail_dhcpip_content_get(void *data, Evas_Object *obj, const char *part)
{
	__COMMON_FUNC_ENTER__;

	if (obj == NULL || part == NULL) {
		return NULL;
	}

	int current_value = view_detail_datamodel_ip_and_dns_type_get();
	Evas_Object* icon = NULL;
	if (!strncmp(part, "elm.icon.1", strlen(part))) {
		icon = elm_radio_add(obj);
		DEBUG_LOG(UG_NAME_NORMAL, "current_value [%d]", current_value);

		if (current_value == IP_TYPE_DHCP_IP) {
			elm_radio_state_value_set(icon, FALSE); /* ON */
		} else {
			elm_radio_state_value_set(icon, TRUE); /* OFF */
		}
	} else if (!strncmp(part, "elm.icon.2", strlen(part))) {
		if (current_value == IP_TYPE_DHCP_IP) {
			icon = elm_button_add(obj);
			assertm_if(NULL == icon, "NULL!!");
			elm_object_style_set(icon, "reveal");
			evas_object_smart_callback_add(icon, "clicked", (Evas_Smart_Cb)_detailview_dhcpip_button_cb, NULL);
			evas_object_propagate_events_set(icon, EINA_FALSE);
		}
		return icon;
	}

	__COMMON_FUNC_EXIT__;
	return icon;
}

static char *_view_detail_description_text_get(void *data, Evas_Object *obj, const char *part)
{
	__COMMON_FUNC_ENTER__;

	_view_detail_description_data* det = (_view_detail_description_data*) data;
	assertm_if(NULL == det, "NULL!!");
	assertm_if(NULL == det->title, "NULL!!");
	assertm_if(NULL == det->description, "NULL!!");
	assertm_if(NULL == part, "NULL!!");

	if(0 == strncmp("elm.text.1", part, strlen(part))) {
		__COMMON_FUNC_EXIT__;
		return strdup(det->title);
	} else if(0 == strncmp("elm.text.2", part, strlen(part))) {
		__COMMON_FUNC_EXIT__;
		return strdup(det->description);
	} else {
		__COMMON_FUNC_EXIT__;
		return NULL;
	}
}

static void _entry_changed_cb(void* data, Evas_Object* obj, void* event_info)
{
	if (obj == NULL)
		return;

	if (elm_object_focus_get(data)) {
		if (elm_entry_is_empty(obj))
			elm_object_signal_emit(data, "elm,state,eraser,hide", "elm");
		else
			elm_object_signal_emit(data, "elm,state,eraser,show", "elm");
	}

	const char* txt = elm_entry_markup_to_utf8(elm_entry_entry_get(obj));
	assertm_if(NULL == txt, "NULL!!");

	DEBUG_LOG( UG_NAME_NORMAL, "text [%s]", txt);
	view_detail_current_proxy_address_set(txt);
}

static void _entry_focused_cb(void *data, Evas_Object *obj, void *event_info)
{
	if (!elm_entry_is_empty(obj))
		elm_object_signal_emit(data, "elm,state,eraser,show", "elm");
	elm_object_signal_emit(data, "elm,state,guidetext,hide", "elm");
}

static void _entry_unfocused_cb(void *data, Evas_Object *obj, void *event_info)
{
	if (elm_entry_is_empty(obj))
		elm_object_signal_emit(data, "elm,state,guidetext,show", "elm");
	elm_object_signal_emit(data, "elm,state,eraser,hide", "elm");
}

static void _eraser_clicked_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	elm_entry_entry_set(data, "");
}

static Evas_Object *_view_detail_proxy_content_get(void *data, Evas_Object *obj, const char *part)
{
	__COMMON_FUNC_ENTER__;

	if(!strncmp(part, "elm.icon", strlen(part))) {
		Evas_Object* layout = NULL;

		_view_detail_description_data* det = (_view_detail_description_data*) data;
		assertm_if(NULL == det, "NULL!!");
		assertm_if(NULL == det->title, "NULL!!");
		assertm_if(NULL == obj, "NULL!!");

		const char* proxy_addr = view_detail_current_proxy_address_get();
		assertm_if(NULL == proxy_addr, "NULL!!");

		DEBUG_LOG(UG_NAME_NORMAL, "title [%s]", det->title);
		DEBUG_LOG(UG_NAME_NORMAL, "description [%s]", proxy_addr);

		layout = elm_layout_add(obj);
		if (layout) {
			elm_layout_theme_set(layout, "layout", "editfield", "title");
			evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
			evas_object_size_hint_align_set(layout, EVAS_HINT_FILL, EVAS_HINT_FILL);
			elm_object_part_text_set(layout, "elm.text", det->title);
			Evas_Object *entry = elm_entry_add(obj);
			elm_object_part_content_set(layout, "elm.swallow.content", entry);
			elm_entry_entry_set(entry, proxy_addr);

			elm_entry_single_line_set(entry, EINA_TRUE);
			elm_entry_scrollable_set(entry, EINA_TRUE);
			elm_entry_cnp_mode_set(entry, ELM_CNP_MODE_NO_IMAGE);
			elm_entry_input_panel_layout_set(entry, ELM_INPUT_PANEL_LAYOUT_URL);

			evas_object_smart_callback_add(entry, "changed", _entry_changed_cb, layout);
			evas_object_smart_callback_add(entry, "focused", _entry_focused_cb, layout);
			evas_object_smart_callback_add(entry, "unfocused", _entry_unfocused_cb, layout);
			elm_object_signal_callback_add(layout, "elm,eraser,clicked", "elm", _eraser_clicked_cb, entry);

			__COMMON_FUNC_EXIT__;
			return layout;
		}
	}

	__COMMON_FUNC_EXIT__;
	return NULL;

}

static int _genlist_item_disable_later(void* data)
{
	if(NULL != data){
		elm_genlist_item_selected_set((Elm_Object_Item*) data, FALSE);
	}
	return FALSE;
}
static void _detailview_description_callback(void* data, Evas_Object* obj, void* event_info)
{
	ecore_idler_add( (Ecore_Task_Cb) _genlist_item_disable_later, event_info);
}

static void _detailview_staticip_button_cb(void* data, Evas_Object* obj, void* event_info)
{
	__COMMON_FUNC_ENTER__;
	assertm_if(NULL == data, "data is NULL!!");
	assertm_if(NULL == obj, "obj is NULL!!");

	view_staticip();

	if (dhcpip_item != NULL) {
		elm_genlist_item_update(dhcpip_item);
	}
	if (staticip_item != NULL) {
		elm_genlist_item_update(staticip_item);
	}

	ecore_idler_add((Ecore_Task_Cb)_genlist_item_disable_later, event_info);
	__COMMON_FUNC_EXIT__;
}

static void _detailview_dhcpip_button_cb(void* data, Evas_Object* obj, void* event_info)
{
	__COMMON_FUNC_ENTER__;
	assertm_if(NULL == data, "data is NULL!!");
	assertm_if(NULL == obj, "obj is NULL!!");

	view_dhcpip();

	if (dhcpip_item != NULL) {
		elm_genlist_item_update(dhcpip_item);
	}
	if (staticip_item != NULL) {
		elm_genlist_item_update(staticip_item);
	}

	ecore_idler_add((Ecore_Task_Cb)_genlist_item_disable_later, event_info);
	__COMMON_FUNC_EXIT__;
}

void _detailview_staticip_item_click_cb(void *data, Evas_Object *obj, void *event_info)
{
	__COMMON_FUNC_ENTER__;

	view_detail_datamodel_ip_and_dns_type_set(IP_TYPE_STATIC_IP);

	if (dhcpip_item != NULL) {
		elm_genlist_item_update(dhcpip_item);
	}
	if (staticip_item != NULL) {
		elm_genlist_item_update(staticip_item);
	}

	ecore_idler_add((Ecore_Task_Cb)_genlist_item_disable_later, event_info);
	__COMMON_FUNC_EXIT__;
}

void _detailview_dhcpip_item_click_cb(void *data, Evas_Object *obj, void *event_info)
{
	__COMMON_FUNC_ENTER__;

	view_detail_datamodel_ip_and_dns_type_set(IP_TYPE_DHCP_IP);

	if (dhcpip_item != NULL) {
		elm_genlist_item_update(dhcpip_item);
	}
	if (staticip_item != NULL) {
		elm_genlist_item_update(staticip_item);
	}

	ecore_idler_add((Ecore_Task_Cb)_genlist_item_disable_later, event_info);
	__COMMON_FUNC_EXIT__;
}

static int _add_seperator(Evas_Object* genlist)
{
	assertm_if(NULL == genlist, "NULL!!");

	Elm_Object_Item* sep2 = elm_genlist_item_append(
					genlist,
					&seperator_itc,
					NULL,
					NULL,
					ELM_GENLIST_ITEM_GROUP,
					NULL,
					NULL);

	assertm_if(NULL == sep2, "NULL!!");

	elm_genlist_item_select_mode_set(sep2, ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);
	return TRUE;
}

static int _add_description(Evas_Object* genlist, char* title, char* description)
{
	assertm_if(NULL == genlist, "NULL!!");
	assertm_if(NULL == title, "NULL!!");
	assertm_if(NULL == description, "NULL!!");

	_view_detail_description_data* description_data = (_view_detail_description_data*) malloc (sizeof( _view_detail_description_data));
	assertm_if(NULL == description_data, "NULL!!");
	memset(description_data, 0, sizeof(_view_detail_description_data));

	description_data->title = strdup(title);
	description_data->description = strdup(description);

	Elm_Object_Item* det = NULL;
	det = elm_genlist_item_append(
			genlist,
			&description_itc,
			description_data,
			NULL,
			ELM_GENLIST_ITEM_NONE,
			_detailview_description_callback,
			NULL);
	assertm_if(NULL == det, "NULL!!");

	elm_genlist_item_select_mode_set(det, ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);

	return TRUE;
}

static int _add_proxy(Evas_Object* genlist, char* title) 
{
	assertm_if(NULL == genlist, "NULL!!");
	assertm_if(NULL == title, "NULL!!");

	_view_detail_description_data* description_data = 
		(_view_detail_description_data*) malloc (sizeof( _view_detail_description_data));
	assertm_if(NULL == description_data, "NULL!!");
	memset(description_data, 0, sizeof(_view_detail_description_data));

	description_data->title = strdup(title);

	Elm_Object_Item* det = NULL;
	det = elm_genlist_item_append(
			genlist,
			&proxy_itc,
			description_data,
			NULL,
			ELM_GENLIST_ITEM_NONE,
			NULL,
			NULL);
	assertm_if(NULL == det, "NULL!!");

	elm_genlist_item_select_mode_set(det, ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);

	return TRUE;
}

static Evas_Object* _create_list(Evas_Object* parent)
{
	assertm_if(NULL == parent, "NULL!!");

	_list = elm_genlist_add(parent);
	assertm_if(NULL == _list, "NULL!!");

	grouptitle_itc.item_style = "grouptitle.dialogue";
	grouptitle_itc.func.text_get = _view_detail_grouptitle_text_get;
	grouptitle_itc.func.content_get = NULL;
	grouptitle_itc.func.state_get = NULL;
	grouptitle_itc.func.del = NULL;//_view_detail_grouptitle_del;

	staticip_itc.item_style = "dialogue/1text.2icon.2";
	staticip_itc.func.text_get = _view_detail_staticip_text_get;
	staticip_itc.func.content_get = _view_detail_staticip_content_get;
	staticip_itc.func.state_get = NULL;
	staticip_itc.func.del = NULL;

	dhcpip_itc.item_style = "dialogue/1text.2icon.2";
	dhcpip_itc.func.text_get = _view_detail_dhcpip_text_get;
	dhcpip_itc.func.content_get = _view_detail_dhcpip_content_get;
	dhcpip_itc.func.state_get = NULL;
	dhcpip_itc.func.del = NULL;

	seperator_itc.item_style = "dialogue/seperator";
	seperator_itc.func.text_get = NULL;
	seperator_itc.func.content_get = NULL;
	seperator_itc.func.state_get = NULL;
	seperator_itc.func.del = NULL;

	description_itc.item_style = "dialogue/2text.3";
	description_itc.func.text_get = _view_detail_description_text_get;
	description_itc.func.content_get = NULL;
	description_itc.func.state_get = NULL;
	description_itc.func.del = NULL;

	proxy_itc.item_style = "dialogue/1icon";
	proxy_itc.func.text_get = NULL;
	proxy_itc.func.content_get = _view_detail_proxy_content_get;
	proxy_itc.func.state_get = NULL;
	proxy_itc.func.del = NULL;

	Elm_Object_Item* sep = elm_genlist_item_append(_list, &grouptitle_itc, NULL, NULL, ELM_GENLIST_ITEM_GROUP, NULL, NULL);
	elm_genlist_item_select_mode_set(sep, ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);

	staticip_item = elm_genlist_item_append(_list, &staticip_itc, NULL, NULL, ELM_GENLIST_ITEM_NONE, _detailview_staticip_item_click_cb, NULL);
	dhcpip_item = elm_genlist_item_append(_list, &dhcpip_itc, NULL, NULL, ELM_GENLIST_ITEM_NONE, _detailview_dhcpip_item_click_cb, NULL);

	_add_seperator(_list);
	_add_description(_list, sc(PACKAGE, I18N_TYPE_Name), _detail_data->ssid);

	int signal_strength = wlan_manager_get_signal_strength(_detail_data->rssi);
	switch (signal_strength) {
	case SIGNAL_STRENGTH_TYPE_EXCELLENT:
		_add_description(_list, sc(PACKAGE, I18N_TYPE_Signal_strength), sc(PACKAGE, I18N_TYPE_Excellent));
		break;
	case SIGNAL_STRENGTH_TYPE_GOOD:
		_add_description(_list, sc(PACKAGE, I18N_TYPE_Signal_strength),  sc(PACKAGE, I18N_TYPE_Good));
		break;
	case SIGNAL_STRENGTH_TYPE_WEAK:
		_add_description(_list, sc(PACKAGE, I18N_TYPE_Signal_strength), sc(PACKAGE, I18N_TYPE_Week));
		break;
	default:
		_add_description(_list, sc(PACKAGE, I18N_TYPE_Signal_strength), "Unknown");
		break;
	}

	/* Security type */
	switch (_detail_data->security_mode) {
	case WLAN_SEC_MODE_NONE:
		_add_description(_list, sc(PACKAGE, I18N_TYPE_Security_type), sc(PACKAGE, I18N_TYPE_No_security));
		break;
	case WLAN_SEC_MODE_WEP:
		_add_description(_list, sc(PACKAGE, I18N_TYPE_Security_type), sc(PACKAGE, I18N_TYPE_WEP));
		break;
	case WLAN_SEC_MODE_WPA_PSK:
		_add_description(_list, sc(PACKAGE, I18N_TYPE_Security_type), sc(PACKAGE, I18N_TYPE_WPA_PSK));
		break;
	case WLAN_SEC_MODE_WPA2_PSK:
		_add_description(_list, sc(PACKAGE, I18N_TYPE_Security_type), sc(PACKAGE, I18N_TYPE_WPA2_PSK));
		break;
	case WLAN_SEC_MODE_IEEE8021X:
		_add_description(_list, sc(PACKAGE, I18N_TYPE_Security_type), sc(PACKAGE, I18N_TYPE_WPA_EAP));
		break;
	default:
		_add_description(_list, sc(PACKAGE, I18N_TYPE_Security_type), "Unknown");
		break;
	}

	/* Proxy Address */
	_add_proxy(_list, sc(PACKAGE, I18N_TYPE_Proxy_address));

	return _list;
}

static Evas_Object* _create_conformant(Evas_Object* parent)
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

void view_detail(wifi_device_info_t *device_info)
{
	__COMMON_FUNC_ENTER__;

	if (device_info == NULL) {
		ERROR_LOG(UG_NAME_NORMAL, "Failed : device_info is NULL");
		return;
	}

	int favourite = 0;
	int ret = connman_profile_manager_check_favourite(device_info->profile_name, &favourite);
	if (ret != WLAN_MANAGER_ERR_NONE) {
		ERROR_LOG(UG_NAME_NORMAL, "Failed : check favourite");
		return;
	}

	if (view_detail_datamodel_create(device_info->profile_name) == 0) {
		ERROR_LOG(UG_NAME_NORMAL, "Failed : data model created");
		return;
	}

	Evas_Object* navi_frame = viewer_manager_get_naviframe();
	if (navi_frame == NULL) {
		ERROR_LOG(UG_NAME_NORMAL, "Failed : get naviframe");
		return;
	}

	view_detail_end = FALSE;

	_detail_data = (view_detail_data *) malloc(sizeof(view_detail_data));
	if (_detail_data == NULL)
		return;

	memset(_detail_data, 0, sizeof(view_detail_data));
	_detail_data->profile_name = strdup(device_info->profile_name);
	_detail_data->ssid = strdup(device_info->ssid);
	_detail_data->rssi = device_info->rssi;
	_detail_data->security_mode = device_info->security_mode;

	Evas_Object* layout = elm_layout_add(navi_frame);
	elm_layout_theme_set(layout, "layout", "application", "noindicator");
	evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

	Evas_Object* bg = elm_bg_add(layout);
	evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_object_style_set(bg, "group_list");
	elm_object_part_content_set(layout, "elm.swallow.bg", bg);

	Evas_Object* conform = _create_conformant(layout);
	assertm_if(NULL == conform, "NULL!!");
	elm_object_part_content_set(layout, "elm.swallow.content", conform);

	evas_object_show(layout);

	Evas_Object* detailview_list = _create_list(conform);
	assertm_if(NULL == detailview_list, "NULL!!");

	elm_object_content_set(conform, detailview_list);

	Elm_Object_Item* navi_it = elm_naviframe_item_push(navi_frame, _detail_data->ssid, NULL, NULL, layout, NULL);

	_button_forget_item = NULL;
	if (favourite) {
		Evas_Object* toolbar = elm_toolbar_add(navi_frame);
		elm_toolbar_shrink_mode_set(toolbar, ELM_TOOLBAR_SHRINK_EXPAND);

		_button_forget_item = elm_toolbar_item_append(toolbar,
																NULL,
																sc(PACKAGE, I18N_TYPE_Forget),
																(Evas_Smart_Cb) forget_sk_cb,
																layout);
		elm_object_item_disabled_set(elm_toolbar_item_append(toolbar, NULL, NULL, NULL, NULL), EINA_TRUE);
		elm_object_item_part_content_set(navi_it, "controlbar", toolbar);
	}

	Evas_Object* button_back = elm_object_item_part_content_get(navi_it, "prev_btn");
	elm_object_focus_allow_set(button_back, EINA_TRUE);
	evas_object_smart_callback_add(button_back, "clicked", (Evas_Smart_Cb)detailview_sk_cb, NULL);

	__COMMON_FUNC_EXIT__;
}
