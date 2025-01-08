/*
 * Copyright (c) 2024 Linkedsemi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_I2C_FILTER_H_
#define ZEPHYR_INCLUDE_DRIVERS_I2C_FILTER_H_

#define LINKEDSEMI_I2C_F_ADDR_NUM             16
#define LINKEDSEMI_I2C_F_REMAP_SIZE_BYTE      32
#define LINKEDSEMI_I2C_F_REMAP_SIZE_U32       8

#ifdef __cplusplus
extern "C" {
#endif

int linkedsemi_i2c_filter_fill_bitmap(const struct device *dev,
                                    uint8_t idx,
                                    uint8_t addr,
                                    uint32_t bitmap[LINKEDSEMI_I2C_F_REMAP_SIZE_U32]);

int linkedsemi_i2c_filter_en(const struct device *dev,
                             bool filter_en,
                             bool wlist_en,
                             bool clr_tbl);

/**
 * @}
 */
#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_I2C_FILTER_H_ */
