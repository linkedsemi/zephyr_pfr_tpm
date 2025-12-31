/*
 * Copyright (c) 2024 Linkedsemi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

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
#include <reg_ssi_type.h>
#include <platform.h>

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


reg_ssi_t *DWSPI3 = ((reg_ssi_t *)APP_DWSPI3_ADDR);
reg_ssi_t *DWSPI4 = ((reg_ssi_t *)APP_DWSPI4_ADDR);


__aligned(32) uint8_t buf_r[FLASH_TEST_SIZE];
__aligned(32) uint8_t buf_w[FLASH_TEST_SIZE];

#define TEST_START_ADDR MB(4)
#define TEST_TOTAL_SIZE MB(4)
#define TEST_STEP_SIZE KB(4)

#if 1

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

//ARRAY_SIZE(legal_cmds)
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

void master_visit_flash_addr(const struct device *flash_dev,uint32_t start_addr,uint32_t erase_size,uint8_t move_num)
{

	for (int i = 0; i < move_num; i++) {
		uint32_t addr = start_addr << i;

		int rc=0;
		rc = flash_erase(flash_dev, addr, erase_size);

		LOG_INF("Master write test @ 0x%08X", addr);
		int ret = flash_write(flash_dev, addr, data_write, sizeof(data_write));
		LOG_INF("flash_write ret = %d", ret);

		memset(data_read, 0, sizeof(data_read));
		flash_read(flash_dev, addr, data_read, sizeof(data_read));
		rc = memcmp(data_read, data_write, sizeof(data_read));

		if (rc == 0) {
			LOG_INF("OK");
		} else {
			LOG_INF("fail: rc %d", rc);
			LOG_INF("ref");
			for (int i = 0; i < sizeof(data_write); i++) {
				printk("%02X ", data_write[i]);
			}
			printk("\n");
			LOG_INF("real");
			for (int i = 0; i < sizeof(data_read); i++) {
				printk("%02X ", data_read[i]);
			}
			printk("\n");
		}

		LOG_INF("flash_read done %d time\n",i);

	}
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

void clear_cmd_whitelist(const struct device *const spifilter)
{
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
}


void master_send_to_slave(const struct device *spi_dev,const struct spi_config spi_cfg)
{
	uint32_t cmd_cnt = ARRAY_SIZE(legal_cmds);
	LOG_INF("Legal CMD count = %d", cmd_cnt);

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

}

void master_send_to_slave_opcode(const struct device *spi_dev,const struct spi_config spi_cfg,uint8_t opcode)
{
		uint8_t tx = opcode;

		struct spi_buf tx_buf = {
			.buf = &tx,
			.len = 1,
		};
		const struct spi_buf_set tx_set = { &tx_buf, 1 };

		LOG_INF("Send opcode = 0x%02X ...", tx);

		int ret = spi_transceive(spi_dev, &spi_cfg, &tx_set, NULL);
		if (ret < 0) {LOG_ERR("CMD 0x%02X send failed: %d", tx, ret);}

}
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

void spi_monitor_table_21(const struct device *const spifiltermaster,const struct device *const spifilter,const struct device *const flash_dev,uint8_t cycle_time)
{
	spif_add_cmd(spifilter, 0x20);
	spif_add_cmd(spifilter, 0x02);
	spif_add_cmd(spifilter, 0x0b);

	int addr_mode=0;
	spif_3byte_mode_config(spifilter);
	addr_mode=spif_addr_mode_peek(spifilter);
	LOG_INF("spifilter addr_mode %s",addr_mode == 1 ? "4byte_mode" : "3byte_mode");

	static const uint8_t write_read_cmd[] = {
		0xff,0x66,0x99,0xe9,0x04,0x05,
		0x9f,0x5a,0xb7,0x35,0x06
	};

	configure_cmd_whitelist(spifilter, write_read_cmd, ARRAY_SIZE(write_read_cmd));

	uint8_t time =cycle_time;//3byte full
	for (int i=0; i<time; i++) {
		spif_memset_addr_whitelist(spifilter, 1);//clear protect area
		configure_address_protection(spifilter,START_ADDR_MAIN << i,FLASH_ERASE_SIZE_MAIN);
		spif_dump_cmd_table(spifilter);
		spif_dump_rw_addr_privilege_table(spifilter);
		// master_visit_flash_addr(flash_dev,START_ADDR_MAIN << i,FLASH_ERASE_SIZE_MAIN,time-i);
		for (int j=0; j<time-i; j++) { // Ensure the access address is within the 3-byte range
			master_visit_flash_include_addr(flash_dev,START_ADDR_MAIN << (i+j),FLASH_ERASE_SIZE_MAIN);
			LOG_INF("flash_read done %d time\n",j);
		}
	}

}

void spi_monitor_table_22(const struct device *const spifiltermaster,const struct device *const spifilter,const struct device *const flash_dev,uint8_t cycle_time)
{
	spif_add_cmd(spifilter, 0x20);
	spif_add_cmd(spifilter, 0x02);
	spif_add_cmd(spifilter, 0x0b);

	int addr_mode=0;
	spif_3byte_mode_config(spifilter);
	addr_mode=spif_addr_mode_peek(spifilter);
	LOG_INF("spifilter addr_mode %s",addr_mode == 1 ? "4byte_mode" : "3byte_mode");

	static const uint8_t write_read_cmd[] = {
		0xff,0x66,0x99,0xe9,0x04,0x05,
		0x9f,0x5a,0xb7,0x35,0x06
	};

	configure_cmd_whitelist(spifilter, write_read_cmd, ARRAY_SIZE(write_read_cmd));

	uint8_t time =cycle_time;//3byte full
	for (int i=0; i<time; i++) {
		spif_memset_addr_whitelist(spifilter, 1);//clear protect area
		configure_address_protection(spifilter,START_ADDR_MAIN << i,FLASH_ERASE_SIZE_MAIN);
		spif_dump_cmd_table(spifilter);
		spif_dump_rw_addr_privilege_table(spifilter);

		for (int j=0; j<(time/3); j++) { // Ensure the access address is within the 3-byte range
			master_visit_flash_include_addr(flash_dev,((START_ADDR_MAIN << i) - (j)*(FLASH_OFFSET_MAIN)),FLASH_ERASE_SIZE_4K);
			LOG_INF("flash_read done %d time\n",j);
		}

		for (int j=1; j<(time/2); j++) { // Ensure the access address is within the 3-byte range
			master_visit_flash_include_addr(flash_dev,((START_ADDR_MAIN << i) + (j)*(FLASH_OFFSET_MAIN)),FLASH_ERASE_SIZE_4K);
			LOG_INF("flash_read done %d time\n",j);
		}
	}

}

void spi_monitor_table_23(const struct device *const spifiltermaster,const struct device *const spifilter,const struct device *const flash_dev,uint8_t cycle_time)
{
	spif_add_cmd(spifilter, 0x20);
	spif_add_cmd(spifilter, 0x02);
	spif_add_cmd(spifilter, 0x0b);

	int addr_mode=0;

#if 0
	spif_3byte_mode_config(spifilter);
	// addr_mode=spif_addr_mode_peek(spifilter);
	// LOG_INF("spifilter addr_mode %s",addr_mode == 1 ? "4byte_mode" : "3byte_mode");
#else
	spif_4byte_mode_config(spifilter);
	// master_visit_flash_include_addr(flash_dev,START_ADDR_MAIN,FLASH_ERASE_SIZE_MAIN);

#endif

	static const uint8_t write_read_cmd[] = {
		0xff,0x66,0x99,0xe9,0x04,0x05,
		0x9f,0x5a,0xb7,0x35,0x06
	};

	configure_cmd_whitelist(spifilter, write_read_cmd, ARRAY_SIZE(write_read_cmd));

	uint8_t time =cycle_time;//3byte full
	for (int i=0; i<time; i++) {
		spif_memset_addr_whitelist(spifilter, 1);//clear protect area
		configure_address_protection(spifilter,START_ADDR_MAIN << i,FLASH_ERASE_SIZE_MAIN);
		spif_dump_cmd_table(spifilter);
		spif_dump_rw_addr_privilege_table(spifilter);
		// master_visit_flash_addr(flash_dev,START_ADDR_MAIN << i,FLASH_ERASE_SIZE_MAIN,time-i);
		for (int j=0; j<time-i; j++) { // Ensure the access address is within the 3-byte range
			master_visit_flash_include_addr(flash_dev,START_ADDR_MAIN << (i+j),FLASH_ERASE_SIZE_MAIN);
			addr_mode=spif_addr_mode_peek(spifilter);
			LOG_INF("spifilter addr_mode %s",addr_mode == 1 ? "4byte_mode" : "3byte_mode");
			LOG_INF("flash_read done %d time\n",j);
		}
	}

}
void spi_monitor_table_23_test(const struct device *const spifiltermaster,const struct device *const spifilter,const struct device *const flash_dev,uint8_t cycle_time)
{
	spif_add_cmd(spifilter, 0x20);
	spif_add_cmd(spifilter, 0x02);
	spif_add_cmd(spifilter, 0x0b);

	int addr_mode=0;

#if 0
	spif_3byte_mode_config(spifilter);
	// addr_mode=spif_addr_mode_peek(spifilter);
	// LOG_INF("spifilter addr_mode %s",addr_mode == 1 ? "4byte_mode" : "3byte_mode");
#else
	// spif_4byte_mode_config(spifilter);
	// master_visit_flash_include_addr(flash_dev,START_ADDR_MAIN,FLASH_ERASE_SIZE_MAIN);
	addr_mode=spif_addr_mode_peek(spifilter);
	LOG_INF("spifilter addr_mode 2 %s",addr_mode == 1 ? "4byte_mode" : "3byte_mode");

#endif

	static const uint8_t write_read_cmd[] = {
		0xff,0x66,0x99,0xe9,0x04,0x05,
		0x9f,0x5a,0xb7,0x35,0x06
	};

	configure_cmd_whitelist(spifilter, write_read_cmd, ARRAY_SIZE(write_read_cmd));

	// uint8_t time =cycle_time;//3byte full
	spif_memset_addr_whitelist(spifilter, 1);//clear protect area
	configure_address_protection(spifilter,START_ADDR_MAIN,FLASH_ERASE_SIZE_MAIN);
	spif_dump_cmd_table(spifilter);
	spif_dump_rw_addr_privilege_table(spifilter);
	master_visit_flash_include_addr(flash_dev,START_ADDR_MAIN,FLASH_ERASE_SIZE_MAIN);

	addr_mode=spif_addr_mode_peek(spifilter);
	LOG_INF("spifilter addr_mode %s",addr_mode == 1 ? "4byte_mode" : "3byte_mode");
	spif_3byte_mode_config(spifilter);//这个标志位不要自己随便设！！！！
	addr_mode=spif_addr_mode_peek(spifilter);
	LOG_INF("spifilter addr_mode %s",addr_mode == 1 ? "4byte_mode" : "3byte_mode");

	// spif_memset_addr_whitelist(spifilter, 1);//clear protect area
	configure_address_protection(spifilter,0x00,FLASH_ERASE_SIZE_MAIN);
	spif_dump_cmd_table(spifilter);
	spif_dump_rw_addr_privilege_table(spifilter);
	master_visit_flash_include_addr(flash_dev,START_ADDR_MAIN,FLASH_ERASE_SIZE_MAIN);
	addr_mode=spif_addr_mode_peek(spifilter);
	LOG_INF("spifilter addr_mode %s",addr_mode == 1 ? "4byte_mode" : "3byte_mode");

}

void spi_monitor_table_24(const struct device *const spifiltermaster,const struct device *const spifilter,const struct device *const flash_dev,uint8_t cycle_time)
{
	spif_add_cmd(spifilter, 0x20);
	spif_add_cmd(spifilter, 0x02);
	spif_add_cmd(spifilter, 0x0b);

	int addr_mode=0;
#if 0
	spif_3byte_mode_config(spifilter);
	// addr_mode=spif_addr_mode_peek(spifilter);
	// LOG_INF("spifilter addr_mode %s",addr_mode == 1 ? "4byte_mode" : "3byte_mode");
#else
	spif_4byte_mode_config(spifilter);
	// master_visit_flash_include_addr(flash_dev,START_ADDR_MAIN,FLASH_ERASE_SIZE_MAIN);

#endif

	static const uint8_t write_read_cmd[] = {
		0xff,0x66,0x99,0xe9,0x04,0x05,
		0x9f,0x5a,0xb7,0x35,0x06
	};

	configure_cmd_whitelist(spifilter, write_read_cmd, ARRAY_SIZE(write_read_cmd));

	uint8_t time =cycle_time;//3byte full
	for (int i=0; i<time; i++) {
		spif_memset_addr_whitelist(spifilter, 1);//clear protect area
		configure_address_protection(spifilter,START_ADDR_MAIN << i,FLASH_ERASE_SIZE_MAIN);
		spif_dump_cmd_table(spifilter);
		spif_dump_rw_addr_privilege_table(spifilter);

		for (int j=0; j<(time/3); j++) { // Ensure the access address is within the 3-byte range
			// spi_nor_config_4byte_mode(flash_dev, false);
			master_visit_flash_include_addr(flash_dev,((START_ADDR_MAIN << i) - (j)*(FLASH_OFFSET_MAIN)),FLASH_ERASE_SIZE_4K);
			addr_mode=spif_addr_mode_peek(spifilter);
			LOG_INF("spifilter addr_mode %s",addr_mode == 1 ? "4byte_mode" : "3byte_mode");
			LOG_INF("flash_read done %d time\n",j);
		}

		for (int j=1; j<(time/2); j++) { // Ensure the access address is within the 3-byte range
			// spi_nor_config_4byte_mode(flash_dev, false);
			master_visit_flash_include_addr(flash_dev,((START_ADDR_MAIN << i) + (j)*(FLASH_OFFSET_MAIN)),FLASH_ERASE_SIZE_4K);
			addr_mode=spif_addr_mode_peek(spifilter);
			LOG_INF("spifilter addr_mode %s",addr_mode == 1 ? "4byte_mode" : "3byte_mode");
			LOG_INF("flash_read done %d time\n",j);
		}
	}

}

void spi_monitor_table_27(const struct device *const spifiltermaster,const struct device *const spifilter,const struct device *const flash_dev,uint8_t cycle_time)
{
	spif_add_cmd(spifilter, 0x20);
	spif_add_cmd(spifilter, 0x02);
	spif_add_cmd(spifilter, 0x0b);

	int addr_mode=0;

	static const uint8_t write_read_cmd[] = {
		0xff,0x66,0x99,0xe9,0x04,0x05,
		0x9f,0x5a,0xb7,0x35,0x06
	};

	configure_cmd_whitelist(spifilter, write_read_cmd, ARRAY_SIZE(write_read_cmd));

	uint8_t time =cycle_time;
	for (int i=0; i<time; i++) {
		spif_memset_addr_whitelist(spifilter, 1);//clear protect area
		configure_address_protection(spifilter,START_ADDR_MAIN << i,FLASH_ERASE_SIZE_MAIN);
		spif_dump_cmd_table(spifilter);
		spif_dump_rw_addr_privilege_table(spifilter);
		// master_visit_flash_addr(flash_dev,START_ADDR_MAIN << i,FLASH_ERASE_SIZE_MAIN,time-i);
		for (int j=0; j<time-i; j++) {
			master_visit_flash_include_addr(flash_dev,START_ADDR_MAIN << (i+j),FLASH_ERASE_SIZE_MAIN);
			addr_mode=spif_addr_mode_peek(spifilter);
			LOG_INF("spifilter addr_mode %s",addr_mode == 1 ? "4byte_mode" : "3byte_mode");
			LOG_INF("flash_read done %d time\n",j);
		}
	}

}

void spi_monitor_table_28(const struct device *const spifiltermaster,const struct device *const spifilter,const struct device *const flash_dev,uint8_t cycle_time)
{
	spif_add_cmd(spifilter, 0x20);
	spif_add_cmd(spifilter, 0x02);
	spif_add_cmd(spifilter, 0x0b);

	int addr_mode=0;

	static const uint8_t write_read_cmd[] = {
		0xff,0x66,0x99,0xe9,0x04,0x05,
		0x9f,0x5a,0xb7,0x35,0x06
	};

	configure_cmd_whitelist(spifilter, write_read_cmd, ARRAY_SIZE(write_read_cmd));

	uint8_t time =cycle_time;//3byte full
	for (int i=0; i<time; i++) {
		spif_memset_addr_whitelist(spifilter, 1);//clear protect area
		configure_address_protection(spifilter,START_ADDR_MAIN << i,FLASH_ERASE_SIZE_MAIN);
		spif_dump_cmd_table(spifilter);
		spif_dump_rw_addr_privilege_table(spifilter);

		for (int j=0; j<(time/3); j++) {
			// spi_nor_config_4byte_mode(flash_dev, false);
			master_visit_flash_include_addr(flash_dev,((START_ADDR_MAIN << i) - (j)*(FLASH_OFFSET_MAIN)),FLASH_ERASE_SIZE_4K);
			addr_mode=spif_addr_mode_peek(spifilter);
			LOG_INF("spifilter addr_mode %s",addr_mode == 1 ? "4byte_mode" : "3byte_mode");
			LOG_INF("flash_read done %d time\n",j);
		}

		for (int j=1; j<(time/2); j++) {
			// spi_nor_config_4byte_mode(flash_dev, false);
			master_visit_flash_include_addr(flash_dev,((START_ADDR_MAIN << i) + (j)*(FLASH_OFFSET_MAIN)),FLASH_ERASE_SIZE_4K);
			addr_mode=spif_addr_mode_peek(spifilter);
			LOG_INF("spifilter addr_mode %s",addr_mode == 1 ? "4byte_mode" : "3byte_mode");
			LOG_INF("flash_read done %d time\n",j);
		}
	}

}

void spi_monitor_table_29_and_35(const struct device *const spifiltermaster,const struct device *const spifilter,const struct device *const flash_dev,uint8_t cycle_time)
{
	spif_add_cmd(spifilter, 0x20);
	spif_add_cmd(spifilter, 0x02);
	spif_add_cmd(spifilter, 0x03);

	int addr_mode=0;

	static const uint8_t write_read_cmd[] = {
		0xff,0x66,0x99,0xe9,0x04,0x05,
		0x9f,0x5a,0xb7,0x35,0x06
	};

	configure_cmd_whitelist(spifilter, write_read_cmd, ARRAY_SIZE(write_read_cmd));

	uint8_t time =cycle_time;
	for (int i=0; i<time; i++) {
		spif_memset_addr_whitelist(spifilter, 1);//clear protect area
		configure_address_protection(spifilter,START_ADDR_MAIN << i,FLASH_ERASE_SIZE_MAIN);
		spif_dump_cmd_table(spifilter);
		spif_dump_rw_addr_privilege_table(spifilter);
		// master_visit_flash_addr(flash_dev,START_ADDR_MAIN << i,FLASH_ERASE_SIZE_MAIN,time-i);
		for (int j=0; j<time-i; j++) {
			master_visit_flash_include_addr(flash_dev,START_ADDR_MAIN << (i+j),FLASH_ERASE_SIZE_MAIN);
			addr_mode=spif_addr_mode_peek(spifilter);
			LOG_INF("spifilter addr_mode %s",addr_mode == 1 ? "4byte_mode" : "3byte_mode");
			LOG_INF("flash_read done %d time\n",j);
		}
	}

}

void spi_monitor_table_30_and_36(const struct device *const spifiltermaster,const struct device *const spifilter,const struct device *const flash_dev,uint8_t cycle_time)
{
	spif_add_cmd(spifilter, 0x20);
	spif_add_cmd(spifilter, 0x02);
	spif_add_cmd(spifilter, 0x03);

	int addr_mode=0;

	static const uint8_t write_read_cmd[] = {
		0xff,0x66,0x99,0xe9,0x04,0x05,
		0x9f,0x5a,0xb7,0x35,0x06
	};

	configure_cmd_whitelist(spifilter, write_read_cmd, ARRAY_SIZE(write_read_cmd));

	uint8_t time =cycle_time;//3byte full
	for (int i=0; i<time; i++) {
		spif_memset_addr_whitelist(spifilter, 1);//clear protect area
		configure_address_protection(spifilter,START_ADDR_MAIN << i,FLASH_ERASE_SIZE_MAIN);
		spif_dump_cmd_table(spifilter);
		spif_dump_rw_addr_privilege_table(spifilter);

		for (int j=0; j<(time/3); j++) {
			// spi_nor_config_4byte_mode(flash_dev, false);
			master_visit_flash_include_addr(flash_dev,((START_ADDR_MAIN << i) - (j)*(FLASH_OFFSET_MAIN)),FLASH_ERASE_SIZE_4K);
			addr_mode=spif_addr_mode_peek(spifilter);
			LOG_INF("spifilter addr_mode %s",addr_mode == 1 ? "4byte_mode" : "3byte_mode");
			LOG_INF("flash_read done %d time\n",j);
		}

		for (int j=1; j<(time/2); j++) {
			// spi_nor_config_4byte_mode(flash_dev, false);
			master_visit_flash_include_addr(flash_dev,((START_ADDR_MAIN << i) + (j)*(FLASH_OFFSET_MAIN)),FLASH_ERASE_SIZE_4K);
			addr_mode=spif_addr_mode_peek(spifilter);
			LOG_INF("spifilter addr_mode %s",addr_mode == 1 ? "4byte_mode" : "3byte_mode");
			LOG_INF("flash_read done %d time\n",j);
		}
	}

}

void spi_monitor_table_51_and_59(const struct device *const spifiltermaster,const struct device *const spifilter,const struct device *const flash_dev,uint8_t cycle_time)
{
	//no need change
	spif_add_cmd(spifilter, 0x21);
	spif_add_cmd(spifilter, 0x12);
	spif_add_cmd(spifilter, 0x0c);

	int addr_mode=0;

	static const uint8_t write_read_cmd[] = {
		0xff,0x66,0x99,0xe9,0x04,0x05,
		0x9f,0x5a,0xb7,0x35,0x06
	};

	configure_cmd_whitelist(spifilter, write_read_cmd, ARRAY_SIZE(write_read_cmd));

	uint8_t time =cycle_time;
	for (int i=0; i<time; i++) {
		spif_memset_addr_whitelist(spifilter, 1);//clear protect area
		configure_address_protection(spifilter,START_ADDR_MAIN << i,FLASH_ERASE_SIZE_MAIN);
		spif_dump_cmd_table(spifilter);
		spif_dump_rw_addr_privilege_table(spifilter);
		// master_visit_flash_addr(flash_dev,START_ADDR_MAIN << i,FLASH_ERASE_SIZE_MAIN,time-i);
		for (int j=0; j<time-i; j++) {
			master_visit_flash_include_addr(flash_dev,START_ADDR_MAIN << (i+j),FLASH_ERASE_SIZE_MAIN);
			addr_mode=spif_addr_mode_peek(spifilter);
			LOG_INF("spifilter addr_mode %s",addr_mode == 1 ? "4byte_mode" : "3byte_mode");
			LOG_INF("flash_read done %d time\n",j);
		}
	}

}

void spi_monitor_table_52_and_60(const struct device *const spifiltermaster,const struct device *const spifilter,const struct device *const flash_dev,uint8_t cycle_time)
{
	spif_add_cmd(spifilter, 0x21);
	spif_add_cmd(spifilter, 0x12);
	spif_add_cmd(spifilter, 0x0c);

	int addr_mode=0;

	static const uint8_t write_read_cmd[] = {
		0xff,0x66,0x99,0xe9,0x04,0x05,
		0x9f,0x5a,0xb7,0x35,0x06
	};

	configure_cmd_whitelist(spifilter, write_read_cmd, ARRAY_SIZE(write_read_cmd));

	uint8_t time =cycle_time;//3byte full
	for (int i=0; i<time; i++) {
		spif_memset_addr_whitelist(spifilter, 1);//clear protect area
		configure_address_protection(spifilter,START_ADDR_MAIN << i,FLASH_ERASE_SIZE_MAIN);
		spif_dump_cmd_table(spifilter);
		spif_dump_rw_addr_privilege_table(spifilter);

		for (int j=0; j<(time/3); j++) {
			// spi_nor_config_4byte_mode(flash_dev, false);
			master_visit_flash_include_addr(flash_dev,((START_ADDR_MAIN << i) - (j)*(FLASH_OFFSET_MAIN)),FLASH_ERASE_SIZE_4K);
			addr_mode=spif_addr_mode_peek(spifilter);
			LOG_INF("spifilter addr_mode %s",addr_mode == 1 ? "4byte_mode" : "3byte_mode");
			LOG_INF("flash_read done %d time\n",j);
		}

		for (int j=1; j<(time/2); j++) {
			// spi_nor_config_4byte_mode(flash_dev, false);
			master_visit_flash_include_addr(flash_dev,((START_ADDR_MAIN << i) + (j)*(FLASH_OFFSET_MAIN)),FLASH_ERASE_SIZE_4K);
			addr_mode=spif_addr_mode_peek(spifilter);
			LOG_INF("spifilter addr_mode %s",addr_mode == 1 ? "4byte_mode" : "3byte_mode");
			LOG_INF("flash_read done %d time\n",j);
		}
	}

}

void spi_monitor_table_67(const struct device *const spifiltermaster,const struct device *const spifilter,const struct device *const flash_dev,uint8_t cycle_time)
{

	spif_add_cmd(spifilter, 0x21);
	spif_add_cmd(spifilter, 0x12);
	spif_add_cmd(spifilter, 0x13);

	int addr_mode=0;

	static const uint8_t write_read_cmd[] = {
		0xff,0x66,0x99,0xe9,0x04,0x05,
		0x9f,0x5a,0xb7,0x35,0x06
	};

	configure_cmd_whitelist(spifilter, write_read_cmd, ARRAY_SIZE(write_read_cmd));

	uint8_t time =cycle_time;
	for (int i=0; i<time; i++) {
		spif_memset_addr_whitelist(spifilter, 1);//clear protect area
		configure_address_protection(spifilter,START_ADDR_MAIN << i,FLASH_ERASE_SIZE_MAIN);
		spif_dump_cmd_table(spifilter);
		spif_dump_rw_addr_privilege_table(spifilter);
		// master_visit_flash_addr(flash_dev,START_ADDR_MAIN << i,FLASH_ERASE_SIZE_MAIN,time-i);
		for (int j=0; j<time-i; j++) {
			master_visit_flash_include_addr(flash_dev,START_ADDR_MAIN << (i+j),FLASH_ERASE_SIZE_MAIN);
			addr_mode=spif_addr_mode_peek(spifilter);
			LOG_INF("spifilter addr_mode %s",addr_mode == 1 ? "4byte_mode" : "3byte_mode");
			LOG_INF("flash_read done %d time\n",j);
		}
	}

}

void spi_monitor_table_67_test(const struct device *const spifiltermaster,const struct device *const spifilter,const struct device *const flash_dev,uint8_t cycle_time)
{

	spif_add_cmd(spifilter, 0x21);
	spif_add_cmd(spifilter, 0x12);
	spif_add_cmd(spifilter, 0x13);

	int addr_mode=0;

	static const uint8_t write_read_cmd[] = {
		0xff,0x66,0x99,0xe9,0x04,0x05,
		0x9f,0x5a,0xb7,0x35,0x06
	};

	configure_cmd_whitelist(spifilter, write_read_cmd, ARRAY_SIZE(write_read_cmd));

	uint8_t time =cycle_time;
	for (int i=0; i<time; i++) {
		spif_memset_addr_whitelist(spifilter, 1);//clear protect area
		configure_address_protection(spifilter,START_ADDR_MAIN << i,FLASH_ERASE_SIZE_MAIN);
		// spif_dump_cmd_table(spifilter);
		// spif_dump_rw_addr_privilege_table(spifilter);
		// master_visit_flash_addr(flash_dev,START_ADDR_MAIN << i,FLASH_ERASE_SIZE_MAIN,time-i);
		for (int j=0; j<time-i; j++) {
			master_visit_flash_include_addr(flash_dev,START_ADDR_MAIN << (i+j),FLASH_ERASE_SIZE_MAIN);
			addr_mode=spif_addr_mode_peek(spifilter);
			LOG_INF("spifilter addr_mode %s",addr_mode == 1 ? "4byte_mode" : "3byte_mode");
			LOG_INF("flash_read done %d time\n",j);
		}
	}

}
void spi_monitor_table_68(const struct device *const spifiltermaster,const struct device *const spifilter,const struct device *const flash_dev,uint8_t cycle_time)
{
	spif_add_cmd(spifilter, 0x21);
	spif_add_cmd(spifilter, 0x12);
	spif_add_cmd(spifilter, 0x13);

	int addr_mode=0;

	static const uint8_t write_read_cmd[] = {
		0xff,0x66,0x99,0xe9,0x04,0x05,
		0x9f,0x5a,0xb7,0x35,0x06
	};

	configure_cmd_whitelist(spifilter, write_read_cmd, ARRAY_SIZE(write_read_cmd));

	uint8_t time =cycle_time;//3byte full
	for (int i=0; i<time; i++) {
		spif_memset_addr_whitelist(spifilter, 1);//clear protect area
		configure_address_protection(spifilter,START_ADDR_MAIN << i,FLASH_ERASE_SIZE_MAIN);
		spif_dump_cmd_table(spifilter);
		spif_dump_rw_addr_privilege_table(spifilter);

		for (int j=0; j<(time/3); j++) {
			// spi_nor_config_4byte_mode(flash_dev, false);
			master_visit_flash_include_addr(flash_dev,((START_ADDR_MAIN << i) - (j)*(FLASH_OFFSET_MAIN)),FLASH_ERASE_SIZE_4K);
			addr_mode=spif_addr_mode_peek(spifilter);
			LOG_INF("spifilter addr_mode %s",addr_mode == 1 ? "4byte_mode" : "3byte_mode");
			LOG_INF("flash_read done %d time\n",j);
		}

		for (int j=1; j<(time/2); j++) {
			// spi_nor_config_4byte_mode(flash_dev, false);
			master_visit_flash_include_addr(flash_dev,((START_ADDR_MAIN << i) + (j)*(FLASH_OFFSET_MAIN)),FLASH_ERASE_SIZE_4K);
			addr_mode=spif_addr_mode_peek(spifilter);
			LOG_INF("spifilter addr_mode %s",addr_mode == 1 ? "4byte_mode" : "3byte_mode");
			LOG_INF("flash_read done %d time\n",j);
		}
	}

}
/*69、75、141、147、*/
void spi_monitor_table_69_75_141_147(const struct device *const spifiltermaster,const struct device *const spifilter,const struct device *const flash_dev,uint8_t cycle_time)
{

	spif_add_cmd(spifilter, 0x20);
	spif_add_cmd(spifilter, 0x32);
	spif_add_cmd(spifilter, 0xeb);

	int addr_mode=0;

	static const uint8_t write_read_cmd[] = {
		0xff,0x66,0x99,0xe9,0x04,0x05,
		0x9f,0x5a,0xb7,0x35,0x06
	};

	configure_cmd_whitelist(spifilter, write_read_cmd, ARRAY_SIZE(write_read_cmd));

	uint8_t time =cycle_time;
	for (int i=0; i<time; i++) {
		spif_memset_addr_whitelist(spifilter, 1);//clear protect area
		configure_address_protection(spifilter,START_ADDR_MAIN << i,FLASH_ERASE_SIZE_MAIN);
		spif_dump_cmd_table(spifilter);
		spif_dump_rw_addr_privilege_table(spifilter);
		// master_visit_flash_addr(flash_dev,START_ADDR_MAIN << i,FLASH_ERASE_SIZE_MAIN,time-i);
		for (int j=0; j<time-i; j++) {
			master_visit_flash_include_addr(flash_dev,START_ADDR_MAIN << (i+j),FLASH_ERASE_SIZE_MAIN);
			addr_mode=spif_addr_mode_peek(spifilter);
			LOG_INF("spifilter addr_mode %s",addr_mode == 1 ? "4byte_mode" : "3byte_mode");
			LOG_INF("flash_read done %d time\n",j);
		}
	}

}

