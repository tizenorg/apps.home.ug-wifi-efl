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
#include "common_datamodel.h"

#define DEFAULT_PROXY_ADDR		"0.0.0.0:80"

struct view_datamodel_basic_info {
	char *profile_name;							/* Profile name */
	char *ap_name;								/* AP name */
	wlan_security_mode_type_t security_mode;		/* Security mode */
	unsigned int signal_strength;					/* Signal strength */
	unsigned int channel_freq;					/* channel frequency */
	boolean is_favourite;								/* Favourite */
	char wps_support;
};

struct view_datamodel_ip_info {
	char *profile_name;							/* Profile name */
	net_ip_config_type_t ip_and_dns_type;
	char* proxy_addr;
	char* ip;
	char* subnet;
	char* gateway;
	char* dns1;
	char* dns2;

	char* MAC_address;							/* MAC address */
};

struct view_datamodel_eap_info {
	char *profile_name;							/* Profile name */
	char *ap_name;							/* AP name */
	wlan_eap_type_t eap_method;							/* EAP method */
	int eap_provision;									/* Provisioning */
	wlan_eap_auth_type_t auth_type;						/* EAP phase 2 Authentication */
	char *user_id;										/* User id */
	char *anonymous_id;							/* Anonymous id */
	char *ca_cert;								/* CA certificate */
	char *user_cert;								/* User certificate */
	char *pswd;
};

static void view_detail_datamodel_init_default_profile(net_profile_info_t *profile_info)
{
	if (profile_info) {
		wlan_eap_info_t *eap_sec_info = &(profile_info->ProfileInfo.Wlan.security_info.authentication.eap);
		eap_sec_info->eap_type = WLAN_SEC_EAP_TYPE_PEAP;
		eap_sec_info->eap_auth = WLAN_SEC_EAP_AUTH_NONE;
	}
	return;
}

view_datamodel_basic_info_t *view_basic_detail_datamodel_create(const char *profile_name)
{
	__COMMON_FUNC_ENTER__;
	view_datamodel_basic_info_t *data_object = NULL;
	net_profile_info_t *profile_info = g_malloc0(sizeof(net_profile_info_t));
	assertm_if(NULL == profile_name, "NULL!!");
	view_detail_datamodel_init_default_profile(profile_info);
	if (!connman_profile_manager_profile_info_get(profile_name, profile_info)) {
		ERROR_LOG(UG_NAME_ERR, "Fatal Could not get the profile info!!!");
		g_free(profile_info);
		return NULL;
	}

	data_object = g_malloc0(sizeof(view_datamodel_basic_info_t));
	assertm_if(NULL == data_object, "NULL!!");

	data_object->profile_name = g_strdup(profile_name);
	data_object->channel_freq = profile_info->ProfileInfo.Wlan.frequency;
	data_object->ap_name = g_strdup(profile_info->ProfileInfo.Wlan.essid);
	data_object->signal_strength = profile_info->ProfileInfo.Wlan.Strength;
	data_object->security_mode = profile_info->ProfileInfo.Wlan.security_info.sec_mode;
	data_object->is_favourite = profile_info->Favourite;
	data_object->wps_support = profile_info->ProfileInfo.Wlan.security_info.wps_support;

	INFO_LOG(UG_NAME_NORMAL, "Profile name : %s", data_object->profile_name);
	INFO_LOG(UG_NAME_NORMAL, "ap name : %s", data_object->ap_name);
	INFO_LOG(UG_NAME_NORMAL, "Signal strength : %u", data_object->signal_strength);
	INFO_LOG(UG_NAME_NORMAL, "Security mode : %u", data_object->security_mode);
	INFO_LOG(UG_NAME_NORMAL, "Channel Freq : %u", data_object->channel_freq);
	INFO_LOG(UG_NAME_NORMAL, "WPS Support : %d", data_object->wps_support);

	g_free(profile_info);
	profile_info = NULL;

	__COMMON_FUNC_EXIT__;

	return data_object;
}

