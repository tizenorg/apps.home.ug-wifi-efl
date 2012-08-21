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



#ifndef __COMMON_IP_INFO_H_
#define __COMMON_IP_INFO_H_

typedef struct ip_info_list ip_info_list_t;

ip_info_list_t *ip_info_append_items(const char *profile_name, const char *pkg_name, Evas_Object *genlist);
void ip_info_save_data(ip_info_list_t *ip_info_list, boolean b_save_to_profile);
void ip_info_remove(ip_info_list_t *ip_info_list);

#endif
