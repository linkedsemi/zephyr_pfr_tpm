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
#define SPIF_GENERAL_CMD_TABLE_NUM 40
#define SPIF_CMD_TABLE_NUM        (SPIF_FIXED_CMD_TABLE_NUM + SPIF_GENERAL_CMD_TABLE_NUM)

#define SPIF_ABS_ADDR(reg_off, bit_off) ((reg_off)*524288 + (bit_off)*16384)

#define SPIF_ADDR_PRIV_REG_NUN 512
#define SPIF_ADDR_PRIV_BIT_NUN (SPIF_ADDR_PRIV_REG_NUN * 32)
#define SPI_CMD_BITMAPF_LOG_SIZE_BIT  256
#define SPIF_CMD_BITMAP_LOG_SIZE_BYTE 32
#define SPI_CMD_BITMAPF_LOG_SIZE_U32  8

struct priv_reg_info {
    uint32_t start_reg_off;
    uint32_t start_bit_off;
    uint32_t end_reg_off;
    uint32_t end_bit_off;
};

enum addr_priv_rw_select {
    FLAG_ADDR_PRIV_READ_SELECT,
    FLAG_ADDR_PRIV_WRITE_SELECT
};

enum addr_priv_op {
    FLAG_ADDR_PRIV_ENABLE,
    FLAG_ADDR_PRIV_DISABLE
};

typedef void (*spif_callback_t)(const struct device *dev,
                                uint32_t callback_idx,
                                void *user_data,
                                void *drv_data);

int linkedsemi_spif_register_callback(const struct device *dev,
                                      uint32_t callback_idx,
                                      spif_callback_t cb,
                                      void *user_data);
void spif_dump_cmd_table(const struct device *dev);
int spif_get_cmd_slot(const struct device *dev, uint8_t cmd, uint32_t start_off);
int spif_add_cmd(const struct device *dev, uint8_t cmd);
int spif_remove_cmd(const struct device *dev, uint8_t cmd);
int spif_get_general_cmd_slot(const struct device *dev, uint8_t cmd, uint32_t start_off);
int spif_add_general_cmd(const struct device *dev, uint8_t cmd);
int spif_remove_general_cmd(const struct device *dev, uint8_t cmd);
void spif_dump_rw_addr_privilege_table(const struct device *dev);
void spif_dump_cmd_bitmap_log(const struct device *dev, uint8_t bitmap[SPIF_CMD_BITMAP_LOG_SIZE_BYTE]);
int spif_address_privilege_config(const struct device *dev,
                                  enum addr_priv_rw_select rw_select,
                                  enum addr_priv_op priv_op,
                                  mm_reg_t addr,
                                  uint32_t len);
void spif_filter_enable(const struct device *dev, bool enable);
void spif_reg_unlock(const struct device *dev);
void spif_reg_lock(const struct device *dev);

/**
 * @}
 */
#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SPI_FILTER_H_ */
