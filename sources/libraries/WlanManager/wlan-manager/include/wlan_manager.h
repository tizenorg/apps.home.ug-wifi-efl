/*
*  Wi-Fi UG
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



#ifndef __WIFI_WLAN_MANAGER_H__
#define __WIFI_WLAN_MANAGER_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <glib.h>
#include <dbus/dbus-glib.h>
#include "connman-request.h"
#include "connman-response.h"
#include "connman-profile-manager.h"

typedef void* DEVICE_OBJECT;

#ifndef _BOOLEAN_TYPE_H_
#define _BOOLEAN_TYPE_H_
typedef unsigned short boolean;
#endif /** _BOOLEAN_TYPE_H_ */


typedef enum {
	WLAN_MANAGER_ERR_NONE = 0,
	WLAN_MANAGER_ERR_UNKNOWN,
	WLAN_MANAGER_ERR_INVALID_PARAM,
	WLAN_MANAGER_ERR_ALREADY_REGISTERED,
	WLAN_MANAGER_ERR_CONNECT_PASSWORD_NEEDED,
	WLAN_MANAGER_ERR_CONNECT_EAP_SEC_TYPE,
	WLAN_MANAGER_ERR_MOBILE_HOTSPOT_OCCUPIED,
	WLAN_MANAGER_ERR_NOSERVICE,
	WLAN_MANAGER_ERR_IN_PROGRESS,
} WLAN_MANAGER_ERR_TYPE;

typedef enum {
	IP_TYPE_NULL,
	IP_TYPE_STATIC_IP,
	IP_TYPE_DHCP_IP,
	IP_TYPE_MAX
} IP_TYPES;

typedef enum {
	SIGNAL_STRENGTH_TYPE_EXCELLENT,
	SIGNAL_STRENGTH_TYPE_GOOD,
	SIGNAL_STRENGTH_TYPE_WEAK,
	SIGNAL_STRENGTH_TYPE_VERY_WEAK,
	SIGNAL_STRENGTH_TYPE_NULL
} STRENGTH_TYPES;

/*
 * JOIN TYPE, SECURITY TYPE and EAP_TYPE
 *
 * Determine it`s security type
 * It should be merged into one enumerations
*/
typedef enum {
	SECURITY_TYPE_NONE=0x01,
	SECURITY_TYPE_WEP,
	SECURITY_TYPE_WPA_PSK,
	SECURITY_TYPE_WPA2_PSK,

	SECURITY_TYPE_WPA_EAP,
	SECURITY_TYPE_WPA2_EAP,

	SECURITY_TYPE_MAX,
	SECURITY_TYPE_NULL
} SECURITY_TYPES;

typedef enum {
	WLAN_MANAGER_EAP_TYPE_NONE, /* only use it`s WPA number */
	WLAN_MANAGER_EAP_TYPE_TLS,
	WLAN_MANAGER_EAP_TYPE_TTLS_PAP,         /** PAP is tunneled protocol */
	WLAN_MANAGER_EAP_TYPE_TTLS_CHAP,             /** CHAP is tunneled protocol */
	WLAN_MANAGER_EAP_TYPE_TTLS_MSCHAP,           /** MSCHAP is tunneled protocol */
	WLAN_MANAGER_EAP_TYPE_TTLS_MSCHAPV2,         /** MSCHAPV2 is tunneled protocol */

	/** Define EAP-TTLS and tunneled EAP sub types here */
	WLAN_MANAGER_EAP_TYPE_TTLS_EAP_GTC,     /** EAP_GTC is tunneled protocol */
	WLAN_MANAGER_EAP_TYPE_TTLS_EAP_MD5,          /** EAP_MD5 is tunneled protocol */
	WLAN_MANAGER_EAP_TYPE_TTLS_EAP_MSCHAPV2,     /** EAP_MSCHAPV2 is tunneled protocol */

	/** Define EAP-PEAP version 0 */
	WLAN_MANAGER_EAP_TYPE_PEAP0_MSCHAPV2,   /** MSCHAPV2 is tunneled protocol, PEAP version is 0 */
	WLAN_MANAGER_EAP_TYPE_PEAP0_MD5,             /** MD5 is tunneled protocol, PEAP version is 0 */
	WLAN_MANAGER_EAP_TYPE_PEAP0_GTC,             /** GTC is tunneled protocol, PEAP version is 0 */

	/** Define EAP-PEAP version 1 */
	WLAN_MANAGER_EAP_TYPE_PEAP1_MSCHAPV2,   /** MSCHAPV2 is tunneled protocol, PEAP version is 1 */
	WLAN_MANAGER_EAP_TYPE_PEAP1_MD5,             /** MD5 is tunneled protocol, PEAP version is 1 */
	WLAN_MANAGER_EAP_TYPE_PEAP1_GTC,             /** GTC is tunneled protocol, PEAP version is 1 */
	WLAN_MANAGER_EAP_TYPE_MAX,
	WLAN_MANAGER_EAP_TYPE_ERROR
} WLAN_MANAGER_EAP_TYPES;

