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
#define START_ADDR_MAIN  0x4000
#define START_ADDR_0x40000  0x40000

static uint8_t data_write[16];
static uint8_t data_read[sizeof(data_write)]={0};

#include <reg_ssi_type.h>
#include <platform.h>
reg_ssi_t *DWSPI3 = ((reg_ssi_t *)APP_DWSPI3_ADDR);
reg_ssi_t *DWSPI4 = ((reg_ssi_t *)APP_DWSPI4_ADDR);


void configure_address_protection(const struct device *spifilter,uint32_t start_addr,uint32_t protect_size)
{
	spif_address_privilege_config(spifilter, FLAG_ADDR_PRIV_READ_SELECT, FLAG_ADDR_PRIV_DISABLE, start_addr, protect_size);
	spif_address_privilege_config(spifilter, FLAG_ADDR_PRIV_WRITE_SELECT, FLAG_ADDR_PRIV_DISABLE, start_addr, protect_size);
}

void spi_monitor_ip_lock0(const struct device *const spifiltermaster,const struct device *const spifilter,const struct device *const flash_dev)//table_250
{
	spif_reg_unlock(spifilter);
	uintptr_t base_address = DT_REG_ADDR(DT_ALIAS(spifilter));
	LOG_INF("SPIFilter base address: 0x%08lx\n", base_address);
	spif_cfg_t spif_cfg;
	spif_cfg.value = sys_read32(base_address+ SPIF_CFG);
	LOG_INF("spif_cfg.value0: 0x%08x\n", spif_cfg.value);
	spif_cfg.EN=0;
	spif_cfg.OPERATION_MODE =1;
	spif_cfg.ADDR_3B_4B_SEL =1;
	spif_cfg.BOW_CMD_SEL =1;
	spif_cfg.DMA_EN = 1;
	sys_write32(spif_cfg.value, base_address + SPIF_CFG);

	spif_cfg.value = sys_read32(base_address+ SPIF_CFG);
	LOG_INF("spif_cfg.value1: 0x%08x\n", spif_cfg.value);
}

void spi_monitor_ip_lock1(const struct device *const spifiltermaster,const struct device *const spifilter,const struct device *const flash_dev)//table_251
{
	spif_reg_lock(spifilter);
	uintptr_t base_address = DT_REG_ADDR(DT_ALIAS(spifilter));
	LOG_INF("SPIFilter base address: 0x%08lx\n", base_address);
	spif_cfg_t spif_cfg;
	spif_cfg.value = sys_read32(base_address+ SPIF_CFG);
	LOG_INF("spif_cfg.value0: 0x%08x\n", spif_cfg.value);
	spif_cfg.EN=0;
	spif_cfg.OPERATION_MODE =1;
	spif_cfg.ADDR_3B_4B_SEL =1;
	spif_cfg.BOW_CMD_SEL =1;
	spif_cfg.DMA_EN = 1;
	sys_write32(spif_cfg.value, base_address + SPIF_CFG);

	spif_cfg.value = sys_read32(base_address+ SPIF_CFG);
	LOG_INF("spif_cfg.value1: 0x%08x\n", spif_cfg.value);

}

void configure_blacklist_range(uint8_t lowest_bit,uintptr_t base_address) {

	sys_write32(lowest_bit, base_address + SPIF_BCMD_RANGE);
	LOG_INF("Configured blacklist range: [%d:%d]", lowest_bit, 7);
}
void random_add_cmds_unique(const struct device *spifilter,uint8_t num)
{
	size_t num_cmds = 256;
	int random_indices[num];  // Store the command indices randomly generated for num times
	bool used[SPIF_CMD_TABLE_NUM] = { false };  // Mark the used indexes

	srand(k_uptime_get());

	for (int i = 0; i < num; i++) {
		int idx =0;
		do {
			//Generate a random index within the range of [0, num_cmds - 1]
			idx = rand() % (num_cmds);
		} while (used[idx]||idx ==0x38||idx==0x3e||idx==0xbb||idx==0xbc||idx==0xc2||idx==0xeb||idx==0xec); // If the index has been used, regenerate it.

		used[idx] = true; // Mark this index as having been used
		random_indices[i] = idx;
		LOG_INF("Selected unique index: 0x%02x", idx);
		// LOG_INF("Selected unique index: %d", idx);

		// Add the corresponding command to the SPI command table
		int ret=spif_add_cmd(spifilter, random_indices[i]);
		if (ret < 0) LOG_ERR("Failed to add command 0x%02x, error %d",random_indices[i], ret);
	}
}