view_datamodel_ip_info_t *view_detail_datamodel_ip_info_create(const char *profile_name)
{
	__COMMON_FUNC_ENTER__;
	view_datamodel_ip_info_t *data_object = NULL;
	net_profile_info_t *profile_info = g_malloc0(sizeof(net_profile_info_t));
	assertm_if(NULL == profile_name, "NULL!!");
	view_detail_datamodel_init_default_profile(profile_info);
	if (!connman_profile_manager_profile_info_get(profile_name, profile_info)) {
		ERROR_LOG(UG_NAME_ERR, "Fatal Could not get the profile info!!!");
		/* Lets continue and create a default data object */
	}

	data_object = g_malloc0(sizeof(view_datamodel_ip_info_t));
	assertm_if(NULL == data_object, "NULL!!");

	data_object->profile_name = g_strdup(profile_name);
	data_object->ip_and_dns_type = profile_info->ProfileInfo.Wlan.net_info.IpConfigType;
	if (strlen(profile_info->ProfileInfo.Wlan.net_info.ProxyAddr) <= 0) {
		data_object->proxy_addr = g_strdup(DEFAULT_PROXY_ADDR);
	} else {
		data_object->proxy_addr = g_strdup(profile_info->ProfileInfo.Wlan.net_info.ProxyAddr);
	}

	data_object->ip = g_strdup(inet_ntoa(profile_info->ProfileInfo.Wlan.net_info.IpAddr.Data.Ipv4));
	data_object->subnet = g_strdup(inet_ntoa(profile_info->ProfileInfo.Wlan.net_info.SubnetMask.Data.Ipv4));
	data_object->gateway = g_strdup(inet_ntoa(profile_info->ProfileInfo.Wlan.net_info.GatewayAddr.Data.Ipv4));
	data_object->dns1 = g_strdup(inet_ntoa(profile_info->ProfileInfo.Wlan.net_info.DnsAddr[0].Data.Ipv4));
	data_object->dns2 = g_strdup(inet_ntoa(profile_info->ProfileInfo.Wlan.net_info.DnsAddr[1].Data.Ipv4));
	data_object->MAC_address = g_strdup(profile_info->ProfileInfo.Wlan.bssid);

	INFO_LOG(UG_NAME_NORMAL, "Profile name : %s", data_object->profile_name);
	INFO_LOG(UG_NAME_NORMAL, "ip_and_dns_type : %d", data_object->ip_and_dns_type);
	INFO_LOG(UG_NAME_NORMAL, "ip : %s", data_object->ip);
	INFO_LOG(UG_NAME_NORMAL, "subnet : %s", data_object->subnet);
	INFO_LOG(UG_NAME_NORMAL, "gateway : %s", data_object->gateway);
	INFO_LOG(UG_NAME_NORMAL, "dns1 : %s", data_object->dns1);
	INFO_LOG(UG_NAME_NORMAL, "dns2 : %s", data_object->dns2);
	INFO_LOG(UG_NAME_NORMAL, "proxy addr : %s", data_object->proxy_addr);
	INFO_LOG(UG_NAME_NORMAL, "MAC addr : %s", data_object->MAC_address);

	g_free(profile_info);
	profile_info = NULL;

	__COMMON_FUNC_EXIT__;

	return data_object;
}

