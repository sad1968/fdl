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

#include "dl_crc.h"
#include "dl_command.h"
#include "dl_packet.h"
#include "dl_channel.h"

#define BOOT_FLAG_USB                   (0x5A)
#define BOOT_FLAG_UART1                 (0x6A)
#define BOOT_FLAG_UART0                 (0x7A)

#define UART_NAME	"UART_0"
struct device *uart_dev;

static int uart_write(struct dl_ch  *channel, const unsigned char *buf, unsigned int len)
{
	struct device *priv = (struct device*)channel->priv;
	return uart_fifo_fill(priv,buf,len);
}

static int uart_put_char(struct dl_ch  *channel, const unsigned char ch)
{
	struct device *priv = (struct device*)channel->priv;
 
	LOG_INF("uart put: %c(0x%02x)", ch, ch);
	uart_poll_out(priv,ch);

	return 0;
}
static struct dl_ch uart_channel = {
	.write = uart_write,
	.put_char = uart_put_char,
};

static u8_t buf[MAX_PKT_SIZE];

void uart_rx_handle(struct dl_ch *ch)
{
	struct device *dev = (struct device *)ch->priv;

	static u32_t offset = 0;
	bool pkt_complete = false;
	u32_t len;
	u32_t i;

	len = uart_fifo_read(dev, &buf[offset], MAX_PKT_SIZE);
	offset += len;
	if (offset > MAX_PKT_SIZE) {
		LOG_ERR("Data packet overflow!\n");
		dl_send_ack(BSL_REP_OPERATION_FAILED);
		return;
	}

	for (i = 1; i < offset; i++) {
		if (buf[i] == HDLC_FLAG) {
			pkt_complete = true;
			break;
		}
	}

	if (!pkt_complete) return;

	LOG_INF("uart read 0x%x bytes: ", offset);
	if (offset < 32) {
		LOG_HEXDUMP_DBG(buf, offset, "rx data");
	}

	/* remove 0x7E at the head and tail. */
	dl_pkt_handler(buf + 1, offset - 2);
	offset = 0;
}

static void uart_callback(void *user_data)
{
	struct dl_ch *uart_ch = (struct dl_ch *)user_data;
	struct device *dev = (struct device *)uart_ch->priv;

	uart_irq_update(dev);

	if (uart_irq_rx_ready(dev)) {
		uart_rx_handle(uart_ch);
	}
}

struct dl_ch *dl_channel_get(void)
{
	return &uart_channel;
}

struct dl_ch *dl_channel_init(void)
{
	struct dl_ch *uart_ch = &uart_channel;

	uart_dev = device_get_binding(UART_NAME);
	if (uart_dev == NULL) {
		LOG_ERR("Open device %s failed", UART_NAME);
		return NULL;
	}

	uart_ch->priv = uart_dev;

	uart_irq_callback_user_data_set(uart_dev, uart_callback,
			(void *)uart_ch);
	uart_irq_rx_enable(uart_dev);

	return uart_ch;
}