//table_253
void spi_monitor_cmd_blacklist(const struct device *const spifiltermaster,const struct device *const spifilter,const struct device *const flash_dev,const struct device *spi_dev,uintptr_t base_address,uint8_t num_of_blacklist,uint8_t lowest_bit,uint32_t freq)
{
	static uint8_t send_data_blacklist[256];
	for (int i=0; i<sizeof(send_data_blacklist); i++) {
		if (i==0x38||i==0x3e||i==0xbb||i==0xbc||i==0xc2||i==0xeb||i==0xec) {
			send_data_blacklist[i] = 0;
		}else {
			send_data_blacklist[i] = i;
		}
	}

	spif_cfg_t spif_cfg;
	spif_cfg.value = sys_read32(base_address+ SPIF_CFG);
	LOG_INF("spif_cfg.value0: 0x%08x", spif_cfg.value);
	spif_cfg.BOW_CMD_SEL =1;
	sys_write32(spif_cfg.value, base_address + SPIF_CFG);

	spif_cfg.value = sys_read32(base_address+ SPIF_CFG);
	if (spif_cfg.BOW_CMD_SEL) {
		LOG_INF("set blacklist  ok ");
	}else {
		LOG_ERR("set blacklist failed! ");
	}

	configure_blacklist_range(lowest_bit,base_address);
	random_add_cmds_unique(spifilter,num_of_blacklist);
	spif_dump_cmd_table(spifilter);
	// spif_add_cmd(spifilter, 0x9f);

	const struct spi_config spi_cfg = {
		.operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_TRANSFER_MSB,
		.frequency = freq,
		.cs = { .gpio = *(spif_spi_cs(spifiltermaster)), },
	};

	uint8_t rx[4] = {0};
	int ret=0;
	for (int j=0; j<sizeof(send_data_blacklist); j++) {
		uint8_t tx =send_data_blacklist[j];
		struct spi_buf tx_buf_0 = {
			.buf = &tx,
			.len = 1,
		};

		struct spi_buf rx_buf_0 = {
			.buf = &rx,
			.len = sizeof(rx),
		};

		const struct spi_buf_set tx_set_0 = { &tx_buf_0, 1 };
		const struct spi_buf_set rx_set_0 = { &rx_buf_0, 1 };

		ret = spi_transceive(spi_dev, &spi_cfg, &tx_set_0, &rx_set_0);
		// test_spif();
		if (ret < 0) {
			// LOG_ERR("CMD 0x%02X send failed: %d", tx, ret);
			LOG_ERR("send failed: %d",ret);
		}else {
			LOG_INF("tx_cmd: 0x%02X",tx);
			LOG_INF("rx");
			for (int i=0; i<sizeof(rx); i++) {
				printk("0x%02X ",rx[i]);
			}
			printk("\n");

		}
	}

}

int main(void)
{
	const struct device *const spifilter = DEVICE_DT_GET(DT_ALIAS(spifilter));
	bool check;
	linkedsemi_spi_filter_cold_reset(spifilter);
	spif_memset_addr_whitelist(spifilter, 1);
	spif_flash_size_set(spifilter, MB(32));
	// spif_dump_cmd_table(spifilter);
	// spif_dump_rw_addr_privilege_table(spifilter);

#if defined(CONFIG_SPI_FILTER_DMA_LOG)
	spif_dma_start(spifilter);
#endif

	spif_filter_enable(spifilter, true);
	check = spif_pinctrl_filter_mode_check(spifilter);
	__ASSERT_NO_MSG(check);

	// spif_operation_mode_config(spifilter, true);
	// printk("Switch to Filter mode (OPERATION_MODE=1)\n");

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

	for (int i=0; i<sizeof(data_write);i++) {data_write[i]=i+1;}

	uintptr_t base_address = DT_REG_ADDR(DT_ALIAS(spifilter));
	LOG_INF("SPIFilter base address: 0x%08lx", base_address);

	// spi_monitor_ip_lock0(spifiltermaster,spifilter,flash_dev);//table_250
	// spi_monitor_ip_lock1(spifiltermaster,spifilter,flash_dev);//table_251
	spi_monitor_cmd_blacklist(spifiltermaster,spifilter,flash_dev,spi_dev,base_address,3,0,freq);//table_253

	LOG_INF("spi_monitor_cmd_blacklist");


	return 0;
}
