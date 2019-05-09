/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "logging/log.h"
LOG_MODULE_DECLARE(FDL);

#include <zephyr.h>
#include <string.h>
#include <flash.h>

#include "dloader.h"
#include "uwp_pkt.h"

/*static root_stat_t root_stat;*/
const char fdl_ver_str[] = {"SPRD3"};


static __inline u32_t convert_operate_status(int err)
{
	switch (err)
	{
		case OPERATE_SUCCESS:
			return BSL_REP_ACK;
		case OPERATE_INVALID_ADDR:
			return BSL_REP_DOWN_DEST_ERROR;
		case OPERATE_INVALID_SIZE:
			return BSL_REP_DOWN_SIZE_ERROR;
		case OPERATE_DEVICE_INIT_ERROR:
			return BSL_UNKNOWN_DEVICE;
		case OPERATE_INVALID_DEVICE_SIZE:
			return BSL_INVALID_DEVICE_SIZE;
		case OPERATE_INCOMPATIBLE_PART:
			return BSL_INCOMPATIBLE_PARTITION;
		case OPERATE_WRITE_ERROR:
			return BSL_WRITE_ERROR;
		case OPERATE_CHECKSUM_DIFF:
			return BSL_CHECKSUM_DIFF;

		default:
		    return BSL_REP_OPERATION_FAILED;
	}
}

int dl_cmd_reply(uint32_t err)
{
	dl_send_ack(convert_operate_status(err));

	if (err == OPERATE_SUCCESS) return 0;

	return -1;
}

struct spi_flash *sf = NULL;
static uint32_t image_addr = 0;
static uint32_t image_size = 0;
#define NORFLASH_ADDRESS 0x2000000

int dl_cmd_connect(unsigned char *payload, unsigned int len)
{
	LOG_INF("dl_cmd_connect.");
	return dl_cmd_reply(OPERATE_SUCCESS);
}

int dl_cmd_start(unsigned char *payload, unsigned int len)
{
	struct uwp_pkt_start *start;

	start = (struct uwp_pkt_start *)payload;

	image_addr = le32(start->addr);
	image_size = le32(start->len);

	LOG_INF("image address: %x", image_addr);
	LOG_INF("image size: %d", image_addr);

	return dl_cmd_reply(OPERATE_SUCCESS);
}

int dl_cmd_midst(unsigned char *payload, unsigned int len)
{
	static u8_t *data_addr = (u8_t *)CONFIG_SYS_LOAD_ADDR;

	memcpy(data_addr, payload, len);
	data_addr += len;

	return dl_cmd_reply(OPERATE_SUCCESS);
}

int dl_cmd_end(unsigned char *payload, unsigned int len)
{
	int ret;
	struct device *dev = device_get_binding(DT_FLASH_AREA_0_DEV);
	u8_t *data_addr = (u8_t *)CONFIG_SYS_LOAD_ADDR;

	LOG_INF("dl_cmd_write_end %p", data_addr);
	image_addr -= NORFLASH_ADDRESS;
	if (dev == NULL) {
		LOG_ERR("Can not open device: %s.", DT_FLASH_AREA_0_DEV);
		return dl_cmd_reply(OPERATE_SYSTEM_ERROR);
	}
	LOG_INF("Open device %s success.", DT_FLASH_AREA_0_DEV);
	flash_write_protection_set(dev, false);

	image_size = (image_size + 0xFFF) & ~0xFFF;

	LOG_INF("Erase flash address: 0x%x size: 0x%x.", image_addr, image_size);
	ret = flash_erase(dev, image_addr, image_size);
	if (ret) {
		LOG_ERR("Erase flash failed.");
		return dl_cmd_reply(OPERATE_SYSTEM_ERROR);
	}
	LOG_INF("Erase success.");

	LOG_INF("Write flash start...");
	ret = flash_write(dev, image_addr, data_addr, image_size);
	if (ret) {
		LOG_ERR("wirte flash failed.");
		return dl_cmd_reply(OPERATE_SYSTEM_ERROR);
	}
	LOG_INF("Write success.");
	flash_write_protection_set(dev, true);

	return dl_cmd_reply(OPERATE_SUCCESS);
}

int dl_cmd_init(void)
{
	struct dl_pkt *pkt;

	pkt = dl_pkt_alloc();
	if (pkt == NULL) {
		LOG_ERR("No packet!");
		return -1;
	}

	pkt->body.type = BSL_REP_VER;
	pkt->body.size = sizeof(fdl_ver_str);
	memcpy((u8_t *)pkt->body.content, fdl_ver_str, sizeof(fdl_ver_str));
	dl_pkt_send(pkt);

	return 0;
}
