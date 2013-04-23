/*
 * Wi-Fi
 *
 * Copyright 2012 Samsung Electronics Co., Ltd
 *
 * Licensed under the Flora License, Version 1.0 (the "License");
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

#ifndef __COMMON_H__
#define __COMMON_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <dlog.h>
#include <assert.h>

#define COMMON_NAME_FUNC	"wifi/func"
#define COMMON_NAME_LIB		"wifi/lib"
#define COMMON_NAME_ERR		"wifi/err"
#define UG_NAME_NORMAL		"wifi_ug"
#define UG_NAME_RESP		"wifi_ug/resp"
#define UG_NAME_REQ			"wifi_ug/req"
#define UG_NAME_SCAN		"wifi_ug/scan"
#define UG_NAME_ERR			"wifi_ug/err"
#define SP_NAME_NORMAL		"wifi_sp"
#define SP_NAME_ERR			"wifi_sp/err"

/* Log Level */
#define COMMON_LOG_DEBUG	LOG_DEBUG
#define COMMON_LOG_INFO		LOG_INFO
#define COMMON_LOG_WARN		LOG_WARN
#define COMMON_LOG_ERROR	LOG_ERROR

#define MAX_DEVICE_ICON_PATH_STR_LEN			256
#define WIFI_UNKNOWN_DEVICE_STATUS_STR			"Unknown"
#define ENTER_SSID_STR							"Enter SSID"
#define EAP_CHECK_YOUR_ID_STR					"Check your identity"
#define EAP_CHECK_YOUR_PASWD_STR				"Check your password"
#define WEP_WRONG_PASSWORD_LEN_ERR_MSG_STR \
	"WEP requires 5, 10, 13, 26 letters for a password.<br>Please, check your input."
#define WPA_WRONG_PASSWORD_LEN_ERR_MSG_STR \
	"WPA2 requires 8 - 63 letters for a password.<br>Please, check your input."
#define WIFI_TETHERING_FAILED_STR \
	"Wi-Fi tethering enabled. Will you turn Wi-Fi tethering off?"
#define OPEN_HIDDEN_NETWORK_STR			"Wi-Fi network detected. Connect?"
#define FIND_HIDDEN_NETWORK_STR			"Find hidden network"
#define SCREEN_TYPE_ID_KEY				"screen_type_id_key"
#define INVALID_PASSWORD				"Your password might be invalid"

/* Syspopup height for portrait mode*/
#define DEVICE_PICKER_POPUP_H		880

/* Syspopup height for landscape mode */
#define DEVICE_PICKER_POPUP_LN_H		480

typedef enum {
	VIEW_MANAGER_VIEW_TYPE_MAIN,
	VIEW_MANAGER_VIEW_TYPE_DETAIL,
	VIEW_MANAGER_VIEW_TYPE_EAP,
} view_manager_view_type_t;

#define __COMMON_FUNC_ENTER__ \
	FUNC_LOG(COMMON_NAME_FUNC, "[<Entering]: %s() [%d]", __func__, __LINE__)
#define __COMMON_FUNC_EXIT__ \
	FUNC_LOG(COMMON_NAME_FUNC, "[Quit/>]: %s() [%d]", __func__, __LINE__)

#define FUNC_LOG(MID, format, args...) \
	SLOG(LOG_INFO, MID, "\033[2m[%s:%d]\033[2m " format "\033[0m", __func__, __LINE__, ##args)
#define DEBUG_LOG(MID, format, args...) \
	SLOG(LOG_DEBUG, MID, "\033[42m[%s:%d]\033[0m\033[32m " format "\033[0m", __func__, __LINE__, ##args)
#define INFO_LOG(MID, format, args...) \
	SLOG(LOG_INFO, MID, "\033[0m[%s:%d]\033[0m " format, __func__, __LINE__, ##args)
#define WARN_LOG(MID, format, args...) \
	SLOG(LOG_WARN, MID, "\033[43m[%s:%d]\033[0m\033[33m " format "\033[0m", __func__, __LINE__, ##args)
#define ERROR_LOG(MID, format, args...) \
	SLOG(LOG_ERROR, MID, "\033[41m[%s:%d]\033[0m\033[31m " format "\033[0m", __func__, __LINE__, ##args)

#define retm_if(expr) do { \
	if (expr) { \
		ERROR_LOG(COMMON_NAME_ERR, "[%s(): %d] (%s) [return]", __FUNCTION__, __LINE__, #expr); \
		return; \
	} \
} while (0)

#define retvm_if(expr, val) do { \
	if (expr) { \
		ERROR_LOG(COMMON_NAME_ERR, "[%s(): %d] (%s) [return]", __FUNCTION__, __LINE__, #expr); \
		return (val); \
	} \
} while (0)

#define assertm_if(expr, fmt, arg...) do { \
	if(expr) { \
		ERROR_LOG(COMMON_NAME_ERR, " ##(%s) -> %s() assert!!## "fmt, #expr, __FUNCTION__, ##arg); \
		assert(1); \
	} \
} while (0) /* retvm if */

#ifdef __cplusplus
}
#endif

#endif