void spi_monitor_table_70_76_142_148(const struct device *const spifiltermaster,const struct device *const spifilter,const struct device *const flash_dev,uint8_t cycle_time)
{
	spif_add_cmd(spifilter, 0x20);
	spif_add_cmd(spifilter, 0x32);
	spif_add_cmd(spifilter, 0xeb);

	int addr_mode=0;

	static const uint8_t write_read_cmd[] = {
		0xff,0x66,0x99,0xe9,0x04,0x05,
		0x9f,0x5a,0xb7,0x35,0x06
	};

	configure_cmd_whitelist(spifilter, write_read_cmd, ARRAY_SIZE(write_read_cmd));

	uint8_t time =cycle_time;//3byte full
	for (int i=0; i<time; i++) {
		spif_memset_addr_whitelist(spifilter, 1);//clear protect area
		configure_address_protection(spifilter,START_ADDR_MAIN << i,FLASH_ERASE_SIZE_MAIN);
		spif_dump_cmd_table(spifilter);
		spif_dump_rw_addr_privilege_table(spifilter);

		for (int j=0; j<(time/3); j++) {
			// spi_nor_config_4byte_mode(flash_dev, false);
			master_visit_flash_include_addr(flash_dev,((START_ADDR_MAIN << i) - (j)*(FLASH_OFFSET_MAIN)),FLASH_ERASE_SIZE_4K);
			addr_mode=spif_addr_mode_peek(spifilter);
			LOG_INF("spifilter addr_mode %s",addr_mode == 1 ? "4byte_mode" : "3byte_mode");
			LOG_INF("flash_read done %d time\n",j);
		}

		for (int j=1; j<(time/2); j++) {
			// spi_nor_config_4byte_mode(flash_dev, false);
			master_visit_flash_include_addr(flash_dev,((START_ADDR_MAIN << i) + (j)*(FLASH_OFFSET_MAIN)),FLASH_ERASE_SIZE_4K);
			addr_mode=spif_addr_mode_peek(spifilter);
			LOG_INF("spifilter addr_mode %s",addr_mode == 1 ? "4byte_mode" : "3byte_mode");
			LOG_INF("flash_read done %d time\n",j);
		}
	}

}

