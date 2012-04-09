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
	I18N_TYPE_Wi_Fi,
	I18N_TYPE_Password,
	I18N_TYPE_Activating,
	I18N_TYPE_Deactivating,
	I18N_TYPE_Searching,
	I18N_TYPE_Connecting,
	I18N_TYPE_Connected,
	I18N_TYPE_Disconnecting,
	I18N_TYPE_Connection_Lost,
	I18N_TYPE_IP_configuration_failed,
	I18N_TYPE_Authentication_failed_Check_your_password,
	I18N_TYPE_Network_notification,
	I18N_TYPE_Select_network,
	I18N_TYPE_Input_password,
	I18N_TYPE_Show_password,
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
	I18N_TYPE_Input_AP_name,
	I18N_TYPE_Network_popup,
	I18N_TYPE_You_can_connect_to_a_specific_AP_via_AP_list_popup,
	I18N_TYPE_Autonomous_connection_to_s_will_be_turned_off_Continue,
	I18N_TYPE_MAX
} I18N_TYPES;

char* i18n_manager_get_text(const char *pkg_name, I18N_TYPES type);

#endif
