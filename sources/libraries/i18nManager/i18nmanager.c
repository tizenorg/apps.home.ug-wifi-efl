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

#include "i18nmanager.h"
#include <libintl.h>
#include <stdio.h>
#include <string.h>

static char* apply_i18n(const char *pkg_name, I18N_MODES mode, const char *string)
{
	switch (mode) {
	case I18N_MODE_SELF_MADE:
		return (char*) dgettext(pkg_name, string);
	case I18N_MODE_NO_NEEDED:
		return (char*) gettext(string);
	default:
		return "(debug)applying_i18n_failed";
	}
}

static char* i18n_get_text_by_system(I18N_TYPES type){
	switch (type) {
	case I18N_TYPE_Yes:
		return (char*)dgettext("sys_string", "IDS_COM_SK_YES");
	case I18N_TYPE_Ok:
		return (char*)dgettext("sys_string", "IDS_COM_SK_OK");
	case I18N_TYPE_No:
		return (char*)dgettext("sys_string", "IDS_COM_SK_NO");
	case I18N_TYPE_Cancel:
		return (char*)dgettext("sys_string", "IDS_COM_SK_CANCEL");
	case I18N_TYPE_Activating:
		return (char*)dgettext("sys_string", "IDS_COM_POP_ACTIVATING");
	case I18N_TYPE_Connecting:
		return (char*)dgettext("sys_string", "IDS_COM_POP_CONNECTING");
	case I18N_TYPE_Connected:
		return (char*)dgettext("sys_string", "IDS_COM_POP_CONNECTED");
	case I18N_TYPE_Wi_Fi:
		return (char*)dgettext("sys_string", "IDS_COM_BODY_WI_FI");
	case I18N_TYPE_Name:
		return (char*)dgettext("sys_string", "IDS_COM_BODY_DETAILS_NAME");
	case I18N_TYPE_Password:
		return (char*)dgettext("sys_string", "IDS_COM_BODY_PASSWORD");
	case I18N_TYPE_Details:
		return (char*)dgettext("sys_string", "IDS_COM_BODY_DETAILS");
	case I18N_TYPE_Unknown:
		return (char*)dgettext("sys_string", "IDS_COM_BODY_UNKNOWN");
	case I18N_TYPE_Scan:
		return (char*)dgettext("sys_string", "IDS_COM_BUTTON_SCAN");
	case I18N_TYPE_Open:
		return (char*)dgettext("sys_string", "IDS_COM_SK_OPEN");
	default:
		return "(debug)system_text_failed";
	}
}

