/*
 * Copyright (c) 2024 Linkedsemi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/sys/util.h"
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

LOG_MODULE_REGISTER(spi_monitor_01_003_003, LOG_LEVEL_INF);
#if 0
#include "/home/htliu/zephyr_work/modules/lib/pfr_tpm/drivers/pfr/spi/spi_filter.h"
#else
#define SPIF_LOG_RAM_MAX_SIZE_U32     1024
#endif

#if 1//01-003-0003
#include <time.h>
#define  Quad_In_Page_Program 0xc2
static const uint8_t legal_cmds[] = {
	SPI_NOR_CMD_RESET_EN,SPI_NOR_CMD_RESET_MEM,SPI_NOR_CMD_NOP,
	CMD_ERASE_4KB,CMD_ERASE_32KB,CMD_ERASE_64KB,SPI_NOR_CMD_CE,
	SPI_NOR_CMD_BULKE,SPI_NOR_CMD_WREN,SPI_NOR_CMD_WRDI,
	CMD_4BYTE_ERASE_4KB,CMD_4BYTE_ERASE_32KB,CMD_4BYTE_ERASE_64KB,
	CMD_4BYTE_MODE_ENTER,CMD_4BYTE_MODE_EXIT,SPI_NOR_CMD_WRSR,SPI_NOR_CMD_WRSR2,SPI_NOR_CMD_WRSR3,
	CMD_4BYTE_READ_EXTENDED_ADDRESS,CMD_4BYTE_WRITE_EXTENDED_ADDRESS,
	SPI_NOR_CMD_RDID,SPI_NOR_CMD_RDSFDP,SPI_NOR_CMD_RDSR,SPI_NOR_CMD_RDSR2,SPI_NOR_CMD_RDSR3,
	CMD_READ,CMD_FAST_READ,SPI_NOR_CMD_DREAD,CMD_READ_QUAD_DATA,CMD_READ_QUAD_ADDR_QUAD_DATA,
	CMD_PAGE_PROGRAM,SPI_NOR_CMD_PP_1_1_4,Quad_In_Page_Program,
	CMD_QUAD_SPI_MODE_EXIT,CMD_4BYTE_PAGE_PROGRAM,SPI_NOR_CMD_PP_1_1_4_4B,
	CMD_4BYTE_READ,CMD_4BYTE_FAST_READ,SPI_NOR_CMD_DREAD_4B,CMD_4BYTE_READ_QUAD_DATA,
};

void random_add_cmds_unique(const struct device *spifilter,uint8_t num)
{
	size_t num_cmds = sizeof(legal_cmds) / sizeof(legal_cmds[0]);
	int random_indices[num];  // Store the command indices randomly generated for num times
	bool used[SPIF_CMD_TABLE_NUM] = { false };  // Mark the used indexes

	srand(k_uptime_get());

	for (int i = 0; i < num; i++) {
		int idx =0;
		do {
			//Generate a random index within the range of [0, num_cmds - 1]
			idx = rand() % (num_cmds);
		} while (used[idx]); // If the index has been used, regenerate it.

		used[idx] = true; // Mark this index as having been used
		random_indices[i] = idx;
		LOG_INF("Selected unique index: %d", idx);

		// Add the corresponding command to the SPI command table
		int ret=spif_add_cmd(spifilter, legal_cmds[random_indices[i]]);
		if (ret < 0) LOG_ERR("Failed to add command 0x%02x, error %d",legal_cmds[random_indices[i]], ret);
	}
}

int main(void)
{
	const struct device *const spifilter = DEVICE_DT_GET(DT_ALIAS(spifilter));
	bool check;
	linkedsemi_spi_filter_cold_reset(spifilter);
	spif_dump_cmd_table(spifilter);
	spif_dump_rw_addr_privilege_table(spifilter);
	spif_memset_addr_whitelist(spifilter, 1);

	uint32_t cmd_num = 0;
	uint8_t cmd_table[SPIF_CMD_TABLE_NUM]={0};
	spif_get_cmd_table(spifilter, cmd_table, &cmd_num);
	LOG_INF("find %u num cmd:", cmd_num);
	for (int i = 0; i < SPIF_CMD_TABLE_NUM; i++) {
		if (cmd_table[i] != 0 && cmd_num>0) {
			spif_remove_cmd(spifilter, cmd_table[i]);
			LOG_INF("  pos %d: cmd 0x%02x", i, cmd_table[i]);
			cmd_num--;
		}
	}

	random_add_cmds_unique(spifilter,16);
	#if 0//front 16 data add to cmd
	for (int i = 0; i < 16; i++) {
		int ret = spif_add_cmd(spifilter, legal_cmds[i]);
		if (ret < 0) LOG_ERR("Failed to add command 0x%02x, error %d\n", legal_cmds[i], ret);
	}
	#endif//front 16 data add to cmd

	spif_dump_cmd_table(spifilter);
	spif_dump_rw_addr_privilege_table(spifilter);
#if defined(CONFIG_SPI_FILTER_DMA_LOG)
	spif_dma_start(spifilter);
#endif
	spif_filter_enable(spifilter, true);
	check = spif_pinctrl_filter_mode_check(spifilter);
	__ASSERT_NO_MSG(check);

	spif_passthrough_analog_mux_enable(spifilter, true);
	check = spif_pinctrl_passthrough_mode_check(spifilter);
	__ASSERT_NO_MSG(check);

	do {
		const struct device *const spifiltermaster = DEVICE_DT_GET(DT_ALIAS(spifiltermaster));
		spif_switch_to_master(spifiltermaster);
		check = spif_pinctrl_master_mode_check(spifiltermaster);
		__ASSERT_NO_MSG(check);
	} while (0);

	LOG_INF("--- Monitor Mode Test Start ----");

	uint32_t cmd_cnt = ARRAY_SIZE(legal_cmds);
	LOG_INF("Legal CMD count = %d", cmd_cnt);

	const struct device *const spifiltermaster = DEVICE_DT_GET(DT_ALIAS(spifiltermaster));

	const struct device *spi_dev = spif_spi_dev(spifiltermaster);

	uint32_t freq = DT_PROP(DT_ALIAS(spiflash), spi_max_frequency);

	const struct spi_config spi_cfg = {
		.operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_TRANSFER_MSB,
		.frequency = freq,
		.cs = { .gpio = *(spif_spi_cs(spifiltermaster)), },
	};


	for (uint32_t i = 0; i < cmd_cnt; i++) {

		uint8_t tx = legal_cmds[i];
		uint8_t rx[4] = {0};

		struct spi_buf tx_buf = {
			.buf = &tx,
			.len = 1,
		};
		struct spi_buf rx_buf = {
			.buf = &rx,
			.len = sizeof(rx),
		};
		const struct spi_buf_set tx_set = { &tx_buf, 1 };
		const struct spi_buf_set rx_set = { &rx_buf, 1 };

		LOG_INF("Send CMD = 0x%02X ...", tx);

		int ret = spi_transceive(spi_dev, &spi_cfg, &tx_set, &rx_set);
		if (ret < 0) {
			LOG_ERR("CMD 0x%02X send failed: %d", tx, ret);
		} else {
			LOG_INF("CMD 0x%02X OK, RX = ", tx);
			for (int j = 0; j < sizeof(rx); j++) {
				printk("0x%02X ", rx[j]);
			}
			printk("\n");
		}

		k_msleep(10);
	}

	LOG_INF("---- Monitor Mode Test Done ----");

	return 0;
}
#endif//01-003-0003
