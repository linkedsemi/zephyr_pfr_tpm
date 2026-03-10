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

LOG_MODULE_REGISTER(spi_monitor_warm_reset, LOG_LEVEL_DBG);
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


int  test_spif(void)
{
	do {
		uint8_t id[3] = { 0 };
		const struct device *const flash_dev = DEVICE_DT_GET(DT_ALIAS(spiflash));
		// spi_nor_re_init(flash_dev);
		int rc = flash_read_jedec_id(flash_dev, id);
		DELAY_MS(10);
		if (rc == 0) {

			LOG_INF("jedec-id = [%02x %02x %02x];\n", id[0], id[1], id[2]);
		} else {
		LOG_INF("JEDEC ID read failed: %d\n", rc);
		}
	} while(0);

	return 0;
}

void spi_monitor_jedec_id(const struct device *const spifiltermaster,const struct device *const spifilter,uint32_t freq)
{

	static const uint8_t send_data_0[] = {
		0x9f,
	};
	uint8_t rx[4] = {0};

	const struct device *spi_dev = spif_spi_dev(spifiltermaster);

	const struct spi_config spi_cfg = {
		.operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_TRANSFER_MSB,
		.frequency = freq,
		.cs = { .gpio = *(spif_spi_cs(spifiltermaster)), },
	};
	LOG_INF("send_freq=%u",freq);
	int ret=0;
	struct spi_buf tx_buf_0 = {
		.buf = (uint8_t*)send_data_0,
		.len = sizeof(send_data_0),
	};

	struct spi_buf rx_buf = {
		.buf = &rx,
		.len = sizeof(rx),
	};

	const struct spi_buf_set tx_set_0 = { &tx_buf_0, 1 };
	const struct spi_buf_set rx_set = { &rx_buf, 1 };

	ret = spi_transceive(spi_dev, &spi_cfg, &tx_set_0, &rx_set);

	if (ret < 0) {
		LOG_ERR("send failed: %d",ret);
	}else {
		for (int i=0; i<sizeof(rx); i++) {
			printk("0x%02X ",rx[i]);
		}
		printk("\n");

	}

}

void spi_monitor_send_read_addr(const struct device *const spifiltermaster,const struct device *const spifilter,uint32_t freq)
{

	static const uint8_t send_data_0[] = {
		0x03,0x00,0x40,0x00
	};
	uint8_t rx[8] = {0};

	const struct device *spi_dev = spif_spi_dev(spifiltermaster);

	const struct spi_config spi_cfg = {
		.operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_TRANSFER_MSB,
		.frequency = freq,
		.cs = { .gpio = *(spif_spi_cs(spifiltermaster)), },
	};
	LOG_INF("send_freq=%u",freq);
	int ret=0;
	struct spi_buf tx_buf_0 = {
		.buf = (uint8_t*)send_data_0,
		.len = sizeof(send_data_0),
	};

	struct spi_buf rx_buf = {
		.buf = &rx,
		.len = sizeof(rx),
	};

	const struct spi_buf_set tx_set_0 = { &tx_buf_0, 1 };
	const struct spi_buf_set rx_set = { &rx_buf, 1 };

	ret = spi_transceive(spi_dev, &spi_cfg, &tx_set_0, &rx_set);

	if (ret < 0) {
		LOG_ERR("send failed: %d",ret);
	}else {
		for (int i=0; i<sizeof(rx); i++) {
			printk("0x%02X ",rx[i]);
		}
		printk("\n");

	}

}
static void spif_dma_callback(void *callback_arg)
{
    const struct device *dev = (const struct device *)callback_arg;
    LOG_INF("DMA callback triggered for device: %s", dev->name);
}
#include <string.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/reboot.h>

void configure_address_protection(const struct device *spifilter,uint32_t start_addr,uint32_t protect_size)
{
	spif_address_privilege_config(spifilter, FLAG_ADDR_PRIV_READ_SELECT, FLAG_ADDR_PRIV_DISABLE, start_addr, protect_size);
	spif_address_privilege_config(spifilter, FLAG_ADDR_PRIV_WRITE_SELECT, FLAG_ADDR_PRIV_DISABLE, start_addr, protect_size);
}


#if 1
static int cmd_spi_test(const struct shell *sh, size_t argc, char **argv)
{

	const struct device *const spifilter = DEVICE_DT_GET(DT_ALIAS(spifilter));
	bool check;
	const struct device *const spifiltermaster = DEVICE_DT_GET(DT_ALIAS(spifiltermaster));

	if (!device_is_ready(spifilter) || !device_is_ready(spifiltermaster)) {
		shell_error(sh, "SPI Filter devices not ready");
		return -ENODEV;
	}

	shell_print(sh, "Starting cmd_spi_test ...");

	linkedsemi_spi_filter_cold_reset(spifilter);

	spif_flash_size_set(spifilter, MB(32));

	#if defined(CONFIG_SPI_FILTER_DMA_LOG)
	linkedsemi_spif_register_callback(spifilter, (void*)spif_dma_callback);
	spif_dma_start(spifilter);
	#endif

	spif_filter_enable(spifilter, true);
	check = spif_pinctrl_filter_mode_check(spifilter);
	__ASSERT_NO_MSG(check);

	// spif_operation_mode_config(spifilter, true);//if change to filter
	// printk("Switch to Filter mode (OPERATION_MODE=1)\n");

	spif_passthrough_analog_mux_enable(spifilter, true);
	spif_switch_to_master(spifiltermaster);

	const struct device *const flash_dev = DEVICE_DT_GET(DT_ALIAS(spiflash));

	uint32_t freq = DT_PROP(DT_ALIAS(spiflash), spi_max_frequency);

	spif_add_cmd(spifilter, 0x03);
	configure_address_protection(spifilter,0x4000,FLASH_ERASE_SIZE_MAIN);

	spif_dump_cmd_table(spifilter);
	spif_dump_rw_addr_privilege_table(spifilter);

	spi_monitor_jedec_id(spifiltermaster,spifilter,freq);
	spi_monitor_send_read_addr(spifiltermaster,spifilter,freq);
	test_spif();
	test_spif();
	shell_print(sh, "cmd_spi_test Completed.");
	return 0;
}

