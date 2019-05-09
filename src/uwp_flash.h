#ifndef _UWP_FLASH_H_
#define _UWP_FLASH_H_

extern void uwp_flash_set_image_addr(unsigned int addr);
extern void uwp_flash_set_image_size(unsigned int size);
extern int uwp_flash_update(u8_t *data, u32_t len);
extern int uwp_flash_write(void);

#endif
