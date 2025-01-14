/*
 * Copyright (c) 2024 Linkedsemi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SPI_FILTER_H_
#define ZEPHYR_INCLUDE_DRIVERS_SPI_FILTER_H_

#ifdef __cplusplus
extern "C" {
#endif

#define SPIF_FIXED_CMD_TABLE_NUM  30
#define SPIF_COMMON_CMD_TABLE_NUM 40
#define SPIF_CMD_TABLE_NUM        (SPIF_FIXED_CMD_TABLE_NUM + SPIF_COMMON_CMD_TABLE_NUM)

int spif_get_cmd_slot(const struct device *dev, uint8_t cmd, uint32_t start_off);
int spif_add_cmd(const struct device *dev, uint8_t cmd);
int spif_remove_cmd(const struct device *dev, uint8_t cmd);

/**
 * @}
 */
#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SPI_FILTER_H_ */
