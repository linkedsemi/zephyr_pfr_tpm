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
#include <zephyr/kernel.h>
#include <soc.h>
#ifndef DELAY_MS
#define DELAY_MS(ms) k_msleep(ms)
#endif

#define READ_LLI_DIS 1
#define NO_ALIGN 0
LOG_MODULE_REGISTER(spi_master_communication, LOG_LEVEL_INF);

#define TEST_READ_SIZE  4096

#define FLASH_TEST_STACK_SIZE 4096
#define FLASH_TEST_PRIORITY 5
#define START_ADDR 0x00
#define ERASE_SIZE 0X9000
#define FLASH_PAGE_SIZE_MAIN 256
static uint8_t write_buf[32896]={0};
static uint8_t read_buf[32896]={0};
// static uint8_t read_total_big_buf[8188]={0};
#if NO_ALIGN

#if READ_LLI_DIS
__aligned(4) static uint8_t read_total_big_noalign_buf[sizeof(write_buf) + 8] = {0};
#else
__aligned(4) static uint8_t read_total_big_lli_noalign_buf[sizeof(write_buf)-24564 + 8] = {0};
#endif

#else

#if READ_LLI_DIS
static uint8_t read_total_big_buf[sizeof(write_buf)]={0};
static uint8_t read_total_big_buf1[sizeof(write_buf)-1]={0};
static uint8_t read_total_big_buf2[sizeof(write_buf)-2]={0};
static uint8_t read_total_big_buf3[sizeof(write_buf)-3]={0};
#else
static uint8_t read_total_big_lli_buf[sizeof(write_buf)-24564]={0};
static uint8_t read_total_big_lli_buf1[sizeof(write_buf)-24565]={0};
static uint8_t read_total_big_lli_buf2[sizeof(write_buf)-24566]={0};
static uint8_t read_total_big_lli_buf3[sizeof(write_buf)-24567]={0};
#endif

#endif
K_THREAD_STACK_DEFINE(flash_read_stack, FLASH_TEST_STACK_SIZE);
struct k_thread flash_read_thread_data;
static uint8_t test_read_buf[TEST_READ_SIZE];


