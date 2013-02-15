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



#include "common.h"
#include "wlan_manager.h"


typedef struct connman_profile_manager {
	int profile_num;
	net_profile_info_t* profile_table;
} connman_profile_manager;
connman_profile_manager *_profile_manager = NULL;

static connman_profile_manager *connman_profile_manager_get_singleton()
{
	if (_profile_manager == NULL) {
		_profile_manager = (connman_profile_manager *)malloc(sizeof(connman_profile_manager));
		_profile_manager->profile_num = 0;
		_profile_manager->profile_table = NULL;
	}

	return _profile_manager;
}

void connman_profile_manager_destroy()
{
	if (_profile_manager) {
		if (_profile_manager->profile_table) {
			g_free(_profile_manager->profile_table);
		}

		g_free(_profile_manager);
		_profile_manager = NULL;
	}
}

int connman_profile_manager_profile_cache(int count)
{
	int *p_num_of_profiles = 0;
	net_profile_info_t** p_profile_table;
	net_profile_info_t* old_profile_table;
	connman_profile_manager *profile_manager = NULL;

	INFO_LOG(COMMON_NAME_LIB, "connman_profile_manager_profile_cache");

	profile_manager = connman_profile_manager_get_singleton();
	old_profile_table = profile_manager->profile_table;
	profile_manager->profile_table = NULL;

	p_num_of_profiles = &(profile_manager->profile_num);
	p_profile_table = &(profile_manager->profile_table);

	net_get_profile_list(NET_DEVICE_WIFI, p_profile_table, p_num_of_profiles);
	g_free(old_profile_table);
	if (*p_num_of_profiles == 0) {
		INFO_LOG(COMMON_NAME_LIB, "count = 0");
	} else if (count < *p_num_of_profiles && count > 0) {
		*p_num_of_profiles = count;
	}

	INFO_LOG(COMMON_NAME_LIB, "count = %d", *p_num_of_profiles);

	return *p_num_of_profiles;
}

int connman_profile_manager_scanned_profile_table_size_get(void)
{
	connman_profile_manager *profile_manager = NULL;
	profile_manager = connman_profile_manager_get_singleton();
	return profile_manager->profile_num;
}

int connman_profile_manager_check_favourite(const char *profile_name, int *favourite)
{
	net_profile_info_t profile;

	int ret = net_get_profile_info(profile_name, &profile);
	if (ret != NET_ERR_NONE) {
		ERROR_LOG(COMMON_NAME_ERR, "Failed - net_get_profile_info : error[%d]", ret);
		return WLAN_MANAGER_ERR_UNKNOWN;
	}

	INFO_LOG(COMMON_NAME_LIB, "Favourite = %d", (int)profile.Favourite);

	*favourite = (int)profile.Favourite;
	return WLAN_MANAGER_ERR_NONE;
}

int connman_profile_manager_connected_ssid_set(const char *profile_name)
{
	INFO_LOG(COMMON_NAME_LIB, "Profile name : %s", profile_name);

	net_profile_info_t profile;
	if (net_get_profile_info(profile_name, &profile) != NET_ERR_NONE)
		return FALSE;

	wlan_manager_set_connected_AP((const net_profile_info_t *)&profile);

	return TRUE;
}

int connman_profile_manager_disconnected_ssid_set(const char *profile_name)
{
	INFO_LOG(COMMON_NAME_LIB, "Profile name : %s", profile_name);

	const char *connected_profile_name = wlan_manager_get_connected_profile();
	if (connected_profile_name)
		if (strncmp(connected_profile_name, profile_name, strlen(connected_profile_name)) == 0)
			wlan_manager_reset_connected_AP();

	return TRUE;
}

int connman_profile_manager_profile_modify(net_profile_info_t *new_profile)
{
	int ret = net_modify_profile(new_profile->ProfileName, new_profile);
	if (ret != NET_ERR_NONE) {
		INFO_LOG(COMMON_NAME_ERR, "Failed to modify profile - %d\n", ret);
		return 0;
	}
	else {
		INFO_LOG(COMMON_NAME_LIB, "Succeed to modify profile - %d\n", ret);
	}

	return 1;
}

int connman_profile_manager_profile_modify_auth(const char* profile_name, void *authdata, int secMode)
{
	if (profile_name == NULL || authdata == NULL)
		return 0;

	net_profile_info_t profile;
	if (net_get_profile_info(profile_name, &profile) != NET_ERR_NONE) {
		return 0;
	}

	wlan_manager_password_data* auth = (wlan_manager_password_data*)authdata;
	char security_key[NETPM_WLAN_MAX_PSK_PASSPHRASE_LEN+1] = "";
	
	if (auth != NULL) {
		strncpy(security_key, auth->password, NETPM_WLAN_MAX_PSK_PASSPHRASE_LEN);
		DEBUG_LOG(COMMON_NAME_LIB, "security_key [%s]", auth->password);
	}
	
	if (0 < strlen(security_key)) {
		if (secMode == WLAN_SEC_MODE_WEP) {
			INFO_LOG(COMMON_NAME_LIB, "SECURITY_TYPE_WEP");
			strncpy(profile.ProfileInfo.Wlan.security_info.authentication.wep.wepKey, 
					security_key,
					NETPM_WLAN_MAX_WEP_KEY_LEN);
		} else {
			INFO_LOG(COMMON_NAME_LIB, "not SECURITY_TYPE_WEP" );
			strncpy(profile.ProfileInfo.Wlan.security_info.authentication.psk.pskKey, security_key,
				NETPM_WLAN_MAX_PSK_PASSPHRASE_LEN);
			profile.ProfileInfo.Wlan.security_info.authentication.psk.pskKey[NETPM_WLAN_MAX_PSK_PASSPHRASE_LEN] = '\0';
		}
	} else {
		INFO_LOG(COMMON_NAME_LIB, "security data <= 0" );
		strncpy(profile.ProfileInfo.Wlan.security_info.authentication.psk.pskKey, security_key,
			NETPM_WLAN_MAX_PSK_PASSPHRASE_LEN);
	}

	int ret = net_modify_profile(profile_name, &profile);
	if (ret != NET_ERR_NONE) {
		INFO_LOG(COMMON_NAME_ERR, "Failed to modify profile - %d\n", ret);
		return 0;
	}
	else {
		INFO_LOG(COMMON_NAME_LIB, "Succeed to modify profile - %d\n", ret);
	}

	return 1;
}

void* connman_profile_manager_profile_table_get(void)
{
	connman_profile_manager *profile_manager = NULL;
	profile_manager = connman_profile_manager_get_singleton();
	return profile_manager->profile_table;
}

int connman_profile_manager_profile_info_get(const char *profile_name, net_profile_info_t *profile)
{
	if (profile_name == NULL)
		return 0;

	if (net_get_profile_info(profile_name, profile) != NET_ERR_NONE) {
		return 0;
	}

	return 1;
}