view_datamodel_eap_info_t *view_detail_datamodel_eap_info_create(const char *profile_name)
{
	__COMMON_FUNC_ENTER__;
	view_datamodel_eap_info_t *data_object = NULL;
	net_profile_info_t *profile_info = g_malloc0(sizeof(net_profile_info_t));
	assertm_if(NULL == profile_name, "NULL!!");
	view_detail_datamodel_init_default_profile(profile_info);
	if (!connman_profile_manager_profile_info_get(profile_name, profile_info)) {
		ERROR_LOG(UG_NAME_ERR, "Fatal Could not get the profile info!!!");
		/* Lets continue and create a default data object */
	}

	/* The EAP info data object would be created only if the device is remembered / connected */
	data_object = g_malloc0(sizeof(view_datamodel_eap_info_t));
	assertm_if(NULL == data_object, "NULL!!");

	data_object->profile_name = g_strdup(profile_name);
	data_object->ap_name = g_strdup(profile_info->ProfileInfo.Wlan.essid);

	/* If the device is connected / remembered only then create and fill the eap_details structure */
	wlan_eap_info_t *eap_sec_info = &(profile_info->ProfileInfo.Wlan.security_info.authentication.eap);
	data_object->eap_method = eap_sec_info->eap_type;
	data_object->eap_provision = 0;	/* This is not yet supported by libnet. So setting it to 0. */
	data_object->auth_type = eap_sec_info->eap_auth;

	if (strlen(eap_sec_info->username))
		data_object->user_id = g_strdup(eap_sec_info->username);

	data_object->anonymous_id = NULL; /* This is not yet supported by libnet. So setting it to NULL. */

	if (strlen(eap_sec_info->password))
		data_object->pswd = g_strdup(eap_sec_info->password);

	if (strlen(eap_sec_info->ca_cert_filename))
		data_object->ca_cert = g_strdup(eap_sec_info->ca_cert_filename);

	if (strlen(eap_sec_info->client_cert_filename))
		data_object->user_cert = g_strdup(eap_sec_info->client_cert_filename);

	INFO_LOG(UG_NAME_NORMAL, "Profile name : %s", data_object->profile_name);
	INFO_LOG(UG_NAME_NORMAL, "EAP method : %u", data_object->eap_method);
	INFO_LOG(UG_NAME_NORMAL, "Provisioning : %u", data_object->eap_provision);
	INFO_LOG(UG_NAME_NORMAL, "Auth type : %u", data_object->auth_type);
	INFO_LOG(UG_NAME_NORMAL, "Id : %s", data_object->user_id);
	INFO_LOG(UG_NAME_NORMAL, "Anonymous id : %s", data_object->anonymous_id);
	INFO_LOG(UG_NAME_NORMAL, "Password : %s", data_object->pswd	);
	INFO_LOG(UG_NAME_NORMAL, "CA certificate : %s", data_object->ca_cert);
	INFO_LOG(UG_NAME_NORMAL, "User certificate : %s", data_object->user_cert);

	g_free(profile_info);
	profile_info = NULL;

	__COMMON_FUNC_EXIT__;

	return data_object;
}

void view_basic_detail_datamodel_destroy(view_datamodel_basic_info_t *data_object)
{
	__COMMON_FUNC_ENTER__;

	assertm_if(NULL == data_object, "NULL!!");

	g_free(data_object->profile_name);
	g_free(data_object->ap_name);
	g_free(data_object);
	data_object = NULL;

	__COMMON_FUNC_EXIT__;

	return;
}

void view_detail_datamodel_ip_info_destroy(view_datamodel_ip_info_t *data_object)
{
	__COMMON_FUNC_ENTER__;

	assertm_if(NULL == data_object, "NULL!!");

	g_free(data_object->profile_name);
	g_free(data_object->proxy_addr);
	g_free(data_object->ip);
	g_free(data_object->subnet);
	g_free(data_object->gateway);
	g_free(data_object->dns1);
	g_free(data_object->dns2);
	g_free(data_object->MAC_address);
	g_free(data_object);
	data_object = NULL;

	__COMMON_FUNC_EXIT__;

	return;
}

void view_detail_datamodel_eap_info_destroy(view_datamodel_eap_info_t *data_object)
{
	__COMMON_FUNC_ENTER__;

	assertm_if(NULL == data_object, "NULL!!");

	g_free(data_object->profile_name);
	g_free(data_object->ap_name);
	g_free(data_object->user_id);
	g_free(data_object->anonymous_id);
	g_free(data_object->pswd);
	g_free(data_object->ca_cert);
	g_free(data_object->user_cert);
	g_free(data_object);
	data_object = NULL;

	__COMMON_FUNC_EXIT__;

	return;
}

