/*
  * Copyright 2012  Samsung Electronics Co., Ltd
  *
  * Licensed under the Flora License, Version 1.0 (the "License");
  * you may not use this file except in compliance with the License.
  * You may obtain a copy of the License at
  *
  *    http://www.tizenopensource.org/license
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  */



#ifndef __VIEW_EAP_H_
#define __VIEW_EAP_H_

typedef enum {
	EAP_SEC_TYPE_PEAP  = 0,
	EAP_SEC_TYPE_TLS,
	EAP_SEC_TYPE_TTLS ,
	EAP_SEC_TYPE_SIM,
	EAP_SEC_TYPE_AKA ,
	EAP_SEC_TYPE_NULL
} eap_type_t;

void view_eap(wifi_device_info_t *device_info);

#endif
