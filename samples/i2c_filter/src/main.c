/*
 * Copyright (c) 2024 Linkedsemi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if !defined(CONFIG_I2C_FILTER_LINKEDSEMI)
    #error no CONFIG_I2C_FILTER_LINKEDSEMI define
#endif

#include <string.h>
#include <zephyr/kernel.h>
#include <i2c_filter.h>

int main(void)
{
    const struct device *const i2cfilter = DEVICE_DT_GET(DT_ALIAS(i2cfilter));
    uint32_t bitmap[LINKEDSEMI_I2C_F_REMAP_SIZE_U32] = {};
    uint32_t dump_bitmap[LINKEDSEMI_I2C_F_REMAP_SIZE_U32] = {};
    uint8_t index = 0;
    uint8_t addr = 0x50;
    uint8_t dump_addr = 0;
    uint8_t bit = 8;
    linkedsemi_i2c_filter_cold_reset(i2cfilter);
    sys_bitfield_set_bit((mem_addr_t)bitmap, bit);
    linkedsemi_i2c_filter_fill_bitmap(i2cfilter, index, addr, bitmap);
    linkedsemi_i2c_filter_dump_bitmap(i2cfilter, index, &dump_addr, dump_bitmap);
    int ret = memcmp(bitmap, dump_bitmap, LINKEDSEMI_I2C_F_REMAP_SIZE_BYTE);
    __ASSERT_NO_MSG(ret == 0);
    __ASSERT_NO_MSG(addr == dump_addr);
    linkedsemi_i2c_filter_en(i2cfilter, true, true, false);

    return 0;
}