boolean view_detail_datamodel_save_ip_info_if_modified(const view_datamodel_ip_info_t *data_object)
{
	assertm_if(NULL == data_object, "NULL!!");

	boolean changed = FALSE;
	net_profile_info_t *profile_info = g_malloc0(sizeof(net_profile_info_t));
	assertm_if(NULL == profile_info, "NULL!!");
	view_detail_datamodel_init_default_profile(profile_info);
	if (!connman_profile_manager_profile_info_get(data_object->profile_name, profile_info)) {
		ERROR_LOG(UG_NAME_ERR, "Fatal Could not get the profile info!!!");
		g_free(profile_info);
		return FALSE;
	}

	if (data_object->ip_and_dns_type != profile_info->ProfileInfo.Wlan.net_info.IpConfigType) {
		profile_info->ProfileInfo.Wlan.net_info.IpConfigType = data_object->ip_and_dns_type;
		changed = TRUE;
	}

	if (!g_strcmp0(data_object->proxy_addr, DEFAULT_PROXY_ADDR)) {	/* Has user entered default proxy "0.0.0.0:80"*/
		if (g_strcmp0(profile_info->ProfileInfo.Wlan.net_info.ProxyAddr, "")) { /* If user entered defaukt proxy, then check if the profile proxy is zero string ("") */
			g_strlcpy(profile_info->ProfileInfo.Wlan.net_info.ProxyAddr, "", WLAN_PROXY_LEN_MAX);
			changed = TRUE;
		}
	} else if (g_strcmp0(data_object->proxy_addr, profile_info->ProfileInfo.Wlan.net_info.ProxyAddr)) {
		g_strlcpy(profile_info->ProfileInfo.Wlan.net_info.ProxyAddr, data_object->proxy_addr, WLAN_PROXY_LEN_MAX);
		changed = TRUE;
	}

	if (data_object->ip_and_dns_type == NET_IP_CONFIG_TYPE_STATIC) {
		char *temp_str = inet_ntoa(profile_info->ProfileInfo.Wlan.net_info.IpAddr.Data.Ipv4);
		if (g_strcmp0(data_object->ip, temp_str)) {
			inet_aton(data_object->ip, &(profile_info->ProfileInfo.Wlan.net_info.IpAddr.Data.Ipv4));
			char *temp_str = inet_ntoa(profile_info->ProfileInfo.Wlan.net_info.IpAddr.Data.Ipv4);
			INFO_LOG(UG_NAME_NORMAL, "IP : %s", temp_str);
			changed = TRUE;
		}
		temp_str = inet_ntoa(profile_info->ProfileInfo.Wlan.net_info.SubnetMask.Data.Ipv4);
		if (g_strcmp0(data_object->subnet, temp_str)) {
			inet_aton(data_object->subnet, &(profile_info->ProfileInfo.Wlan.net_info.SubnetMask.Data.Ipv4));
			temp_str = inet_ntoa(profile_info->ProfileInfo.Wlan.net_info.SubnetMask.Data.Ipv4);
			INFO_LOG(UG_NAME_NORMAL, "Subnet : %s", temp_str);
			changed = TRUE;
		}

		temp_str = inet_ntoa(profile_info->ProfileInfo.Wlan.net_info.GatewayAddr.Data.Ipv4);
		if (g_strcmp0(data_object->gateway, temp_str)) {
			inet_aton(data_object->gateway, &(profile_info->ProfileInfo.Wlan.net_info.GatewayAddr.Data.Ipv4));
			temp_str = inet_ntoa(profile_info->ProfileInfo.Wlan.net_info.GatewayAddr.Data.Ipv4);
			INFO_LOG(UG_NAME_NORMAL, "Gateway : %s", temp_str);
			changed = TRUE;
		}

		temp_str = inet_ntoa(profile_info->ProfileInfo.Wlan.net_info.DnsAddr[0].Data.Ipv4);
		if (g_strcmp0(data_object->dns1, temp_str)) {
			inet_aton(data_object->dns1, &(profile_info->ProfileInfo.Wlan.net_info.DnsAddr[0].Data.Ipv4));
			profile_info->ProfileInfo.Wlan.net_info.DnsCount = 1;
			temp_str = inet_ntoa(profile_info->ProfileInfo.Wlan.net_info.DnsAddr[0].Data.Ipv4);
			INFO_LOG(UG_NAME_NORMAL, "DNS1 : %s", temp_str);
			changed = TRUE;
		}

		temp_str = inet_ntoa(profile_info->ProfileInfo.Wlan.net_info.DnsAddr[1].Data.Ipv4);
		if (g_strcmp0(data_object->dns2, temp_str)) {
			inet_aton(data_object->dns2, &(profile_info->ProfileInfo.Wlan.net_info.DnsAddr[1].Data.Ipv4));
			profile_info->ProfileInfo.Wlan.net_info.DnsCount = 2;
			temp_str = inet_ntoa(profile_info->ProfileInfo.Wlan.net_info.DnsAddr[1].Data.Ipv4);
			INFO_LOG(UG_NAME_NORMAL, "DNS2 : %s", temp_str);
			changed = TRUE;
		}
		INFO_LOG(UG_NAME_NORMAL, "DNS count : %d", profile_info->ProfileInfo.Wlan.net_info.DnsCount);
	}
	if (changed) {
		wlan_manager_profile_modify_by_device_info(profile_info);
	}

	g_free(profile_info);

	return changed;
}