char* i18n_manager_get_text(const char *pkg_name, I18N_TYPES type)
{
	switch (type) {
	case I18N_TYPE_Yes:
	case I18N_TYPE_Ok:
	case I18N_TYPE_No:
	case I18N_TYPE_Cancel:
	case I18N_TYPE_Name:
	case I18N_TYPE_Wi_Fi:
	case I18N_TYPE_Password:
	case I18N_TYPE_Activating:
	case I18N_TYPE_Connecting:
	case I18N_TYPE_Connected:
	case I18N_TYPE_Details:
	case I18N_TYPE_Unknown:
	case I18N_TYPE_Scan:
	case I18N_TYPE_Open:
		return i18n_get_text_by_system(type);
	case I18N_TYPE_Provisioning:
		return apply_i18n(pkg_name, I18N_MODE_SELF_MADE, "IDS_WIFI_BODY_PROVISIONING");
	case I18N_TYPE_User_Certificate:
		return apply_i18n(pkg_name, I18N_MODE_SELF_MADE, "IDS_WIFI_BODY_USER_CERTIFICATE_ABB");
	case I18N_TYPE_Unspecified:
		return apply_i18n(pkg_name, I18N_MODE_SELF_MADE, "IDS_WIFI_BODY_UNSPECIFIED");
	case I18N_TYPE_Enter_Ssid:
		return apply_i18n(pkg_name, I18N_MODE_SELF_MADE, "IDS_WIFI_BODY_ENTER_SSID");
	case I18N_TYPE_Enter_Your_Id:
		return apply_i18n(pkg_name, I18N_MODE_SELF_MADE, "IDS_WIFI_BODY_ENTER_YOUR_ID");
	case I18N_TYPE_Enter_Your_Password:
		return apply_i18n(pkg_name, I18N_MODE_SELF_MADE, "IDS_WIFI_BODY_ENTER_YOUR_PASSWORD");
	case I18N_TYPE_Identity:
		return apply_i18n(pkg_name, I18N_MODE_SELF_MADE, "IDS_WIFI_BODY_IDENTITY");
	case I18N_TYPE_Ssid:
		return apply_i18n(pkg_name, I18N_MODE_SELF_MADE, "IDS_WIFI_BODY_SSID");
	case I18N_TYPE_Find_Hidden_Network:
		return apply_i18n(pkg_name, I18N_MODE_SELF_MADE, "IDS_WIFI_OPT_FIND_HIDDEN_NETWORK");
	case I18N_TYPE_Connect:
		return apply_i18n(pkg_name, I18N_MODE_SELF_MADE, "IDS_WIFI_BODY_CONNECT");
	case I18N_TYPE_Forget:
		return apply_i18n(pkg_name, I18N_MODE_SELF_MADE, "IDS_WIFI_SK_FORGET");
	case I18N_TYPE_Static_IP:
		return apply_i18n(pkg_name, I18N_MODE_SELF_MADE, "IDS_WIFI_BODY_STATIC_IP");
	case I18N_TYPE_Excellent:
		return apply_i18n(pkg_name, I18N_MODE_SELF_MADE, "IDS_ST_BODY_EXCELLENT");
	case I18N_TYPE_Good:
		return apply_i18n(pkg_name, I18N_MODE_SELF_MADE, "IDS_ST_BODY_GOOD_M_STRENGTH");
	case I18N_TYPE_Week:
		return apply_i18n(pkg_name, I18N_MODE_SELF_MADE, "IDS_WIFI_BODY_WEAK_M_STRENGTH");
	case I18N_TYPE_Disable_WiFi_Tethering_To_Connect_To_A_WiFi_Network:
		return apply_i18n(pkg_name, I18N_MODE_SELF_MADE, "IDS_WIFI_BODY_DISABLE_WI_FI_TETHERING_TO_CONNECT_TO_A_WI_FI_NETWORK");
	case I18N_TYPE_Disconnecting:
		return apply_i18n(pkg_name, I18N_MODE_SELF_MADE, "IDS_WIFI_BODY_DISCONNECTING_ING");
	case I18N_TYPE_IP_address:
		return apply_i18n(pkg_name, I18N_MODE_SELF_MADE, "IDS_WIFI_BODY_IP_ADDRESS");
	case I18N_TYPE_Proxy_address:
		return apply_i18n(pkg_name, I18N_MODE_SELF_MADE, "IDS_WIFI_HEADER_PROXY_ADDRESS");
	case I18N_TYPE_Subnet_mask:
		return apply_i18n(pkg_name, I18N_MODE_SELF_MADE, "IDS_WIFI_BODY_SUBNET_MASK");
	case I18N_TYPE_DNS_1:
		return apply_i18n(pkg_name, I18N_MODE_SELF_MADE, "IDS_WIFI_BODY_DNS_1");
	case I18N_TYPE_DNS_2:
		return apply_i18n(pkg_name, I18N_MODE_SELF_MADE, "IDS_WIFI_BODY_DNS_2");
	case I18N_TYPE_Gateway:
		return apply_i18n(pkg_name, I18N_MODE_SELF_MADE, "IDS_WIFI_BODY_GATEWAY");
	case I18N_TYPE_Gateway_address:
		return apply_i18n(pkg_name, I18N_MODE_SELF_MADE, "IDS_WIFI_BODY_GATEWAY_ADDRESS");
	case I18N_TYPE_Deactivating:
		return apply_i18n(pkg_name, I18N_MODE_SELF_MADE, "IDS_WIFI_POP_DEACTIVATING_WI_FI_ING");
	case I18N_TYPE_WiFi_network:
		return apply_i18n(pkg_name, I18N_MODE_SELF_MADE, "IDS_ST_BODY_WI_FI_NETWORKS");
	case I18N_TYPE_Enter_password:
		return apply_i18n(pkg_name, I18N_MODE_SELF_MADE, "IDS_WIFI_BODY_ENTER_PASSWORD");
	case I18N_TYPE_Enter_Identity:
		return apply_i18n(pkg_name, I18N_MODE_SELF_MADE, "IDS_WIFI_BODY_ENTER_IDENTITY");
	case I18N_TYPE_Show_password:
		return apply_i18n(pkg_name, I18N_MODE_SELF_MADE, "IDS_ST_BODY_SHOW_PASSWORD");
	case I18N_TYPE_Autonomous_connection_to_s_will_be_turned_off_Continue:
		return apply_i18n(pkg_name, I18N_MODE_SELF_MADE, "IDS_WIFI_POP_AUTOMATIC_CONNECTION_NETWORK_WILL_DISABLED_CONTINUE_Q_MSG");
	case I18N_TYPE_Wait:
		return apply_i18n(pkg_name, I18N_MODE_SELF_MADE, "IDS_WIFI_POP_PLEASE_WAIT_ING");
	case I18N_TYPE_TRY_CONNECT:
		return apply_i18n(pkg_name, I18N_MODE_SELF_MADE, "IDS_ST_BODY_WI_FI_NETWORK_DETECTED_CONNECT_Q");
	case I18N_TYPE_FIND_FAIL:
		return apply_i18n(pkg_name, I18N_MODE_SELF_MADE, "IDS_WIFI_POP_UNABLE_TO_FIND_NETWORK");
	case I18N_TYPE_Invalid_password:
		return apply_i18n(pkg_name, I18N_MODE_SELF_MADE, "IDS_ST_POP_INVALID_PASSWORD");
	case I18N_TYPE_WPS_Available:
		return apply_i18n(pkg_name, I18N_MODE_SELF_MADE, "IDS_ST_BODY_WPS_AVAILABLE");
	case I18N_TYPE_Secured:
		return apply_i18n(pkg_name, I18N_MODE_SELF_MADE, "IDS_ST_BODY_SECURED_ABB_M_WIFI_AP_SUMMARY");
	case I18N_TYPE_Channel:
		return apply_i18n(pkg_name, I18N_MODE_SELF_MADE, "IDS_WIFI_BODY_CHANNEL");
	case I18N_TYPE_MAC_addr:
		return apply_i18n(pkg_name, I18N_MODE_SELF_MADE, "IDS_WIFI_BODY_MAC_ADDRESS");
	case I18N_TYPE_Proxy_port:
		return apply_i18n(pkg_name, I18N_MODE_SELF_MADE, "IDS_WIFI_BODY_PROXY_PORT");
	case I18N_TYPE_EAP:
		return apply_i18n(pkg_name, I18N_MODE_SELF_MADE, "IDS_WIFI_BODY_EAP");
	case I18N_TYPE_EAP_method:
		return apply_i18n(pkg_name, I18N_MODE_SELF_MADE, "IDS_WIFI_BODY_EAP_METHOD");
	case I18N_TYPE_Phase_2_authentication:
		return apply_i18n(pkg_name, I18N_MODE_SELF_MADE, "IDS_COM_BODY_PHASE_2_AUTHENTICATION");
	case I18N_TYPE_Activating_WiFi:
		return apply_i18n(pkg_name, I18N_MODE_SELF_MADE, "IDS_WIFI_POP_ACTIVATING_WI_FI_ING");
	case I18N_TYPE_Unchanged:
		return apply_i18n(pkg_name, I18N_MODE_SELF_MADE, "IDS_ST_BODY_HUNCHANGED");
	case I18N_TYPE_WPS_Button_Connection:
		return apply_i18n(pkg_name, I18N_MODE_SELF_MADE, "IDS_WIFI_BODY_WPS_BUTTON_CONNECTION");
	case I18N_TYPE_Ca_Certificate:
		return apply_i18n(pkg_name, I18N_MODE_SELF_MADE, "IDS_WIFI_BODY_CA_CERTIFICATE");
	case I18N_TYPE_Enter_Anonymous_Identity:
			return apply_i18n(pkg_name, I18N_MODE_SELF_MADE, "IDS_WIFI_BODY_ENTER_ANONYMOUS_IDENTITY");
	case I18N_TYPE_Anonymous_Identity:
			return apply_i18n(pkg_name, I18N_MODE_SELF_MADE, "IDS_WIFI_BODY_ANONYMOUS_IDENTITY");
	case I18N_TYPE_Network_notification:
		return apply_i18n(pkg_name, I18N_MODE_SELF_MADE, "IDS_ST_BODY_NETWORK_NOTIFICATION");
	case I18N_TYPE_Network_notify_me_later:
		return apply_i18n(pkg_name, I18N_MODE_SELF_MADE, "IDS_WIFI_BODY_NOTIFY_WHEN_WI_FI_NETWORK_IS_FOUND");
	case I18N_TYPE_WPS:
		return apply_i18n(pkg_name, I18N_MODE_SELF_MADE, "IDS_ST_HEADER_WPS_CONNECTION_ABB");
	case I18N_TYPE_Press_WPS_On_Your_Wi_Fi_Access_Point:
		return apply_i18n(pkg_name, I18N_MODE_SELF_MADE, "IDS_ST_BODY_PRESS_WPS_ON_YOUR_WI_FI_ACCESS_POINT_WITHIN_2_MINUTES");
	case I18N_TYPE_WPS_Button:
		return apply_i18n(pkg_name, I18N_MODE_SELF_MADE, "IDS_WIFI_BUTTON_WPS_BUTTON");

	default:
		return apply_i18n(pkg_name, I18N_MODE_NO_NEEDED, "(debugapplying_i18n_failed");
	}
}