/*
 * RESPONSES
 */
typedef enum {
	WLAN_MANAGER_RESPONSE_TYPE_NONE,
	WLAN_MANAGER_RESPONSE_TYPE_CONNECTING,
	WLAN_MANAGER_RESPONSE_TYPE_CONNECTION_OK,
	WLAN_MANAGER_RESPONSE_TYPE_CONNECTION_FAIL_IN_PROGRESS,
	WLAN_MANAGER_RESPONSE_TYPE_CONNECTION_FAIL_ABORTED,
	WLAN_MANAGER_RESPONSE_TYPE_CONNECTION_FAIL_ALREADY_EXIST,
	WLAN_MANAGER_RESPONSE_TYPE_CONNECTION_FAIL_TIMEOUT,
	WLAN_MANAGER_RESPONSE_TYPE_CONNECTION_FAIL_UNKNOWN,
	WLAN_MANAGER_RESPONSE_TYPE_CONNECTION_FAIL_UNKNOWN_METHOD,
	WLAN_MANAGER_RESPONSE_TYPE_CONNECTION_CONNECT_FAILED,
	WLAN_MANAGER_RESPONSE_TYPE_CONNECTION_INVALID_KEY,
	WLAN_MANAGER_RESPONSE_TYPE_DISCONNECTION_OK,
	WLAN_MANAGER_RESPONSE_TYPE_POWER_ON_OK,
	WLAN_MANAGER_RESPONSE_TYPE_POWER_ON_NOT_SUPPORTED,
	WLAN_MANAGER_RESPONSE_TYPE_POWER_ON_RESTRICTED,
	WLAN_MANAGER_RESPONSE_TYPE_POWER_OFF_OK,
	WLAN_MANAGER_RESPONSE_TYPE_SCAN_OK,
	WLAN_MANAGER_RESPONSE_TYPE_WPS_ENROLL_OK,
	WLAN_MANAGER_RESPONSE_TYPE_WPS_ENROLL_FAIL,
	WLAN_MANAGER_RESPONSE_TYPE_CANCEL_WPS_ENROLL_OK,
	WLAN_MANAGER_RESPONSE_TYPE_CANCEL_WPS_ENROLL_FAIL,
	WLAN_MANAGER_RESPONSE_TYPE_CONNECTION_IND,
	WLAN_MANAGER_RESPONSE_TYPE_DISCONNECTION_IND,
	WLAN_MANAGER_RESPONSE_TYPE_SCAN_RESULT_IND,
	WLAN_MANAGER_RESPONSE_TYPE_MAC_ID_IND,
	WLAN_MANAGER_RESPONSE_TYPE_SPECIFIC_SCAN_OK,
	WLAN_MANAGER_RESPONSE_TYPE_SPECIFIC_SCAN_FAIL,
	WLAN_MANAGER_RESPONSE_TYPE_SPECIFIC_SCAN_IND,
	WLAN_MANAGER_RESPONSE_TYPE_MAX
} WLAN_MANAGER_RESPONSE_TYPES;

#define WLAN_RSSI_LEVEL_EXCELLENT		64
#define WLAN_RSSI_LEVEL_GOOD			59
#define WLAN_RSSI_LEVEL_WEAK			34

#define WLAN_PROXY_LEN_MAX 64

