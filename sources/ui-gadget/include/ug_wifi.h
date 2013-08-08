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

#ifndef __NEW_WIFI_H__
#define __NEW_WIFI_H__

#define TARGET

#ifdef __cplusplus
extern "C"
{
#endif

#include <ui-gadget-module.h>

#include "common.h"
#include "common_pswd_popup.h"
#include "view_ime_hidden.h"
#include "common_eap_connect.h"
#include "winset_popup.h"
#include "wlan_manager.h"
#include "viewer_manager.h"

#define PACKAGE					"ug-wifi-efl-UG"
#define LOCALEDIR				"/usr/ug/res/locale"
#define CUSTOM_EDITFIELD_PATH \
					"/usr/ug/res/edje/wifi-efl-UG/custom_editfield.edj"

#define FACTORYFS				"/usr/ug"
#define WIFI_APP_IMAGE_DIR		FACTORYFS "/res/images/wifi-efl-UG"
#define WIFI_APP_ICON_PATH_SCAN \
				WIFI_APP_IMAGE_DIR"/01_controlbar_icon_update.png"
#define WIFI_APP_ICON_PATH_DONE \
				WIFI_APP_IMAGE_DIR"/01_controlbar_icon_edit.png"
#define WIFI_APP_ICON_PATH_FORGET \
				WIFI_APP_IMAGE_DIR"/01_controlbar_icon_delete.png"

#define UG_CALLER "caller"
#define UG_MAIN_MESSAGE_DESTROY 1

typedef enum {
	UG_VIEW_DEFAULT = 0,
	UG_VIEW_SETUP_WIZARD
} UG_TYPE;

typedef struct {
	/* ui gadget object */
	void* gadget;
	ui_gadget_h ug;

	//Basic Evas_Objects
	Evas_Object *layout_main;
	Evas *evas;
	pswd_popup_t *passpopup;
	hiddep_ap_popup_data_t *hidden_ap_popup;

	UG_TYPE ug_type;
	Eina_Bool bAlive;

	char *lbutton_setup_wizard_prev;
	char *rbutton_setup_wizard_next;
	char *rbutton_setup_wizard_skip;
	popup_manager_object_t *popup_manager;
	eap_connect_data_t *eap_view;
	bool is_disconnect;
} wifi_appdata ;

int wifi_exit(void);

#ifdef __cplusplus
}
#endif

#endif /* __WIFI_H__ */
