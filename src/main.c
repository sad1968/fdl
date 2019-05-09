/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "logging/log.h"
LOG_MODULE_REGISTER(FDL);

#include <zephyr.h>

#include "uwp_cmd.h"
#include "uwp_intf.h"

unsigned char debug_mode(void)
{
#ifdef CONFIG_FDL_DEBUG
	return 1;
#else
	return 0;
#endif
}

#define INTF_NAME	"zephyr_uart"
#define DEV_NAME	"UART_0"

void main(void)
{
	int ret;
	LOG_INF("UNISOC FDL.");

	ret = uwp_intf_init(INTF_NAME, DEV_NAME);
	if (ret) {
		LOG_ERR("Init channel failed.");
		return;
	}
	
	ret = uwp_cmd_init();
	if (ret) {
		LOG_ERR("Init command failed.");
		return;
	}

	LOG_INF("start download...");
}
