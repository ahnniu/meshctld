/*
 * Copyright (C) 2014 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <stdbool.h>

#include "emulator/bthost.h"
#include "tester-main.h"
#include "android/utils.h"

static struct queue *list; /* List of pan test cases */

struct emu_cid_data {
	uint16_t nap_handle;
	uint16_t nap_cid;
};

struct emu_cid_data cid_data;
static uint8_t pan_conn_req_pdu[] = {0x01, 0x01, 0x02, 0x11, 0x16, 0x11, 0x15};
static uint8_t pan_conn_rsp_pdu[] = {0x01, 0x02, 0x00, 0x00};

static void pan_nap_cid_hook_cb(const void *data, uint16_t len, void *user_data)
{
	struct test_data *t_data = tester_get_data();
	struct emu_cid_data *cid_data = user_data;
	struct bthost *bthost = hciemu_client_get_host(t_data->hciemu);

	if (!memcmp((uint8_t *) data, pan_conn_req_pdu,
						sizeof(pan_conn_req_pdu)))
		bthost_send_cid(bthost, cid_data->nap_handle, cid_data->nap_cid,
				pan_conn_rsp_pdu, sizeof(pan_conn_rsp_pdu));
}

static void pan_connect_request_cb(uint16_t handle, uint16_t cid,
							void *user_data)
{
	struct test_data *data = tester_get_data();
	struct bthost *bthost = hciemu_client_get_host(data->hciemu);

	cid_data.nap_handle = handle;
	cid_data.nap_cid = cid;

	bthost_add_cid_hook(bthost, handle, cid, pan_nap_cid_hook_cb,
								&cid_data);
}

static struct emu_set_l2cap_data l2cap_setup_data = {
	.psm = 15,
	.func = pan_connect_request_cb,
	.user_data = NULL,
};

static void pan_connect_action(void)
{
	struct test_data *data = tester_get_data();
	const uint8_t *pan_addr = hciemu_get_client_bdaddr(data->hciemu);
	struct step *step = g_new0(struct step, 1);
	bt_bdaddr_t bdaddr;

	bdaddr2android((const bdaddr_t *) pan_addr, &bdaddr);

	step->action_status = data->if_pan->connect(&bdaddr,
					BTPAN_ROLE_PANU, BTPAN_ROLE_PANNAP);

	schedule_action_verification(step);
}

static struct test_case test_cases[] = {
	TEST_CASE_BREDRLE("PAN Init",
		ACTION_SUCCESS(dummy_action, NULL),
	),
	TEST_CASE_BREDRLE("PAN Connect - Success",
		ACTION_SUCCESS(bluetooth_enable_action, NULL),
		CALLBACK_STATE(CB_BT_ADAPTER_STATE_CHANGED, BT_STATE_ON),
		ACTION_SUCCESS(emu_setup_powered_remote_action, NULL),
		ACTION_SUCCESS(emu_set_ssp_mode_action, NULL),
		ACTION_SUCCESS(emu_add_l2cap_server_action, &l2cap_setup_data),
		ACTION_SUCCESS(pan_connect_action, NULL),
		CALLBACK_PAN_CONN_STATE(CB_PAN_CONNECTION_STATE,
					BT_STATUS_SUCCESS,
					BTPAN_STATE_CONNECTING,
					BTPAN_ROLE_PANU, BTPAN_ROLE_PANNAP),
		CALLBACK_PAN_CTRL_STATE(CB_PAN_CONTROL_STATE, BT_STATUS_SUCCESS,
					BTPAN_STATE_ENABLED, BTPAN_ROLE_PANU),
		CALLBACK_PAN_CONN_STATE(CB_PAN_CONNECTION_STATE,
					BT_STATUS_SUCCESS,
					BTPAN_STATE_CONNECTED,
					BTPAN_ROLE_PANU, BTPAN_ROLE_PANNAP),
		ACTION_SUCCESS(bluetooth_disable_action, NULL),
		CALLBACK_PAN_CONN_STATE(CB_PAN_CONNECTION_STATE,
					BT_STATUS_SUCCESS,
					BTPAN_STATE_DISCONNECTED,
					BTPAN_ROLE_PANU, BTPAN_ROLE_PANNAP),
		CALLBACK_STATE(CB_BT_ADAPTER_STATE_CHANGED, BT_STATE_OFF),
	),
};

struct queue *get_pan_tests(void)
{
	uint16_t i = 0;

	list = queue_new();

	for (; i < sizeof(test_cases) / sizeof(test_cases[0]); ++i)
		if (!queue_push_tail(list, &test_cases[i]))
			return NULL;

	return list;
}

void remove_pan_tests(void)
{
	queue_destroy(list, NULL);
}
