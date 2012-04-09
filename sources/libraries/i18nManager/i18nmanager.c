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
	case I18N_TYPE_Ok:
		return (char*)dgettext("sys_string", "IDS_COM_SK_OK");
	case I18N_TYPE_Yes:
		return (char*)dgettext("sys_string", "IDS_COM_SK_YES");
	case I18N_TYPE_No:
		return (char*)dgettext("sys_string", "IDS_COM_SK_NO");
	case I18N_TYPE_Save:
		return (char*)dgettext("sys_string", "IDS_COM_OPT_SAVE");
	case I18N_TYPE_Done:
		return (char*)dgettext("sys_string", "IDS_COM_SK_DONE");
	case I18N_TYPE_Back:
		return (char*)dgettext("sys_string", "IDS_COM_BODY_BACK");
	case I18N_TYPE_Cancel:
		return (char*)dgettext("sys_string", "IDS_COM_SK_CANCEL");
	case I18N_TYPE_Activating:
		return (char*)dgettext("sys_string", "IDS_COM_POP_ACTIVATING");
	case I18N_TYPE_Connecting:
		return (char*)dgettext("sys_string", "IDS_COM_POP_CONNECTING");
	case I18N_TYPE_Connected:
		return (char*)dgettext("sys_string", "IDS_COM_POP_CONNECTED");
	case I18N_TYPE_Searching:
		return (char*)dgettext("sys_string", "IDS_COM_POP_SEARCHING");
	case I18N_TYPE_Wi_Fi:
		return (char*)dgettext("sys_string", "IDS_COM_BODY_WI_FI");
	case I18N_TYPE_Name:
		return (char*)dgettext("sys_string", "IDS_COM_BODY_DETAILS_NAME");
	case I18N_TYPE_Password:
		return (char*)dgettext("sys_string", "IDS_COM_BODY_PASSWORD");
	default:
		return "(debug)system_text_failed";
	}
}

char* i18n_manager_get_text(const char *pkg_name, I18N_TYPES type)
{
	switch (type) {
	case I18N_TYPE_Ok:
	case I18N_TYPE_Yes:
	case I18N_TYPE_No:
	case I18N_TYPE_Save:
	case I18N_TYPE_Done:
	case I18N_TYPE_Back:
	case I18N_TYPE_Cancel:
	case I18N_TYPE_Name:
	case I18N_TYPE_Wi_Fi:
	case I18N_TYPE_Password:
	case I18N_TYPE_Activating:
	case I18N_TYPE_Connecting:
	case I18N_TYPE_Connected:
	case I18N_TYPE_Searching:
		return i18n_get_text_by_system(type);

	case I18N_TYPE_Scan:
		return apply_i18n(pkg_name, I18N_MODE_SELF_MADE, "IDS_ST_BODY_SCAN");
	case I18N_TYPE_Forget:
		return apply_i18n(pkg_name, I18N_MODE_SELF_MADE, "IDS_WIFI_SK_FORGET");
	case I18N_TYPE_Dynamic_IP:
		return apply_i18n(pkg_name, I18N_MODE_SELF_MADE, "IDS_WIFI_BODY_DYNAMIC_IP");
	case I18N_TYPE_Static_IP:
		return apply_i18n(pkg_name, I18N_MODE_SELF_MADE, "IDS_WIFI_BODY_STATIC_IP");
	case I18N_TYPE_Security_type:
		return apply_i18n(pkg_name, I18N_MODE_SELF_MADE, "IDS_WIFI_BODY_SECURITY_TYPE");
	case I18N_TYPE_Excellent:
		return apply_i18n(pkg_name, I18N_MODE_SELF_MADE, "IDS_ST_BODY_EXCELLENT");
	case I18N_TYPE_Good:
		return apply_i18n(pkg_name, I18N_MODE_SELF_MADE, "IDS_ST_BODY_GOOD_M_STRENGTH");
	case I18N_TYPE_Disconnecting:
		return apply_i18n(pkg_name, I18N_MODE_SELF_MADE, "IDS_WIFI_BODY_DISCONNECTING_ING");
	case I18N_TYPE_Hidden_AP:
		return apply_i18n(pkg_name, I18N_MODE_SELF_MADE, "IDS_WIFI_HEADER_HIDDEN_NETWORK");
	case I18N_TYPE_No_AP:
		return apply_i18n(pkg_name, I18N_MODE_SELF_MADE, "IDS_WIFI_BODY_NO_APS");
	case I18N_TYPE_Signal_strength:
		return apply_i18n(pkg_name, I18N_MODE_SELF_MADE, "IDS_WIFI_BODY_SIGNAL_STRENGTH");
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
	case I18N_TYPE_WEP:
		return apply_i18n(pkg_name, I18N_MODE_SELF_MADE, "IDS_WIFI_BODY_SECURITYTYPE_WEP");
	case I18N_TYPE_Gateway:
		return apply_i18n(pkg_name, I18N_MODE_SELF_MADE, "IDS_WIFI_BODY_GATEWAY");
	case I18N_TYPE_Deactivating:
		return apply_i18n(pkg_name, I18N_MODE_SELF_MADE, "IDS_ST_POP_DEACTIVATING_ING");
	case I18N_TYPE_No_security:
		return apply_i18n(pkg_name, I18N_MODE_SELF_MADE, "IDS_WIFI_BODY_CONFIGURATION_NO_SECURITY");
	case I18N_TYPE_Network_notification:
		return apply_i18n(pkg_name, I18N_MODE_SELF_MADE, "IDS_ST_BODY_NETWORK_NOTIFICATION");
	case I18N_TYPE_Select_network:
		return apply_i18n(pkg_name, I18N_MODE_SELF_MADE, "IDS_WIFI_HEADER_SELECT_NETWORK");
	case I18N_TYPE_Input_password:
		return apply_i18n(pkg_name, I18N_MODE_SELF_MADE, "IDS_CST_BODY_INPUT_PASSWORD");
	case I18N_TYPE_Show_password:
		return apply_i18n(pkg_name, I18N_MODE_SELF_MADE, "IDS_ST_BODY_SHOW_PASSWORD");

	case I18N_TYPE_Week:
		return gettext("Weak");
	case I18N_TYPE_IP_configuration_failed:
		return gettext("IP configuration failed");
	case I18N_TYPE_Authentication_failed_Check_your_password:
		return gettext("Authentication failed Check your password");
	case I18N_TYPE_Input_AP_name:
		return gettext("Input AP name");
	case I18N_TYPE_Network_popup:
		return gettext("Network popup");
	case I18N_TYPE_You_can_connect_to_a_specific_AP_via_AP_list_popup:
		return gettext("You can connect to a specific AP<br>via AP list poup");
	case I18N_TYPE_Autonomous_connection_to_s_will_be_turned_off_Continue:
		return gettext("Autonomous connecting to<br>%s<br>will be turned off.<br>Continue?");
	case I18N_TYPE_WPA_PSK:
		return gettext("WPA-PSK");
	case I18N_TYPE_WPA2_PSK:
		return gettext("WPA2-PSK");
	case I18N_TYPE_WPA_EAP:
		return gettext("WPA-EAP");
	case I18N_TYPE_WPA2_EAP:
		return gettext("WPA2-EAP");
	default:
		return "(debugapplying_i18n_failed";
	}
}

