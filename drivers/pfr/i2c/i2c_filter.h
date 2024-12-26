/*
 * Copyright (c) 2024 Linkedsemi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_I2C_FILTER_H_
#define ZEPHYR_INCLUDE_DRIVERS_I2C_FILTER_H_

/* filter capability define */
#define LINKEDSEMI_I2C_F_ADDR_NUM             16
#define LINKEDSEMI_I2C_F_REMAP_SIZE_BYTE      32
#define LINKEDSEMI_I2C_F_REMAP_SIZE_U32       8

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief update i2c filter device
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param idx Value to the index of re-map table.
 * @param addr Value to the white list address.
 * @param table Pointer to the filter bitmap table.
 * @retval 0 If successful.
 * @retval -EINVAL Invalid data pointer or offset
 */
int linkedsemi_i2c_filter_fill_bitmap(const struct device *dev,
                                    uint8_t idx,
                                    uint8_t addr,
                                    uint8_t bitmap[LINKEDSEMI_I2C_F_REMAP_SIZE_BYTE]);
/**
 * @brief enable i2c filter device
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param filter_en Value to the filter device enable.
 * @param wlist_en Value to the white list enable.
 * @param clr_idx Value to the clear index.
 * @param clr_tbl Value to the clear white list table.
 * @retval 0 If successful.
 * @retval -EINVAL Invalid data pointer or offset
 */
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
