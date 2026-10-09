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
#include <zephyr/drivers/spi_dw_api.h>
#include <reg_spi_filter.h>
#include <spi_filter.h>
#include <ls_soc_gpio.h>
#include <zephyr/logging/log.h>
#include <time.h>

LOG_MODULE_REGISTER(spi_monitor_sample, LOG_LEVEL_INF);
extern int spi_nor_config_4byte_mode(const struct device *dev, bool en4b);

#if 0
#include "/home/htliu/zephyr_work/modules/lib/pfr_tpm/drivers/pfr/spi/spi_filter.h"
#else
#define SPIF_LOG_RAM_MAX_SIZE_U32     1024
#endif

#define DELAY_MS(ms) k_msleep(ms)

static uint8_t data_write[16];

static uint8_t data_read[sizeof(data_write)+3]={0};

// static const struct device *const spi_dev = DEVICE_DT_GET(DT_PARENT(DT_ALIAS(spiflash)));

static int log_calib_value(const char *stage, const struct device *spi_dev, uint8_t slave)
{
	struct spi_dw_timing_calib_value value;
	int rc;

	if (!device_is_ready(spi_dev)) {
		LOG_ERR("[%s] SPI device not ready", stage);
		return -1;
	}

	rc = spi_dw_timing_calibration_value(spi_dev, slave, &value);
	if (rc != 0) {
		LOG_ERR("[%s] calib value query failed: %d", stage, rc);
		return -1;
	}

	LOG_INF("[%s] calib value: slave=%u rx_sample_dly=%u",
		stage, slave, value.rx_sample_dly);

	return 0;
}


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

int  test_spifiltermaster2(void)
{
	do {
		uint8_t id[3] = { 0 };
		const struct device *const flash_dev2 = DEVICE_DT_GET(DT_ALIAS(spiflash2));
		// spi_nor_re_init(flash_dev);
		int rc = flash_read_jedec_id(flash_dev2, id);
		DELAY_MS(10);
		if (rc == 0) {

			LOG_INF("jedec-id = [%02x %02x %02x];\n", id[0], id[1], id[2]);
		} else {
		LOG_INF("JEDEC ID read failed: %d\n", rc);
		}
	} while(0);

	return 0;
}

int main(void)
{
	LOG_INF("spi_master_tranceive_start");
	// const struct device *const spifilter = DEVICE_DT_GET(DT_ALIAS(spifilter));
	bool check;
	int re;
	const struct device *const spifiltermaster = DEVICE_DT_GET(DT_ALIAS(spifilter));
	spif_switch_to_master(spifiltermaster);
	check = spif_pinctrl_master_mode_check(spifiltermaster);
	__ASSERT_NO_MSG(check);

	const struct device *const flash_dev = DEVICE_DT_GET(DT_ALIAS(spiflash));
	test_spif();
	log_calib_value("after spif3 jedec", DEVICE_DT_GET(DT_PARENT(DT_ALIAS(spiflash))),
			DT_REG_ADDR(DT_ALIAS(spiflash)));
	re=flash_read(flash_dev, 0x00, data_read, sizeof(data_read));
	if (re != 0) {
		LOG_ERR("flash_read failed");
		return -1;
	}
	for (int i = 0; i < sizeof(data_read); i++) {
		if (i%16==0) {
			LOG_RAW("\n");
		}
		LOG_RAW("%02X ", data_read[i]);// act
	}
	LOG_RAW("\n");
	log_calib_value("after spif3 read", DEVICE_DT_GET(DT_PARENT(DT_ALIAS(spiflash))),
			DT_REG_ADDR(DT_ALIAS(spiflash)));

	/*spif3切成filter*/
	spif_switch_to_filter(spifiltermaster);
	check = spif_pinctrl_filter_mode_check(spifiltermaster);
	__ASSERT_NO_MSG(check);

	const struct device *const spifiltermaster2 = DEVICE_DT_GET(DT_ALIAS(spifiltermaster2));
	spif_switch_to_master(spifiltermaster2);
	check = spif_pinctrl_master_mode_check(spifiltermaster2);
	__ASSERT_NO_MSG(check);

	test_spifiltermaster2();
	log_calib_value("after spif4 jedec", DEVICE_DT_GET(DT_PARENT(DT_ALIAS(spiflash2))),
			DT_REG_ADDR(DT_ALIAS(spiflash2)));

	const struct device *const flash_dev2 = DEVICE_DT_GET(DT_ALIAS(spiflash2));
	re=flash_read(flash_dev2, 0x00, data_read, sizeof(data_read));
	if (re != 0) {
		LOG_ERR("flash_read failed");
		return -1;
	}
	for (int i = 0; i < sizeof(data_read); i++) {
		if (i%16==0) {
			LOG_RAW("\n");
		}
		LOG_RAW("%02X ", data_read[i]);// act
	}
	log_calib_value("after spif4 read", DEVICE_DT_GET(DT_PARENT(DT_ALIAS(spiflash2))),
			DT_REG_ADDR(DT_ALIAS(spiflash2)));
	LOG_INF("all test pass");
	return 0;
}