void spi_monitor_table_83_155(const struct device *const spifiltermaster,const struct device *const spifilter,const struct device *const flash_dev,uint8_t cycle_time)
{

	spif_add_cmd(spifilter, 0x21);
	spif_add_cmd(spifilter, 0x34);
	spif_add_cmd(spifilter, 0xec);

	int addr_mode=0;

	static const uint8_t write_read_cmd[] = {
		0xff,0x66,0x99,0xe9,0x04,0x05,
		0x9f,0x5a,0xb7,0x35,0x06
	};

	configure_cmd_whitelist(spifilter, write_read_cmd, ARRAY_SIZE(write_read_cmd));

	uint8_t time =cycle_time;
	for (int i=0; i<time; i++) {
		spif_memset_addr_whitelist(spifilter, 1);//clear protect area
		configure_address_protection(spifilter,START_ADDR_MAIN << i,FLASH_ERASE_SIZE_MAIN);
		spif_dump_cmd_table(spifilter);
		spif_dump_rw_addr_privilege_table(spifilter);
		// master_visit_flash_addr(flash_dev,START_ADDR_MAIN << i,FLASH_ERASE_SIZE_MAIN,time-i);
		for (int j=0; j<time-i; j++) {
			master_visit_flash_include_addr(flash_dev,START_ADDR_MAIN << (i+j),FLASH_ERASE_SIZE_MAIN);
			addr_mode=spif_addr_mode_peek(spifilter);
			LOG_INF("spifilter addr_mode %s",addr_mode == 1 ? "4byte_mode" : "3byte_mode");
			LOG_INF("flash_read done %d time\n",j);
		}
	}

}

