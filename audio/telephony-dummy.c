/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2006-2007  Nokia Corporation
 *  Copyright (C) 2004-2008  Marcel Holtmann <marcel@holtmann.org>
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <glib.h>
#include <dbus/dbus.h>
#include <gdbus.h>

#include "logging.h"
#include "telephony.h"

static char *subscriber_number = NULL;
static char *active_call_number = NULL;
static int active_call_status = 0;
static int active_call_dir = 0;

static gboolean events_enabled = FALSE;

/* Response and hold state
 * -1 = none
 *  0 = incoming call is put on hold in the AG
 *  1 = held incoming call is accepted in the AG
 *  2 = held incoming call is rejected in the AG
 */
static int response_and_hold = -1;

static struct indicator dummy_indicators[] =
{
	{ "battchg",	"0-5",	5 },
	{ "signal",	"0-5",	5 },
	{ "service",	"0,1",	1 },
	{ "call",	"0,1",	0 },
	{ "callsetup",	"0-3",	0 },
	{ "callheld",	"0-2",	0 },
	{ "roam",	"0,1",	0 },
	{ NULL }
};

void telephony_device_connected(void *telephony_device)
{
	debug("telephony-dummy: device %p connected", telephony_device);
}

void telephony_device_disconnected(void *telephony_device)
{
	debug("telephony-dummy: device %p disconnected", telephony_device);
	events_enabled = FALSE;
}

void telephony_event_reporting_req(void *telephony_device, int ind)
{
	events_enabled = ind == 1 ? TRUE : FALSE;

	telephony_event_reporting_rsp(telephony_device, CME_ERROR_NONE);
}

void telephony_response_and_hold_req(void *telephony_device, int rh)
{
	response_and_hold = rh;

	telephony_response_and_hold_ind(response_and_hold);

	telephony_response_and_hold_rsp(telephony_device, CME_ERROR_NONE);
}

void telephony_last_dialed_number_req(void *telephony_device)
{
	telephony_last_dialed_number_rsp(telephony_device, CME_ERROR_NONE);

	/* Notify outgoing call set-up successfully initiated */
	telephony_update_indicator(dummy_indicators, "callsetup",
					EV_CALLSETUP_OUTGOING);
	telephony_update_indicator(dummy_indicators, "callsetup",
					EV_CALLSETUP_ALERTING);

	active_call_status = CALL_STATUS_ALERTING;
	active_call_dir = CALL_DIR_OUTGOING;
}

void telephony_terminate_call_req(void *telephony_device)
{
	g_free(active_call_number);
	active_call_number = NULL;

	telephony_terminate_call_rsp(telephony_device, CME_ERROR_NONE);

	if (telephony_get_indicator(dummy_indicators, "callsetup") > 0)
		telephony_update_indicator(dummy_indicators, "callsetup",
						EV_CALLSETUP_INACTIVE);
	else
		telephony_update_indicator(dummy_indicators, "call",
						EV_CALL_INACTIVE);
}

void telephony_answer_call_req(void *telephony_device)
{
	telephony_answer_call_rsp(telephony_device, CME_ERROR_NONE);

	telephony_update_indicator(dummy_indicators, "call", EV_CALL_ACTIVE);
	telephony_update_indicator(dummy_indicators, "callsetup",
					EV_CALLSETUP_INACTIVE);

	active_call_status = CALL_STATUS_ACTIVE;
}

void telephony_dial_number_req(void *telephony_device, const char *number)
{
	g_free(active_call_number);
	active_call_number = g_strdup(number);

	debug("telephony-dummy: dial request to %s", active_call_number);

	telephony_dial_number_rsp(telephony_device, CME_ERROR_NONE);

	/* Notify outgoing call set-up successfully initiated */
	telephony_update_indicator(dummy_indicators, "callsetup",
					EV_CALLSETUP_OUTGOING);
	telephony_update_indicator(dummy_indicators, "callsetup",
					EV_CALLSETUP_ALERTING);

	active_call_status = CALL_STATUS_ALERTING;
	active_call_dir = CALL_DIR_OUTGOING;
}

void telephony_transmit_dtmf_req(void *telephony_device, char tone)
{
	debug("telephony-dummy: transmit dtmf: %c", tone);
	telephony_transmit_dtmf_rsp(telephony_device, CME_ERROR_NONE);
}

void telephony_subscriber_number_req(void *telephony_device)
{
	debug("telephony-dummy: subscriber number request");
	if (subscriber_number)
		telephony_subscriber_number_ind(subscriber_number, 0,
						SUBSCRIBER_SERVICE_VOICE);
	telephony_subscriber_number_rsp(telephony_device, CME_ERROR_NONE);
}

void telephony_list_current_calls_req(void *telephony_device)
{
	debug("telephony-dummy: list current calls request");
	if (active_call_number)
		telephony_list_current_call_ind(1, active_call_dir,
						active_call_status,
						CALL_MODE_VOICE,
						CALL_MULTIPARTY_NO,
						active_call_number,
						0);
	telephony_list_current_calls_rsp(telephony_device, CME_ERROR_NONE);
}

/* D-Bus method handlers */
static DBusMessage *outgoing_call(DBusConnection *conn, DBusMessage *msg,
					void *data)
{
	const char *number;

	if (!dbus_message_get_args(msg, NULL, DBUS_TYPE_STRING, &number,
						DBUS_TYPE_INVALID))
		return NULL;

	debug("telephony-dummy: outgoing call to %s", number);

	g_free(active_call_number);
	active_call_number = g_strdup(number);

	telephony_update_indicator(dummy_indicators, "callsetup",
					EV_CALLSETUP_OUTGOING);
	telephony_update_indicator(dummy_indicators, "callsetup",
					EV_CALLSETUP_ALERTING);

	active_call_status = CALL_STATUS_ALERTING;
	active_call_dir = CALL_DIR_OUTGOING;

	return dbus_message_new_method_return(msg);
}

