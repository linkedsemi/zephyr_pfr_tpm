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

#include <reg_spi_filter.h>

#define SPIF_FIXED_CMD_TABLE_NUM   30
#define SPIF_GENERAL_CMD_TABLE_NUM 40
#define SPIF_CMD_TABLE_NUM         (SPIF_FIXED_CMD_TABLE_NUM + SPIF_GENERAL_CMD_TABLE_NUM)

#define SPIF_ABS_ADDR(reg_off, bit_off) ((reg_off)*524288 + (bit_off)*16384)

#define SPIF_ADDR_PRIV_REG_NUN        512
#define SPIF_ADDR_PRIV_BIT_NUN        (SPIF_ADDR_PRIV_REG_NUN * 32)
#define SPI_CMD_BITMAPF_LOG_SIZE_BIT  256
#define SPIF_CMD_BITMAP_LOG_SIZE_BYTE 32
#define SPI_CMD_BITMAPF_LOG_SIZE_U32  8
#define SPIF_LOG_RAM_MAX_SIZE_U32     2047
#define SPIF_DMA_RX_THREAD_STACK_SIZE 1024

struct priv_reg_info {
    uint32_t start_reg_off;
    uint32_t start_bit_off;
    uint32_t end_reg_off;
    uint32_t end_bit_off;
};

struct spif_log_info {
    mem_addr_t log_ram_addr;
    uint32_t log_max_sz;
    union {
        uint32_t log_idx_reg;
        uint32_t log_idx;
    };
};

enum addr_priv_rw_select {
    FLAG_ADDR_PRIV_READ_SELECT,
    FLAG_ADDR_PRIV_WRITE_SELECT
};

enum addr_priv_op {
    FLAG_ADDR_PRIV_ENABLE,
    FLAG_ADDR_PRIV_DISABLE
};

enum target_addr_mode {
    FLAG_TARGET_ADDR_32BIT,
    FLAG_TARGET_ADDR_M19BIT,
};

/* allow command table control */
#define FLAG_CMD_TABLE_VALID         0x00000000
#define FLAG_CMD_TABLE_VALID_ONCE    0x00000001
#define FLAG_CMD_TABLE_LOCK_ALL      0x00000002

typedef void (*spif_callback_t)(const struct device *dev);
#define spim_log_info spif_log_info
#define spim_callback_t spif_callback_t

int linkedsemi_spif_register_callback(const struct device *dev, spif_callback_t cb);
void spif_dump_cmd_table(const struct device *dev);
void spif_get_cmd_table(const struct device *dev, uint8_t cmd[SPIF_CMD_TABLE_NUM], uint32_t *cmd_num);
int spif_add_cmd(const struct device *dev, uint8_t cmd);
int spif_add_cmd_with_dummy(const struct device *dev, uint8_t cmd, uint8_t dummy_cycle);
void spif_set_cmd_by_idx(const struct device *dev, uint8_t cmd, uint8_t idx);
void spif_set_dummy_by_idx(const struct device *dev, uint8_t dummy_cycle, uint8_t idx);
void spif_get_cmd_by_idx(const struct device *dev, uint8_t *cmd, uint8_t idx);
void spif_get_dummy_by_idx(const struct device *dev, uint8_t *dummy_cycle, uint8_t idx);
int spif_get_cmd_slot(const struct device *dev, uint8_t cmd, uint32_t start_off);
int spif_remove_cmd(const struct device *dev, uint8_t cmd);
int spif_get_general_cmd_slot(const struct device *dev, uint8_t cmd, uint32_t start_off);
int spif_add_general_cmd(const struct device *dev, uint8_t cmd);
int spif_remove_general_cmd(const struct device *dev, uint8_t cmd);
void spif_remove_cmd_by_idx(const struct device *dev, uint8_t cmd, uint8_t idx);
int spif_dump_rw_addr_privilege_table(const struct device *dev);
void spif_dump_cmd_bitmap_log(const struct device *dev, uint8_t bitmap[SPIF_CMD_BITMAP_LOG_SIZE_BYTE]);
void spif_clear_cmd_bitmap_log(const struct device *dev);
int spif_address_privilege_config(const struct device *dev,
                                  enum addr_priv_rw_select rw_select,
                                  enum addr_priv_op priv_op,
                                  mm_reg_t addr,
                                  uint32_t len);
void spif_filter_enable(const struct device *dev, bool enable);
void spif_reg_unlock(const struct device *dev);
void spif_reg_lock(const struct device *dev);
void spif_clk_check_config(const struct device *dev,
                           uint8_t div,
                           uint16_t threshold_high_cycle,
                           uint16_t threshold_low_cycle,
                           bool enable_intr);
uint16_t spif_clk_check_peek(const struct device *dev);
void spif_target_addr_config(const struct device *dev, uint32_t addr, enum target_addr_mode mode, bool enable_intr);
void spif_3byte_mode_config(const struct device *dev);
void spif_4byte_mode_config(const struct device *dev);
uint8_t spif_addr_mode_peek(const struct device *dev);
/**
 * @brief set filter mode or monitor mode
 *
 * @param filter_en true: filter mode | false: monitor mode
 */
void spif_operation_mode_config(const struct device *dev, bool filter_en);
spif_dma_data_t *spif_log_dma_buf(const struct device *dev);
void spif_memset_addr_whitelist(const struct device *dev, uint8_t num);
void spif_memset_read_addr_whitelist(const struct device *dev, uint8_t num);
void spif_memset_write_addr_whitelist(const struct device *dev, uint8_t num);
int spif_set_pinctrl_state(const struct device *dev, uint8_t pinctrl_state);
int linkedsemi_spi_filter_cold_reset(const struct device *dev);
const struct device *spif_spi_dev(const struct device *dev);
const struct gpio_dt_spec *spif_spi_cs(const struct device *dev);
int spif_switch_to_master(const struct device *dev);
int spif_switch_to_filter(const struct device *dev);

int spif_dma_start(const struct device *dev);
int spi_filter_dma_thread_init(const struct device *dev);
uint32_t spif_get_ctrl_idx(const struct device *dev);
void spif_get_log_info(const struct device *dev, struct spif_log_info *info);
void spif_enable(const struct device *dev, bool enable);

void spim_dump_allow_command_table(const struct device *dev);
int spim_add_allow_command(const struct device *dev, uint8_t cmd, uint32_t flag);
int spim_remove_allow_command(const struct device *dev, uint8_t cmd);
void spim_dump_rw_addr_privilege_table(const struct device *dev);
int spim_address_privilege_config(const struct device *dev,
								enum addr_priv_rw_select rw_select,
								enum addr_priv_op priv_op,
								mm_reg_t addr,
								uint32_t len);
void spim_lock_common(const struct device *dev);
void spim_monitor_enable(const struct device *dev, bool enable);
typedef void (*spim_isr_callback_t)(const struct device *dev);
void spim_isr_callback_install(const struct device *dev, spim_isr_callback_t isr_callback);
void spim_get_log_info(const struct device *dev, struct spim_log_info *info);
uint32_t spim_get_ctrl_idx(const struct device *dev);
void spim_allow_command_get(const struct device *dev, uint8_t cmd[SPIF_CMD_TABLE_NUM], uint32_t *cmd_num);
void spim_log_parser(const struct device *dev, uint32_t idx, uint32_t log_val);
/**
 * @}
 */
#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SPI_FILTER_H_ */