void flash_read_loop_thread(void *p1, void *p2, void *p3)
{

	const struct device *const flash_dev = DEVICE_DT_GET(DT_ALIAS(spiflash));
	uint32_t read_count = 0;

	if (!device_is_ready(flash_dev)) {
		LOG_ERR("Flash device not ready, stress test aborted.");
		return;
	}

	LOG_INF("Flash stress test thread started on %s", flash_dev->name);

	while (1) {

		int ret = flash_read(flash_dev, START_ADDR, test_read_buf, TEST_READ_SIZE);

		if (ret == 0) {

		if (read_count % 500 == 0) {
			LOG_INF("Flash read healthy. Total reads: %u", read_count);
		}
		} else {
		LOG_ERR("Flash read FAILED at count %u, error: %d", read_count, ret);
		}

		read_count++;


		k_msleep(20);
	}
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

void spi_monitor_jedec_id(const struct device *const spifiltermaster,const struct device *const spifilter,uint32_t freq)
{

	// spif_add_cmd(spifilter, 0x9f);

	static const uint8_t send_data_0[] = {
		0x9f,
	};
	uint8_t rx[4] = {0};
	// spif_dump_cmd_table(spifilter);
	// spif_dump_rw_addr_privilege_table(spifilter);

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

int flash_helper_erase(const struct device *dev, uint32_t addr, uint32_t size)
{
	int ret = flash_erase(dev, addr, size);
	if (ret != 0) {
		DEV_ERR(dev,"Erase Error at 0x%08x (ret: %d)", addr, ret);
	}
	return ret;
}

int flash_helper_write(const struct device *dev, uint32_t addr, const uint8_t *data, uint32_t size)
{
	int ret;
	for (uint32_t offset = 0; offset < size; offset += FLASH_PAGE_SIZE_MAIN) {
		uint32_t chunk = (size - offset < FLASH_PAGE_SIZE_MAIN) ? (size - offset) : FLASH_PAGE_SIZE_MAIN;

		ret = flash_write(dev, addr + offset, &data[offset], chunk);
		if (ret != 0) {
			DEV_ERR(dev,"Write Error at 0x%08x", addr + offset);
		return ret;
		}
	}
	return 0;
}

int flash_helper_read(const struct device *dev, uint32_t addr, uint8_t *dest, uint32_t size)
{
	int ret = flash_read(dev, addr, dest, size);
	if (ret != 0) {
		DEV_ERR(dev,"Read Error at 0x%08x (ret: %d)", addr, ret);
	}
	return ret;
}

int flash_helper_verify(const uint8_t *expected, const uint8_t *actual, uint32_t size)
{
	if (memcmp(expected, actual, size) == 0) {
		return 0;
	}

	for (uint32_t i = 0; i < size; i++) {
		if (expected[i] != actual[i]) {
		LOG_ERR("Mismatch! Offset: 0x%x, Exp: 0x%02x, Act: 0x%02x", i, expected[i], actual[i]);
		break;
		}
	}
	return -EIO;
}

static int flash_read_big_and_verify_data(const struct device *dev, uint32_t addr,
                            const uint8_t *expected, uint8_t *actual,
                            size_t len, const char *test_name)
{
	int ret;

	ret = flash_helper_read(dev, addr, actual, len);
	if (ret != 0) {
		DEV_ERR(dev, "[%s] Read operation failed! (ret: %d)", test_name, ret);
		return ret;
	}

	if (memcmp(expected, actual, len) == 0) {
		DEV_INF(dev, ">>> %s PASSED! (Size: %zu) <<<", test_name, len);
		return 0;
	} else {

		DEV_ERR(dev, ">>> %s BULK CHECK FAILED! (Addr: 0x%08x) <<<", test_name, addr);
		LOG_HEXDUMP_ERR(expected, len, "Expected Data:");
		LOG_HEXDUMP_ERR(actual, len, "Actual Read From Flash:");

		return -EIO;
	}
}

static void spif_dma_callback(void *callback_arg)
{
	const struct device *dev = (const struct device *)callback_arg;
	LOG_INF("DMA callback triggered for device: %s", dev->name);
}

int main(void)
{
	const struct device *const spifilter = DEVICE_DT_GET(DT_ALIAS(spifilter));
	bool check;
#if 0
	linkedsemi_spi_filter_cold_reset(spifilter);
	spif_memset_addr_whitelist(spifilter, 1);
	spif_flash_size_set(spifilter, MB(32));
	// spif_dump_cmd_table(spifilter);
	// spif_dump_rw_addr_privilege_table(spifilter);

#if defined(CONFIG_SPI_FILTER_DMA_LOG)
	linkedsemi_spif_register_callback(spifilter, (void*)spif_dma_callback);
	spif_dma_start(spifilter);
#endif

	spif_filter_enable(spifilter, true);
	check = spif_pinctrl_filter_mode_check(spifilter);
	__ASSERT_NO_MSG(check);

	// spif_operation_mode_config(spifilter, true);//filter
	// printk("Switch to Filter mode (OPERATION_MODE=1)\n");

	spif_passthrough_analog_mux_enable(spifilter, true);
	check = spif_pinctrl_passthrough_mode_check(spifilter);
	__ASSERT_NO_MSG(check);

	spif_switch_to_filter(spifilter);
	check = spif_pinctrl_filter_mode_check(spifilter);
	__ASSERT_NO_MSG(check);
#endif
	spif_switch_to_master(spifilter);
	check = spif_pinctrl_master_mode_check(spifilter);
	__ASSERT_NO_MSG(check);

	test_spif();
	LOG_INF("spi_master_communication");
	const struct device *const flash_dev = DEVICE_DT_GET(DT_ALIAS(spiflash));
	int ret=0;
	uint32_t current_addr = START_ADDR;
	memset(write_buf, 0, sizeof(write_buf));
	memset(read_buf, 0, sizeof(read_buf));

/*Only channels 0 and 1 have lli*/
	#if NO_ALIGN

	#if READ_LLI_DIS
	memset(read_total_big_noalign_buf, 0, sizeof(read_total_big_noalign_buf));
	#else
	memset(read_total_big_lli_noalign_buf, 0, sizeof(read_total_big_lli_noalign_buf));
	#endif

	#else

	#if READ_LLI_DIS
	memset(read_total_big_buf, 0, sizeof(read_total_big_buf));
	memset(read_total_big_buf1, 0, sizeof(read_total_big_buf1));
	memset(read_total_big_buf2, 0, sizeof(read_total_big_buf2));
	memset(read_total_big_buf3, 0, sizeof(read_total_big_buf3));
	#else
	memset(read_total_big_lli_buf, 0, sizeof(read_total_big_lli_buf));
	memset(read_total_big_lli_buf1, 0, sizeof(read_total_big_lli_buf1));
	memset(read_total_big_lli_buf2, 0, sizeof(read_total_big_lli_buf2));
	memset(read_total_big_lli_buf3, 0, sizeof(read_total_big_lli_buf3));
	#endif

	#endif
	flash_helper_erase(flash_dev, START_ADDR, ERASE_SIZE);

	for (uint32_t len = 1; len <= FLASH_PAGE_SIZE_MAIN; len++) {
		uint32_t offset = current_addr - START_ADDR;
		if ((current_addr + len) > (START_ADDR + ERASE_SIZE)) {
			DEV_ERR(flash_dev,"Test reached boundary of erased area at addr %u len: %u",current_addr, len);
			break;
		}

		// In each round, the first byte is equal to the current length.
		for (uint32_t i = 0; i < len; i++) {
			write_buf[offset+i] = (uint8_t)((len + i) & 0xFF);
		}

		// 0-len write
		ret = flash_helper_write(flash_dev, current_addr, &write_buf[offset], len);
		if (ret != 0) {
			DEV_ERR(flash_dev,"Write failed at addr 0x%x, len %u", current_addr, len);
			goto test_end;
		}

		ret = flash_helper_read(flash_dev, current_addr, &read_buf[offset], len);
		if (ret != 0) {
			DEV_ERR(flash_dev,"Read failed at addr 0x%x, len %u", current_addr, len);
			goto test_end;
		}

		if (flash_helper_verify(&write_buf[offset], &read_buf[offset], len) != 0) {
			// DEV_ERR(dev,"Failed at write length: %u", len);
			DEV_ERR(flash_dev, "verify Error Length: %u, Flash Addr: 0x%08x, Offset: 0x%x", len, current_addr, offset);
			uint32_t error_count = 0; // error cnt
			for (uint32_t i = 0; i < len; i++) {
					if (write_buf[offset + i] != read_buf[offset + i]) {
						DEV_ERR(flash_dev, "Mismatch at i=%d (Addr: 0x%x): Exp 0x%02x, Act 0x%02x",
							i, current_addr + i, write_buf[offset + i], read_buf[offset + i]);
							error_count++;

						if (error_count >= 10) {
							DEV_ERR(flash_dev, "!TO many data no pass.");
							break;
						}
					}
			}
			LOG_HEXDUMP_ERR(&write_buf[offset], len, "Expected Data Buffer:");
			LOG_HEXDUMP_ERR(&read_buf[offset], len, "Actual Read From Flash:");
			goto test_end;
		}

		current_addr += len;
	}

	uint32_t total_tested_size = current_addr - START_ADDR;
	DEV_INF(flash_dev,"Performing final bulk verification of %u bytes...", total_tested_size);
	if (memcmp(write_buf, read_buf, total_tested_size) == 0) {
		DEV_INF(flash_dev,">>> FINAL BULK CHECK PASSED! <<<");
	} else {
		DEV_ERR(flash_dev,">>> FINAL BULK CHECK FAILED! <<<");
		ret = -EIO;
	}

	#if NO_ALIGN

	#if READ_LLI_DIS
	size_t base_len = sizeof(write_buf); // L is raw write_buf length
	// extra poll 3 addr (+1, +2, +3)
	for (int off = 1; off <= 3; off++) {
		uint8_t *current_dst = read_total_big_noalign_buf + off;

		// inside poll 4 read length (L, L-1, L-2, L-3)
		for (int l_off = 0; l_off < 4; l_off++) {
			size_t current_len = base_len - l_off;

			memset(read_total_big_noalign_buf, 0, sizeof(read_total_big_noalign_buf));
			ret = flash_helper_read(flash_dev, START_ADDR, current_dst, current_len);

			if (ret == 0 && memcmp(write_buf, current_dst, current_len) == 0) {
				DEV_INF(flash_dev, "PASSED: Offset +%d, Len L-%d (%zu)", off, l_off, current_len);
			} else {
				DEV_ERR(flash_dev, "FAILED: Offset +%d, Len L-%d (%zu)!", off, l_off, current_len);

				DEV_ERR(flash_dev, "Fail Addr: %p", (void*)current_dst);
				goto test_end;
			}
		}
	}
	#else
	size_t base_len = sizeof(write_buf)-24564;
	for (int off = 1; off <= 3; off++) {
		uint8_t *current_dst = read_total_big_lli_noalign_buf + off;

		for (int l_off = 0; l_off < 4; l_off++) {
			size_t current_len = base_len - l_off;

			memset(read_total_big_lli_noalign_buf, 0, sizeof(read_total_big_lli_noalign_buf));

			ret = flash_helper_read(flash_dev, START_ADDR, current_dst, current_len);

			if (ret == 0 && memcmp(write_buf, current_dst, current_len) == 0) {
				DEV_INF(flash_dev, "PASSED: Offset +%d, Len L-%d (%zu)", off, l_off, current_len);
			} else {
				DEV_ERR(flash_dev, "FAILED: Offset +%d, Len L-%d (%zu)!", off, l_off, current_len);

				DEV_ERR(flash_dev, "Fail Addr: %p", (void*)current_dst);
				goto test_end;
			}
		}
	}
	#endif

	#else
	#if READ_LLI_DIS
	ret = flash_read_big_and_verify_data(flash_dev,START_ADDR,write_buf,read_total_big_buf,sizeof(read_total_big_buf),"read_total_big_buf");
	if (ret != 0) {DEV_ERR(flash_dev,"Read failed read_total_big_buf");goto test_end;}
	ret = flash_read_big_and_verify_data(flash_dev,START_ADDR,write_buf,read_total_big_buf1,sizeof(read_total_big_buf1),"read_total_big_buf1");
	if (ret != 0) {DEV_ERR(flash_dev,"Read failed read_total_big_buf1");goto test_end;}
	ret = flash_read_big_and_verify_data(flash_dev,START_ADDR,write_buf,read_total_big_buf2,sizeof(read_total_big_buf2),"read_total_big_buf2");
	if (ret != 0) {DEV_ERR(flash_dev,"Read failed read_total_big_buf2");goto test_end;}
	ret = flash_read_big_and_verify_data(flash_dev,START_ADDR,write_buf,read_total_big_buf3,sizeof(read_total_big_buf3),"read_total_big_buf3");
	if (ret != 0) {DEV_ERR(flash_dev,"Read failed read_total_big_buf3");goto test_end;}
	#else
	ret = flash_read_big_and_verify_data(flash_dev,START_ADDR,write_buf,read_total_big_lli_buf,sizeof(read_total_big_lli_buf),"read_total_big_lli_buf");
	if (ret != 0) {DEV_ERR(flash_dev,"Read failed read_total_big_buf");goto test_end;}
	ret = flash_read_big_and_verify_data(flash_dev,START_ADDR,write_buf,read_total_big_lli_buf1,sizeof(read_total_big_lli_buf1),"read_total_big_lli_buf1");
	if (ret != 0) {DEV_ERR(flash_dev,"Read failed read_total_big_buf1");goto test_end;}
	ret = flash_read_big_and_verify_data(flash_dev,START_ADDR,write_buf,read_total_big_lli_buf2,sizeof(read_total_big_lli_buf2),"read_total_big_lli_buf2");
	if (ret != 0) {DEV_ERR(flash_dev,"Read failed read_total_big_buf2");goto test_end;}
	ret = flash_read_big_and_verify_data(flash_dev,START_ADDR,write_buf,read_total_big_lli_buf3,sizeof(read_total_big_lli_buf3),"read_total_big_lli_buf3");
	if (ret != 0) {DEV_ERR(flash_dev,"Read failed read_total_big_buf3");goto test_end;}
	#endif
	#endif

test_end:
	if (ret != 0) {
		DEV_ERR(flash_dev,"Flash Test Failed!");
	} else {
		DEV_INF(flash_dev,"========== All Flash Tests Passed! ==========");
	}


	// k_thread_create(&flash_read_thread_data, flash_read_stack,
	// 		K_THREAD_STACK_SIZEOF(flash_read_stack),
	// 		flash_read_loop_thread, NULL, NULL, NULL,
	// 		FLASH_TEST_PRIORITY, 0, K_NO_WAIT);

	return 0;
}