static DBusMessage *incoming_call(DBusConnection *conn, DBusMessage *msg,
					void *data)
{
	const char *number;

	if (!dbus_message_get_args(msg, NULL, DBUS_TYPE_STRING, &number,
						DBUS_TYPE_INVALID))
		return NULL;

	debug("telephony-dummy: incoming call to %s", number);

	g_free(active_call_number);
	active_call_number = g_strdup(number);

	telephony_update_indicator(dummy_indicators, "callsetup",
					EV_CALLSETUP_INCOMING);

	active_call_status = CALL_STATUS_INCOMING;
	active_call_dir = CALL_DIR_INCOMING;

	telephony_incoming_call_ind(number, 0);

	return dbus_message_new_method_return(msg);
}

static DBusMessage *cancel_call(DBusConnection *conn, DBusMessage *msg,
					void *data)
{
	debug("telephony-dummy: cancel call");

	g_free(active_call_number);
	active_call_number = NULL;

	if (telephony_get_indicator(dummy_indicators, "callsetup") > 0) {
		telephony_update_indicator(dummy_indicators, "callsetup",
						EV_CALLSETUP_INACTIVE);
		telephony_calling_stopped_ind();
	}

	if (telephony_get_indicator(dummy_indicators, "call") > 0)
		telephony_update_indicator(dummy_indicators, "call",
						EV_CALL_INACTIVE);

	return dbus_message_new_method_return(msg);
}

static DBusMessage *signal_strength(DBusConnection *conn, DBusMessage *msg,
					void *data)
{
	dbus_uint32_t strength;

	if (!dbus_message_get_args(msg, NULL, DBUS_TYPE_UINT32, &strength,
						DBUS_TYPE_INVALID))
		return NULL;

	if (strength > 5)
		return NULL;

	telephony_update_indicator(dummy_indicators, "signal", strength);

	debug("telephony-dummy: signal strength set to %u", strength);

	return dbus_message_new_method_return(msg);
}

static DBusMessage *battery_level(DBusConnection *conn, DBusMessage *msg,
					void *data)
{
	dbus_uint32_t level;

	if (!dbus_message_get_args(msg, NULL, DBUS_TYPE_UINT32, &level,
						DBUS_TYPE_INVALID))
		return NULL;

	if (level > 5)
		return NULL;

	telephony_update_indicator(dummy_indicators, "battchg", level);

	debug("telephony-dummy: battery level set to %u", level);

	return dbus_message_new_method_return(msg);
}

static DBusMessage *roaming_status(DBusConnection *conn, DBusMessage *msg,
					void *data)
{
	dbus_bool_t roaming;
	int val;

	if (!dbus_message_get_args(msg, NULL, DBUS_TYPE_BOOLEAN, &roaming,
						DBUS_TYPE_INVALID))
		return NULL;

	val = roaming ? EV_ROAM_ACTIVE : EV_ROAM_INACTIVE;

	telephony_update_indicator(dummy_indicators, "roam", val);

	debug("telephony-dummy: roaming status set to %d", val);

	return dbus_message_new_method_return(msg);
}

static DBusMessage *registration_status(DBusConnection *conn, DBusMessage *msg,
					void *data)
{
	dbus_bool_t registration;
	int val;

	if (!dbus_message_get_args(msg, NULL, DBUS_TYPE_BOOLEAN, &registration,
						DBUS_TYPE_INVALID))
		return NULL;

	val = registration ? EV_SERVICE_PRESENT : EV_SERVICE_NONE;

	telephony_update_indicator(dummy_indicators, "service", val);

	debug("telephony-dummy: registration status set to %d", val);

	return dbus_message_new_method_return(msg);
}

static DBusMessage *set_subscriber_number(DBusConnection *conn,
						DBusMessage *msg,
						void *data)
{
	const char *number;

	if (!dbus_message_get_args(msg, NULL, DBUS_TYPE_STRING, &number,
						DBUS_TYPE_INVALID))
		return NULL;

	g_free(subscriber_number);
	subscriber_number = g_strdup(number);

	debug("telephony-dummy: subscriber number set to %s", number);

	return dbus_message_new_method_return(msg);
}

static GDBusMethodTable dummy_methods[] = {
	{ "OutgoingCall",	"s",	"",	outgoing_call		},
	{ "IncomingCall",	"s",	"",	incoming_call		},
	{ "CancelCall",		"",	"",	cancel_call		},
	{ "SignalStrength",	"u",	"",	signal_strength		},
	{ "BatteryLevel",	"u",	"",	battery_level		},
	{ "RoamingStatus",	"b",	"",	roaming_status		},
	{ "RegistrationStatus",	"b",	"",	registration_status	},
	{ "SetSubscriberNumber","s",	"",	set_subscriber_number	},
	{ }
};

static DBusConnection *connection = NULL;

int telephony_init(void)
{
	uint32_t features = AG_FEATURE_REJECT_A_CALL |
				AG_FEATURE_ENHANCED_CALL_STATUS |
				AG_FEATURE_EXTENDED_ERROR_RESULT_CODES;

	connection = dbus_bus_get(DBUS_BUS_SYSTEM, NULL);

	g_dbus_register_interface(connection, "/org/bluez/test",
					"org.bluez.TelephonyTest",
					dummy_methods, NULL,
					NULL, NULL, NULL);

	telephony_ready_ind(features, dummy_indicators, response_and_hold);

	return 0;
}

void telephony_exit(void)
{
	dbus_connection_unref(connection);
	connection = NULL;
}
