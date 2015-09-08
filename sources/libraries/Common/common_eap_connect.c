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

#include <openssl/x509.h>
#include <openssl/pem.h>
#include <cert-service.h>
#include <cert-svc/cpkcs12.h>
#include <cert-svc/cprimitives.h>
#include <ui-gadget.h>
#include <efl_assist.h>
#include <utilX.h>
#include <Ecore_X.h>
#include <vconf-keys.h>
#include <glib/gstdio.h>

#include "common.h"
#include "ug_wifi.h"
#include "common_eap_connect.h"
#include "i18nmanager.h"
#include "common_utils.h"
#include "common_ip_info.h"

#define GENLIST_ITEM_HEIGHT				96

#define EAP_TLS_PATH			"/tmp/"
#define EAP_TLS_CA_CERT_PATH		"ca_cert.pem"
#define EAP_TLS_USER_CERT_PATH		"user_cert.pem"
#define EAP_TLS_PRIVATEKEY_PATH		"privatekey.pem"

#define EAP_TYPE_UNKNOWN		"UNKNOWN"
#define EAP_TYPE_PEAP			"PEAP"
#define EAP_TYPE_TLS				"TLS"
#define EAP_TYPE_TTLS			"TTLS"
#define EAP_TYPE_SIM				"SIM"
#define EAP_TYPE_AKA				"AKA"

#define EAP_AUTH_TYPE_PAP			"PAP"
#define EAP_AUTH_TYPE_MSCHAP				"MSCHAP"
#define EAP_AUTH_TYPE_MSCHAPV2			"MSCHAPV2"
#define EAP_AUTH_TYPE_GTC				"GTC"
#define EAP_AUTH_TYPE_MD5				"MD5"
#define VCONF_TELEPHONY_DEFAULT_DATA_SERVICE	"db/telephony/dualsim/default_data_service"