boolean view_detail_datamodel_save_eap_info_if_modified(const view_datamodel_eap_info_t *data_object)
{
	assertm_if(NULL == data_object, "NULL!!");

	boolean changed = FALSE;
	net_profile_info_t *profile_info = g_malloc0(sizeof(net_profile_info_t));
	assertm_if(NULL == profile_info, "NULL!!");
	view_detail_datamodel_init_default_profile(profile_info);
	if (!connman_profile_manager_profile_info_get(data_object->profile_name, profile_info)) {
		ERROR_LOG(UG_NAME_ERR, "Fatal Could not get the profile info!!!");
		g_free(profile_info);
		return FALSE;
	}

	wlan_eap_info_t *eap_sec_info = &(profile_info->ProfileInfo.Wlan.security_info.authentication.eap);
	if (data_object->eap_method != eap_sec_info->eap_type) {
		eap_sec_info->eap_type = data_object->eap_method;
		changed = TRUE;
	}

	if (data_object->auth_type != eap_sec_info->eap_auth) {
		eap_sec_info->eap_auth = data_object->auth_type;
		changed = TRUE;
	}

	if (g_strcmp0(data_object->user_id, eap_sec_info->username)) {
		g_strlcpy(eap_sec_info->username, data_object->user_id, NETPM_WLAN_USERNAME_LEN);
		changed = TRUE;
	}

	if (g_strcmp0(data_object->pswd, eap_sec_info->password)) {
		g_strlcpy(eap_sec_info->password, data_object->pswd, NETPM_WLAN_PASSWORD_LEN);
		changed = TRUE;
	}

	if (changed) {
		INFO_LOG(UG_NAME_NORMAL, "EAP type : %d", eap_sec_info->eap_type);
		INFO_LOG(UG_NAME_NORMAL, "EAP auth : %d", eap_sec_info->eap_auth);
		INFO_LOG(UG_NAME_NORMAL, "User name : %s", eap_sec_info->username);
		INFO_LOG(UG_NAME_NORMAL, "Password : %s", eap_sec_info->password);

		wlan_manager_profile_modify_by_device_info(profile_info);
	}

	g_free(profile_info);

	return changed;
}

int view_detail_datamodel_ip_and_dns_type_set(view_datamodel_ip_info_t *data_object, const IP_TYPES type)
{
	assertm_if(NULL == data_object, "NULL!!");
	data_object->ip_and_dns_type = type;
	return TRUE;
}

int view_detail_datamodel_proxy_address_set(view_datamodel_ip_info_t *data_object, const char* proxy)
{
	assertm_if(NULL == data_object, "NULL!!");
	if(NULL != proxy) {
		g_free(data_object->proxy_addr);
		data_object->proxy_addr = g_strdup(proxy);
	}
	return TRUE;
}

int view_detail_datamodel_static_ip_address_set(view_datamodel_ip_info_t *data_object, const char* addr)
{
	assertm_if(NULL == data_object, "NULL!!");
	if(NULL != addr) {
		DEBUG_LOG(UG_NAME_NORMAL, "* set as [%s]", addr);
		g_free(data_object->ip);
		data_object->ip = g_strdup(addr);
	}
	return TRUE;
}

int view_detail_datamodel_static_gateway_address_set(view_datamodel_ip_info_t *data_object, const char* addr)
{
	assertm_if(NULL == data_object, "NULL!!");
	if(NULL != addr) {
		g_free(data_object->gateway);
		data_object->gateway = g_strdup(addr);
	}
	return TRUE;
}

int view_detail_datamodel_static_subnet_mask_set(view_datamodel_ip_info_t *data_object, const char* addr)
{
	assertm_if(NULL == data_object, "NULL!!");
	if(NULL != addr) {
		g_free(data_object->subnet);
		data_object->subnet = g_strdup(addr);
	}
	return TRUE;
}

int view_detail_datamodel_static_dns1_address_set(view_datamodel_ip_info_t *data_object, const char* addr)
{
	assertm_if(NULL == data_object, "NULL!!");
	if(NULL != addr) {
		g_free(data_object->dns1);
		data_object->dns1 = g_strdup(addr);
	}
	return TRUE;
}

