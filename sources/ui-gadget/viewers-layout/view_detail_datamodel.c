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



#include "common.h"
#include "view_detail_datamodel.h"


struct view_detail_datamodel {
	net_ip_config_type_t ip_and_dns_type_initial;
	char* proxy_addr_initial;
	char* ip_initial;
	char* subnet_initial;
	char* gateway_initial;
	char* dns1_initial;
	char* dns2_initial;

	net_ip_config_type_t ip_and_dns_type_current;
	char* proxy_addr_current;
	char* ip_current;
	char* subnet_current;
	char* gateway_current;
	char* dns1_current;
	char* dns2_current;
};

static struct view_detail_datamodel *manager_object = NULL;

int view_detail_datamodel_create(const char *profile_name)
{
	__COMMON_FUNC_ENTER__;

	manager_object = g_malloc0(sizeof(struct view_detail_datamodel));
	assertm_if(NULL == manager_object, "NULL!!");

	net_profile_info_t profile_info;
	int ret = connman_profile_manager_profile_info_get(profile_name, &profile_info);
	if (ret == 0)
		return 0;

	manager_object->ip_and_dns_type_initial = profile_info.ProfileInfo.Wlan.net_info.IpConfigType;
	manager_object->ip_and_dns_type_current = profile_info.ProfileInfo.Wlan.net_info.IpConfigType;
	if (strlen(profile_info.ProfileInfo.Wlan.net_info.ProxyAddr) < 1) {
		manager_object->proxy_addr_initial = strdup("0.0.0.0:80");
		manager_object->proxy_addr_current = strdup("0.0.0.0:80");
	} else {
		manager_object->proxy_addr_initial = strdup(profile_info.ProfileInfo.Wlan.net_info.ProxyAddr);
		manager_object->proxy_addr_current = strdup(profile_info.ProfileInfo.Wlan.net_info.ProxyAddr);
	}

	manager_object->ip_initial = strdup(inet_ntoa(profile_info.ProfileInfo.Wlan.net_info.IpAddr.Data.Ipv4));
	manager_object->ip_current = strdup(inet_ntoa(profile_info.ProfileInfo.Wlan.net_info.IpAddr.Data.Ipv4));
	manager_object->subnet_initial = strdup(inet_ntoa(profile_info.ProfileInfo.Wlan.net_info.SubnetMask.Data.Ipv4));
	manager_object->subnet_current = strdup(inet_ntoa(profile_info.ProfileInfo.Wlan.net_info.SubnetMask.Data.Ipv4));
	manager_object->gateway_initial = strdup(inet_ntoa(profile_info.ProfileInfo.Wlan.net_info.GatewayAddr.Data.Ipv4));
	manager_object->gateway_current = strdup(inet_ntoa(profile_info.ProfileInfo.Wlan.net_info.GatewayAddr.Data.Ipv4));
	manager_object->dns1_initial = strdup(inet_ntoa(profile_info.ProfileInfo.Wlan.net_info.DnsAddr[0].Data.Ipv4));
	manager_object->dns1_current = strdup(inet_ntoa(profile_info.ProfileInfo.Wlan.net_info.DnsAddr[0].Data.Ipv4));
	manager_object->dns2_initial = strdup(inet_ntoa(profile_info.ProfileInfo.Wlan.net_info.DnsAddr[1].Data.Ipv4));
	manager_object->dns2_current = strdup(inet_ntoa(profile_info.ProfileInfo.Wlan.net_info.DnsAddr[1].Data.Ipv4));

	INFO_LOG(UG_NAME_NORMAL, "ip : %s", manager_object->ip_current);
	INFO_LOG(UG_NAME_NORMAL, "subnet : %s", manager_object->subnet_current);
	INFO_LOG(UG_NAME_NORMAL, "gateway : %s", manager_object->gateway_current);
	INFO_LOG(UG_NAME_NORMAL, "dns1 : %s", manager_object->dns1_current);
	INFO_LOG(UG_NAME_NORMAL, "dns2 : %s", manager_object->dns2_current);

	__COMMON_FUNC_EXIT__;

	return 1;
}