void spi_monitor_table_84_156(const struct device *const spifiltermaster,const struct device *const spifilter,const struct device *const flash_dev,uint8_t cycle_time)
{
	spif_add_cmd(spifilter, 0x21);
	spif_add_cmd(spifilter, 0x34);
	spif_add_cmd(spifilter, 0xec);

	int addr_mode=0;

	static const uint8_t write_read_cmd[] = {
		0xff,0x66,0x99,0xe9,0x04,0x05,
		0x9f,0x5a,0xb7,0x35,0x06
	};

	configure_cmd_whitelist(spifilter, write_read_cmd, ARRAY_SIZE(write_read_cmd));

	uint8_t time =cycle_time;
	for (int i=0; i<time; i++) {
		spif_memset_addr_whitelist(spifilter, 1);//clear protect area
		configure_address_protection(spifilter,START_ADDR_MAIN << i,FLASH_ERASE_SIZE_MAIN);
		spif_dump_cmd_table(spifilter);
		spif_dump_rw_addr_privilege_table(spifilter);

		for (int j=0; j<(time/3); j++) {
			// spi_nor_config_4byte_mode(flash_dev, false);
			master_visit_flash_include_addr(flash_dev,((START_ADDR_MAIN << i) - (j)*(FLASH_OFFSET_MAIN)),FLASH_ERASE_SIZE_4K);
			addr_mode=spif_addr_mode_peek(spifilter);
			LOG_INF("spifilter addr_mode %s",addr_mode == 1 ? "4byte_mode" : "3byte_mode");
			LOG_INF("flash_read done %d time\n",j);
		}

		for (int j=1; j<(time/2); j++) {
			// spi_nor_config_4byte_mode(flash_dev, false);
			master_visit_flash_include_addr(flash_dev,((START_ADDR_MAIN << i) + (j)*(FLASH_OFFSET_MAIN)),FLASH_ERASE_SIZE_4K);
			addr_mode=spif_addr_mode_peek(spifilter);
			LOG_INF("spifilter addr_mode %s",addr_mode == 1 ? "4byte_mode" : "3byte_mode");
			LOG_INF("flash_read done %d time\n",j);
		}
	}

}
void spi_monitor_table_85_91_109_115(const struct device *const spifiltermaster,const struct device *const spifilter,const struct device *const flash_dev,uint8_t cycle_time)//187
{

	spif_add_cmd(spifilter, 0x20);
	spif_add_cmd(spifilter, 0xc2);//144write
	spif_add_cmd(spifilter, 0x6b);//114read

	int addr_mode=0;

	static const uint8_t write_read_cmd[] = {
		0xff,0x66,0x99,0xe9,0x04,0x05,
		0x9f,0x5a,0xb7,0x35,0x06
	};

	configure_cmd_whitelist(spifilter, write_read_cmd, ARRAY_SIZE(write_read_cmd));

	uint8_t time =cycle_time;
	for (int i=0; i<time; i++) {
		spif_memset_addr_whitelist(spifilter, 1);//clear protect area
		configure_address_protection(spifilter,START_ADDR_MAIN << i,FLASH_ERASE_SIZE_MAIN);
		spif_dump_cmd_table(spifilter);
		spif_dump_rw_addr_privilege_table(spifilter);
		// master_visit_flash_addr(flash_dev,START_ADDR_MAIN << i,FLASH_ERASE_SIZE_MAIN,time-i);
		for (int j=0; j<time-i; j++) {
			master_visit_flash_include_addr(flash_dev,START_ADDR_MAIN << (i+j),FLASH_ERASE_SIZE_MAIN);
			addr_mode=spif_addr_mode_peek(spifilter);
			LOG_INF("spifilter addr_mode %s",addr_mode == 1 ? "4byte_mode" : "3byte_mode");
			LOG_INF("flash_read done %d time\n",j);
		}
	}

}
void spi_monitor_table_86_92_110_116(const struct device *const spifiltermaster,const struct device *const spifilter,const struct device *const flash_dev,uint8_t cycle_time)
{
	spif_add_cmd(spifilter, 0x20);
	spif_add_cmd(spifilter, 0xc2);//144write
	spif_add_cmd(spifilter, 0x6b);//114read

	int addr_mode=0;

	static const uint8_t write_read_cmd[] = {
		0xff,0x66,0x99,0xe9,0x04,0x05,
		0x9f,0x5a,0xb7,0x35,0x06
	};

	configure_cmd_whitelist(spifilter, write_read_cmd, ARRAY_SIZE(write_read_cmd));

	uint8_t time =cycle_time;//3byte full
	for (int i=0; i<time; i++) {
		spif_memset_addr_whitelist(spifilter, 1);//clear protect area
		configure_address_protection(spifilter,START_ADDR_MAIN << i,FLASH_ERASE_SIZE_MAIN);
		spif_dump_cmd_table(spifilter);
		spif_dump_rw_addr_privilege_table(spifilter);

		for (int j=0; j<(time/3); j++) {
			// spi_nor_config_4byte_mode(flash_dev, false);
			master_visit_flash_include_addr(flash_dev,((START_ADDR_MAIN << i) - (j)*(FLASH_OFFSET_MAIN)),FLASH_ERASE_SIZE_4K);
			addr_mode=spif_addr_mode_peek(spifilter);
			LOG_INF("spifilter addr_mode %s",addr_mode == 1 ? "4byte_mode" : "3byte_mode");
			LOG_INF("flash_read done %d time\n",j);
		}

		for (int j=1; j<(time/2); j++) {
			// spi_nor_config_4byte_mode(flash_dev, false);
			master_visit_flash_include_addr(flash_dev,((START_ADDR_MAIN << i) + (j)*(FLASH_OFFSET_MAIN)),FLASH_ERASE_SIZE_4K);
			addr_mode=spif_addr_mode_peek(spifilter);
			LOG_INF("spifilter addr_mode %s",addr_mode == 1 ? "4byte_mode" : "3byte_mode");
			LOG_INF("flash_read done %d time\n",j);
		}
	}

}

