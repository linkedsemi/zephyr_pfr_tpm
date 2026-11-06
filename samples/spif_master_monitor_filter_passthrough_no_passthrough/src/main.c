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

// LOG_MODULE_REGISTER(spi_monitor_01_003_005, LOG_LEVEL_INF);
LOG_MODULE_REGISTER(spi_monitor_01_003_005, LOG_LEVEL_DBG);
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
// static uint8_t data_write[] = {
//     0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,
//     0xAA,0xBB,0xCC,0xDD,0xEE,0xFF,0x00,0x99,
// };
static uint8_t data_write[16];

static uint8_t data_read[sizeof(data_write)]={0};

// static uint8_t baud_freq[8]={1,5,10,25};
#include <reg_ssi_type.h>
#include <platform.h>
reg_ssi_t *DWSPI3 = ((reg_ssi_t *)APP_DWSPI3_ADDR);
reg_ssi_t *DWSPI4 = ((reg_ssi_t *)APP_DWSPI4_ADDR);


__aligned(32) uint8_t buf_r[FLASH_TEST_SIZE];
__aligned(32) uint8_t buf_w[FLASH_TEST_SIZE];

#define TEST_START_ADDR MB(4)
#define TEST_TOTAL_SIZE MB(4)
#define TEST_STEP_SIZE KB(4)