int view_detail_datamodel_static_dns2_address_set(view_datamodel_ip_info_t *data_object, const char* addr)
{
	assertm_if(NULL == data_object, "NULL!!");
	if (NULL != addr) {
		g_free(data_object->dns2);
		data_object->dns2 = g_strdup(addr);
	}
	return TRUE;
}

int view_detail_datamodel_eap_ap_name_set(view_datamodel_eap_info_t *data_object, const char *ssid)
{
	assertm_if(NULL == data_object, "NULL!!");
	g_free(data_object->ap_name);
	data_object->ap_name = g_strdup(ssid);
	return TRUE;
}

int view_detail_datamodel_eap_method_set(view_datamodel_eap_info_t *data_object, const wlan_eap_type_t eap_method)
{
	assertm_if(NULL == data_object, "NULL!!");
	data_object->eap_method = eap_method;
	return TRUE;
}

int view_detail_datamodel_eap_provision_set(view_datamodel_eap_info_t *data_object, const int provision)
{
	assertm_if(NULL == data_object, "NULL!!");
	data_object->eap_provision = provision;
	return TRUE;
}

int view_detail_datamodel_eap_auth_set(view_datamodel_eap_info_t *data_object, const wlan_eap_auth_type_t auth_type)
{
	assertm_if(NULL == data_object, "NULL!!");
	data_object->auth_type = auth_type;
	return TRUE;
}

int view_detail_datamodel_eap_user_id_set(view_datamodel_eap_info_t *data_object, const char* user_id)
{
	assertm_if(NULL == data_object, "NULL!!");
	g_free(data_object->user_id);
	data_object->user_id = g_strdup(user_id);
	return TRUE;
}

int view_detail_datamodel_eap_anonymous_id_set(view_datamodel_eap_info_t *data_object, const char* anonymous_id)
{
	assertm_if(NULL == data_object, "NULL!!");
	g_free(data_object->anonymous_id);
	data_object->anonymous_id = g_strdup(anonymous_id);
	return TRUE;
}

int view_detail_datamodel_eap_pswd_set(view_datamodel_eap_info_t *data_object, const char* pswd)
{
	assertm_if(NULL == data_object, "NULL!!");
	g_free(data_object->pswd);
	data_object->pswd = g_strdup(pswd);
	return TRUE;
}

int view_detail_datamodel_eap_ca_cert_set(view_datamodel_eap_info_t *data_object, const char* ca_cert)
{
	assertm_if(NULL == data_object, "NULL!!");
	g_free(data_object->ca_cert);
	data_object->ca_cert = g_strdup(ca_cert);
	return TRUE;
}

int view_detail_datamodel_eap_user_cert_set(view_datamodel_eap_info_t *data_object, const char* user_cert)
{
	assertm_if(NULL == data_object, "NULL!!");
	g_free(data_object->user_cert);
	data_object->user_cert = g_strdup(user_cert);
	return TRUE;
}

IP_TYPES view_detail_datamodel_ip_and_dns_type_get(view_datamodel_ip_info_t *data_object)
{
	assertm_if(NULL == data_object, "NULL!!");
	return data_object->ip_and_dns_type;
}

char *view_detail_datamodel_static_ip_address_get(view_datamodel_ip_info_t *data_object)
{
	assertm_if(NULL == data_object, "NULL!!");
	return g_strdup(data_object->ip);
}

char *view_detail_datamodel_static_gateway_address_get(view_datamodel_ip_info_t *data_object)
{
	assertm_if(NULL == data_object, "NULL!!");
	return g_strdup(data_object->gateway);
}

char *view_detail_datamodel_static_subnet_mask_get(view_datamodel_ip_info_t *data_object)
{
	assertm_if(NULL == data_object, "NULL!!");
	return g_strdup(data_object->subnet);
}

char *view_detail_datamodel_static_dns1_address_get(view_datamodel_ip_info_t *data_object)
{
	assertm_if(NULL == data_object, "NULL!!");
	return g_strdup(data_object->dns1);
}

char *view_detail_datamodel_static_dns2_address_get(view_datamodel_ip_info_t *data_object)
{
	assertm_if(NULL == data_object, "NULL!!");
	return g_strdup(data_object->dns2);
}