void spi_monitor_table_3e_123(const struct device *const spifiltermaster,const struct device *const spifilter,const struct device *const flash_dev,uint8_t cycle_time)//187
{

	spif_add_cmd(spifilter, 0x20);
	spif_add_cmd(spifilter, 0x3e);
	spif_add_cmd(spifilter, 0x6c);

	int addr_mode=0;

	static const uint8_t write_read_cmd[] = {
		0xff,0x66,0x99,0xe9,0x04,0x05,
		0x9f,0x5a,0xb7,0x35,0x06
	};

	configure_cmd_whitelist(spifilter, write_read_cmd, ARRAY_SIZE(write_read_cmd));

	uint8_t time =cycle_time;
	for (int i=0; i<time; i++) {
		spif_memset_addr_whitelist(spifilter, 1);//clear protect area
		configure_address_protection(spifilter,START_ADDR_MAIN << i,FLASH_ERASE_SIZE_MAIN);
		spif_dump_cmd_table(spifilter);
		spif_dump_rw_addr_privilege_table(spifilter);
		// master_visit_flash_addr(flash_dev,START_ADDR_MAIN << i,FLASH_ERASE_SIZE_MAIN,time-i);
		for (int j=0; j<time-i; j++) {
			master_visit_flash_include_addr(flash_dev,START_ADDR_MAIN << (i+j),FLASH_ERASE_SIZE_MAIN);
			addr_mode=spif_addr_mode_peek(spifilter);
			LOG_INF("spifilter addr_mode %s",addr_mode == 1 ? "4byte_mode" : "3byte_mode");
			LOG_INF("flash_read done %d time\n",j);
		}
	}

}