int test(void)
{
	do {
		uint8_t id[3] = { 0 };
		const struct device *const flash_dev = DEVICE_DT_GET(DT_ALIAS(spiflash));
		// spi_nor_re_init(flash_dev);
		int rc = flash_read_jedec_id(flash_dev, id);
		if (rc == 0) {
		printk("jedec-id = [%02x %02x %02x];\n", id[0], id[1], id[2]);
		} else {
		printk("JEDEC ID read failed: %d\n", rc);
		}

		for (int i = 0; i < sizeof(buf_w); i++) {
		buf_w[i] = i & 0xff;
		}

		buf_w[0] = ((uint32_t)flash_dev >> 24) & 0xff;
		buf_w[1] = ((uint32_t)flash_dev >> 16) & 0xff;
		buf_w[2] = ((uint32_t)flash_dev >> 8) & 0xff;
		buf_w[3] = (uint32_t)flash_dev & 0xff;

		// flash_erase(flash_dev, FLASH_OFFSET, FLASH_ERASE_SIZE);
		// flash_write(flash_dev, FLASH_OFFSET, buf_w, sizeof(buf_w));

		// flash_read(flash_dev, MB(16), buf_r, sizeof(buf_r));
		uint64_t time_stamp = 0;
		uint64_t milliseconds_spent = 0;
		time_stamp = k_uptime_get();
		for (int off = 0; off < 1 * TEST_STEP_SIZE; off += TEST_STEP_SIZE) {
		flash_read(flash_dev, TEST_START_ADDR + off, buf_r, sizeof(buf_r));
		}
		milliseconds_spent += k_uptime_delta(&time_stamp);
		printk("time: %llu.%llu(s)\n", milliseconds_spent / 1000, milliseconds_spent % 1000);
		printk("time: %llu(ms)\n", milliseconds_spent);
	} while(0);

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

#define SPIF_LAZY_INIT_READ_LEN 4

static int spif_lazy_init_flash_read(const struct device *flash_dev, off_t addr)
{
	uint8_t buf[SPIF_LAZY_INIT_READ_LEN];
	int rc = flash_read(flash_dev, addr, buf, sizeof(buf));

	if (rc == 0) {
		LOG_INF("flash lazy init read @ 0x%lx ok: %02x %02x %02x %02x",
			(long)addr, buf[0], buf[1], buf[2], buf[3]);
	} else {
		LOG_ERR("flash lazy init read @ 0x%lx failed: %d", (long)addr, rc);
	}

	return rc;
}

void spi_monitor_jedec_id(const struct device *const spifiltermaster,const struct device *const spifilter,uint32_t freq)
{

	// uint8_t send_freq[]={10};
	// spif_add_cmd(spifilter, 0x9f);

	static const uint8_t send_data_0[] = {
		0x9f,
	};
	uint8_t rx[4] = {0};
	// spif_dump_cmd_table(spifilter);
	// spif_dump_rw_addr_privilege_table(spifilter);

	const struct device *spi_dev = spif_spi_dev(spifiltermaster);

	// uint32_t freq =send_freq[i]*1000000;
	// uint32_t freq =freq;
	const struct spi_config spi_cfg = {
		.operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_TRANSFER_MSB,
		.frequency = freq,
		.cs = { .gpio = *(spif_spi_cs(spifiltermaster)), },
	};
	// LOG_INF("send_freq=%u",freq);
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

void spi_monitor_send_cmd(const struct device *const spifiltermaster,const struct device *const spifilter,uint32_t freq)
{

	// uint8_t send_freq[]={10};
	// spif_add_cmd(spifilter, 0x9f);

	static const uint8_t send_data_0[] = {
		0x02,
	};
	uint8_t rx[4] = {0};
	// spif_dump_cmd_table(spifilter);
	// spif_dump_rw_addr_privilege_table(spifilter);

	const struct device *spi_dev = spif_spi_dev(spifiltermaster);

	// uint32_t freq =send_freq[i]*1000000;
	// uint32_t freq =freq;
	const struct spi_config spi_cfg = {
		.operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_TRANSFER_MSB,
		.frequency = freq,
		.cs = { .gpio = *(spif_spi_cs(spifiltermaster)), },
	};
	// LOG_INF("send_freq=%u",freq);
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

	// uint8_t send_freq[]={10};
	// spif_add_cmd(spifilter, 0x9f);

	static const uint8_t send_data_0[] = {
		0x03,0x00,0x40,0x00
	};
	uint8_t rx[8] = {0};
	// spif_dump_cmd_table(spifilter);
	// spif_dump_rw_addr_privilege_table(spifilter);

	const struct device *spi_dev = spif_spi_dev(spifiltermaster);

	// uint32_t freq =send_freq[i]*1000000;
	// uint32_t freq =freq;
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

void spi_monitor_send_read_addr_allow(const struct device *const spifiltermaster,const struct device *const spifilter,uint32_t freq)
{
	static const uint8_t send_data_0[] = {
		0x03,0x00,0x00,0x00
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

void configure_cmd_whitelist(const struct device *spifilter, const uint8_t *legal_cmds, size_t cmd_count)
{
	LOG_INF("cmd_count%02x ", cmd_count);
	for (size_t i = 0; i < cmd_count; i++) {
		// LOG_INF("i%02x ", i);
		spif_add_cmd(spifilter, legal_cmds[i]);
		LOG_INF("Added command 0x%02x to whitelist", legal_cmds[i]);
	}
}

void configure_address_protection(const struct device *spifilter,uint32_t start_addr,uint32_t protect_size)
{
	spif_address_privilege_config(spifilter, FLAG_ADDR_PRIV_READ_SELECT, FLAG_ADDR_PRIV_DISABLE, start_addr, protect_size);
	spif_address_privilege_config(spifilter, FLAG_ADDR_PRIV_WRITE_SELECT, FLAG_ADDR_PRIV_DISABLE, start_addr, protect_size);
}
void init_white_cmd(const struct device *const spifiltermaster,const struct device *const spifilter,const struct device *const flash_dev)
{
	static const uint8_t write_read_cmd[] = {
		0xff,0x66,0x99,0xe9,0x04,0x05,
		0x9f,0x5a,0xb7,0x35,0x06
	};
	configure_cmd_whitelist(spifilter, write_read_cmd, ARRAY_SIZE(write_read_cmd));
}
void master_visit_flash_include_addr(const struct device *flash_dev,uint32_t addr,uint32_t erase_size)
{
		int rc=0;
		// REG_FIELD_WR(DWSPI3->RXSAMPLE_DLY, SSI_RSD, 3);
		rc = flash_erase(flash_dev, addr, erase_size);
		LOG_INF("flash_erase rc = %d", rc);

		LOG_INF("Master write test @ 0x%08X", addr);
		int ret = flash_write(flash_dev, addr, data_write, sizeof(data_write));
		LOG_INF("flash_write ret = %d", ret);

		memset(data_read, 0, sizeof(data_read));
		int re=flash_read(flash_dev, addr, data_read, sizeof(data_read));
		LOG_INF("flash_read ret = %d", re);
		rc = memcmp(data_read, data_write, sizeof(data_read));

		if (rc == 0) {
			LOG_INF("OK");
		} else {
			LOG_INF("fail: rc %d", rc);
			LOG_INF("ref");
			for (int i = 0; i < sizeof(data_write); i++) {
				printk("%02X ", data_write[i]);//expect
			}
			printk("\n");
			LOG_INF("real");
			for (int i = 0; i < sizeof(data_read); i++) {
				printk("%02X ", data_read[i]);// act
			}
			printk("\n");
		}

}


int main(void)
{
	bool check;
	int ret;
	/*主机*/
	const struct device *const spifiltermaster = DEVICE_DT_GET(DT_ALIAS(spifiltermaster));
	spif_switch_to_master(spifiltermaster);
	check = spif_pinctrl_master_mode_check(spifiltermaster);
	__ASSERT_NO_MSG(check);

	
	const struct device *const spifilter = DEVICE_DT_GET(DT_ALIAS(spifilter));

	/* 切到 master 后直接访问 flash（master 模式下 state 参数被忽略） */
	ret = spif_switch_to_mode(spifilter, spif_master, spif_pinctrl_master);
	__ASSERT_NO_MSG(ret == 0);
	check = (spif_pinctrl_state_get(spifilter) == spif_pinctrl_master);
	__ASSERT_NO_MSG(check);
	uint32_t freq1 = DT_PROP(DT_ALIAS(spiflash), spi_max_frequency);
	spi_monitor_jedec_id(spifilter,spifilter,freq1);

	linkedsemi_spi_filter_cold_reset(spifilter);
	// spif_memset_addr_whitelist(spifilter, 1);
	spif_flash_size_set(spifilter, MB(32));
	// spif_dump_cmd_table(spifilter);
	// spif_dump_rw_addr_privilege_table(spifilter);

#if defined(CONFIG_SPI_FILTER_DMA_LOG)
	linkedsemi_spif_register_callback(spifilter, (void*)spif_dma_callback);
	spif_dma_start(spifilter);
#endif

	/*
	 * 被测 filter 的工作模式与 out 侧配置（按需修改）：
	 *   mode ：spif_monitor / spif_filter
	 *   state：spif_single_passthrough / spif_multi_passthrough / spif_no_passthrough
	 * filter + no_passthrough 是非法组合，接口返回 -EINVAL。
	 */
	const spif_mode_t test_mode = spif_monitor;
	const spif_pinctrl_state_t test_state = spif_single_passthrough;

	ret = spif_switch_to_mode(spifilter, test_mode, test_state);
	__ASSERT_NO_MSG(ret == 0);

	check = (spif_pinctrl_state_get(spifilter) == test_state);
	__ASSERT_NO_MSG(check);

	/* 非法组合示例（需要时放开）：
	 * ret = spif_switch_to_mode(spifilter, spif_filter, spif_no_passthrough);
	 * __ASSERT_NO_MSG(ret == -EINVAL);
	 */
	// ret = spif_switch_to_mode(spifilter, spif_filter, spif_no_passthrough);
	// __ASSERT_NO_MSG(ret == -EINVAL);

	// ret = spif_switch_to_mode(spifilter, spif_filter, spif_no_passthrough);
	// __ASSERT_NO_MSG(ret == -EINVAL);
	spif_add_cmd(spifilter, 0x03);
	
	configure_address_protection(spifilter,0x4000,FLASH_ERASE_SIZE_MAIN);
	
	const struct device *const flash_dev = DEVICE_DT_GET(DT_ALIAS(spiflash));

	const struct device *spi_dev = spif_spi_dev(spifiltermaster);
	uint32_t freq = DT_PROP(DT_ALIAS(spiflash), spi_max_frequency);

	uintptr_t base_address = DT_REG_ADDR(DT_ALIAS(spifilter));
	LOG_INF("SPIFilter base address: 0x%08lx", base_address);


	spif_dump_cmd_table(spifilter);
	spif_dump_rw_addr_privilege_table(spifilter);
	spi_monitor_send_read_addr_allow(spifiltermaster,spifilter,freq);
	spi_monitor_jedec_id(spifiltermaster,spifilter,freq);
	spi_monitor_send_read_addr(spifiltermaster,spifilter,freq);
	test_spif();

	/* 切到 master 后直接访问 flash（master 模式下 state 参数被忽略） */
	ret = spif_switch_to_mode(spifilter, spif_master, spif_pinctrl_master);
	__ASSERT_NO_MSG(ret == 0);
	check = (spif_pinctrl_state_get(spifilter) == spif_pinctrl_master);
	__ASSERT_NO_MSG(check);

	spi_monitor_jedec_id(spifilter,spifilter,freq);

	return 0;
}