typedef struct {
	DBusGProxy *proxy;
	DBusGProxyCall * pending_call;
	boolean is_handled;
} wifi_pending_call_info_t;

wifi_pending_call_info_t	g_pending_call;

typedef struct _wifi_device_info_t {
	char* profile_name;
	char* ssid;
	char* ap_status_txt;
	char* ap_image_path;
	int ipconfigtype;
	int rssi;
	wlan_security_mode_type_t security_mode;
	boolean wps_mode;
} wifi_device_info_t;

typedef struct {
	char* profile_name;
	WLAN_MANAGER_RESPONSE_TYPES type;
} callback_data;

typedef struct {
	const char* password;
	char* category;
	char* subcategory;
	char* username;
	char* userpassword;
	char* ca_cert_filename;
	char* client_cert_filename;
	char* private_key;
	char* private_key_password;
	int wlan_eap_type;
} wlan_manager_password_data;

/* it should be implement. */
typedef enum {
	WLAN_MANAGER_ERROR=0x01,
	WLAN_MANAGER_OFF,
	WLAN_MANAGER_UNCONNECTED,
	WLAN_MANAGER_CONNECTED,
	WLAN_MANAGER_CONNECTING,
	WLAN_MANAGER_DISCONNECTING,
	WLAN_MANAGER_MAX
} WLAN_MANAGER_STATES;


struct access_point_info {
	char* profile_name;
	char* ssid;
};

typedef void (*wlan_manager_ui_refresh_func_t)(void);

/** It should be hide to others */
typedef struct wlan_manager_object {

	void (*message_func)(void *user_data, void *data);
	wlan_manager_ui_refresh_func_t refresh_func;
	boolean b_scan_blocked;
	boolean b_ui_refresh;
	/* AP SSID  & saved profile */
	/** should be changed to DEVICE_OBJECT **/
	struct access_point_info connected_AP;

} wlan_manager_object;

/////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////
// FUNCTIONS 
/////////////////////////////////////////////////////////////////


wlan_manager_object* wlan_manager_get_singleton(void);
void* wlan_manager_create();
int wlan_manager_destroy();
int wlan_manager_start();

int wlan_manager_state_get(char* profile_name);
void wlan_manager_set_message_callback(void *func);
void wlan_manager_set_refresh_callback(wlan_manager_ui_refresh_func_t func);

void wlan_manager_set_connected_AP(const net_profile_info_t *profile);
void wlan_manager_reset_connected_AP(void);
void wlan_manager_enable_scan_result_update(void);
void wlan_manager_disable_scan_result_update(void);
const char *wlan_manager_get_connected_profile(void);
const char *wlan_manager_get_connected_ssid(void);

// * request
int wlan_manager_request_power_on(void);
int wlan_manager_request_power_off(void);
int wlan_manager_request_connection(void *data);
int wlan_manager_request_disconnection(void *data);
int wlan_manager_request_cancel_connecting(const char *profile_name);
int wlan_manager_request_scan(void);
int wlan_manager_request_wps_connection(const char *profile_name);
int wlan_manager_request_cancel_wps_connection(const char *profile_name);

// * connect, disconnect and forget
int wlan_manager_connect_with_profile(const char *profile_name);
int wlan_manager_connect_with_password(const char *profile_name, int security_mode, void* authdata);
int wlan_manager_disconnect(void* data);
int wlan_manager_forget(const char *profile_name);
void *wlan_manager_profile_table_get();

// * profile add/modify/delete
int wlan_manager_request_profile_add(const char *profile_name, int security_mode, void* authdata);
int wlan_manager_profile_modify_by_device_info(net_profile_info_t *profiles);

STRENGTH_TYPES wlan_manager_get_signal_strength(int rssi);

//// profile refresh /////////////////////////////////////////////
void wlan_manager_scanned_profile_refresh(void);
int wlan_manager_scanned_profile_refresh_with_count(int count);
int wlan_manager_profile_scanned_length_get();
void *wlan_manager_profile_device_info_blank_create(void);

int wlan_manager_network_syspopup_message(const char *title, const char *content, const char *type);

#ifdef __cplusplus
}
#endif

#endif /* __WIFI_REQUEST_HANDLER_H__ */