void spi_monitor_table_3e_124(const struct device *const spifiltermaster,const struct device *const spifilter,const struct device *const flash_dev,uint8_t cycle_time)
{
	spif_add_cmd(spifilter, 0x20);
	spif_add_cmd(spifilter, 0x3e);
	spif_add_cmd(spifilter, 0x6c);

	int addr_mode=0;

	static const uint8_t write_read_cmd[] = {
		0xff,0x66,0x99,0xe9,0x04,0x05,
		0x9f,0x5a,0xb7,0x35,0x06
	};

	configure_cmd_whitelist(spifilter, write_read_cmd, ARRAY_SIZE(write_read_cmd));

	uint8_t time =cycle_time;
	for (int i=0; i<time; i++) {
		spif_memset_addr_whitelist(spifilter, 1);//clear protect area
		configure_address_protection(spifilter,START_ADDR_MAIN << i,FLASH_ERASE_SIZE_MAIN);
		spif_dump_cmd_table(spifilter);
		spif_dump_rw_addr_privilege_table(spifilter);

		for (int j=0; j<(time/3); j++) {
			// spi_nor_config_4byte_mode(flash_dev, false);
			master_visit_flash_include_addr(flash_dev,((START_ADDR_MAIN << i) - (j)*(FLASH_OFFSET_MAIN)),FLASH_ERASE_SIZE_4K);
			addr_mode=spif_addr_mode_peek(spifilter);
			LOG_INF("spifilter addr_mode %s",addr_mode == 1 ? "4byte_mode" : "3byte_mode");
			LOG_INF("flash_read done %d time\n",j);
		}

		for (int j=1; j<(time/2); j++) {
			// spi_nor_config_4byte_mode(flash_dev, false);
			master_visit_flash_include_addr(flash_dev,((START_ADDR_MAIN << i) + (j)*(FLASH_OFFSET_MAIN)),FLASH_ERASE_SIZE_4K);
			addr_mode=spif_addr_mode_peek(spifilter);
			LOG_INF("spifilter addr_mode %s",addr_mode == 1 ? "4byte_mode" : "3byte_mode");
			LOG_INF("flash_read done %d time\n",j);
		}
	}

}

void spi_monitor_table_125_131(const struct device *const spifiltermaster,const struct device *const spifilter,const struct device *const flash_dev,uint8_t cycle_time)//172、193
{

	spif_add_cmd(spifilter, 0x20);
	spif_add_cmd(spifilter, 0x02);
	spif_add_cmd(spifilter, 0xbb);

	int addr_mode=0;

	static const uint8_t write_read_cmd[] = {
		0xff,0x66,0x99,0xe9,0x04,0x05,
		0x9f,0x5a,0xb7,0x35,0x06
	};

	configure_cmd_whitelist(spifilter, write_read_cmd, ARRAY_SIZE(write_read_cmd));

	uint8_t time =cycle_time;
	for (int i=0; i<time; i++) {
		spif_memset_addr_whitelist(spifilter, 1);//clear protect area
		configure_address_protection(spifilter,START_ADDR_MAIN << i,FLASH_ERASE_SIZE_MAIN);
		spif_dump_cmd_table(spifilter);
		spif_dump_rw_addr_privilege_table(spifilter);
		// master_visit_flash_addr(flash_dev,START_ADDR_MAIN << i,FLASH_ERASE_SIZE_MAIN,time-i);
		for (int j=0; j<time-i; j++) {
			master_visit_flash_include_addr(flash_dev,START_ADDR_MAIN << (i+j),FLASH_ERASE_SIZE_MAIN);
			addr_mode=spif_addr_mode_peek(spifilter);
			LOG_INF("spifilter addr_mode %s",addr_mode == 1 ? "4byte_mode" : "3byte_mode");
			LOG_INF("flash_read done %d time\n",j);
		}
	}

}