int view_detail_datamodel_destroy(void)
{
	__COMMON_FUNC_ENTER__;

	assertm_if(NULL == manager_object, "NULL!!");

	g_free(manager_object->proxy_addr_initial);
	g_free(manager_object->proxy_addr_current);
	g_free(manager_object->ip_initial);
	g_free(manager_object->ip_current);
	g_free(manager_object->subnet_initial);
	g_free(manager_object->subnet_current);
	g_free(manager_object->gateway_initial);
	g_free(manager_object->gateway_current);
	g_free(manager_object->dns1_initial);
	g_free(manager_object->dns1_current);
	g_free(manager_object->dns2_initial);
	g_free(manager_object->dns2_current);
	g_free(manager_object);

	manager_object = NULL;

	__COMMON_FUNC_EXIT__;

	return 1;
}

static int _zstrcmp(char* s, char* t)
{
	if(NULL == s && NULL == t){
		return 0;
	}
	if(NULL != s && NULL == t){
		return -1;
	}
	if(NULL == s && NULL != t){
		return 1;
	}
	return strcmp(s, t);
}

int view_detail_datamodel_determine_staticip_modified(void)
{
	if(0 != _zstrcmp(manager_object->ip_initial, 
				manager_object->ip_current)){
		return TRUE;
	}

	if(0 != _zstrcmp(manager_object->subnet_initial, 
				manager_object->subnet_current)){
		return TRUE;
	}

	if(0 != _zstrcmp(manager_object->gateway_initial, 
				manager_object->gateway_current)){
		return TRUE;
	}

	if(0 != _zstrcmp(manager_object->dns1_initial, 
				manager_object->dns1_current)){
		return TRUE;
	}

	if(0 != _zstrcmp(manager_object->dns2_initial, 
				manager_object->dns2_current)){
		return TRUE;
	}
	return FALSE;
}

void view_detail_datamodel_determine_modified(const char *profile_name)
{
	assertm_if(NULL == manager_object, "NULL!!");

	Eina_Bool changed = EINA_FALSE;
	net_profile_info_t profile;
	memset(&profile, 0, sizeof(net_profile_info_t));

	if (manager_object->ip_and_dns_type_initial != manager_object->ip_and_dns_type_current) {
		profile.ProfileInfo.Wlan.net_info.IpConfigType = manager_object->ip_and_dns_type_current;
		changed = EINA_TRUE;
	}

	if (_zstrcmp(manager_object->proxy_addr_initial, manager_object->proxy_addr_current) != 0) {
		strncpy(profile.ProfileInfo.Wlan.net_info.ProxyAddr, manager_object->proxy_addr_current, WLAN_PROXY_LEN_MAX);
		changed = EINA_TRUE;
	}

	if (manager_object->ip_and_dns_type_current == NET_IP_CONFIG_TYPE_STATIC) {
		if (_zstrcmp(manager_object->ip_initial, manager_object->ip_current) != 0) {
			inet_aton(manager_object->ip_current, &(profile.ProfileInfo.Wlan.net_info.IpAddr.Data.Ipv4));
			changed = EINA_TRUE;
		}

		if (_zstrcmp(manager_object->subnet_initial, manager_object->subnet_current) != 0) {
			inet_aton(manager_object->subnet_current, &(profile.ProfileInfo.Wlan.net_info.SubnetMask.Data.Ipv4));
			changed = EINA_TRUE;
		}

		if (_zstrcmp(manager_object->gateway_initial, manager_object->gateway_current) != 0) {
			inet_aton(manager_object->subnet_current, &(profile.ProfileInfo.Wlan.net_info.GatewayAddr.Data.Ipv4));
			changed = EINA_TRUE;
		}

		if (_zstrcmp(manager_object->dns1_initial, manager_object->dns1_current) != 0) {
			inet_aton(manager_object->subnet_current, &(profile.ProfileInfo.Wlan.net_info.DnsAddr[0].Data.Ipv4));
			changed = EINA_TRUE;
		}

		if (_zstrcmp(manager_object->dns2_initial, manager_object->dns2_current) != 0) {
			inet_aton(manager_object->subnet_current, &(profile.ProfileInfo.Wlan.net_info.DnsAddr[1].Data.Ipv4));
			changed = EINA_TRUE;
		}
	}

	if (changed) {
		profile.ProfileInfo.Wlan.net_info.IpConfigType = manager_object->ip_and_dns_type_current;
		strncpy(profile.ProfileName, profile_name, NET_PROFILE_NAME_LEN_MAX);
		wlan_manager_profile_modify_by_device_info(profile);
	}
}

