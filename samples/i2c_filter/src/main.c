/*
 * Copyright (c) 2024 Linkedsemi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if !defined(CONFIG_I2C_FILTER_LINKEDSEMI)
    #error no CONFIG_I2C_FILTER_LINKEDSEMI define
#endif

#include <zephyr/kernel.h>
#include <i2c_filter.h>

int main(void)
{
    const struct device *const i2cfilter = DEVICE_DT_GET(DT_ALIAS(i2cfilter));
    uint8_t bitmap[LINKEDSEMI_I2C_F_REMAP_SIZE_BYTE] = {};
    linkedsemi_i2c_filter_fill_bitmap(i2cfilter, 0, 0x50, bitmap);
    linkedsemi_i2c_filter_en(i2cfilter, true, true, false);

    return 0;
}