void spi_monitor_table_139(const struct device *const spifiltermaster,const struct device *const spifilter,const struct device *const flash_dev,uint8_t cycle_time)//172、193
{

	spif_add_cmd(spifilter, 0x20);
	spif_add_cmd(spifilter, 0x12);
	spif_add_cmd(spifilter, 0xbc);

	int addr_mode=0;

	static const uint8_t write_read_cmd[] = {
		0xff,0x66,0x99,0xe9,0x04,0x05,
		0x9f,0x5a,0xb7,0x35,0x06
	};

	configure_cmd_whitelist(spifilter, write_read_cmd, ARRAY_SIZE(write_read_cmd));

	uint8_t time =cycle_time;
	for (int i=0; i<time; i++) {
		spif_memset_addr_whitelist(spifilter, 1);//clear protect area
		configure_address_protection(spifilter,START_ADDR_MAIN << i,FLASH_ERASE_SIZE_MAIN);
		spif_dump_cmd_table(spifilter);
		spif_dump_rw_addr_privilege_table(spifilter);
		// master_visit_flash_addr(flash_dev,START_ADDR_MAIN << i,FLASH_ERASE_SIZE_MAIN,time-i);
		for (int j=0; j<time-i; j++) {
			master_visit_flash_include_addr(flash_dev,START_ADDR_MAIN << (i+j),FLASH_ERASE_SIZE_MAIN);
			addr_mode=spif_addr_mode_peek(spifilter);
			LOG_INF("spifilter addr_mode %s",addr_mode == 1 ? "4byte_mode" : "3byte_mode");
			LOG_INF("flash_read done %d time\n",j);
		}
	}

}
void spi_monitor_table_140(const struct device *const spifiltermaster,const struct device *const spifilter,const struct device *const flash_dev,uint8_t cycle_time)
{
	spif_add_cmd(spifilter, 0x20);
	spif_add_cmd(spifilter, 0x12);
	spif_add_cmd(spifilter, 0xbc);

	int addr_mode=0;

	static const uint8_t write_read_cmd[] = {
		0xff,0x66,0x99,0xe9,0x04,0x05,
		0x9f,0x5a,0xb7,0x35,0x06
	};

	configure_cmd_whitelist(spifilter, write_read_cmd, ARRAY_SIZE(write_read_cmd));

	uint8_t time =cycle_time;
	for (int i=0; i<time; i++) {
		spif_memset_addr_whitelist(spifilter, 1);//clear protect area
		configure_address_protection(spifilter,START_ADDR_MAIN << i,FLASH_ERASE_SIZE_MAIN);
		spif_dump_cmd_table(spifilter);
		spif_dump_rw_addr_privilege_table(spifilter);

		for (int j=0; j<(time/3); j++) {
			// spi_nor_config_4byte_mode(flash_dev, false);
			master_visit_flash_include_addr(flash_dev,((START_ADDR_MAIN << i) - (j)*(FLASH_OFFSET_MAIN)),FLASH_ERASE_SIZE_4K);
			addr_mode=spif_addr_mode_peek(spifilter);
			LOG_INF("spifilter addr_mode %s",addr_mode == 1 ? "4byte_mode" : "3byte_mode");
			LOG_INF("flash_read done %d time\n",j);
		}

		for (int j=1; j<(time/2); j++) {
			// spi_nor_config_4byte_mode(flash_dev, false);
			master_visit_flash_include_addr(flash_dev,((START_ADDR_MAIN << i) + (j)*(FLASH_OFFSET_MAIN)),FLASH_ERASE_SIZE_4K);
			addr_mode=spif_addr_mode_peek(spifilter);
			LOG_INF("spifilter addr_mode %s",addr_mode == 1 ? "4byte_mode" : "3byte_mode");
			LOG_INF("flash_read done %d time\n",j);
		}
	}

}

void spi_monitor_table_69_test(const struct device *const spifiltermaster,const struct device *const spifilter,const struct device *const flash_dev,uint8_t cycle_time)
{

	spif_add_cmd(spifilter, 0x20);
	spif_add_cmd(spifilter, 0xc2);
	spif_add_cmd(spifilter, 0xeb);


	static const uint8_t write_read_cmd[] = {
		0xff,0x66,0x99,0xe9,0x04,0x05,
		0x9f,0x5a,0xb7,0x35,0x06
	};

	configure_cmd_whitelist(spifilter, write_read_cmd, ARRAY_SIZE(write_read_cmd));
	#if 1
	spif_memset_addr_whitelist(spifilter, 1);//clear protect area

		int rc=0;
		int ret;
		rc = flash_erase(flash_dev, 0x00, FLASH_ERASE_SIZE_4K);

		LOG_INF("Master  test addr@ 0x%08X", 0x00);
		ret = flash_write(flash_dev, 0x00, data_write, sizeof(data_write));
		LOG_INF("flash_write ret = %d", ret);

		memset(data_read, 0, sizeof(data_read));
		ret = flash_read(flash_dev, 0x00, data_read, sizeof(data_read));
		LOG_INF("flash_read ret = %d", ret);

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
		for (int i = 0; i < sizeof(data_read); i++) {
				printk("%02X ", data_read[i]);// act
			}
			printk("\n");
	#else
	uint8_t time =cycle_time;
	for (int i=0; i<time; i++) {

		// master_visit_flash_include_addr(flash_dev,0x00,FLASH_ERASE_SIZE_MAIN);
		addr_mode=spif_addr_mode_peek(spifilter);
		LOG_INF("spifilter addr_mode %s",addr_mode == 1 ? "4byte_mode" : "3byte_mode");

		spif_memset_addr_whitelist(spifilter, 1);//clear protect area
		configure_address_protection(spifilter,START_ADDR_MAIN << i,FLASH_ERASE_SIZE_MAIN);
		spif_dump_cmd_table(spifilter);
		spif_dump_rw_addr_privilege_table(spifilter);
		// master_visit_flash_addr(flash_dev,START_ADDR_MAIN << i,FLASH_ERASE_SIZE_MAIN,time-i);
		for (int j=0; j<time-i; j++) {
			master_visit_flash_include_addr(flash_dev,START_ADDR_MAIN << (i+j),FLASH_ERASE_SIZE_MAIN);
			addr_mode=spif_addr_mode_peek(spifilter);
			LOG_INF("spifilter addr_mode %s",addr_mode == 1 ? "4byte_mode" : "3byte_mode");
			LOG_INF("flash_read done %d time\n",j);
		}
	}
	#endif
}


void spi_monitor_table_125_test(const struct device *const spifiltermaster,const struct device *const spifilter,const struct device *const flash_dev,uint8_t cycle_time)
{

	spif_add_cmd(spifilter, 0x21);
	spif_add_cmd(spifilter, 0x12);
	spif_add_cmd(spifilter, 0xbb);

	static const uint8_t write_read_cmd[] = {
		0xff,0x66,0x99,0x9f,0xe9,0x05,
		0x5a,0xb7,0x35,0x06
	};

	static const uint8_t send_cmds[] = {
		0x04,0x04
	};
	const struct device *spi_dev = spif_spi_dev(spifiltermaster);
	uint32_t freq = DT_PROP(DT_ALIAS(spiflash), spi_max_frequency);

	const struct spi_config spi_cfg = {
		.operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_TRANSFER_MSB,
		.frequency = freq,
		.cs = { .gpio = *(spif_spi_cs(spifiltermaster)), },
	};


	configure_cmd_whitelist(spifilter, write_read_cmd, ARRAY_SIZE(write_read_cmd));

	configure_address_protection(spifilter,START_ADDR_MAIN,FLASH_ERASE_SIZE_MAIN);
	spif_dump_rw_addr_privilege_table(spifilter);

	flash_read(flash_dev, START_ADDR_MAIN, data_read, sizeof(data_read));
	for (int m=0; m<sizeof(send_cmds); m++) {
		uint8_t tx = send_cmds[m];
		// uint8_t rx[4] = {0};

		struct spi_buf tx_buf = {
			.buf = &tx,
			.len = 1,
		};
		// struct spi_buf rx_buf = {
		// 	.buf = &rx,
		// 	.len = sizeof(rx),
		// };
		const struct spi_buf_set tx_set = { &tx_buf, 1 };
		// const struct spi_buf_set rx_set = { &rx_buf, 1 };

		LOG_INF("Send CMD = 0x%02X ...", tx);

		int ret = spi_transceive(spi_dev, &spi_cfg, &tx_set, NULL);

		if (ret < 0) {
			LOG_ERR("CMD 0x%02X send failed: %d", tx, ret);
		}

	}
}