typedef enum {
	EAP_SEC_TYPE_UNKNOWN  = 0,
	EAP_SEC_TYPE_PEAP,
	EAP_SEC_TYPE_TLS,
	EAP_SEC_TYPE_TTLS,
	EAP_SEC_TYPE_SIM,
	EAP_SEC_TYPE_AKA,
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

typedef enum {
	EAP_TYPE_BTN = 0,
	EAP_AUTH_BTN,
	EAP_CERT_BTN,
} eap_btn_t;

typedef struct {
	char *name;
	Elm_Genlist_Item_Type flags;
} _Expand_List_t;

typedef struct {
	int btn_click[3];
	Evas_Object *btn_obj[3];
} _Btn_click_t;

struct eap_info_list {
	wifi_ap_h ap;
	eap_type_t eap_type;
	Elm_Object_Item *eap_method_item;
	Elm_Object_Item *eap_auth_item;
	Elm_Object_Item *user_cert_item;
	Elm_Object_Item *id_item;
	Elm_Object_Item *pswd_item;
};

static const _Expand_List_t list_eap_type[] = {
	{"UNKNOWN", ELM_GENLIST_ITEM_NONE},
	{"PEAP", ELM_GENLIST_ITEM_NONE},
	{"TLS", ELM_GENLIST_ITEM_NONE},
	{"TTLS", ELM_GENLIST_ITEM_NONE},
	{"SIM", ELM_GENLIST_ITEM_NONE},
	{"AKA", ELM_GENLIST_ITEM_NONE},
	{NULL, ELM_GENLIST_ITEM_NONE}
};

static const _Expand_List_t list_eap_auth[] = {
	{"IDS_ST_BODY_NONE", ELM_GENLIST_ITEM_NONE},
	{"PAP", ELM_GENLIST_ITEM_NONE},
	{"MSCHAP", ELM_GENLIST_ITEM_NONE},
	{"MSCHAPV2", ELM_GENLIST_ITEM_NONE},
	{"GTC", ELM_GENLIST_ITEM_NONE},
	{NULL, ELM_GENLIST_ITEM_NONE}
};

static unsigned short selected_cert = 0;

struct common_eap_connect_data {
	Elm_Object_Item *eap_type_item;
	Elm_Object_Item *eap_auth_item;
	Elm_Object_Item *eap_user_cert_item;
	Elm_Object_Item *eap_id_item;
	Elm_Object_Item *eap_pw_item;
	Elm_Object_Item *eap_chkbox_item;
	Evas_Object *popup;
	Evas_Object *sub_popup;
	Evas_Object *info_popup;
	Evas_Object *genlist;
	Eina_Bool eap_done_ok;
	Evas_Object *win;
	Evas_Object *conf;
	const char *str_pkg_name;
	wifi_ap_h ap;
	char *cert_alias;
	char *ca_cert_path;
	char *user_cert_path;
	char *privatekey_path;
	GSList *cert_candidates;
	Evas_Object *confirm;
	char *ssid;

	int key_status;

	bool is_hidden;
};

static Elm_Genlist_Item_Class g_eap_type_itc;
static Elm_Genlist_Item_Class g_eap_auth_itc;
static Elm_Genlist_Item_Class g_eap_user_cert_itc;
static Elm_Genlist_Item_Class g_eap_entry_itc;
static Elm_Genlist_Item_Class g_eap_chkbox_itc;
static Evas_Object *g_pwd_entry = NULL;
static gboolean keypad_state = FALSE;
static _Btn_click_t click;

static void (*_eap_view_deref_cb)(void) = NULL;

static void _create_and_update_list_items_based_on_rules(eap_type_t new_type, eap_connect_data_t *eap_data);
static void _update_eap_id_item_enter_key(eap_connect_data_t *eap_data);
static void _delete_eap_auth_item(eap_connect_data_t *eap_data);
static void _delete_eap_user_cert_item(eap_connect_data_t *eap_data);
static void _delete_eap_id_item(eap_connect_data_t *eap_data);
static void _delete_eap_pw_items(eap_connect_data_t *eap_data);
static void _delete_eap_entry_items(eap_connect_data_t *eap_data);
static eap_type_t __common_eap_connect_popup_get_eap_type(wifi_ap_h ap);
static eap_auth_t __common_eap_connect_popup_get_auth_type(wifi_ap_h ap);
static wifi_eap_type_e __common_eap_connect_popup_get_wlan_eap_type(eap_type_t eap_type);
static wifi_eap_auth_type_e __common_eap_connect_popup_get_wlan_auth_type(eap_auth_t auth_type);
static void _info_popup_ok_cb(void *data, Evas_Object *obj, void *event_info);
static gboolean __cert_extract_files(const char *cert_alias,eap_connect_data_t *eap_data);
static void _eap_popup_keypad_off_cb(void *data, Evas_Object *obj,
		void *event_info);
static void _eap_popup_keypad_on_cb(void *data, Evas_Object *obj,
		void *event_info);

static void ctxpopup_dismissed_cb(void *data, Evas_Object *obj,
		void *event_info)
{
	eap_connect_data_t *eap_data = (eap_connect_data_t *)data;

	if (eap_data->sub_popup != NULL) {
		evas_object_del(eap_data->sub_popup);
		eap_data->sub_popup = NULL;
	}
}

static void cert_ctxpopup_dismissed_cb(void *data, Evas_Object *obj,
		void *event_info)
{
	eap_connect_data_t *eap_data = (eap_connect_data_t *)data;

	if (eap_data->sub_popup != NULL) {
		evas_object_del(eap_data->sub_popup);
		eap_data->sub_popup = NULL;
	}

	if (eap_data->cert_candidates != NULL) {
		g_slist_free_full(eap_data->cert_candidates, g_free);
		eap_data->cert_candidates = NULL;
	}
}

static void move_dropdown(eap_connect_data_t *eap_data, Evas_Object *obj)
{
	Evas_Coord x, y, w , h;

	evas_object_geometry_get(obj, &x, &y, &w, &h);
	evas_object_move(eap_data->sub_popup, x + (w / 2), y + h);
}

static void _gl_editbox_sel_cb(void *data, Evas_Object *obj, void *event_info)
{
	Elm_Object_Item *item = (Elm_Object_Item *)event_info;
	elm_genlist_item_selected_set(item, FALSE);
}

static void _select_confirm_popup_ok_cb(void *data, Evas_Object *obj, void *event_info)
{
	eap_connect_data_t *eap_data = (eap_connect_data_t *)data;
	retm_if(eap_data == NULL);

	if (eap_data->confirm != NULL) {
		evas_object_del(eap_data->confirm);
		eap_data->confirm = NULL;
	}
}

static void __gl_eap_type_sub_sel_language_changed_cb(void *data, Evas_Object *obj, void *event_info)
{
	__COMMON_FUNC_ENTER__;
	retm_if(obj == NULL);

	int val = (int)data;
	char str[1024];
	char *txt = NULL;

	g_snprintf(str, 1024, sc(PACKAGE,I18N_TYPE_SIM_method_desc_popup),
			(val == 1) ? "SIM2" : "SIM1");
	txt = evas_textblock_text_utf8_to_markup(NULL, str);
	elm_object_domain_translatable_text_set(obj, PACKAGE, txt);
	g_free(txt);
	__COMMON_FUNC_EXIT__;
}

static void _gl_eap_type_sub_sel(void *data, Evas_Object *obj, void *event_info)
{
	eap_connect_data_t *eap_data = (eap_connect_data_t *) data;
	eap_type_t sel_index = EAP_SEC_TYPE_UNKNOWN;
	char buf[1024] = {'\0'};

	eap_type_t pre_index = __common_eap_connect_popup_get_eap_type(eap_data->ap);
	const char *label = elm_object_item_text_get((Elm_Object_Item *) event_info);

	if(label != NULL){
		if (strcmp(label, EAP_TYPE_UNKNOWN) == 0)
			sel_index = EAP_SEC_TYPE_UNKNOWN;
		else if (strcmp(label, EAP_TYPE_PEAP) == 0)
			sel_index = EAP_SEC_TYPE_PEAP;
		else if (strcmp(label, EAP_TYPE_TLS) == 0)
			sel_index = EAP_SEC_TYPE_TLS;
		else if (strcmp(label, EAP_TYPE_TTLS) == 0)
			sel_index = EAP_SEC_TYPE_TTLS;
		else if (strcmp(label, EAP_TYPE_SIM) == 0)
			sel_index = EAP_SEC_TYPE_SIM;
		else if (strcmp(label, EAP_TYPE_AKA) == 0)
			sel_index = EAP_SEC_TYPE_AKA;
	}

	DEBUG_LOG(UG_NAME_NORMAL, "previous index = %d; selected index = %d;",
			pre_index, sel_index);
	if ((pre_index != EAP_SEC_TYPE_SIM && sel_index == EAP_SEC_TYPE_SIM) ||
			(pre_index != EAP_SEC_TYPE_AKA && sel_index == EAP_SEC_TYPE_AKA)) {
		popup_btn_info_t popup_data;
		int value = -1;
		value = common_util_get_system_registry(
				VCONF_TELEPHONY_DEFAULT_DATA_SERVICE);

		memset(&popup_data, 0, sizeof(popup_data));
		g_snprintf(buf, sizeof(buf),
				sc(PACKAGE, I18N_TYPE_SIM_method_desc_popup),
				(value == 1) ? "SIM2" : "SIM1");
		popup_data.title_txt = "IDS_WIFI_BODY_EAP_METHOD";
		popup_data.info_txt = evas_textblock_text_utf8_to_markup(NULL, buf);
		popup_data.btn1_txt = "IDS_WIFI_SK2_OK";
		popup_data.btn1_cb = _select_confirm_popup_ok_cb;
		popup_data.btn1_data = eap_data;

		eap_data->confirm = common_utils_show_info_popup(eap_data->win,
				&popup_data);
		g_free(popup_data.info_txt);
		evas_object_smart_callback_add(eap_data->confirm, "language,changed",
				__gl_eap_type_sub_sel_language_changed_cb, (void *)value);

		_create_and_update_list_items_based_on_rules(sel_index, data);
		wifi_eap_type_e type;
		wifi_ap_set_eap_type(eap_data->ap,
				__common_eap_connect_popup_get_wlan_eap_type(sel_index));
		wifi_ap_get_eap_type(eap_data->ap, &type);
		DEBUG_LOG(UG_NAME_NORMAL, "set to new index = %d", type);
	}

	if (eap_data->sub_popup != NULL) {
		evas_object_del(eap_data->sub_popup);
		eap_data->sub_popup = NULL;
	}

	if (pre_index != sel_index) {
		wifi_eap_type_e type;
		wifi_ap_set_eap_type(eap_data->ap,
				__common_eap_connect_popup_get_wlan_eap_type(sel_index));
		wifi_ap_get_eap_type(eap_data->ap, &type);
		DEBUG_LOG(UG_NAME_NORMAL, "set to new index = %d", type);
		_create_and_update_list_items_based_on_rules(sel_index, eap_data);

		if (sel_index == EAP_SEC_TYPE_PEAP) {
			/* If previous auth type was PAP or MSCHAP & when PEAP
			 * EAP method is selected, then set back MSCHAPV2
			 */
			eap_auth_t auth_type;

			auth_type = __common_eap_connect_popup_get_auth_type(eap_data->ap);
			if (auth_type == EAP_SEC_AUTH_PAP ||
					auth_type == EAP_SEC_AUTH_MSCHAP) {
				wifi_ap_set_eap_auth_type(eap_data->ap,
						WIFI_EAP_AUTH_TYPE_MSCHAPV2);

				if(eap_data->eap_auth_item != NULL)
					elm_genlist_item_update(eap_data->eap_auth_item);
			}
		}
	} else {
		DEBUG_LOG(UG_NAME_NORMAL, "pre_index == sel_index[%d]",
				sel_index);
	}

	if(eap_data->eap_type_item != NULL)
		elm_genlist_item_update(eap_data->eap_type_item);
}

static CertSvcStringList stringList;
static CertSvcInstance instance;

static void _gl_eap_user_cert_sel(void *data, Evas_Object *obj,
		void *event_info)
{
	eap_connect_data_t *eap_data = (eap_connect_data_t *) data;
	const char *cert_alias = elm_object_item_text_get((Elm_Object_Item *) event_info);

	if (!eap_data)
		return;

	if (eap_data->sub_popup != NULL) {
		evas_object_del(eap_data->sub_popup);
		eap_data->sub_popup = NULL;
	}

	if (eap_data->ca_cert_path) {
		g_unlink(eap_data->ca_cert_path);
		eap_data->ca_cert_path = NULL;
	}
	if (eap_data->user_cert_path) {
		g_unlink(eap_data->user_cert_path);
		eap_data->user_cert_path = NULL;
	}
	if (eap_data->privatekey_path) {
		g_unlink(eap_data->privatekey_path);
		eap_data->privatekey_path = NULL;
	}

	if(cert_alias != NULL){
		if (strcmp(cert_alias, sc(PACKAGE, I18N_TYPE_None)) == 0) {
			if (eap_data->cert_alias != NULL) {
				g_free(eap_data->cert_alias);
				eap_data->cert_alias = NULL;
			}
		} else if (__cert_extract_files(cert_alias, eap_data)) {
			if (eap_data->cert_alias != NULL) {
				g_free(eap_data->cert_alias);
				eap_data->cert_alias = NULL;
			}
			eap_data->cert_alias = g_strdup(cert_alias);
		}
	}

	if(eap_data->eap_user_cert_item != NULL)
		elm_genlist_item_update(eap_data->eap_user_cert_item);

	if (eap_data->cert_candidates != NULL) {
		g_slist_free_full(eap_data->cert_candidates, g_free);
		eap_data->cert_candidates = NULL;
	}
}

static void _create_eap_cert_list(eap_connect_data_t *eap_data,
		Evas_Object *btn)
{
	int list_length = 0;
	int index = 0;
	Evas_Object *ctxpopup;
	Elm_Object_Item *it = NULL;

	if (!eap_data)
		return;

	if (eap_data->sub_popup != NULL) {
		evas_object_del(eap_data->sub_popup);
		eap_data->sub_popup = NULL;
	}

	ctxpopup = elm_ctxpopup_add(eap_data->win);
	eap_data->sub_popup = ctxpopup;
	elm_object_style_set(ctxpopup, "dropdown/list");
	ea_object_event_callback_add(ctxpopup, EA_CALLBACK_BACK,
			cert_ctxpopup_dismissed_cb, NULL);
	evas_object_smart_callback_add(ctxpopup,"dismissed",
			cert_ctxpopup_dismissed_cb, eap_data);
	elm_ctxpopup_direction_priority_set(ctxpopup,
			ELM_CTXPOPUP_DIRECTION_DOWN,
			ELM_CTXPOPUP_DIRECTION_UNKNOWN,
			ELM_CTXPOPUP_DIRECTION_UNKNOWN,
			ELM_CTXPOPUP_DIRECTION_UNKNOWN);

	if (certsvc_instance_new(&instance) == CERTSVC_FAIL) {
		INFO_LOG(UG_NAME_ERR, "Failed to create new instance");
		return;
	}

	certsvc_pkcs12_get_id_list(instance, &stringList);
	certsvc_string_list_get_length(stringList, &list_length);

	if (eap_data->cert_candidates) {
		g_slist_free_full(eap_data->cert_candidates, g_free);
		eap_data->cert_candidates = NULL;
	}

	it = elm_ctxpopup_item_append(ctxpopup, "IDS_ST_BODY_NONE", NULL,
			_gl_eap_user_cert_sel, eap_data);
	elm_object_item_domain_text_translatable_set(it,
			PACKAGE, EINA_TRUE);

	for (index = 0; index < list_length; index++) {
		char *char_buffer = NULL;
		CertSvcString buffer;
		int ret = certsvc_string_list_get_one(stringList, index, &buffer);
		if (ret == CERTSVC_SUCCESS) {
			char_buffer = g_strndup(buffer.privateHandler, buffer.privateLength);
			if (char_buffer == NULL)
				goto exit;

			elm_ctxpopup_item_append(ctxpopup, char_buffer, NULL,
					_gl_eap_user_cert_sel, eap_data);
			eap_data->cert_candidates =
					g_slist_prepend(eap_data->cert_candidates, char_buffer);

			certsvc_string_free(buffer);
		} else
			ERROR_LOG(UG_NAME_NORMAL, "Failed to get certificates");
	}

	move_dropdown(eap_data, btn);
	evas_object_show(ctxpopup);

exit:
	certsvc_instance_free(instance);
}

static void _gl_eap_cert_list_btn_cb(void *data, Evas_Object *obj,
		void *event_info)
{
	Elm_Object_Item *item = (Elm_Object_Item *)event_info;
	eap_connect_data_t *eap_data = (eap_connect_data_t *) data;

	if (item)
		elm_genlist_item_selected_set(item, EINA_FALSE);

	if (keypad_state == FALSE) {
		_create_eap_cert_list(eap_data, obj);

		click.btn_click[EAP_CERT_BTN] = FALSE;
		click.btn_obj[EAP_CERT_BTN] = NULL;
	} else {
		click.btn_click[EAP_CERT_BTN] = TRUE;
		click.btn_obj[EAP_CERT_BTN] = obj;
	}
}

static char *_gl_eap_user_cert_text_get(void *data, Evas_Object *obj, const char *part)
{
	eap_connect_data_t *eap_data = (eap_connect_data_t *)data;

	if (!g_strcmp0(part, "elm.text.main")) {
		return g_strdup(sc(eap_data->str_pkg_name,
				I18N_TYPE_User_Certificate));
	}

	return NULL;
}

static Evas_Object *_gl_eap_user_cert_content_get(void *data,
		Evas_Object *obj, const char *part)
{
	eap_connect_data_t *eap_data = (eap_connect_data_t *)data;
	Evas_Object *btn = NULL;
	Evas_Object *ly = NULL;
	char buf[100];

	if (!strcmp(part, "elm.icon.entry")) {
		ly = elm_layout_add(obj);
		elm_layout_file_set(ly, CUSTOM_EDITFIELD_PATH,
				"eap_dropdown_button");
		btn = elm_button_add(obj);

		if (eap_data->cert_alias == NULL) {
			g_snprintf(buf, sizeof(buf), "<align=left>%s</align>",
					sc(eap_data->str_pkg_name, I18N_TYPE_None));
		} else {
			g_snprintf(buf, sizeof(buf), "<align=left>%s</align>",
					eap_data->cert_alias);
		}

		elm_object_domain_translatable_text_set(btn, PACKAGE, buf);
		elm_object_style_set(btn, "dropdown/label");
		evas_object_propagate_events_set(btn, EINA_FALSE);
		evas_object_size_hint_weight_set(btn, EVAS_HINT_EXPAND,
				EVAS_HINT_EXPAND);
		evas_object_size_hint_align_set(btn, EVAS_HINT_FILL,
				EVAS_HINT_FILL);
		evas_object_smart_callback_add(btn, "clicked",
				_gl_eap_cert_list_btn_cb, eap_data);

		elm_layout_content_set(ly, "btn", btn);
		return ly;
	}
	return NULL;
}

static void _gl_eap_item_sel_cb(void *data, Evas_Object *obj, void *event_info)
{
	Elm_Object_Item *item = (Elm_Object_Item *)event_info;

	if (item) {
		elm_genlist_item_selected_set(item, EINA_FALSE);
	}
}

static void _create_eap_type_list(eap_connect_data_t *eap_data,
		Evas_Object *btn)
{
	Evas_Object *ctxpopup = NULL;
	int i = EAP_SEC_TYPE_PEAP;
	int sim_state = VCONFKEY_TELEPHONY_SIM_UNKNOWN;
	Elm_Object_Item *it = NULL;

	sim_state = common_utils_get_sim_state();

	if (eap_data->sub_popup != NULL) {
		evas_object_del(eap_data->sub_popup);
	}

	ctxpopup = elm_ctxpopup_add(eap_data->win);
	eap_data->sub_popup = ctxpopup;
	elm_object_style_set(ctxpopup, "dropdown/list");
	ea_object_event_callback_add(ctxpopup, EA_CALLBACK_BACK,
			ea_ctxpopup_back_cb, NULL);
	evas_object_smart_callback_add(ctxpopup,"dismissed",
			ctxpopup_dismissed_cb, eap_data);
	elm_ctxpopup_direction_priority_set(ctxpopup,
			ELM_CTXPOPUP_DIRECTION_DOWN,
			ELM_CTXPOPUP_DIRECTION_UNKNOWN,
			ELM_CTXPOPUP_DIRECTION_UNKNOWN,
			ELM_CTXPOPUP_DIRECTION_UNKNOWN);

	/* eap_type = __common_eap_connect_popup_get_eap_type(eap_data->ap); */
	while (list_eap_type[i].name != NULL) {
		it = elm_ctxpopup_item_append(ctxpopup, list_eap_type[i].name,
				NULL, _gl_eap_type_sub_sel, eap_data);

		if ((i == EAP_SEC_TYPE_SIM || i == EAP_SEC_TYPE_AKA) &&
				sim_state != VCONFKEY_TELEPHONY_SIM_INSERTED) {
			elm_object_item_disabled_set(it, EINA_TRUE);
		}

		i++;
	}
	move_dropdown(eap_data, btn);
	evas_object_show(ctxpopup);
}

static void _gl_eap_type_btn_cb(void *data, Evas_Object *obj, void *event_info)
{
	Elm_Object_Item *item = (Elm_Object_Item *)event_info;
	eap_connect_data_t *eap_data = (eap_connect_data_t *) data;

	if (item)
		elm_genlist_item_selected_set(item, EINA_FALSE);

	if (keypad_state == FALSE) {
		_create_eap_type_list(eap_data, obj);

		click.btn_click[EAP_TYPE_BTN] = FALSE;
		click.btn_obj[EAP_TYPE_BTN] = NULL;
	} else {
		click.btn_click[EAP_TYPE_BTN] = TRUE;
		click.btn_obj[EAP_TYPE_BTN] = obj;
	}
}

static char *_gl_eap_type_text_get(void *data, Evas_Object *obj, const char *part)
{
	eap_connect_data_t *eap_data = (eap_connect_data_t *)data;

	if (!g_strcmp0(part, "elm.text.main")) {
		return g_strdup(sc(eap_data->str_pkg_name, I18N_TYPE_EAP_method));
	}

	return NULL;
}

static Evas_Object *_gl_eap_type_content_get(void *data,
		Evas_Object *obj, const char *part)
{
	eap_connect_data_t *eap_data = (eap_connect_data_t *)data;
	eap_type_t sel_sub_item_id = __common_eap_connect_popup_get_eap_type(eap_data->ap);
	Evas_Object *btn = NULL;
	Evas_Object *ly = NULL;
	char buf[100];

	if (!strcmp(part, "elm.icon.entry")) {
		ly = elm_layout_add(obj);
		elm_layout_file_set(ly, CUSTOM_EDITFIELD_PATH,
				"eap_dropdown_button");
		btn = elm_button_add(obj);

		g_snprintf(buf, sizeof(buf), "<align=left>%s</align>",
				list_eap_type[sel_sub_item_id].name);

		elm_object_text_set(btn, buf);
		elm_object_style_set(btn, "dropdown/label");
		evas_object_propagate_events_set(btn, EINA_FALSE);
		evas_object_size_hint_weight_set(btn, EVAS_HINT_EXPAND,
				EVAS_HINT_EXPAND);
		evas_object_size_hint_align_set(btn, EVAS_HINT_FILL,
				EVAS_HINT_FILL);
		evas_object_smart_callback_add(btn, "clicked",
				_gl_eap_type_btn_cb, eap_data);

		elm_layout_content_set(ly, "btn", btn);
		return ly;
	}
	return NULL;
}

static void _gl_eap_auth_sub_sel(void *data, Evas_Object *obj, void *event_info)
{
	eap_connect_data_t *eap_data = (eap_connect_data_t *) data;
	eap_auth_t sel_index = EAP_SEC_AUTH_NONE;

	const char *label = elm_object_item_text_get((Elm_Object_Item *) event_info);

	if(label != NULL){
		if (strcmp(label, sc(PACKAGE, I18N_TYPE_None)) == 0)
			sel_index = EAP_SEC_AUTH_NONE;
		else if (strcmp(label, EAP_AUTH_TYPE_PAP) == 0)
			sel_index = EAP_SEC_AUTH_PAP;
		else if (strcmp(label, EAP_AUTH_TYPE_MSCHAP) == 0)
			sel_index = EAP_SEC_AUTH_MSCHAP;
		else if (strcmp(label, EAP_AUTH_TYPE_MSCHAPV2) == 0)
			sel_index = EAP_SEC_AUTH_MSCHAPV2;
		else if (strcmp(label, EAP_AUTH_TYPE_GTC) == 0)
			sel_index = EAP_SEC_AUTH_GTC;
		else if (strcmp(label, EAP_AUTH_TYPE_MD5) == 0)
			sel_index = EAP_SEC_AUTH_MD5;
	}

	wifi_ap_set_eap_auth_type(eap_data->ap,
		__common_eap_connect_popup_get_wlan_auth_type(sel_index));

	if (eap_data->sub_popup != NULL) {
		evas_object_del(eap_data->sub_popup);
		eap_data->sub_popup = NULL;
	}

	if(eap_data->eap_auth_item != NULL)
		elm_genlist_item_update(eap_data->eap_auth_item);
}

static void _create_eap_auth_list(eap_connect_data_t *eap_data,
		Evas_Object *btn)
{
	Elm_Object_Item *it = NULL;
	eap_type_t eap_type = EAP_SEC_TYPE_UNKNOWN;
	Evas_Object *ctxpopup;
	int i = 0;

	eap_type = __common_eap_connect_popup_get_eap_type(eap_data->ap);

	if (eap_data->sub_popup != NULL) {
		evas_object_del(eap_data->sub_popup);
	}

	ctxpopup = elm_ctxpopup_add(eap_data->win);
	eap_data->sub_popup = ctxpopup;
	elm_object_style_set(ctxpopup, "dropdown/list");
	ea_object_event_callback_add(ctxpopup, EA_CALLBACK_BACK, ea_ctxpopup_back_cb, NULL);
	evas_object_smart_callback_add(ctxpopup,"dismissed", ctxpopup_dismissed_cb, eap_data);
	elm_ctxpopup_direction_priority_set(ctxpopup,
			ELM_CTXPOPUP_DIRECTION_DOWN,
			ELM_CTXPOPUP_DIRECTION_UNKNOWN,
			ELM_CTXPOPUP_DIRECTION_UNKNOWN,
			ELM_CTXPOPUP_DIRECTION_UNKNOWN);

	while (list_eap_auth[i].name != NULL) {
		if ((eap_type != EAP_SEC_TYPE_PEAP) ||
				(eap_type == EAP_SEC_TYPE_PEAP && i != 1 &&
						i != 2)) {
			it = elm_ctxpopup_item_append(ctxpopup, list_eap_auth[i].name,
				NULL, _gl_eap_auth_sub_sel, eap_data);
			if (i == 0) {
				elm_object_item_domain_text_translatable_set(it,
						PACKAGE, EINA_TRUE);
			}
		}
		i++;
	}
	move_dropdown(eap_data, btn);
	evas_object_show(ctxpopup);
}

static void _gl_eap_auth_btn_cb(void *data, Evas_Object *obj, void *event_info)
{
	Elm_Object_Item *item = (Elm_Object_Item *)event_info;
	eap_connect_data_t *eap_data = (eap_connect_data_t *) data;

	if (item)
		elm_genlist_item_selected_set(item, EINA_FALSE);

	if (keypad_state == FALSE) {
		_create_eap_auth_list(eap_data, obj);

		click.btn_click[EAP_AUTH_BTN] = FALSE;
		click.btn_obj[EAP_AUTH_BTN] = NULL;
	} else {
		click.btn_click[EAP_AUTH_BTN] = TRUE;
		click.btn_obj[EAP_AUTH_BTN] = obj;
	}
}

static char *_gl_eap_auth_text_get(void *data, Evas_Object *obj, const char *part)
{
	eap_connect_data_t *eap_data = (eap_connect_data_t *)data;

	if (!g_strcmp0(part, "elm.text.main")) {
		return g_strdup(sc(eap_data->str_pkg_name,
				I18N_TYPE_Phase_2_authentication));
	}

	return NULL;
}

static Evas_Object *_gl_eap_auth_content_get(void *data,
		Evas_Object *obj, const char *part)
{
	eap_connect_data_t *eap_data = (eap_connect_data_t *)data;
	eap_auth_t sel_sub_item_id = __common_eap_connect_popup_get_auth_type(eap_data->ap);
	Evas_Object *btn = NULL;
	Evas_Object *ly = NULL;
	char buf[100];

	if (!strcmp(part, "elm.icon.entry")) {
		ly = elm_layout_add(obj);
		elm_layout_file_set(ly, CUSTOM_EDITFIELD_PATH,
				"eap_dropdown_button");
		btn = elm_button_add(obj);

		if (sel_sub_item_id == EAP_SEC_AUTH_NONE) {
			g_snprintf(buf, sizeof(buf), "<align=left>%s</align>",
					sc(PACKAGE, I18N_TYPE_None));
		} else {
			g_snprintf(buf, sizeof(buf), "<align=left>%s</align>",
					list_eap_auth[sel_sub_item_id].name);
		}

		elm_object_domain_translatable_text_set(btn, PACKAGE, buf);
		elm_object_style_set(btn, "dropdown/label");
		evas_object_propagate_events_set(btn, EINA_FALSE);
		evas_object_size_hint_weight_set(btn, EVAS_HINT_EXPAND,
				EVAS_HINT_EXPAND);
		evas_object_size_hint_align_set(btn, EVAS_HINT_FILL,
				EVAS_HINT_FILL);
		evas_object_smart_callback_add(btn, "clicked",
				_gl_eap_auth_btn_cb, eap_data);

		elm_layout_content_set(ly, "btn", btn);
		return ly;
	}
	return NULL;
}

static void _gl_eap_entry_key_enter_cb(void *data, Evas_Object *obj, void *event_info)
{
	common_utils_entry_info_t *entry_info = (common_utils_entry_info_t *)data;
	if (!entry_info) {
		return;
	}

	Evas_Object *entry = NULL;
	Elm_Object_Item *next_item = NULL;
	eap_type_t eap_type;

	switch (entry_info->entry_id) {
	case ENTRY_TYPE_USER_ID:
		eap_type = __common_eap_connect_popup_get_eap_type(
				entry_info->ap);

		if (eap_type == EAP_SEC_TYPE_TLS) {
			entry = elm_object_item_part_content_get(entry_info->item,
					"elm.icon.entry");
			if (entry) {
				elm_object_focus_set(entry, EINA_FALSE);
			}
		} else {
			next_item = elm_genlist_item_next_get(entry_info->item);
			while (next_item) {
				if (elm_object_item_disabled_get(next_item) == EINA_FALSE &&
					elm_genlist_item_select_mode_get(next_item) !=
							ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY) {
					entry = elm_object_item_part_content_get(
							next_item, "elm.icon.entry");
					if (entry) {
						elm_object_focus_set(entry, EINA_TRUE);
						return;
					}
				}

				next_item = elm_genlist_item_next_get(next_item);
			}
		}
		break;
	case ENTRY_TYPE_PASSWORD:
		entry = elm_object_item_part_content_get(entry_info->item,
				"elm.icon.entry");
		if (entry) {
			elm_object_focus_set(entry, EINA_FALSE);
		}
		break;
	default:
		break;
	}
}

static void _gl_eap_entry_cursor_changed_cb(void* data, Evas_Object* obj, void* event_info)
{
	common_utils_entry_info_t *entry_info = (common_utils_entry_info_t *)data;
	if (!entry_info) {
		return;
	}

	if (elm_object_focus_get(obj)) {
		if (elm_entry_is_empty(obj)) {
			elm_object_item_signal_emit(entry_info->item, "elm,state,eraser,hide", "");
		} else {
			elm_object_item_signal_emit(entry_info->item, "elm,state,eraser,show", "");
		}
	}

	if (entry_info->entry_txt) {
		g_free(entry_info->entry_txt);
		entry_info->entry_txt = NULL;
	}

	char *entry_text = elm_entry_markup_to_utf8(elm_entry_entry_get(obj));

	if (entry_text != NULL && entry_text[0] != '\0') {
		entry_info->entry_txt = elm_entry_markup_to_utf8(elm_entry_entry_get(obj));
	}

	g_free(entry_text);
}

static void _gl_eap_entry_changed_cb(void* data, Evas_Object* obj, void* event_info)
{
	common_utils_entry_info_t *entry_info = (common_utils_entry_info_t *)data;
	if (!entry_info) {
		return;
	}

	if (obj == NULL) {
		return;
	}

	if (elm_object_focus_get(obj)) {
		if (elm_entry_is_empty(obj)) {
			elm_object_item_signal_emit(entry_info->item, "elm,state,eraser,hide", "");
		} else {
			elm_object_item_signal_emit(entry_info->item, "elm,state,eraser,show", "");
		}
	}
}

static void _gl_eap_entry_focused_cb(void *data, Evas_Object *obj, void *event_info)
{
	common_utils_entry_info_t *entry_info = (common_utils_entry_info_t *)data;
	if (!entry_info) {
		return;
	}

	if (!elm_entry_is_empty(obj)) {
		elm_object_item_signal_emit(entry_info->item, "elm,state,eraser,show", "");
	}

	elm_object_item_signal_emit(entry_info->item, "elm,state,rename,hide", "");
}

static void _gl_eap_entry_unfocused_cb(void *data, Evas_Object *obj, void *event_info)
{
	common_utils_entry_info_t *entry_info = (common_utils_entry_info_t *)data;
	if (!entry_info) {
		return;
	}

	if (entry_info->entry_txt) {
		g_free(entry_info->entry_txt);
		entry_info->entry_txt = NULL;
	}

	char *entry_text = elm_entry_markup_to_utf8(elm_entry_entry_get(obj));
	if (entry_text != NULL && entry_text[0] != '\0')
		entry_info->entry_txt = elm_entry_markup_to_utf8(elm_entry_entry_get(obj));

	g_free(entry_text);

	elm_object_item_signal_emit(entry_info->item, "elm,state,eraser,hide", "");
	elm_object_item_signal_emit(entry_info->item, "elm,state,rename,show", "");
}

static void _gl_eap_entry_maxlength_reached(void *data, Evas_Object *obj,
		void *event_info)
{
	common_utils_send_message_to_net_popup("Password length",
			"Lengthy Password", "notification", NULL);
}

static void _gl_eap_entry_eraser_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	common_utils_entry_info_t *entry_info = (common_utils_entry_info_t *)data;
	if (!entry_info) {
		return;
	}

	Evas_Object *entry = elm_object_item_part_content_get(entry_info->item, "elm.icon.entry");
	if (entry) {
		elm_object_focus_set(entry, EINA_TRUE);
		elm_entry_entry_set(entry, "");
	}
}

