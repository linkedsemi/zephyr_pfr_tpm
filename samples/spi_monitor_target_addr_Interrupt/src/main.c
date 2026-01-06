/*
 * Copyright (c) 2024 Linkedsemi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/sys/printk.h"
#include "zephyr/sys/util.h"
#include "zephyr/types.h"
#include <stdbool.h>
#include <stdint.h>
#if !defined(CONFIG_SPI_FILTER_LINKEDSEMI)
    #error no CONFIG_SPI_FILTER_LINKEDSEMI define
#endif

#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/spi_nor.h>
#include <zephyr/kernel.h>
#include <stdlib.h>
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/flash.h>
#include <reg_spi_filter.h>
#include <spi_filter.h>
#include <ls_soc_gpio.h>
#include <zephyr/logging/log.h>
#include <time.h>

LOG_MODULE_REGISTER(spi_monitor_01_003_005, LOG_LEVEL_INF);
extern int spi_nor_config_4byte_mode(const struct device *dev, bool en4b);

#if 0
#include "/home/htliu/zephyr_work/modules/lib/pfr_tpm/drivers/pfr/spi/spi_filter.h"
#else
#define SPIF_LOG_RAM_MAX_SIZE_U32     1024
#endif

#define FLASH_OFFSET MB(0)
#define FLASH_ERASE_SIZE 4096
#define FLASH_TEST_SIZE 4096

#define FLASH_OFFSET_MAIN 0x1000
#define FLASH_ERASE_SIZE_4K 0X1000
#define FLASH_ERASE_SIZE_MAIN 0x4000//16K 0X1000//4k
// #define start_addr_main  0x4000
#define START_ADDR_MAIN  0x4000
#define START_ADDR_0x40000  0x40000

#include <reg_ssi_type.h>
#include <platform.h>
reg_ssi_t *DWSPI3 = ((reg_ssi_t *)APP_DWSPI3_ADDR);
reg_ssi_t *DWSPI4 = ((reg_ssi_t *)APP_DWSPI4_ADDR);


__aligned(32) uint8_t buf_r[FLASH_TEST_SIZE];
__aligned(32) uint8_t buf_w[FLASH_TEST_SIZE];

#define TEST_START_ADDR MB(4)
#define TEST_TOTAL_SIZE MB(4)
#define TEST_STEP_SIZE KB(4)

void spi_monitor_target_addr(const struct device *const spifiltermaster,const struct device *const spifilter)
{

	spif_add_cmd(spifilter, 0x0c);

	static const uint8_t send_data_0[] = {
		0x0c,0x00,0x00,0x00,0x00
	};

	spif_dump_cmd_table(spifilter);
	spif_dump_rw_addr_privilege_table(spifilter);

	const struct device *spi_dev = spif_spi_dev(spifiltermaster);
	uint32_t freq = DT_PROP(DT_ALIAS(spiflash), spi_max_frequency);

	const struct spi_config spi_cfg = {
		.operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_TRANSFER_MSB,
		.frequency = freq,
		.cs = { .gpio = *(spif_spi_cs(spifiltermaster)), },
	};
	#if 1
	int ret=0;
	struct spi_buf tx_buf_0 = {
		.buf = (uint8_t*)send_data_0,
		.len = sizeof(send_data_0),
	};

	const struct spi_buf_set tx_set_0 = { &tx_buf_0, 1 };


	ret = spi_transceive(spi_dev, &spi_cfg, &tx_set_0, NULL);

	if (ret < 0) {
		LOG_ERR("send failed: %d",ret);
	}

}

int main(void)
{

	const struct device *const spifilter = DEVICE_DT_GET(DT_ALIAS(spifilter));
	bool check;

	linkedsemi_spi_filter_cold_reset(spifilter);
	// spif_dump_cmd_table(spifilter);
	// spif_dump_rw_addr_privilege_table(spifilter);
	spif_memset_addr_whitelist(spifilter, 1);
	spif_flash_size_set(spifilter, MB(32));

#if defined(CONFIG_SPI_FILTER_DMA_LOG)
	spif_dma_start(spifilter);
#endif

	spif_filter_enable(spifilter, true);
	check = spif_pinctrl_filter_mode_check(spifilter);
	__ASSERT_NO_MSG(check);

	#if 0
	spif_operation_mode_config(spifilter, true);
	printk("Switch to Filter mode (OPERATION_MODE=1)\n");
	#endif

	spif_passthrough_analog_mux_enable(spifilter, true);
	check = spif_pinctrl_passthrough_mode_check(spifilter);
	__ASSERT_NO_MSG(check);

	const struct device *const spifiltermaster = DEVICE_DT_GET(DT_ALIAS(spifiltermaster));
	spif_switch_to_master(spifiltermaster);
	check = spif_pinctrl_master_mode_check(spifiltermaster);
	__ASSERT_NO_MSG(check);


	const struct device *const flash_dev = DEVICE_DT_GET(DT_ALIAS(spiflash));

	const struct device *spi_dev = spif_spi_dev(spifiltermaster);
	uint32_t freq = DT_PROP(DT_ALIAS(spiflash), spi_max_frequency);

	uintptr_t base_address = DT_REG_ADDR(DT_ALIAS(spifilter));
	LOG_INF("SPIFilter base address: 0x%08lx", base_address);
	spif_target_addr_config(spifilter, 0x00,FLAG_TARGET_ADDR_32BIT,1);
	spi_monitor_target_addr(spifiltermaster,spifilter);

	return 0;
}
#endif