int view_detail_datamodel_ip_and_dns_type_set(IP_TYPES type)
{
	assertm_if(NULL == manager_object, "NULL!!");
	manager_object->ip_and_dns_type_current = type;
	return TRUE;
}

IP_TYPES view_detail_datamodel_ip_and_dns_type_get(void)
{
	assertm_if(NULL == manager_object, "NULL!!");
	return manager_object->ip_and_dns_type_current;
}

int view_detail_datamodel_proxy_address_set(const char* proxy)
{
	assertm_if(NULL == manager_object, "NULL!!");
	g_free(manager_object->proxy_addr_current);
	if(NULL != proxy) {
		manager_object->proxy_addr_current = strdup(proxy);
	}
	return TRUE;
}

const char* view_detail_datamodel_proxy_address_get(void)
{
	assertm_if(NULL == manager_object, "NULL!!");
	const char* ret = strdup(manager_object->proxy_addr_current);
	return ret;
}

int view_detail_datamodel_static_ip_address_set(const char* addr)
{
	assertm_if(NULL == manager_object, "NULL!!");
	g_free(manager_object->ip_current);
	manager_object->ip_current=NULL;
	if(NULL != addr) {
		DEBUG_LOG(UG_NAME_NORMAL, "* set as [%s]", addr);
		manager_object->ip_current = strdup(addr);
	}
	return TRUE;
}

int view_detail_datamodel_static_gateway_address_set(const char* addr)
{
	assertm_if(NULL == manager_object, "NULL!!");
	g_free(manager_object->gateway_current);
	manager_object->gateway_current = NULL;
	if(NULL != addr) {
		manager_object->gateway_current = strdup(addr);
	}
	return TRUE;
}

int view_detail_datamodel_static_subnet_mask_set(const char* addr)
{
	assertm_if(NULL == manager_object, "NULL!!");
	g_free(manager_object->subnet_current);
	manager_object->subnet_current = NULL;
	if(NULL != addr) {
		manager_object->subnet_current = strdup(addr);
	}
	return TRUE;
}

int view_detail_datamodel_static_dns1_address_set(const char* addr)
{
	assertm_if(NULL == manager_object, "NULL!!");
	g_free(manager_object->dns1_current);
	manager_object->dns1_current = NULL;
	if(NULL != addr) {
		manager_object->dns1_current = strdup(addr);
	}
	return TRUE;
}

int view_detail_datamodel_static_dns2_address_set(const char* addr)
{
	assertm_if(NULL == manager_object, "NULL!!");
	g_free(manager_object->dns2_current);
	manager_object->dns2_current = NULL;
	if (NULL != addr) {
		manager_object->dns2_current = strdup(addr);
	}
	return TRUE;
}

char *view_detail_datamodel_static_ip_address_get(void)
{
	assertm_if(NULL == manager_object, "NULL!!");

	return strdup(manager_object->ip_current);
}

char *view_detail_datamodel_static_gateway_address_get(void)
{
	assertm_if(NULL == manager_object, "NULL!!");
	if (NULL == manager_object->gateway_current) {
		return NULL;
	} else {
		return strdup(manager_object->gateway_current);
	}
}

char *view_detail_datamodel_static_subnet_mask_get(void)
{
	assertm_if(NULL == manager_object, "NULL!!");
	if (NULL == manager_object->subnet_current) {
		return NULL;
	} else {
		return strdup(manager_object->subnet_current);
	}
}

char *view_detail_datamodel_static_dns1_address_get(void)
{
	assertm_if(NULL == manager_object, "NULL!!");
	if (NULL == manager_object->dns1_current) {
		return NULL;
	} else {
		return strdup(manager_object->dns1_current);
	}
}

char *view_detail_datamodel_static_dns2_address_get(void)
{
	assertm_if(NULL == manager_object, "NULL!!");
	if (NULL == manager_object->dns2_current) {
		return NULL;
	} else {
		return strdup(manager_object->dns2_current);
	}
}
