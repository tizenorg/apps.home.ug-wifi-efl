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



#ifndef __I18NMANAGER_H_
#define __I18NMANAGER_H_

#define sc(pkg_name, I18N_TYPE) i18n_manager_get_text(pkg_name, I18N_TYPE)

typedef enum {
	I18N_MODE_SELF_MADE, /* self made, reserve to add platform string convert service */
	I18N_MODE_NO_NEEDED, /* No need to convert other language */
	I18N_MODE_MAX
} I18N_MODES;

typedef enum {
	I18N_TYPE_Ok,
	I18N_TYPE_Yes,
	I18N_TYPE_No,
	I18N_TYPE_Save,
	I18N_TYPE_Done,
	I18N_TYPE_Back,
	I18N_TYPE_Cancel,
	I18N_TYPE_Scan,
	I18N_TYPE_Forget,
	I18N_TYPE_User_ID,
	I18N_TYPE_IP_address,
	I18N_TYPE_Static_IP,
	I18N_TYPE_Dynamic_IP,
	I18N_TYPE_Proxy_address,
	I18N_TYPE_Details,
	I18N_TYPE_Name,
	I18N_TYPE_Signal_strength,
	I18N_TYPE_Excellent,
	I18N_TYPE_Good,
	I18N_TYPE_Week,
	I18N_TYPE_No_security,
	I18N_TYPE_Subnet_mask,
	I18N_TYPE_Gateway,
	I18N_TYPE_Gateway_address,
	I18N_TYPE_Wi_Fi,
	I18N_TYPE_Password,
	I18N_TYPE_Activating,
	I18N_TYPE_Deactivating,
	I18N_TYPE_Searching,
	I18N_TYPE_Connect,
	I18N_TYPE_Connecting,
	I18N_TYPE_Connected,
	I18N_TYPE_Disconnecting,
	I18N_TYPE_Connection_Lost,
	I18N_TYPE_Network_SSID,
	I18N_TYPE_Network_notification,
	I18N_TYPE_Network_notify_me_later,
	I18N_TYPE_Select_network,
	I18N_TYPE_WiFi_network,
	I18N_TYPE_Enter_password,
	I18N_TYPE_Enter_Identity,
	I18N_TYPE_Enter_Anonymous_Identity,
	I18N_TYPE_Input_password,
	I18N_TYPE_Show_password,
	I18N_TYPE_Add_WiFi_network,
	I18N_TYPE_EAP,
	I18N_TYPE_EAP_method,
	I18N_TYPE_Phase_2_authentication,
	I18N_TYPE_Activating_WiFi,

	I18N_TYPE_Provisioning,
	I18N_TYPE_Ca_Certificate,
	I18N_TYPE_User_Certificate,
	I18N_TYPE_Unspecified,
	I18N_TYPE_Unknown,
	I18N_TYPE_Enter_Ssid,
	I18N_TYPE_Identity,
	I18N_TYPE_Anonymous_Identity,
	I18N_TYPE_Wifi_Opt_Find_Hidden_Network,
	I18N_TYPE_WPS_Button_Connection,
	I18N_TYPE_Press_WPS_On_Your_Wi_Fi_Access_Point,
	I18N_TYPE_Button_Cancel,
	I18N_TYPE_Ssid,
	I18N_TYPE_Find_Hidden_Network,

/* etc */
	I18N_TYPE_DNS_1,
	I18N_TYPE_DNS_2,
	I18N_TYPE_WEP,
	I18N_TYPE_WPA_PSK,
	I18N_TYPE_WPA2_PSK,
	I18N_TYPE_WPA_EAP,
	I18N_TYPE_WPA2_EAP,
	I18N_TYPE_Hidden_AP,
	I18N_TYPE_No_AP,
	I18N_TYPE_Security_type,
	I18N_TYPE_Autonomous_connection_to_s_will_be_turned_off_Continue,
	I18N_TYPE_Open,
	I18N_TYPE_WPS_Available,
	I18N_TYPE_Secured,
	I18N_TYPE_Obtaining_IP_addr,
	I18N_TYPE_Channel,
	I18N_TYPE_MAC_addr,
	I18N_TYPE_Proxy_port,
	I18N_TYPE_MAX
} I18N_TYPES;

char* i18n_manager_get_text(const char *pkg_name, I18N_TYPES type);

#endif
