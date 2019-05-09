/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "logging/log.h"
LOG_MODULE_DECLARE(FDL);

#include <zephyr.h>
#include <string.h>
#include <uart.h>

#include "uwp_pkt.h"
#include "uwp_cmd.h"
#include "uwp_intf.h"

#define BOOT_FLAG_USB                   (0x5A)
#define BOOT_FLAG_UART1                 (0x6A)
#define BOOT_FLAG_UART0                 (0x7A)

notify_func notify = NULL;
struct device *uart_dev;

static int uart_send(struct uwp_pkt *pkt)
{
    int send;
    unsigned int len = uwp_pkt_size(pkt);
    unsigned char *data = uwp_pkt_data(pkt);

	if ((pkt == NULL) || (uwp_pkt_size(pkt) == 0)) {
        LOG_ERR("invalid uwp packet.\n");
        return -1;
    }

    uwp_pkt_dump(pkt, "UART SEND");

    send = uart_fifo_fill(uart_dev, data, len);
    if (send == len)
        return 0;

    return -1;
}

static int uart_recv(struct uwp_pkt *pkt)
{
	int len;

	uart_irq_update(uart_dev);

	if (uart_irq_rx_ready(uart_dev)) {
		len = uart_fifo_read(uart_dev, uwp_pkt_write_ptr(pkt),
				uwp_pkt_available_len(pkt));

		if (len > 0) {
			uwp_pkt_increase(pkt, len);
			uwp_pkt_dump(pkt, "UART RECV");
			return 0;
		}
	}

	return -1;
}

int uart_init(char *dev)
{
	uart_dev = device_get_binding(dev);
	if (uart_dev == NULL) {
		LOG_ERR("Open device %s failed", dev);
		return -1;
	}

	if (notify != NULL) {
		uart_irq_callback_user_data_set(uart_dev, notify,
				(void *)uart_dev);
		uart_irq_rx_enable(uart_dev);

		LOG_INF("enable interrupt mode.");
	}

	LOG_INF("zephyr uart init success.");

	return 0;
}

int uart_register_notify(notify_func func)
{
    notify = func;
    return 0;
}

struct uwp_intf zephyr_uart_intf = {
	.name = "zephyr_uart",
	.init = uart_init,
	.send = uart_send,
	.recv = uart_recv,
	.register_notify = uart_register_notify,
};
