/*
 * Wi-Fi
 *
 * Copyright 2012 Samsung Electronics Co., Ltd
 *
 * Licensed under the Flora License, Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.tizenopensource.org/license
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <wifi.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib-lowlevel.h>

#include "common.h"
#include "common_invalid_password.h"

#define SUPPLICANT_INTERFACE				"fi.w1.wpa_supplicant1"
#define SUPPLICANT_INTERFACE_SIGNAL_FILTER \
			"type='signal',interface='fi.w1.wpa_supplicant1.Interface'"

static gboolean invalid_key = FALSE;
static DBusConnection *connection = NULL;

static void __common_pop_invalid_password(void)
{
	invalid_key = TRUE;

	ERROR_LOG(UG_NAME_NORMAL, "Invalid password");
}

static DBusHandlerResult __common_check_invalid_password(
		DBusConnection* connection, DBusMessage* message, void* user_data)
{
	DBusMessageIter iter, dict, entry, value;
	const char *key;
	const char *state;
	static char old_state[30] = { 0, };

	if (dbus_message_is_signal(message,
			SUPPLICANT_INTERFACE ".Interface", "PropertiesChanged") != TRUE)
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	if (dbus_message_iter_init(message, &iter) == FALSE)
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_ARRAY)
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	dbus_message_iter_recurse(&iter, &dict);
	while (dbus_message_iter_get_arg_type(&dict) == DBUS_TYPE_DICT_ENTRY) {
		dbus_message_iter_recurse(&dict, &entry);
		if (dbus_message_iter_get_arg_type(&entry) != DBUS_TYPE_STRING)
			return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

		dbus_message_iter_get_basic(&entry, &key);
		dbus_message_iter_next(&entry);

		if (dbus_message_iter_get_arg_type(&entry) != DBUS_TYPE_VARIANT)
			return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

		dbus_message_iter_recurse(&entry, &value);

		if (g_strcmp0(key, "State") == 0) {
			dbus_message_iter_get_basic(&value, &state);
			if (state == NULL)
				return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

			if (g_strcmp0(old_state, "4way_handshake") == 0 &&
					g_strcmp0(state, "disconnected") == 0)
				__common_pop_invalid_password();

			g_strlcpy(old_state, state, 30);
		}

		dbus_message_iter_next(&dict);
	}

	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

int _common_register_invalid_password_popup(void)
{
	DBusError error;

	invalid_key = FALSE;

	if (connection != NULL)
		return WIFI_ERROR_NONE;

	connection = dbus_bus_get(DBUS_BUS_SYSTEM, NULL);
	if (connection == NULL)
		return WIFI_ERROR_OUT_OF_MEMORY;

	dbus_connection_setup_with_g_main(connection, NULL);

	dbus_error_init(&error);
	dbus_bus_add_match(connection, SUPPLICANT_INTERFACE_SIGNAL_FILTER, &error);
	dbus_connection_flush(connection);
	if (dbus_error_is_set(&error) == TRUE) {
		dbus_error_free(&error);

		dbus_connection_unref(connection);
		connection = NULL;

		return WIFI_ERROR_OUT_OF_MEMORY;
	}

	if (dbus_connection_add_filter(connection,
			__common_check_invalid_password, NULL, NULL) == FALSE) {
		dbus_connection_unref(connection);
		connection = NULL;

		return WIFI_ERROR_OUT_OF_MEMORY;
	}

	return WIFI_ERROR_NONE;
}

int _common_deregister_invalid_password_popup(void)
{
	if (connection == NULL)
		return WIFI_ERROR_NONE;

	dbus_bus_remove_match(connection, SUPPLICANT_INTERFACE_SIGNAL_FILTER, NULL);
	dbus_connection_flush(connection);

	dbus_connection_remove_filter(connection,
									__common_check_invalid_password, NULL);

	dbus_connection_unref(connection);
	connection = NULL;

	return WIFI_ERROR_NONE;
}

gboolean _common_is_invalid_password(void)
{
	return invalid_key;
}
