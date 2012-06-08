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



#ifndef __WIFI_ENGINE_CALLBACK_H_
#define __WIFI_ENGINE_CALLBACK_H_

#ifdef __cplusplus
extern "C"
{
#endif

void wlan_engine_callback(void *user_data, void *wlan_data);
void wlan_engine_refresh_callback(int is_scan);

#ifdef __cplusplus
}
#endif

#endif /* __WIFI_ENGINE_CALLBACKS_H_ */

