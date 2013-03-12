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

#ifndef __COMMON_INVALID_PASSWORD_H__
#define __COMMON_INVALID_PASSWORD_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <glib.h>

int _common_register_invalid_password_popup(void);
int _common_deregister_invalid_password_popup(void);

gboolean _common_is_invalid_password(void);

#ifdef __cplusplus
}
#endif

#endif /* __COMMON_INVALID_PASSWORD_H__ */
