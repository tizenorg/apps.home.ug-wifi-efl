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


#ifndef __COMMON_PSWD_POPUP_H_
#define __COMMON_PSWD_POPUP_H_

typedef struct {
	char *title;
	Eina_Bool	show_wps_btn;
	Evas_Smart_Cb ok_cb;
	Evas_Smart_Cb cancel_cb;
	Evas_Smart_Cb wps_btn_cb;
	void *cb_data;
} pswd_popup_create_req_data_t;

typedef struct pswd_popup pswd_popup_t;

pswd_popup_t *common_pswd_popup_create(Evas_Object *win_main, const char *pkg_name, pswd_popup_create_req_data_t *popup_info);
void common_pswd_popup_pbc_popup_create(pswd_popup_t *pswd_popup_data, Evas_Smart_Cb cancel_cb, void *cancel_cb_data);
char *common_pswd_popup_get_txt(pswd_popup_t *pswd_popup_data);
void common_pswd_popup_destroy(pswd_popup_t *pswd_popup_data);

#endif
