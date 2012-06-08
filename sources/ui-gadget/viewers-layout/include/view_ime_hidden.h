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



#ifndef __VIEW_IME_HIDDEN_H_
#define __VIEW_IME_HIDDEN_H_


typedef enum {
	HIDDEN_AP_SECURITY_PUBLIC = 0,
	HIDDEN_AP_SECURITY_WEP = 1,
	HIDDEN_AP_SECURITY_WPAPSK = 2,
	HIDDEN_AP_SECURITY_ENTERPRISE = 3,
	HIDDEN_AP_SECURITY_DYNAMICWEP = 4
} hidden_ap_security_type_t;

void view_ime_hidden(void);

#endif