char* view_detail_datamodel_proxy_address_get(view_datamodel_ip_info_t *data_object)
{
	assertm_if(NULL == data_object, "NULL!!");
	char* ret = g_strdup(data_object->proxy_addr);
	return ret;
}

char* view_detail_datamodel_MAC_addr_get(view_datamodel_ip_info_t *data_object)
{
	assertm_if(NULL == data_object, "NULL!!");
	char* ret = g_strdup(data_object->MAC_address);
	return ret;
}
#if 0
int view_detail_datamodel_channel_freq_get(view_datamodel_ip_info_t *data_object)
{
	view_detail_datamodel_t *data_object = (view_detail_datamodel_t *)mvc_object;
	assertm_if(NULL == data_object, "NULL!!");
	switch (data_object->channel_freq) {
	default:
	case 2412:
		return 1;
	case 2417:
		return 2;
	case 2422:
		return 3;
	case 2427:
		return 4;
	case 2432:
		return 5;
	case 2437:
		return 6;
	case 2442:
		return 7;
	case 2447:
		return 8;
	case 2452:
		return 9;
	case 2457:
		return 10;
	case 2462:
		return 11;
	}
	return 0;
}
#endif

wlan_eap_type_t view_detail_datamodel_eap_method_get(view_datamodel_eap_info_t *data_object)
{
	assertm_if(NULL == data_object, "NULL!!");
	return data_object->eap_method;
}

int view_detail_datamodel_eap_provision_get(view_datamodel_eap_info_t *data_object)
{
	assertm_if(NULL == data_object, "NULL!!");
	return data_object->eap_provision;
}

wlan_eap_auth_type_t view_detail_datamodel_eap_auth_get(view_datamodel_eap_info_t *data_object)
{
	assertm_if(NULL == data_object, "NULL!!");
	return data_object->auth_type;
}

char *view_detail_datamodel_user_id_get(view_datamodel_eap_info_t *data_object)
{
	assertm_if(NULL == data_object, "NULL!!");
	return g_strdup(data_object->user_id);
}

char *view_detail_datamodel_anonymous_id_get(view_datamodel_eap_info_t *data_object)
{
	assertm_if(NULL == data_object, "NULL!!");
	return g_strdup(data_object->anonymous_id);
}

char *view_detail_datamodel_pswd_get(view_datamodel_eap_info_t *data_object)
{
	assertm_if(NULL == data_object, "NULL!!");
	return g_strdup(data_object->pswd);
}

char *view_detail_datamodel_ca_cert_get(view_datamodel_eap_info_t *data_object)
{
	assertm_if(NULL == data_object, "NULL!!");
	return g_strdup(data_object->ca_cert);
}

char *view_detail_datamodel_user_cert_get(view_datamodel_eap_info_t *data_object)
{
	assertm_if(NULL == data_object, "NULL!!");
	return g_strdup(data_object->user_cert);
}

char *view_detail_datamodel_eap_ap_name_get(view_datamodel_eap_info_t *data_object)
{
	assertm_if(NULL == data_object, "NULL!!");
	return g_strdup(data_object->ap_name);
}

char *view_detail_datamodel_basic_info_profile_name_get(view_datamodel_basic_info_t *data_object)
{
	assertm_if(NULL == data_object, "NULL!!");
	return g_strdup(data_object->profile_name);
}

char *view_detail_datamodel_ap_name_get(view_datamodel_basic_info_t *data_object)
{
	assertm_if(NULL == data_object, "NULL!!");
	return g_strdup(data_object->ap_name);
}

unsigned int view_detail_datamodel_sig_strength_get(view_datamodel_basic_info_t *data_object)
{
	assertm_if(NULL == data_object, "NULL!!");
	return data_object->signal_strength;
}

unsigned int view_detail_datamodel_sec_mode_get(view_datamodel_basic_info_t *data_object)
{
	assertm_if(NULL == data_object, "NULL!!");
	return data_object->security_mode;
}

char view_detail_datamodel_wps_support_get(view_datamodel_basic_info_t *data_object)
{
	assertm_if(NULL == data_object, "NULL!!");
	return data_object->wps_support;
}

boolean view_detail_datamodel_is_favourite_get(view_datamodel_basic_info_t *data_object)
{
	assertm_if(NULL == data_object, "NULL!!");
	return data_object->is_favourite;
}
