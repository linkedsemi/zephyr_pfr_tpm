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

int linkedsemi_i2c_filter_en(const struct device *dev,
                             bool filter_en,
                             bool wlist_en,
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
