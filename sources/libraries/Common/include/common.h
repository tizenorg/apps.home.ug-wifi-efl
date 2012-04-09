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



#ifndef __COMMON_H_
#define __COMMON_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <glib.h>
#include <assert.h>

#define true 1
#define false 0

#ifndef _BOOLEAN_TYPE_H_
#define  _BOOLEAN_TYPE_H_
typedef unsigned short boolean;
#endif /** _BOOLEAN_TYPE_H_ */

#ifndef _UINT32_TYPE_H_
#define _UINT32_TYPE_H_
typedef unsigned int uint32;
#endif /** _UINT32_TYPE_H_ */

#include <linux/sockios.h>
#include <linux/ethtool.h>
#include <errno.h>

#ifndef FACTORYFS
#define FACTORYFS "/opt/ug"
#endif

#include <Elementary.h>
#include <appcore-efl.h>
#include <dlog.h>


#define COMMON_NAME_FUNC	"wifi/func"
#define COMMON_NAME_LIB		"wifi/lib"
#define COMMON_NAME_ERR		"wifi/err"
#define UG_NAME_NORMAL		"wifi_ug"
#define UG_NAME_RESP				"wifi_ug/resp"
#define UG_NAME_REQ				"wifi_ug/req"
#define UG_NAME_SCAN			"wifi_ug/scan"
#define UG_NAME_ERR				"wifi_ug/err"
#define SP_NAME_NORMAL		"wifi_sp"
#define SP_NAME_ERR				"wifi_sp/err"

/* Log Level */
#define COMMON_LOG_DEBUG	LOG_DEBUG
#define COMMON_LOG_INFO		LOG_INFO
#define COMMON_LOG_WARN	LOG_WARN
#define COMMON_LOG_ERROR	LOG_ERROR

#define __COMMON_FUNC_ENTER__ FUNC_LOG(COMMON_NAME_FUNC, "[<Entering]: %s() [%d]", __func__, __LINE__)
#define __COMMON_FUNC_EXIT__ FUNC_LOG(COMMON_NAME_FUNC, "[Quit/>]: %s() [%d]", __func__, __LINE__)

#define FUNC_LOG(MID, format, args...) \
	SLOG(LOG_INFO,  MID, "\033[2m[%s:%d]\033[2m " format "\033[0m", __func__, __LINE__, ##args)
#define DEBUG_LOG(MID, format, args...) \
	SLOG(LOG_DEBUG,  MID, "\033[42m[%s:%d]\033[0m\033[32m " format "\033[0m", __func__, __LINE__, ##args)
#define INFO_LOG(MID, format, args...) \
	SLOG(LOG_INFO,  MID, "\033[0m[%s:%d]\033[0m " format, __func__, __LINE__, ##args)
#define WARN_LOG(MID, format, args...) \
	SLOG(LOG_WARN,  MID, "\033[43m[%s:%d]\033[0m\033[33m " format "\033[0m", __func__, __LINE__, ##args)
#define ERROR_LOG(MID, format, args...) \
	SLOG(LOG_ERROR,  MID, "\033[41m[%s:%d]\033[0m\033[31m " format "\033[0m", __func__, __LINE__, ##args)


#define assertm_if(expr, fmt, arg...) do { \
	if(expr) { \
		ERROR_LOG(COMMON_NAME_ERR, " ##(%s) -> %s() assert!!## "fmt, #expr, __FUNCTION__, ##arg); \
		assert(1); \
	} \
} while (0) /* retvm if */

#endif