static char *_gl_eap_entry_item_text_get(void *data, Evas_Object *obj, const char *part)
{
	common_utils_entry_info_t *entry_info = (common_utils_entry_info_t *)data;
	if (!entry_info) {
		return NULL;
	}

	if (!g_strcmp0(part, "elm.text.main")) {
		return g_strdup(dgettext(PACKAGE, entry_info->title_txt));
	}

	return NULL;
}

static Evas_Object *_gl_eap_entry_item_content_get(void *data, Evas_Object *obj, const char *part)
{
	common_utils_entry_info_t *entry_info = (common_utils_entry_info_t *)data;
	if (!entry_info) {
		return NULL;
	}

	if (g_strcmp0(part, "elm.icon.entry") == 0) {
		Evas_Object *entry = NULL;
		char *guide_txt = NULL;
		char *accepted = NULL;
		Eina_Bool hide_entry_txt = EINA_FALSE;
		Elm_Input_Panel_Layout panel_type;
		int return_key_type;
		eap_type_t eap_type;

		eap_type = __common_eap_connect_popup_get_eap_type(entry_info->ap);

		static Elm_Entry_Filter_Limit_Size limit_filter_data;

		switch (entry_info->entry_id)
		{
		case ENTRY_TYPE_USER_ID:
			panel_type = ELM_INPUT_PANEL_LAYOUT_NORMAL;
			guide_txt = entry_info->guide_txt;

			if (eap_type == EAP_SEC_TYPE_TLS) {
				return_key_type = ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_DONE;
			} else {
				return_key_type = ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_NEXT;
			}
			break;
		case ENTRY_TYPE_PASSWORD:
			panel_type = ELM_INPUT_PANEL_LAYOUT_PASSWORD;
			guide_txt = entry_info->guide_txt;
			hide_entry_txt = EINA_TRUE;
			return_key_type = ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_DONE;
			break;
		default:
			return NULL;
		}

		entry = ea_editfield_add(obj, EA_EDITFIELD_SCROLL_SINGLELINE);
		retvm_if(NULL == entry, NULL);

		elm_entry_password_set(entry, hide_entry_txt);
		elm_entry_prediction_allow_set(entry, EINA_FALSE);
		elm_entry_autocapital_type_set(entry, ELM_AUTOCAPITAL_TYPE_NONE);

		elm_object_domain_translatable_part_text_set(entry, "elm.guide",
				PACKAGE, guide_txt);
		if (entry_info->entry_txt && (strlen(entry_info->entry_txt) > 0)) {
			elm_entry_entry_set(entry, entry_info->entry_txt);
		}

		elm_entry_input_panel_layout_set(entry, panel_type);
		elm_entry_input_panel_return_key_type_set(entry, return_key_type);

		limit_filter_data.max_char_count = 200;
		elm_entry_markup_filter_append(entry, elm_entry_filter_limit_size, &limit_filter_data);

		Elm_Entry_Filter_Accept_Set digits_filter_data;
		memset(&digits_filter_data, 0, sizeof(Elm_Entry_Filter_Accept_Set));
		digits_filter_data.accepted = accepted;
		elm_entry_markup_filter_append(entry, elm_entry_filter_accept_set, &digits_filter_data);

		Ecore_IMF_Context *imf_ctxt = elm_entry_imf_context_get(entry);
		if (imf_ctxt && entry_info->input_panel_cb) {
			ecore_imf_context_input_panel_event_callback_add(imf_ctxt,
					ECORE_IMF_INPUT_PANEL_STATE_EVENT,
					entry_info->input_panel_cb,
					entry_info->input_panel_cb_data);
			DEBUG_LOG(UG_NAME_NORMAL, "set the imf ctxt cbs");
		}

		evas_object_smart_callback_add(entry, "activated", _gl_eap_entry_key_enter_cb, entry_info);
		evas_object_smart_callback_add(entry, "cursor,changed", _gl_eap_entry_cursor_changed_cb, entry_info);
		evas_object_smart_callback_add(entry, "changed", _gl_eap_entry_changed_cb, entry_info);
		evas_object_smart_callback_add(entry, "focused", _gl_eap_entry_focused_cb, entry_info);
		evas_object_smart_callback_add(entry, "unfocused", _gl_eap_entry_unfocused_cb, entry_info);
		evas_object_smart_callback_add(entry, "maxlength,reached", _gl_eap_entry_maxlength_reached, NULL);

		if (entry_info->entry_id == ENTRY_TYPE_PASSWORD) {
			g_pwd_entry = entry;
		}

		return entry;
	} else if (g_strcmp0(part, "elm.icon.eraser") == 0) {
		Evas_Object *btn = elm_button_add(obj);
		elm_object_style_set(btn, "editfield_clear");
		evas_object_smart_callback_add(btn, "clicked", _gl_eap_entry_eraser_clicked_cb, entry_info);
		return btn;
	}

	return NULL;
}

