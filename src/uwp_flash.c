#include "logging/log.h"
LOG_MODULE_DECLARE(FDL);

#include <zephyr.h>
#include <flash.h>
#include <string.h>

#include "uwp_flash.h"

struct device *dev;
#define CONFIG_SYS_LOAD_ADDR	0x120000
#define NORFLASH_ADDRESS 0x2000000
#define MAX_FLASH_SIZE	(64 * 1024)
static u8_t *buf_addr = (u8_t *)CONFIG_SYS_LOAD_ADDR;
static u32_t buf_len = 0;
static u32_t write_offset = 0;
unsigned int image_addr = 0;
unsigned int image_size = 0;

#define FLASH_BY_PIECE

void uwp_flash_set_image_addr(unsigned int addr)
{
	image_addr = addr;
	LOG_INF("image address: %x", addr);

	/* In case of flash operation, it requires the flash offset
	 * for read and wirte operations
	 */
	image_addr -= NORFLASH_ADDRESS;
	write_offset = 0;
	buf_len = 0;
}

void uwp_flash_set_image_size(unsigned int size)
{
	image_size = size;
	LOG_INF("image size: %d", size);
}

int uwp_flash_write(void)
{
	int ret;
	u32_t write_addr;

	if (buf_len == 0) return 0;

#ifdef FLASH_BY_PIECE
	write_addr = image_addr + write_offset;
#else
	write_addr = image_addr;
	if (image_size != buf_len) {
		LOG_ERR("Image length not match.\n");
		return -1;
	}
#endif

	dev = device_get_binding(DT_FLASH_AREA_0_DEV);
	if (dev == NULL) {
		LOG_ERR("Open flash %s failed.", DT_FLASH_AREA_0_DEV);
		return -1;
	}
	flash_write_protection_set(dev, false);

	/* flash write require alligned */
	buf_len = (buf_len + 0xFFF) & ~0xFFF;

	LOG_INF("Erase flash address: 0x%x size: 0x%x.", write_addr, buf_len);

	ret = flash_erase(dev, write_addr, buf_len);
	if (ret) {
		LOG_ERR("Erase flash failed.");
		return -1;
	}
	LOG_INF("Erase success.");

	LOG_INF("Write flash start...");
	ret = flash_write(dev, write_addr, buf_addr, buf_len);
	if (ret) {
		LOG_ERR("wirte flash failed.");
		return -1;
	}
	LOG_INF("Write success.");
	flash_write_protection_set(dev, true);

	write_offset += buf_len;
	buf_len = 0;

	return 0;
}

int uwp_flash_update(u8_t *data, u32_t len)
{
#ifdef FLASH_BY_PIECE
	int ret;

	u32_t used_len = 0;

	if (buf_len + len <= MAX_FLASH_SIZE) {
		memcpy(buf_addr + buf_len, data, len);
		buf_len += len;
	} else {
		used_len = MAX_FLASH_SIZE - buf_len;
		memcpy(buf_addr + buf_len, data,  used_len);
		buf_len = MAX_FLASH_SIZE;

		ret = uwp_flash_write();
		if (ret) return ret;

		return uwp_flash_update(data + used_len, len - used_len);
	}
#else
	memcpy(buf_addr + buf_len, data, len);
	buf_len += len;
#endif

	return 0;
}

