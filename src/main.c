/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "logging/log.h"
LOG_MODULE_REGISTER(FDL);

#include <zephyr.h>

#include "dl_channel.h"
#include "dl_command.h"
#include "dl_packet.h"


void main(void)
{
	int ret;
	struct dl_ch *ch;
	LOG_INF("UNISOC fdl.");

	ch = dl_channel_init();
	if (ch == NULL) {
		LOG_ERR("Init channel failed.");
		return;
	}

	dl_packet_init(ch);
	
	ret = dl_cmd_init();
	if (ret) {
		LOG_ERR("Init command failed.");
		return;
	}

	LOG_INF("start download...");
}
