/*
 * Copyright (c) 2024 Linkedsemi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ls_soc_gpio_def.h"
#include "platform.h"
#include "zephyr/sys/printk.h"
#include <stdint.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/spi_nor.h>
#include <zephyr/kernel.h>
#include <stdlib.h>
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/flash.h>
#include <ls_soc_gpio.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <reg_spid.h>
#include <spid_linkedsemi.h>
#include <reg_spi_filter.h>
#include <spi_filter.h>
#include <ls_soc_gpio.h>
#include <zephyr/drivers/watchdog.h>
#include <ls_soc_gpio.h>

LOG_MODULE_REGISTER(tpm_back_pressure_wwdt, LOG_LEVEL_INF);

#define tpm_spis2  0x4000e000
#define cfg_base_reg  0x4000e000

void spid_callback(const struct device *dev, uint32_t callback_idx, void *user_data, void *drv_data) {
	uint32_t stat = sys_read32(SPID_TPM_ACCESS_0); //
	LOG_INF("Callback: TPM_ACCESS_X register state: 0x%x", stat);

	if ((stat & (1 << callback_idx)) == 0) {
		LOG_ERR("Error: Bit %d not set correctly!", callback_idx);
	}
}

void spi_monitor_target_addr_send(const struct device *const spifiltermaster)
{
	static const uint8_t send_data_0[] = {
		0x03,0xd4,0x00,0x00,0x98,0x76,0x54,0x32,0x10
	};
	uint8_t rx[9] = {0};
	const struct device *spi_dev = spif_spi_dev(spifiltermaster);
	// uint32_t freq = DT_PROP(DT_ALIAS(spiflash), spi_max_frequency);
	uint32_t freq = 10*1000000;
	const struct spi_config spi_cfg = {
		.operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_TRANSFER_MSB,
		.frequency = freq,
		.cs = { .gpio = *(spif_spi_cs(spifiltermaster)), },
	};

	int ret=0;
	struct spi_buf tx_buf_0 = {
		.buf = (uint8_t*)send_data_0,
		.len = sizeof(send_data_0),
	};
	struct spi_buf rx_buf_0 = {
			.buf = &rx,
			.len = sizeof(rx),
		};
	const struct spi_buf_set tx_set_0 = { &tx_buf_0, 1 };
	const struct spi_buf_set rx_set_0 = { &rx_buf_0, 1 };

	ret = spi_transceive(spi_dev, &spi_cfg, &tx_set_0, &rx_set_0);

	if (ret < 0) {
		LOG_ERR("send failed: %d",ret);
	}else {

		for (int j = 0; j < sizeof(rx); j++) {
			printk("0x%02X ", rx[j]);
		}
		printk("\n");

	}

}
void spi_monitor_target_addr(const struct device *const spifiltermaster)
{
	static const uint8_t send_data_0[] = {
		0x83,0xd4,0x00,0x00,0x00,0xff,0xff,0xff,0xff
	};
	uint8_t rx[9] = {0};
	const struct device *spi_dev = spif_spi_dev(spifiltermaster);
	// uint32_t freq = DT_PROP(DT_ALIAS(spiflash), spi_max_frequency);
	uint32_t freq = 10*1000000;
	const struct spi_config spi_cfg = {
		.operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_TRANSFER_MSB,
		.frequency = freq,
		.cs = { .gpio = *(spif_spi_cs(spifiltermaster)), },
	};

	int ret=0;
	struct spi_buf tx_buf_0 = {
		.buf = (uint8_t*)send_data_0,
		.len = sizeof(send_data_0),
	};
	struct spi_buf rx_buf_0 = {
			.buf = &rx,
			.len = sizeof(rx),
		};
	const struct spi_buf_set tx_set_0 = { &tx_buf_0, 1 };
	const struct spi_buf_set rx_set_0 = { &rx_buf_0, 1 };

	ret = spi_transceive(spi_dev, &spi_cfg, &tx_set_0, &rx_set_0);

	if (ret < 0) {
		LOG_ERR("send failed: %d",ret);
	}else {

		for (int j = 0; j < sizeof(rx); j++) {
			printk("0x%02X ", rx[j]);
		}
		printk("\n");

	}

}


void main(void) {
	printk("run\n");
	io_cfg_output(PA05);
	io_clr_pin(PA05);
	io_toggle_pin(PA05);
	const struct device *tpm = DEVICE_DT_GET(DT_ALIAS(tpm));

	if (!device_is_ready(tpm)) {
		LOG_ERR("SPI device not ready!");
		return;
	}

	init_spid_registers(tpm, INTF_FIFO_MODE);

	int ret = spid_linkedsemi_cold_reset(tpm);
	if (ret != 0) {
		LOG_ERR("Cold reset failed: %d", ret);
		return;
	}
	// FIFO
	init_spid_registers(tpm, INTF_FIFO_MODE);

	spid_linkedsemi_register_callback(tpm, 0, spid_callback, NULL);

	// init_spid_registers(tpm, INTF_CRB_MODE);
	uint32_t tpm_cfg_num = sys_read32(tpm_spis2+SPID_TPM_CFG);
	LOG_INF("tpm_cfg_num 0x%08x ", tpm_cfg_num);
	sys_write32(0x12345678, tpm_spis2+SPID_TPM_ACCESS_0);
	uint32_t ACCESS_0 = sys_read32(tpm_spis2+SPID_TPM_ACCESS_0);
	LOG_INF("SPID_TPM_ACCESS_0 0x%08x ", ACCESS_0);

	const struct device *const spifiltermaster = DEVICE_DT_GET(DT_ALIAS(spifiltermaster));
	spif_switch_to_master(spifiltermaster);
	int check = spif_pinctrl_master_mode_check(spifiltermaster);
	__ASSERT_NO_MSG(check);
	const struct device *const flash_dev = DEVICE_DT_GET(DT_ALIAS(spiflash));
	const struct device *spi_dev = spif_spi_dev(spifiltermaster);
	uint32_t freq = DT_PROP(DT_ALIAS(spiflash), spi_max_frequency);
	uintptr_t base_address = DT_REG_ADDR(DT_ALIAS(spifiltermaster));
	LOG_INF("spifiltermaster base address: 0x%08lx", base_address);

	uint32_t cs = sys_read32(0x40021018);
	LOG_INF("0x40021018 = 0x%08x ", cs);
	sys_write32(cs|0x200000, 0x40021018);//设置TPM2的cs 给到wwdt1 的输入
	uint32_t n1 = sys_read32(0x40021018);
	LOG_INF("0x40021018 = 0x%08x ", n1);

	sys_write32( 0x1ACCE551, 0x400A1D00);//解除WWDT1的lock通过校验lock字
	uint32_t n2 = sys_read32(0x400A1D00);
	LOG_INF("0x400A1D00 = 0x%08x ", n2);

	sys_write32( 0x20000000, 0x400A1C20);//将WWDT1的复位功能与TPM2联系起来
	uint32_t n3 = sys_read32(0x400A1C20);
	LOG_INF("0x400A1C20 = 0x%08x ", n3);

	sys_write32( 0x80, 0x400A1C00);//设置对应的timeout 时间，以32khz为单位
	uint32_t n4 = sys_read32(0x400A1C00);
	LOG_INF("0x400A1C00 = 0x%08x ", n4);

	sys_write32(0xd, 0x400A1C08);//使能WWDT1,选择时钟源为32Khz
	uint32_t n5 = sys_read32(0x400A1C08);
	LOG_INF("0x400A1C08 = 0x%08x ", n5);

	spi_monitor_target_addr(spifiltermaster);

	LOG_INF("spi_back_pressure");

	while (1) {
		io_toggle_pin(PA05);
	}


}