void spi_monitor_erase4byte_addr3byte(const struct device *const spifiltermaster,const struct device *const spifilter,const struct device *const flash_dev,uint8_t cycle_time)
{

	spif_add_cmd(spifilter, 0x21);

	static const uint8_t send_data_0[] = {
		0x21,0x00,0x04,0x00,0x00
	};

	static const uint8_t send_data[] = {
		0x21,0x04,0x00,0x00
	};

	// configure_cmd_whitelist(spifilter, write_read_cmd, ARRAY_SIZE(write_read_cmd));
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
		LOG_ERR("send failed: %d",ret);
	}

	struct spi_buf tx_buf = {
		.buf = (uint8_t*)send_data,
		.len = sizeof(send_data),
	};

	const struct spi_buf_set tx_set = { &tx_buf, 1 };

	ret = spi_transceive(spi_dev, &spi_cfg, &tx_set, NULL);

	if (ret < 0) {

		LOG_ERR("send failed: %d",ret);
	}


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

void spi_monitor_erase4byte_addr3byte_sel_flag(const struct device *const spifiltermaster,const struct device *const spifilter,const struct device *const flash_dev,uint8_t cycle_time)
{

	spif_add_cmd(spifilter, 0x21);

	static const uint8_t send_data_0[] = {
		0x21,0x00,0x04,0x00,0x00
	};

	static const uint8_t send_data[] = {
		0x21,0x04,0x00,0x00
	};

	// configure_cmd_whitelist(spifilter, write_read_cmd, ARRAY_SIZE(write_read_cmd));
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

	int addr_mode=0;
	// spif_4byte_mode_config(spifilter);
	addr_mode=spif_addr_mode_peek(spifilter);
	LOG_INF("spifilter addr_mode0 %s",addr_mode == 1 ? "4byte_mode" : "3byte_mode");
	ret = spi_transceive(spi_dev, &spi_cfg, &tx_set_0, NULL);

	if (ret < 0) {

		LOG_ERR("send failed: %d",ret);
	}

	addr_mode=spif_addr_mode_peek(spifilter);
	LOG_INF("spifilter addr_mode1 %s",addr_mode == 1 ? "4byte_mode" : "3byte_mode");

	struct spi_buf tx_buf = {
		.buf = (uint8_t*)send_data,
		.len = sizeof(send_data),
	};

	const struct spi_buf_set tx_set = { &tx_buf, 1 };

	// spif_4byte_mode_config(spifilter);
	spif_3byte_mode_config(spifilter);

	ret = spi_transceive(spi_dev, &spi_cfg, &tx_set, NULL);
	addr_mode=spif_addr_mode_peek(spifilter);
	LOG_INF("spifilter addr_mode2 %s",addr_mode == 1 ? "4byte_mode" : "3byte_mode");

	if (ret < 0) {
		LOG_ERR("send failed: %d",ret);
	}

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

void master_flash_test(const struct device *const spifiltermaster,const struct device *const spifilter,const struct device *flash_dev,uint32_t addr,uint32_t erase_size)
{

	static uint8_t data_write_test[256];
	static uint8_t data_read_test[sizeof(data_write_test)]={0};
	for (int i=0; i<sizeof(data_write_test);i++) {data_write_test[i]=i+1;}
	int rc=0;
	#if 0
	spif_add_cmd(spifilter, 0x21);
	spif_add_cmd(spifilter, 0x34);
	spif_add_cmd(spifilter, 0xec);

	static const uint8_t write_read_cmd[] = {
		0xff,0x66,0x99,0xe9,0x04,0x05,
		0x9f,0x5a,0xb7,0x35,0x06
	};
	configure_cmd_whitelist(spifilter, write_read_cmd, ARRAY_SIZE(write_read_cmd));
	// REG_FIELD_WR(DWSPI3->RXSAMPLE_DLY, SSI_RSD, 3);
	rc = flash_erase(flash_dev, addr, erase_size);

	LOG_INF("write test @ 0x%08X", addr);
	int ret = flash_write(flash_dev, addr, data_write_test, sizeof(data_write_test));
	LOG_INF("flash_write ret = %d", ret);
	#endif
	memset(data_read_test, 0, sizeof(data_read_test));
	flash_read(flash_dev, addr, data_read_test, sizeof(data_read_test));
	rc = memcmp(data_read_test, data_write_test, sizeof(data_read_test));

	// for (int i = 0; i < sizeof(data_write); i++) {
	// 		printk("%02X ", data_write[i]);//expect
	// 	}
	if (rc == 0) {
		LOG_INF("OK");
	} else {
		LOG_INF("fail: rc %d", rc);
		LOG_INF("ref");
		for (int i = 0; i < sizeof(data_write_test); i++) {
			printk("%02X ", data_write_test[i]);//expect
		}
		printk("\n");
		LOG_INF("real");
		for (int i = 0; i < sizeof(data_read_test); i++) {
			printk("%02X ", data_read_test[i]);// act
		}
		printk("\n");
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

	// spi_monitor_table_21(spifiltermaster,spifilter,flash_dev,10);
	// spi_monitor_table_22(spifiltermaster,spifilter,flash_dev,10);
	// spi_monitor_table_23(spifiltermaster,spifilter,flash_dev,10);
	// spi_monitor_table_24(spifiltermaster,spifilter,flash_dev,10);
	// spi_monitor_table_23_test(spifiltermaster,spifilter,flash_dev,3);
	// spi_monitor_table_27(spifiltermaster,spifilter,flash_dev,10);//the same 38、43
	// spi_monitor_table_28(spifiltermaster,spifilter,flash_dev,10);//the same 39、44
	// spi_monitor_table_29_and_35(spifiltermaster,spifilter,flash_dev,10);
	// spi_monitor_table_30_and_36(spifiltermaster,spifilter,flash_dev,10);
	// spi_monitor_table_51_and_59(spifiltermaster,spifilter,flash_dev,10);
	// spi_monitor_table_52_and_60(spifiltermaster,spifilter,flash_dev,10);
	// spi_monitor_table_67(spifiltermaster,spifilter,flash_dev,10);
	// spi_monitor_table_67_test(spifiltermaster,spifilter,flash_dev,10);
	// spi_monitor_table_68(spifiltermaster,spifilter,flash_dev,10);
	// spi_monitor_table_69_75_141_147(spifiltermaster,spifilter,flash_dev,10);
	// spi_monitor_table_70_76_142_148(spifiltermaster,spifilter,flash_dev,10);
	spi_monitor_table_83_155(spifiltermaster,spifilter,flash_dev,10);
	// spi_monitor_table_84_156(spifiltermaster,spifilter,flash_dev,10);
	// spi_monitor_table_85_91_109_115(spifiltermaster,spifilter,flash_dev,10);
	// spi_monitor_table_86_92_110_116(spifiltermaster,spifilter,flash_dev,10);
	// spi_monitor_table_3e_123(spifiltermaster,spifilter,flash_dev,10);
	// spi_monitor_table_3e_124(spifiltermaster,spifilter,flash_dev,10);
	// spi_monitor_table_125_131(spifiltermaster,spifilter,flash_dev,10);
	// spi_monitor_table_139(spifiltermaster,spifilter,flash_dev,10);
	// spi_monitor_table_140(spifiltermaster,spifilter,flash_dev,10);
	LOG_INF("---- Monitor Mode Test Done ----");
	return 0;
}
#endif//01-003-0005