static int cmd_spi_test1(const struct shell *sh, size_t argc, char **argv)
{

	const struct device *const spifilter = DEVICE_DT_GET(DT_ALIAS(spifilter));
	const struct device *const spifiltermaster = DEVICE_DT_GET(DT_ALIAS(spifiltermaster));

	if (!device_is_ready(spifilter) || !device_is_ready(spifiltermaster)) {
		shell_error(sh, "SPI Filter devices not ready");
		return -ENODEV;
	}

	shell_print(sh, "Starting cmd_spi_test1...");

	linkedsemi_spi_filter_cold_reset(spifilter);

	spif_flash_size_set(spifilter, MB(32));

	#if defined(CONFIG_SPI_FILTER_DMA_LOG)
	linkedsemi_spif_register_callback(spifilter, (void*)spif_dma_callback);
	spif_dma_start(spifilter);
	#endif

	spif_filter_enable(spifilter, true);
	spif_passthrough_analog_mux_enable(spifilter, true);
	spif_switch_to_master(spifiltermaster);


	uint32_t freq = DT_PROP(DT_ALIAS(spiflash), spi_max_frequency);

	spif_dump_cmd_table(spifilter);
	spif_dump_rw_addr_privilege_table(spifilter);

	spi_monitor_jedec_id(spifiltermaster, spifilter, freq);

	spi_monitor_send_read_addr(spifiltermaster,spifilter,freq);
	test_spif();
	shell_print(sh, "cmd_spi_test1 Completed.");
	return 0;
}

static int cmd_spi_test2(const struct shell *sh, size_t argc, char **argv)
{

	const struct device *const spifilter = DEVICE_DT_GET(DT_ALIAS(spifilter));
	const struct device *const spifiltermaster = DEVICE_DT_GET(DT_ALIAS(spifiltermaster));

	if (!device_is_ready(spifilter) || !device_is_ready(spifiltermaster)) {
		shell_error(sh, "SPI Filter devices not ready");
		return -ENODEV;
	}

	shell_print(sh, "Starting cmd_spi_test2...");

	spif_flash_size_set(spifilter, MB(32));

	#if defined(CONFIG_SPI_FILTER_DMA_LOG)
	linkedsemi_spif_register_callback(spifilter, (void*)spif_dma_callback);
	spif_dma_start(spifilter);
	#endif

	spif_filter_enable(spifilter, true);
	spif_passthrough_analog_mux_enable(spifilter, true);
	spif_switch_to_master(spifiltermaster);

	uint32_t freq = DT_PROP(DT_ALIAS(spiflash), spi_max_frequency);
	spif_dump_cmd_table(spifilter);
	spif_dump_rw_addr_privilege_table(spifilter);
	spi_monitor_jedec_id(spifiltermaster,spifilter,freq);
	spi_monitor_send_read_addr(spifiltermaster,spifilter,freq);
	test_spif();
	shell_print(sh, "cmd_spi_test2 Completed.");
	return 0;
}
static int cmd_spi_dump(const struct shell *sh, size_t argc, char **argv)
{

	const struct device *const spifilter = DEVICE_DT_GET(DT_ALIAS(spifilter));
	const struct device *const spifiltermaster = DEVICE_DT_GET(DT_ALIAS(spifiltermaster));

	if (!device_is_ready(spifilter) || !device_is_ready(spifiltermaster)) {
		shell_error(sh, "SPI Filter devices not ready");
		return -ENODEV;
	}

	shell_print(sh, "Starting cmd_spi_dump...");

	spif_dump_cmd_table(spifilter);
	spif_dump_rw_addr_privilege_table(spifilter);

	shell_print(sh, "cmd_spi_dump Completed.");
	return 0;
}


SHELL_CMD_REGISTER(spi_test, NULL, "Run SPI Filter and JEDEC ID monitor test", cmd_spi_test);
SHELL_CMD_REGISTER(spi_test1, NULL, "Run SPI Filter and JEDEC ID monitor test", cmd_spi_test1);
SHELL_CMD_REGISTER(spi_test2, NULL, "Run SPI Filter and JEDEC ID monitor test", cmd_spi_test2);
SHELL_CMD_REGISTER(spi_dump, NULL, "Run SPI Filter and JEDEC ID monitor test", cmd_spi_dump);
#endif

int main(void)
{
	k_msleep(1000);
	LOG_INF("3.10.0");
	while (1) {
		k_sleep(K_FOREVER);
	}

	return 0;
}