static void _gl_eap_entry_item_del(void *data, Evas_Object *obj)
{
	common_utils_entry_info_t *entry_info = (common_utils_entry_info_t *)data;
	if (entry_info == NULL) {
		return;
	}

	if (entry_info->entry_txt) {
		g_free(entry_info->entry_txt);
	}

	g_free(entry_info);
}

static void _chk_changed_cb(void *data, Evas_Object *obj, void *ei)
{
	if (obj == NULL || g_pwd_entry == NULL) {
		return;
	}

	Eina_Bool state = elm_check_state_get(obj);
	if (state) {
		elm_entry_password_set(g_pwd_entry, EINA_FALSE);
	} else {
		elm_entry_password_set(g_pwd_entry, EINA_TRUE);
	}
}

static char *_gl_eap_chkbox_item_text_get(void *data, Evas_Object *obj,
		const char *part)
{
	char *str_pkg_name = (char *)data;

	if (!g_strcmp0(part, "elm.text.main.left")) {
		char buf[1024];
		snprintf(buf, 1023, "%s", sc(str_pkg_name, I18N_TYPE_Show_password));
		return strdup(buf);
	}
	return NULL;

}

static Evas_Object *_gl_eap_chkbox_item_content_get(void *data,
		Evas_Object *obj, const char *part)
{
	Evas_Object *check = NULL;

	if(!g_strcmp0(part, "elm.icon.right")) {
		check = elm_check_add(obj);
		evas_object_propagate_events_set(check, EINA_FALSE);

		evas_object_size_hint_align_set(check, EVAS_HINT_FILL, EVAS_HINT_FILL);
		evas_object_size_hint_weight_set(check, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		evas_object_smart_callback_add(check, "changed",
				_chk_changed_cb, NULL);

		elm_object_focus_allow_set(check, EINA_FALSE);

		return check;
	}
	return NULL;
}

static void _gl_eap_chkbox_sel(void *data, Evas_Object *obj, void *ei)
{
	Elm_Object_Item *item = NULL;

	item = (Elm_Object_Item *)ei;
	if (item == NULL) {
		return;
	}

	Evas_Object *ck = elm_object_item_part_content_get(ei, "elm.icon.right");

	elm_genlist_item_selected_set(item, EINA_FALSE);

	Eina_Bool state = elm_check_state_get(ck);
	elm_check_state_set(ck, !state);

	_chk_changed_cb(NULL, ck, NULL);
}

static void gl_lang_changed(void *data, Evas_Object *obj, void *event_info)
{
	elm_genlist_realized_items_update(obj);
}

static void __common_eap_connect_popup_init_item_class(void *data)
{
	g_eap_type_itc.item_style = "entry.main";
	g_eap_type_itc.func.text_get = _gl_eap_type_text_get;
	g_eap_type_itc.func.content_get = _gl_eap_type_content_get;
	g_eap_type_itc.func.state_get = NULL;
	g_eap_type_itc.func.del = NULL;

	g_eap_auth_itc.item_style = "entry.main";
	g_eap_auth_itc.func.text_get = _gl_eap_auth_text_get;
	g_eap_auth_itc.func.content_get = _gl_eap_auth_content_get;
	g_eap_auth_itc.func.state_get = NULL;
	g_eap_auth_itc.func.del = NULL;

	g_eap_user_cert_itc.item_style = "entry.main";
	g_eap_user_cert_itc.func.text_get = _gl_eap_user_cert_text_get;
	g_eap_user_cert_itc.func.content_get = _gl_eap_user_cert_content_get;
	g_eap_user_cert_itc.func.state_get = NULL;
	g_eap_user_cert_itc.func.del = NULL;

	g_eap_entry_itc.item_style = "entry.main";
	g_eap_entry_itc.func.text_get = _gl_eap_entry_item_text_get;
	g_eap_entry_itc.func.content_get = _gl_eap_entry_item_content_get;
	g_eap_entry_itc.func.state_get = NULL;
	g_eap_entry_itc.func.del = _gl_eap_entry_item_del;

	g_eap_chkbox_itc.item_style = "1line";
	g_eap_chkbox_itc.func.text_get = _gl_eap_chkbox_item_text_get;
	g_eap_chkbox_itc.func.content_get = _gl_eap_chkbox_item_content_get;
	g_eap_chkbox_itc.func.state_get = NULL;
	g_eap_chkbox_itc.func.del = NULL;
}

static gboolean __cert_extract_files(const char *cert_alias,
		eap_connect_data_t *eap_data)
{
	int ret;
	int validity;
	int cert_counts = 0;
	int cert_index;
	gchar *ca_cert_path = NULL;
	gchar *user_cert_path = NULL;
	gchar *privatekey_path = NULL;
	FILE *fp;
	CertSvcInstance cert_instance;
	CertSvcString cert_alias_str;
	CertSvcCertificateList cert_list;
	CertSvcCertificate user_certificate;
	CertSvcCertificate ca_certificate;
	CertSvcCertificate *selected_certificate = NULL;
	X509 *x509 = NULL;
	EVP_PKEY *privatekey = NULL;

	ret = certsvc_instance_new(&cert_instance);
	if (ret != CERTSVC_SUCCESS) {
		ERROR_LOG(UG_NAME_NORMAL, "failed to certsvc_instance_new");
		goto error;
	}
	ret = certsvc_string_new(cert_instance, cert_alias, strlen(cert_alias), &cert_alias_str);
	if (ret != CERTSVC_SUCCESS) {
		ERROR_LOG(UG_NAME_NORMAL, "failed to certsvc_string_new");
		goto error;
	}
	ret = certsvc_pkcs12_load_certificate_list(cert_instance, cert_alias_str, &cert_list);
	if (ret != CERTSVC_SUCCESS) {
		ERROR_LOG(UG_NAME_NORMAL, "failed to certsvc_pkcs12_load_certificate_list");
		goto error;
	}
	ret = certsvc_certificate_list_get_length(cert_list, &cert_counts);
	if (cert_counts < 1) {
		ERROR_LOG(UG_NAME_NORMAL, "there is no certificates");
		goto error;
	}
	INFO_LOG(UG_NAME_NORMAL, "cert counts: %d", cert_counts);
	selected_certificate = g_try_new0(CertSvcCertificate, cert_counts);
	if (selected_certificate == NULL) {
		ERROR_LOG(UG_NAME_NORMAL, "failed to allocate memory");
		goto error;
	}
	ret = certsvc_certificate_list_get_one(cert_list, 0, &user_certificate);
	if (ret != CERTSVC_SUCCESS) {
		ERROR_LOG(UG_NAME_NORMAL, "failed to certsvc_certificate_list_get_one");
		goto error;
	}
	cert_index = cert_counts - 1;

	selected_certificate[0] = user_certificate;

	ret = certsvc_certificate_dup_x509(user_certificate, &x509);

	user_cert_path = g_strdup_printf("%s%s_%s", EAP_TLS_PATH,
				cert_alias, EAP_TLS_USER_CERT_PATH);
	if ((fp = fopen(user_cert_path, "w")) == NULL) {
		goto error;
	}
	ret = PEM_write_X509(fp, x509);
	fclose(fp);
	certsvc_certificate_free_x509(x509);
	INFO_LOG(UG_NAME_NORMAL, "success to save user_cert file");

	ca_cert_path = g_strdup_printf("%s%s_%s", EAP_TLS_PATH, cert_alias,
				EAP_TLS_CA_CERT_PATH);
	while (cert_index) {
		ret = certsvc_certificate_list_get_one(cert_list, cert_index, &ca_certificate);
		if (ret != CERTSVC_SUCCESS) {
			ERROR_LOG(UG_NAME_NORMAL, "failed to certsvc_certificate_list_get_one");
			goto error;
		}

		selected_certificate[cert_counts-cert_index] = ca_certificate;
		cert_index--;

		ret = certsvc_certificate_dup_x509(ca_certificate, &x509);
		if ((fp = fopen(ca_cert_path, "a")) == NULL) {
			goto error;
		}
		ret = PEM_write_X509(fp, x509);
		fclose(fp);
		certsvc_certificate_free_x509(x509);
	}
	INFO_LOG(UG_NAME_NORMAL, "success to save ca_cert file");
	ret = certsvc_certificate_verify(selected_certificate[0], selected_certificate, cert_counts, NULL, 0, &validity);
	if (ret != CERTSVC_SUCCESS) {
		ERROR_LOG(UG_NAME_NORMAL, "failed to verify ca_certificate");
		goto error;
	}
	if (validity == 0) {
		ERROR_LOG(UG_NAME_NORMAL, "Invalid certificate");
		goto error;
	}

	ret = certsvc_pkcs12_dup_evp_pkey(cert_instance, cert_alias_str, &privatekey);

	privatekey_path = g_strdup_printf("%s%s_%s", EAP_TLS_PATH,
				cert_alias, EAP_TLS_PRIVATEKEY_PATH);

	if ((fp = fopen(privatekey_path, "w")) == NULL) {
		goto error;
	}
	ret = PEM_write_PrivateKey(fp, privatekey, NULL, NULL, 0, NULL, NULL);
	fclose(fp);
	certsvc_pkcs12_free_evp_pkey(privatekey);
	INFO_LOG(UG_NAME_NORMAL, "success to save privatekey file");

	g_free(selected_certificate);
	certsvc_instance_free(cert_instance);

	eap_data->ca_cert_path = ca_cert_path;
	eap_data->user_cert_path = user_cert_path;
	eap_data->privatekey_path = privatekey_path;

	return TRUE;

error:
	g_free(ca_cert_path);
	g_free(user_cert_path);
	g_free(privatekey_path);

	if (selected_certificate) {
		g_free(selected_certificate);
	}

	certsvc_instance_free(cert_instance);
	return FALSE;
}

/* This creates EAP type, Auth type, CA certificate, User certificate,
 * User Id, Anonymous Id and Password items.
 */
static void _create_and_update_list_items_based_on_rules(eap_type_t new_type,
		eap_connect_data_t *eap_data)
{
	__COMMON_FUNC_ENTER__;
	Evas_Object* view_list = eap_data->genlist;
	Elm_Object_Item *insert_after_item = NULL;
	Elm_Object_Item *prev_item = NULL;
	common_utils_entry_info_t *edit_box_details;
	Eina_Bool auth_reqd = EINA_FALSE;
	Eina_Bool user_cert_reqd = EINA_FALSE;
	Eina_Bool id_reqd = EINA_FALSE;
	Eina_Bool pw_reqd = EINA_FALSE;

	if (NULL == eap_data->eap_type_item) {
		/* Create EAP method/type */
		eap_data->eap_type_item = elm_genlist_item_append(
						view_list, &g_eap_type_itc,
						eap_data, NULL,
						ELM_GENLIST_ITEM_NONE,
						_gl_eap_item_sel_cb, eap_data);
	}

	switch (new_type) {
	case EAP_SEC_TYPE_PEAP:
		insert_after_item = eap_data->eap_type_item;
		auth_reqd = EINA_TRUE;
		user_cert_reqd = EINA_FALSE;
		id_reqd = EINA_TRUE;
		pw_reqd = EINA_TRUE;
		break;
	case EAP_SEC_TYPE_TLS:
		insert_after_item = eap_data->eap_type_item;
		auth_reqd = EINA_FALSE;
		user_cert_reqd = EINA_TRUE;
		id_reqd = EINA_TRUE;
		pw_reqd = EINA_FALSE;
		break;
	case EAP_SEC_TYPE_TTLS:
		insert_after_item = eap_data->eap_type_item;
		auth_reqd = EINA_TRUE;
		user_cert_reqd = EINA_FALSE;
		id_reqd = EINA_TRUE;
		pw_reqd = EINA_TRUE;
		break;
	case EAP_SEC_TYPE_SIM:
	case EAP_SEC_TYPE_AKA:
		_delete_eap_entry_items(eap_data);
		auth_reqd = EINA_FALSE;
		user_cert_reqd = EINA_FALSE;
		id_reqd = EINA_FALSE;
		pw_reqd = EINA_FALSE;
		break;
	default:
		break;
	}

	if (auth_reqd == EINA_TRUE) {
		if (eap_data->eap_auth_item == NULL) {
			/* Add EAP phase2 authentication */
			eap_data->eap_auth_item = elm_genlist_item_insert_after(
					view_list, &g_eap_auth_itc, eap_data, NULL,
					insert_after_item, ELM_GENLIST_ITEM_NONE,
					_gl_eap_item_sel_cb, eap_data);
		}
	} else {
		_delete_eap_auth_item(eap_data);
	}

	if (user_cert_reqd == EINA_TRUE) {
		if (eap_data->eap_user_cert_item == NULL) {
			prev_item = eap_data->eap_type_item;

			/* Add User certificate */
			eap_data->eap_user_cert_item = elm_genlist_item_insert_after(
					view_list, &g_eap_user_cert_itc, eap_data, NULL,
					prev_item, ELM_GENLIST_ITEM_NONE,
					_gl_eap_item_sel_cb, eap_data);
		}
	} else {
		_delete_eap_user_cert_item(eap_data);
	}

	if (id_reqd == EINA_TRUE) {
		if (eap_data->eap_id_item == NULL) {
			if (new_type == EAP_SEC_TYPE_PEAP ||
					new_type == EAP_SEC_TYPE_TTLS) {
				prev_item = eap_data->eap_auth_item;
			} else {
				prev_item = eap_data->eap_user_cert_item;
			}

			/* Add EAP ID */
			edit_box_details = g_try_new0(common_utils_entry_info_t, 1);
			if (edit_box_details == NULL) {
				return;
			}

			edit_box_details->ap = eap_data->ap;
			edit_box_details->entry_id = ENTRY_TYPE_USER_ID;
			edit_box_details->title_txt = "IDS_WIFI_BODY_IDENTITY";
			edit_box_details->guide_txt = "IDS_WIFI_BODY_ENTER_IDENTITY";
			edit_box_details->item = elm_genlist_item_insert_after(
					view_list, &g_eap_entry_itc, edit_box_details,
					NULL, prev_item,
					ELM_GENLIST_ITEM_NONE, _gl_editbox_sel_cb, NULL);
			eap_data->eap_id_item = edit_box_details->item;
		}
	} else {
		_delete_eap_id_item(eap_data);
	}

	if (pw_reqd == EINA_TRUE) {
		if (eap_data->eap_pw_item == NULL) {
			/* Add EAP Password */
			edit_box_details = g_try_new0(common_utils_entry_info_t, 1);
			if (edit_box_details == NULL) {
				return;
			}

			edit_box_details->ap = eap_data->ap;
			edit_box_details->entry_id = ENTRY_TYPE_PASSWORD;
			edit_box_details->title_txt = "IDS_WIFI_HEADER_PASSWORD";
			edit_box_details->guide_txt = "IDS_WIFI_HEADER_ENTER_PASSWORD";
			edit_box_details->item = elm_genlist_item_insert_after(
					view_list, &g_eap_entry_itc,
					edit_box_details, NULL,
					eap_data->eap_id_item,
					ELM_GENLIST_ITEM_NONE,
					_gl_editbox_sel_cb, NULL);
			eap_data->eap_pw_item = edit_box_details->item;

			_update_eap_id_item_enter_key(eap_data);
		}

		if (eap_data->eap_chkbox_item == NULL) {
			/* Add Show Password checkbox */
			eap_data->eap_chkbox_item = elm_genlist_item_insert_after(
					view_list, &g_eap_chkbox_itc,
					eap_data->str_pkg_name, NULL,
					eap_data->eap_pw_item,
					ELM_GENLIST_ITEM_NONE,
					_gl_eap_chkbox_sel, NULL);
		}
	} else {
		_delete_eap_pw_items(eap_data);
	}

	__COMMON_FUNC_EXIT__;
	return;
}

static void _update_eap_id_item_enter_key(eap_connect_data_t *eap_data)
{
	if (eap_data->eap_id_item == NULL)
		return;

	Evas_Object *entry = NULL;
	eap_type_t eap_type;

	eap_type = __common_eap_connect_popup_get_eap_type(eap_data->ap);
	entry = elm_object_item_part_content_get(eap_data->eap_id_item,
			"elm.icon.entry");
	if (entry) {
		if (eap_type == EAP_SEC_TYPE_TLS) {
			elm_entry_input_panel_return_key_type_set(entry,
					ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_DONE);
		} else {
			elm_entry_input_panel_return_key_type_set(entry,
					ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_NEXT);
		}
	}
}

static void _delete_eap_auth_item(eap_connect_data_t *eap_data)
{
	__COMMON_FUNC_ENTER__;

	if (eap_data->eap_auth_item != NULL) {
		elm_object_item_del(eap_data->eap_auth_item);
		eap_data->eap_auth_item = NULL;
	}

	__COMMON_FUNC_EXIT__;
	return;
}

static void _delete_eap_user_cert_item(eap_connect_data_t *eap_data)
{
	__COMMON_FUNC_ENTER__;

	if (eap_data->eap_user_cert_item != NULL) {
		elm_object_item_del(eap_data->eap_user_cert_item);
		eap_data->eap_user_cert_item = NULL;
	}

	__COMMON_FUNC_EXIT__;
	return;
}

static void _delete_eap_id_item(eap_connect_data_t *eap_data)
{
	__COMMON_FUNC_ENTER__;

	if (eap_data->eap_id_item != NULL) {
		elm_object_item_del(eap_data->eap_id_item);
		eap_data->eap_id_item = NULL;
	}

	__COMMON_FUNC_EXIT__;
	return;
}

static void _delete_eap_pw_items(eap_connect_data_t *eap_data)
{
	__COMMON_FUNC_ENTER__;

	_update_eap_id_item_enter_key(eap_data);

	if (eap_data->eap_pw_item != NULL) {
		elm_object_item_del(eap_data->eap_pw_item);
		eap_data->eap_pw_item = NULL;
	}

	if (eap_data->eap_chkbox_item) {
		elm_object_item_del(eap_data->eap_chkbox_item);
		eap_data->eap_chkbox_item = NULL;
	}

	__COMMON_FUNC_EXIT__;
	return;
}

void _delete_eap_entry_items(eap_connect_data_t *eap_data)
{
	__COMMON_FUNC_ENTER__;

	if (eap_data->eap_auth_item != NULL) {
		elm_object_item_del(eap_data->eap_auth_item);
		eap_data->eap_auth_item = NULL;
	}

	if (eap_data->eap_user_cert_item != NULL) {
		elm_object_item_del(eap_data->eap_user_cert_item);
		eap_data->eap_user_cert_item = NULL;
	}

	if (eap_data->eap_id_item != NULL) {
		elm_object_item_del(eap_data->eap_id_item);
		eap_data->eap_id_item = NULL;
	}

	if (eap_data->eap_pw_item != NULL) {
		elm_object_item_del(eap_data->eap_pw_item);
		eap_data->eap_pw_item = NULL;
	}

	if (eap_data->eap_chkbox_item != NULL) {
		elm_object_item_del(eap_data->eap_chkbox_item);
		eap_data->eap_chkbox_item = NULL;
	}

	__COMMON_FUNC_EXIT__;
	return;
}

static Evas_Object* _create_list(Evas_Object* parent, void *data)
{
	__COMMON_FUNC_ENTER__;
	assertm_if(NULL == parent, "NULL!!");

	eap_connect_data_t *eap_data = (eap_connect_data_t *)data;
	Evas_Object* view_list = NULL;
	eap_type_t eap_type = EAP_SEC_TYPE_UNKNOWN;
	retvm_if(eap_data == NULL, NULL);

	__common_eap_connect_popup_init_item_class(eap_data);

	eap_data->eap_done_ok = FALSE;
	eap_data->genlist = view_list = elm_genlist_add(parent);
	elm_genlist_realization_mode_set(view_list, TRUE);
	elm_genlist_mode_set(view_list, ELM_LIST_COMPRESS);
	elm_scroller_content_min_limit(view_list, EINA_FALSE, EINA_TRUE);

	evas_object_size_hint_weight_set(view_list, EVAS_HINT_EXPAND,
			EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(view_list, EVAS_HINT_FILL, EVAS_HINT_FILL);

	/* Set default values. eap type = PEAP, auth type = MSCHAPv2 */
	eap_type = __common_eap_connect_popup_get_eap_type(eap_data->ap);
	wifi_ap_set_eap_type(eap_data->ap,
			(eap_type == EAP_SEC_TYPE_UNKNOWN) ?
			WIFI_EAP_TYPE_PEAP :
			__common_eap_connect_popup_get_wlan_eap_type(eap_type));

	wifi_ap_set_eap_auth_type(eap_data->ap,
					WIFI_EAP_AUTH_TYPE_MSCHAPV2);

	selected_cert = 0;

	/* Create the entry items */
	_create_and_update_list_items_based_on_rules(eap_type, eap_data);

	evas_object_smart_callback_add(view_list, "language,changed",
			gl_lang_changed, NULL);

	__COMMON_FUNC_EXIT__;
	return view_list;
}

static Eina_Bool _enable_scan_updates_cb(void *data)
{
	/* Lets enable the scan updates */
	wlan_manager_enable_scan_result_update();

	/* Reset the ecore timer handle */
	common_util_manager_ecore_scan_update_timer_reset();

	return ECORE_CALLBACK_CANCEL;
}

static void __common_eap_connect_cleanup(eap_connect_data_t *eap_data)
{
	if (eap_data == NULL) {
		return;
	}

	if (eap_data->conf != NULL) {
		evas_object_smart_callback_del(eap_data->conf,
				"virtualkeypad,state,on",
				_eap_popup_keypad_on_cb);
		evas_object_smart_callback_del(eap_data->conf,
				"virtualkeypad,state,off",
				_eap_popup_keypad_off_cb);
	}

	if (eap_data->ssid != NULL) {
		g_free(eap_data->ssid);
		eap_data->ssid = NULL;
	}

	if (eap_data->cert_alias) {
		g_free(eap_data->cert_alias);
		eap_data->cert_alias = NULL;
	}

	if (eap_data->ca_cert_path) {
		g_free(eap_data->ca_cert_path);
		eap_data->ca_cert_path = NULL;
	}

	if (eap_data->user_cert_path) {
		g_free(eap_data->user_cert_path);
		eap_data->user_cert_path = NULL;
	}

	if (eap_data->privatekey_path) {
		g_free(eap_data->privatekey_path);
		eap_data->privatekey_path = NULL;
	}

	wifi_ap_destroy(eap_data->ap);
	eap_data->ap = NULL;

	if(eap_data->info_popup){
		evas_object_del(eap_data->info_popup);
		eap_data->info_popup = NULL;
	}

	if (eap_data->popup != NULL) {
		evas_object_hide(eap_data->popup);
		evas_object_del(eap_data->popup);
	}

	if(_eap_view_deref_cb != NULL) {
		_eap_view_deref_cb();
		_eap_view_deref_cb = NULL;
	}

	/* A delay is needed to get the smooth Input panel closing animation effect */
	common_util_managed_ecore_scan_update_timer_add(0.1,
			_enable_scan_updates_cb, NULL);
}

static void __common_eap_connect_destroy_cb(void *data,  Evas_Object *obj,
		void *event_info)
{
	__common_eap_connect_cleanup((eap_connect_data_t *)data);
}

static void _info_popup_ok_cb(void *data, Evas_Object *obj, void *event_info)
{
	eap_connect_data_t *eap_data = (eap_connect_data_t *)data;

	if (eap_data->info_popup != NULL) {
		evas_object_del(eap_data->info_popup);
		eap_data->info_popup = NULL;
	}
}

static void __common_eap_connect_done_cb(void *data, Evas_Object *obj,
		void *event_info)
{
	char *str_id = NULL;
	char *str_pw = NULL;
	bool favorite = FALSE;
	wifi_eap_type_e eap_type;
	wifi_eap_auth_type_e eap_auth_type;
	eap_connect_data_t *eap_data = (eap_connect_data_t *)data;
	popup_btn_info_t popup_data;

	__COMMON_FUNC_ENTER__;
	if (eap_data->eap_done_ok == TRUE) {
		__COMMON_FUNC_EXIT__;
		return;
	}

	eap_data->eap_done_ok = TRUE;

	wifi_ap_get_eap_type(eap_data->ap, &eap_type);
	wifi_ap_get_eap_auth_type(eap_data->ap, &eap_auth_type);

	wifi_ap_is_favorite(eap_data->ap, &favorite);
	if (favorite == TRUE) {
		wlan_manager_forget(eap_data->ap);
		wifi_ap_refresh(eap_data->ap);

		wifi_ap_set_eap_type(eap_data->ap, eap_type);
		wifi_ap_set_eap_auth_type(eap_data->ap, eap_auth_type);
	}

	wifi_ap_set_eap_ca_cert_file(eap_data->ap, "");
	wifi_ap_set_eap_client_cert_file(eap_data->ap, "");
	wifi_ap_set_eap_private_key_info(eap_data->ap, "", "");

	switch (eap_type) {
	case WIFI_EAP_TYPE_PEAP:
	case WIFI_EAP_TYPE_TTLS:

		str_id = common_utils_get_list_item_entry_txt(eap_data->eap_id_item);
		if (str_id == NULL || str_id[0] == '\0') {
			memset(&popup_data, 0, sizeof(popup_data));
			popup_data.title_txt = eap_data->ssid;
			popup_data.btn1_txt = "IDS_WIFI_SK2_OK";
			popup_data.btn1_cb = _info_popup_ok_cb;
			popup_data.btn1_data = eap_data;
			popup_data.info_txt = "IDS_WIFI_BODY_ENTER_IDENTITY";
			eap_data->eap_done_ok = FALSE;
			eap_data->info_popup = common_utils_show_info_popup(eap_data->win,
						&popup_data);
			if(str_id) {
				g_free(str_id);
				str_id = NULL;
			}
			return;
		}

		str_pw = common_utils_get_list_item_entry_txt(eap_data->eap_pw_item);
		if (str_pw == NULL || str_pw[0] == '\0') {
			memset(&popup_data, 0, sizeof(popup_data));
			popup_data.title_txt = eap_data->ssid;
			popup_data.btn1_txt = "IDS_WIFI_SK2_OK";
			popup_data.btn1_cb = _info_popup_ok_cb;
			popup_data.btn1_data = eap_data;
			popup_data.info_txt = "IDS_WIFI_HEADER_ENTER_PASSWORD";
			eap_data->eap_done_ok = FALSE;
			eap_data->info_popup = common_utils_show_info_popup(eap_data->win,
						&popup_data);
			if(str_id) {
				g_free(str_id);
				str_id = NULL;
			}
			if(str_pw) {
				g_free(str_pw);
				str_pw = NULL;
			}
			return;
		}

		wifi_ap_set_eap_type(eap_data->ap, eap_type);
		wifi_ap_set_eap_auth_type(eap_data->ap, eap_auth_type);
		wifi_ap_set_eap_passphrase(eap_data->ap, str_id, str_pw);
		break;

	case WIFI_EAP_TYPE_TLS:
		str_id = common_utils_get_list_item_entry_txt(eap_data->eap_id_item);
		str_pw = common_utils_get_list_item_entry_txt(eap_data->eap_pw_item);

		wifi_ap_set_eap_type(eap_data->ap, eap_type);
		wifi_ap_set_eap_auth_type(eap_data->ap, eap_auth_type);
		wifi_ap_set_eap_passphrase(eap_data->ap, str_id, str_pw);
		wifi_ap_set_eap_ca_cert_file(eap_data->ap, eap_data->ca_cert_path);
		wifi_ap_set_eap_client_cert_file(eap_data->ap, eap_data->user_cert_path);
		wifi_ap_set_eap_private_key_info(eap_data->ap,
				eap_data->privatekey_path, NULL);
		break;

	case WIFI_EAP_TYPE_SIM:
	case WIFI_EAP_TYPE_AKA:
		break;

	default:
		ERROR_LOG(UG_NAME_NORMAL, "Unknown EAP method %d", eap_type);
		break;
	}

	if (eap_data->is_hidden) {
		wifi_ap_h hidden_ap;
		char *ssid;
		wifi_ap_get_essid(eap_data->ap, &ssid);
		wifi_ap_hidden_create(ssid, &hidden_ap);
		g_free(ssid);

		switch (eap_type) {
			case WIFI_EAP_TYPE_PEAP:
			case WIFI_EAP_TYPE_TTLS:
				wifi_ap_set_eap_type(hidden_ap, eap_type);
				wifi_ap_set_eap_auth_type(hidden_ap, eap_auth_type);
				wifi_ap_set_eap_passphrase(hidden_ap, str_id, str_pw);
				break;
			case WIFI_EAP_TYPE_TLS:
			wifi_ap_set_eap_type(hidden_ap, eap_type);
			wifi_ap_set_eap_auth_type(hidden_ap, eap_auth_type);
			wifi_ap_set_eap_passphrase(hidden_ap, str_id, str_pw);
			wifi_ap_set_eap_ca_cert_file(hidden_ap, eap_data->ca_cert_path);
			wifi_ap_set_eap_client_cert_file(hidden_ap,eap_data->user_cert_path);
			wifi_ap_set_eap_private_key_info(hidden_ap, eap_data->privatekey_path, NULL);
				break;
			case WIFI_EAP_TYPE_SIM:
			case WIFI_EAP_TYPE_AKA:
				break;
		}
		wlan_manager_connect(hidden_ap);
	} else
		wlan_manager_connect(eap_data->ap);

	__common_eap_connect_cleanup(eap_data);

	if(str_id){
		g_free(str_id);
		str_id = NULL;
	}
	if(str_pw){
		g_free(str_pw);
		str_pw = NULL;
	}

	__COMMON_FUNC_EXIT__;
}

static gboolean delay_create_context_popup(gpointer data)
{
	eap_connect_data_t *eap_data = (eap_connect_data_t *)data;

	if (click.btn_click[EAP_CERT_BTN] == TRUE) {
		_create_eap_cert_list(eap_data, click.btn_obj[EAP_CERT_BTN]);

		click.btn_click[EAP_CERT_BTN] = FALSE;
		click.btn_obj[EAP_CERT_BTN] = NULL;
	} else if (click.btn_click[EAP_AUTH_BTN] == TRUE) {
		_create_eap_auth_list(eap_data, click.btn_obj[EAP_AUTH_BTN]);

		click.btn_click[EAP_AUTH_BTN] = FALSE;
		click.btn_obj[EAP_AUTH_BTN] = NULL;
	} else if (click.btn_click[EAP_TYPE_BTN] == TRUE) {
		_create_eap_type_list(eap_data, click.btn_obj[EAP_TYPE_BTN]);

		click.btn_click[EAP_TYPE_BTN] = FALSE;
		click.btn_obj[EAP_TYPE_BTN] = NULL;
	}
	return FALSE;
}

static void _eap_popup_keypad_off_cb(void *data, Evas_Object *obj,
		void *event_info)
{
	if (data == NULL) {
		return;
	}

	eap_connect_data_t *eap_data = (eap_connect_data_t *)data;

	keypad_state = FALSE;

	common_util_managed_idle_add(delay_create_context_popup,
			(gpointer)eap_data);

	INFO_LOG(UG_NAME_NORMAL,"Keypad is down");
}

static void _eap_popup_keypad_on_cb(void *data, Evas_Object *obj,
		void *event_info)
{
	if (data == NULL) {
		return;
	}

	keypad_state = TRUE;
	INFO_LOG(UG_NAME_NORMAL,"Keypad is up");
}

eap_connect_data_t *create_eap_view(Evas_Object *layout_main, Evas_Object *win,
		Evas_Object *conf, const char *pkg_name,
		wifi_device_info_t *device_info, void (*deref_func)(void))
{
	__COMMON_FUNC_ENTER__;

	Evas_Object *popup = NULL;
	Evas_Object *list = NULL;

	if (layout_main == NULL || device_info == NULL || pkg_name == NULL) {
		return NULL;
	}

	eap_connect_data_t *eap_data = g_try_new0(eap_connect_data_t, 1);
	if (eap_data == NULL) {
		return NULL;
	}

	eap_data->str_pkg_name = pkg_name;
	eap_data->win = win;
	eap_data->conf = conf;
	eap_data->ssid = g_strdup(device_info->ssid);

	if (device_info->is_hidden == true) {
		/* Hidden Wi-Fi network */
		char *ssid = NULL;

		wifi_ap_get_essid(device_info->ap, &ssid);
		if (ssid == NULL)
			return NULL;

		wifi_ap_hidden_create(ssid, &(eap_data->ap));
		g_free(ssid);

		eap_data->is_hidden = TRUE;
	} else {
		/* Clone the Wi-Fi AP handle */
		wifi_ap_clone(&(eap_data->ap), device_info->ap);
	}

	/* Lets disable the scan updates so that the UI is not refreshed unnecessarily */
	wlan_manager_disable_scan_result_update();

	_eap_view_deref_cb = deref_func;
	click.btn_click[EAP_TYPE_BTN] = FALSE;
	click.btn_obj[EAP_TYPE_BTN] = NULL;
	click.btn_click[EAP_AUTH_BTN] = FALSE;
	click.btn_obj[EAP_AUTH_BTN] = NULL;
	click.btn_click[EAP_CERT_BTN] = FALSE;
	click.btn_obj[EAP_CERT_BTN] = NULL;
	keypad_state = FALSE;

	popup_btn_info_t popup_btn_data;
	memset(&popup_btn_data, 0, sizeof(popup_btn_data));

	popup_btn_data.title_txt = device_info->ssid;
	popup_btn_data.btn1_cb = __common_eap_connect_destroy_cb;
	popup_btn_data.btn1_data = eap_data;
	popup_btn_data.btn1_txt = "IDS_WIFI_SK_CANCEL";
	popup_btn_data.btn2_cb = __common_eap_connect_done_cb;
	popup_btn_data.btn2_data = eap_data;
	popup_btn_data.btn2_txt = "IDS_WIFI_BODY_CONNECT";

	popup = common_utils_show_info_popup(layout_main,
			&popup_btn_data);
	eap_data->popup = popup;
	evas_object_show(popup);
	elm_object_focus_set(popup, EINA_TRUE);

	/* Create an EAP connect view list */
	list = _create_list(popup, eap_data);
	elm_object_content_set(popup, list);

	evas_object_smart_callback_add(eap_data->conf,
			"virtualkeypad,state,on", _eap_popup_keypad_on_cb,
			eap_data);
	evas_object_smart_callback_add(eap_data->conf,
			"virtualkeypad,state,off", _eap_popup_keypad_off_cb,
			eap_data);

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
	default:
		ERROR_LOG(UG_NAME_NORMAL, "Err! This case should never occur");
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
		ret = wifi_ap_set_eap_type(ap, WIFI_EAP_TYPE_PEAP);
	}

	if (WIFI_ERROR_NONE != ret) {
		ERROR_LOG(UG_NAME_ERR, "Unable to get the eap type. err = %d", ret);
		return EAP_SEC_TYPE_UNKNOWN;
	}

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
		ret = wifi_ap_set_eap_auth_type(ap, EAP_SEC_AUTH_NONE);
	}

	if (WIFI_ERROR_NONE != ret) {
		ERROR_LOG(UG_NAME_ERR, "Unable to get the eap auth type. err = %d", ret);
		return EAP_SEC_AUTH_NONE;
	}

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

static char *_eap_info_get_user_cert_alias(wifi_ap_h ap)
{
	char *path = NULL;
	char *alias = NULL;
	char *filename = NULL;
	char *cert_name = NULL;
	int alias_len = 0;
	int filename_len = 0;

	wifi_ap_get_eap_client_cert_file(ap, &path);
	if (path == NULL)
		return NULL;

	filename = strrchr(path, '/');
	if (filename == NULL) {
		ERROR_LOG(UG_NAME_ERR, "Invalid file name");
		goto EXIT;
	}

	filename++;
	cert_name = strstr(filename, EAP_TLS_USER_CERT_PATH);
	if (cert_name == NULL) {
		/* For truncated path, available filename will be followed
		 * with ellipsis(...) & excluding any remaining part of
		 * "_user_cert.pem" - 14 chars (-14+3+1=10)*/
		filename_len = strlen(filename);
		alias = g_try_malloc0(filename_len - 10);
		if (alias == NULL) {
			ERROR_LOG(UG_NAME_ERR, "malloc fail");
			goto EXIT;
		}
		g_strlcpy(alias, filename, filename_len - 13);
		g_strlcat(alias, "...", filename_len - 10);
		INFO_LOG(UG_NAME_NORMAL, "Truncated alias [%s]", alias);
		goto EXIT;
	}

	cert_name--;
	alias_len = cert_name - filename;

	alias = g_try_malloc0(alias_len + 1);
	if (alias == NULL) {
		ERROR_LOG(UG_NAME_ERR, "malloc fail");
		goto EXIT;
	}

	g_strlcpy(alias, filename, alias_len + 1);
	INFO_LOG(UG_NAME_NORMAL, "Alias [%s] length [%d]", alias, alias_len);

EXIT:
	g_free(path);

	return alias;
}

/* This creates Auth type, ID, Anonymous Id and Password items
 * This function should be called after creating the EAP type item
 */
eap_info_list_t *eap_info_append_items(wifi_ap_h ap, Evas_Object* view_list,
		const char *str_pkg_name, imf_ctxt_panel_cb_t input_panel_cb,
		void *input_panel_cb_data)
{
	__COMMON_FUNC_ENTER__;

	eap_type_t eap_type;
	eap_auth_t auth_type;
	char *temp_str = NULL;
	Eina_Bool append_continue = TRUE;
	eap_info_list_t *eap_info_list_data = NULL;
	Elm_Object_Item* item = NULL;
	if (!view_list || !str_pkg_name || !ap) {
		ERROR_LOG(UG_NAME_ERR, "Invalid params passed!");
		return NULL;
	}

	eap_info_list_data = g_try_new0(eap_info_list_t, 1);
	if (eap_info_list_data == NULL) {
		return NULL;
	}

	eap_info_list_data->ap = ap;
	eap_type = __common_eap_connect_popup_get_eap_type(ap);
	auth_type = __common_eap_connect_popup_get_auth_type(ap);

	item = common_utils_add_2_line_txt_disabled_item(view_list,
			"2line.top",
			sc(str_pkg_name, I18N_TYPE_EAP_method),
			list_eap_type[eap_type].name);
	eap_info_list_data->eap_method_item = item;

	eap_info_list_data->eap_type = eap_type;

	switch (eap_type) {
	case EAP_SEC_TYPE_UNKNOWN:
	case EAP_SEC_TYPE_PEAP:
	case EAP_SEC_TYPE_TLS:
	case EAP_SEC_TYPE_TTLS:
		break;
	case EAP_SEC_TYPE_SIM:
	case EAP_SEC_TYPE_AKA:
	default:
		append_continue = FALSE;
		break;
	}

	if (append_continue) {
		if (eap_type == EAP_SEC_TYPE_PEAP ||
				eap_type == EAP_SEC_TYPE_TTLS) {
			/* Add EAP phase2 authentication */
			item = common_utils_add_2_line_txt_disabled_item(
					view_list, "2line.top",
					sc(str_pkg_name, I18N_TYPE_Phase_2_authentication),
					list_eap_auth[auth_type].name);
			eap_info_list_data->eap_auth_item = item;
		}

		if (eap_type == EAP_SEC_TYPE_TLS) {
			/* Add User certificate */
			temp_str = _eap_info_get_user_cert_alias(ap);

			if (temp_str == NULL || strlen(temp_str) == 0) {
				if (temp_str != NULL) {
					g_free(temp_str);
				}
				temp_str = g_strdup(sc(str_pkg_name,
						I18N_TYPE_Unspecified));
			}

			item = common_utils_add_2_line_txt_disabled_item(
					view_list, "2line.top",
					sc(str_pkg_name, I18N_TYPE_User_Certificate),
					temp_str);
			eap_info_list_data->user_cert_item = item;
			g_free(temp_str);
		}

		/* Add EAP ID */
		bool is_paswd_set;
		temp_str = NULL;
		wifi_ap_get_eap_passphrase(ap, &temp_str, &is_paswd_set);
		item = common_utils_add_2_line_txt_disabled_item(view_list, "2line.top",
				sc(str_pkg_name, I18N_TYPE_Identity), temp_str);
		eap_info_list_data->id_item = item;
		g_free(temp_str);

#if 0
		common_utils_entry_info_t *edit_box_details;

		/* Add EAP Password */
		g_eap_entry_itc.item_style = "entry.main";
		g_eap_entry_itc.func.text_get = _gl_eap_entry_item_text_get;
		g_eap_entry_itc.func.content_get = _gl_eap_entry_item_content_get;
		g_eap_entry_itc.func.state_get = NULL;
		g_eap_entry_itc.func.del = _gl_eap_entry_item_del;

		edit_box_details = g_try_new0(common_utils_entry_info_t, 1);
		if (edit_box_details == NULL) {
			g_free(eap_info_list_data);
			return NULL;
		}

		edit_box_details->entry_id = ENTRY_TYPE_PASSWORD;
		edit_box_details->title_txt = sc(str_pkg_name, I18N_TYPE_Password);
		edit_box_details->entry_txt = NULL;
		edit_box_details->guide_txt = sc(str_pkg_name, I18N_TYPE_Enter_password);
		edit_box_details->input_panel_cb = input_panel_cb;
		edit_box_details->input_panel_cb_data = input_panel_cb_data;
		edit_box_details->item = elm_genlist_item_append(view_list,
				&g_eap_entry_itc, edit_box_details, NULL,
				ELM_GENLIST_ITEM_NONE, _gl_editbox_sel_cb, NULL);
		elm_genlist_item_select_mode_set(edit_box_details->item,
				ELM_OBJECT_SELECT_MODE_NONE);
		eap_info_list_data->pswd_item = edit_box_details->item;
#endif
	}

	__COMMON_FUNC_EXIT__;
	return eap_info_list_data;
}

#if 0
void eap_info_save_data(eap_info_list_t *eap_info_list_data)
{
	if (!eap_info_list_data) {
		ERROR_LOG(UG_NAME_ERR, "Invalid params passed!");
		return;
	}
	char *txt = common_utils_get_list_item_entry_txt(eap_info_list_data->pswd_item);

	wifi_ap_set_eap_passphrase(eap_info_list_data->ap, NULL, txt);
	g_free(txt);
	return;
}
#endif

void eap_info_remove(eap_info_list_t *eap_info_list_data)
{
	if (!eap_info_list_data) {
		ERROR_LOG(UG_NAME_ERR, "Invalid params passed!");
		return;
	}

	g_free(eap_info_list_data);
}

void eap_connect_data_free(eap_connect_data_t *eap_data)
{
	__common_eap_connect_cleanup(eap_data);
}
