/*
 * Copyright (c) 2024 Linkedsemi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/sys/printk.h"
#include "zephyr/sys/util.h"
#include "zephyr/toolchain.h"
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
#include <reg_ssi_type.h>
#include <platform.h>
#include <soc.h>

LOG_MODULE_REGISTER(spi_monitor_high_bit_ignore, LOG_LEVEL_INF);

#if 0
#include "/home/htliu/zephyr_work/modules/lib/pfr_tpm/drivers/pfr/spi/spi_filter.h"
#else
#define SPIF_LOG_RAM_MAX_SIZE_U32     1024
#endif

#define FLASH_ERASE_SIZE_MAIN 0x4000//16K 0X1000//4k
#define START_ADDR_0x40000  0x40000


int  test_spif(void)
{
	do {
		uint8_t id[3] = { 0 };
		const struct device *const flash_dev = DEVICE_DT_GET(DT_ALIAS(spiflash));
		// spi_nor_re_init(flash_dev);
		int rc = flash_read_jedec_id(flash_dev, id);
		DELAY_MS(10);
		if (rc == 0) {

			printk("jedec-id = [%02x %02x %02x];\n", id[0], id[1], id[2]);
		} else {
		printk("JEDEC ID read failed: %d\n", rc);
		}
	} while(0);

	return 0;
}



void clear_cmd_whitelist(const struct device *const spifilter)
{
	uint32_t cmd_num = 0;
	uint8_t cmd_table[SPIF_CMD_TABLE_NUM]={0};
	spif_get_cmd_table(spifilter, cmd_table, &cmd_num);
	DEV_INF(spifilter,"find %u num cmd:", cmd_num);
	for (int i = 0; i < SPIF_CMD_TABLE_NUM; i++) {
		if (cmd_table[i] != 0 && cmd_num>0) {
			spif_remove_cmd(spifilter, cmd_table[i]);
			DEV_INF(spifilter,"  pos %d: cmd 0x%02x", i, cmd_table[i]);
			cmd_num--;
		}
	}
}

void configure_address_protection(const struct device *spifilter,uint32_t start_addr,uint32_t protect_size)
{
	// Set address protection
	spif_address_privilege_config(spifilter, FLAG_ADDR_PRIV_READ_SELECT, FLAG_ADDR_PRIV_DISABLE, start_addr, protect_size);
	spif_address_privilege_config(spifilter, FLAG_ADDR_PRIV_WRITE_SELECT, FLAG_ADDR_PRIV_DISABLE, start_addr, protect_size);
}


void spi_monitor_high_bit(const struct device *const spifiltermaster,const struct device *const spifilter)
{

	spif_add_cmd(spifilter, 0x21);

	static const uint8_t send_data_0[] = {
		0x21,0xf0,0x04,0x00,0x00
	};

	configure_address_protection(spifilter,START_ADDR_0x40000,FLASH_ERASE_SIZE_MAIN);

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
		DEV_ERR(spi_dev,"send failed: %d",ret);
	}else{
		DEV_INF(spi_dev,"addr 0xf0040000 send ok");
	}


#if 1

	static const uint8_t send_data[] = {
		0x21,0xf0,0x00,0x00,0x00
	};
	struct spi_buf tx_buf = {
		.buf = (uint8_t*)send_data,
		.len = sizeof(send_data),
	};

	const struct spi_buf_set tx_set = { &tx_buf, 1 };

	ret = spi_transceive(spi_dev, &spi_cfg, &tx_set, NULL);

	if (ret < 0) {

		LOG_ERR("send failed: %d",ret);
	}else{
		DEV_INF(spi_dev,"addr 0xf0000000 send ok");
	}
#endif

#else

	for(int m=0;m<sizeof(send_data);m++){
		uint8_t tx =send_data[m];
		struct spi_buf tx_buf = {
			.buf = &tx,
			.len = sizeof(tx),
		};
		const struct spi_buf_set tx_set = { &tx_buf, 1 };
		int ret = spi_transceive(spi_dev, &spi_cfg, &tx_set, NULL);
		if (ret < 0) {
			LOG_ERR("data 0x%02X send failed: %d", tx, ret);
		}
	}

#endif

}

int main(void)
{

	const struct device *const spifilter = DEVICE_DT_GET(DT_ALIAS(spifilter));
	bool check;

	linkedsemi_spi_filter_cold_reset(spifilter);
	spif_memset_addr_whitelist(spifilter, 1);
	spif_flash_size_set(spifilter, MB(32));
	#if 0
	clear_cmd_whitelist(spifilter);
	#endif
	// spif_dump_cmd_table(spifilter);
	// spif_dump_rw_addr_privilege_table(spifilter);

#if defined(CONFIG_SPI_FILTER_DMA_LOG)
	spif_dma_start(spifilter);
#endif

	spif_filter_enable(spifilter, true);
	check = spif_pinctrl_filter_mode_check(spifilter);
	__ASSERT_NO_MSG(check);

	// spif_operation_mode_config(spifilter, true);//filter mode
	// printk("Switch to Filter mode (OPERATION_MODE=1)\n");

	spif_passthrough_analog_mux_enable(spifilter, true);
	check = spif_pinctrl_passthrough_mode_check(spifilter);
	__ASSERT_NO_MSG(check);

	const struct device *const spifiltermaster = DEVICE_DT_GET(DT_ALIAS(spifiltermaster));
	spif_switch_to_master(spifiltermaster);
	check = spif_pinctrl_master_mode_check(spifiltermaster);
	__ASSERT_NO_MSG(check);

	// spif_switch_to_master(spifilter);
	// check = spif_pinctrl_master_mode_check(spifilter);
	// __ASSERT_NO_MSG(check);

	spi_monitor_high_bit(spifiltermaster,spifilter);

	return 0;
}

