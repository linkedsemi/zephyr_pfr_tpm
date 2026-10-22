/*
 * Copyright (c) 2024 Linkedsemi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_I2C_FILTER_H_
#define ZEPHYR_INCLUDE_DRIVERS_I2C_FILTER_H_

#include <stdint.h>

#define LINKEDSEMI_I2C_F_ADDR_NUM             16
#define LINKEDSEMI_I2C_F_REMAP_SIZE_BYTE      32
#define LINKEDSEMI_I2C_F_REMAP_SIZE_U32       8

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*i2c_filter_callback_t)(const struct device *dev,
                                    uint32_t callback_idx,
                                    void *user_data,
                                    void *drv_data);

int linkedsemi_i2c_filter_register_callback(const struct device *dev,
                                      uint32_t callback_idx,
                                      i2c_filter_callback_t cb,
                                      void *user_data);

int linkedsemi_i2c_filter_fill_bitmap(const struct device *dev,
                                    uint8_t idx,
                                    uint8_t addr,
                                    uint32_t bitmap[LINKEDSEMI_I2C_F_REMAP_SIZE_U32]);

int linkedsemi_i2c_filter_dump_bitmap(const struct device *dev,
                                    uint8_t idx,
                                    uint8_t *addr,
                                    uint32_t bitmap[LINKEDSEMI_I2C_F_REMAP_SIZE_U32]);

/*
 * Configure SMBUS_FILTER_SET.
 *
 *   filter_en        -> BLOCK_DISABLE  (!filter_en)
 *   wlist_en         -> FILTER_DISABLE (!wlist_en)
 *   relay_all_addr   -> RELAY_ALL_ADDRESS; bypass the address filter, so no
 *                       ADDRESS_BEYOND_WHITELIST interrupt is generated. The
 *                       command-byte bitmap is still checked for addresses that
 *                       match a whitelist entry, but only when the frame carries
 *                       at least two master-written bytes after the address
 *                       (measured: one-byte SMBus Send Byte / Read Byte frames
 *                       are not command-checked by the IP).
 *   clr_tbl          -> clear all whitelist command bitmaps after programming.
 *
 * Measured mode combinations (LSQSH):
 *   0b00 filter_en=1, wlist_en=1  : forward + block violations (filter mode)
 *   0b01 filter_en=0, wlist_en=1  : does not forward (NACK) - not a monitor mode
 *   0b10 filter_en=1, wlist_en=0  : forward + report, no block (monitor mode)
 *   0b11 filter_en=0, wlist_en=0  : does not forward (NACK)
 * Use (true, false) for monitor mode.
 */
int linkedsemi_i2c_filter_en(const struct device *dev,
                             bool filter_en,
                             bool wlist_en,
                             bool relay_all_addr,
                             bool clr_tbl);

int linkedsemi_i2c_filter_cold_reset(const struct device *dev);

int linkedsemi_i2c_filter_enable_channel(const struct device *dev,
                                        uint8_t idx,
                                        bool enable);

int linkedsemi_i2c_filter_switch_to_master(const struct device *dev);

int linkedsemi_i2c_filter_switch_to_filter(const struct device *dev);

const struct device *i2cf_i2c_dev(const struct device *dev);

/**
 * @}
 */
#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_I2C_FILTER_H_ */
