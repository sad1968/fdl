/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "logging/log.h"
LOG_MODULE_DECLARE(FDL);

#include <zephyr.h>
#include <string.h>

#include "dloader.h"
#include "dl_packet.h"
#include "dl_channel.h"
#include "uwp_pkt.h"
#include "uwp_cmd.h"

struct dl_ch *fdl_ch;
struct dl_ch *gFdlPrintChannel;

static struct dl_pkt  packet[PACKET_MAX_NUM];

static struct dl_pkt *packet_free_list;

void dl_pkt_init(struct dl_ch *ch)
{
	uint32_t i = 0;

	packet_free_list = &packet[0];

	for (i = 0; i < PACKET_MAX_NUM; i++) {
		packet[i].next   = &packet[i+1];
	}
	packet[PACKET_MAX_NUM-1].next = NULL;

	fdl_ch = ch;
	LOG_INF("fdl_ch: %p",fdl_ch);
}

struct dl_pkt *dl_pkt_alloc (void)
{
	struct dl_pkt   *tmp_ptr = NULL;

	if (NULL != packet_free_list) {
		tmp_ptr = packet_free_list;
		packet_free_list = tmp_ptr->next;

		tmp_ptr->next       = NULL;
		tmp_ptr->pkt_state  = PKT_NONE;
		tmp_ptr->ack_flag   = 0;
		tmp_ptr->data_size  = 0;
	}

	return tmp_ptr;
}

void dl_pkt_free(struct dl_pkt *ptr)
{
	ptr->next = packet_free_list;
	packet_free_list = ptr;
}

void dl_pkt_write (const void *buf, int len)
{
	fdl_ch->write (fdl_ch, buf, len);
}

void dl_pkt_send(struct dl_pkt *pkt)
{
	u32_t send_len;
	u8_t c = 0x7E;
	u16_t crc;
	u16_t *crc_ptr;

	u16_t size = pkt->body.size;
	send_len = PACKET_HEADER_SIZE + size + sizeof(u16_t);

	pkt->body.size = EndianConv_16(size);
	pkt->body.type = EndianConv_16(pkt->body.type);

	crc = frm_chk((const u16_t *)&pkt->body,
			size + PACKET_HEADER_SIZE);

	crc_ptr = (u16_t *)(pkt->body.content + size);
	*crc_ptr = crc;

	dl_pkt_write(&c, 1);
	dl_pkt_write((u8_t *)&(pkt->body), send_len);
	dl_pkt_write(&c, 1);

	dl_pkt_free(pkt);
}

void dl_send_ack(u32_t cmd)
{
	struct dl_pkt *pkt;

	pkt = dl_pkt_alloc();
	if (pkt == NULL) {
		LOG_INF("Alloc packet failed!");
		uwp_cmd_reply(9);
		return;
	}

	pkt->body.type = cmd;
	pkt->body.size = 0;

	dl_pkt_send(pkt);
}

#if 0
struct dl_cmd dl_cmds[] = {
	{BSL_CMD_CONNECT, uwp_cmd_connect_resp},
	{BSL_CMD_START_DATA, uwp_cmd_start_resp},
	{BSL_CMD_MIDST_DATA, uwp_cmd_midst_resp},
	{BSL_CMD_END_DATA, uwp_cmd_end_resp},
	{0, NULL}
};

int dl_pkt_handler(struct uwp_pkt *pkt)
{
	u32_t i;
	u32_t cmds_cnt = sizeof(dl_cmds) / sizeof(struct dl_cmd);
	struct uwp_pkt_hdr *hdr;

	struct dl_cmd *cmd;

	hdr = uwp_pkt_hdr(pkt);
	hdr->type = le16(hdr->type);
	hdr->size = le16(hdr->size);

	LOG_INF("pkt type: %d.", hdr->type);
	for (i = 0, cmd = dl_cmds; i < cmds_cnt; i++, cmd++) {
		if ((hdr->type == cmd->type) && (cmd->handle != NULL)) {
			cmd->handle(uwp_pkt_payload(pkt), hdr->size);
			return 0;
		}
	}

	/* cannot found the cmd in cmdlist */
	return uwp_cmd_reply(0);
}
#endif
