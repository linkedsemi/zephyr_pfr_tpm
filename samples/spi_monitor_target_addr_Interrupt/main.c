/*
 * Copyright (c) 2024 Linkedsemi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

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
#include "/home/htliu/zephyr_work/modules/lib/pfr_tpm/drivers/pfr/spi/spi_filter.h"

#define FLASH_OFFSET MB(0)
#define FLASH_ERASE_SIZE 4096
#define FLASH_TEST_SIZE 4096
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

int test_spif(void)
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
    } while(0);

    return 0;
}

int main(void)
{
    const struct device *const spifilter = DEVICE_DT_GET(DT_ALIAS(spifilter));
    bool check;

    linkedsemi_spi_filter_cold_reset(spifilter);
    spif_dump_cmd_table(spifilter);
    spif_dump_rw_addr_privilege_table(spifilter);
    // spif_add_cmd(spifilter, CMD_READ);
    // spif_add_cmd(spifilter, 0x9f);
    spif_add_cmd(spifilter, 0x5f);
    spif_add_cmd(spifilter, 0x5a);
    spif_add_cmd(spifilter, 0xb7);
    spif_add_cmd(spifilter, 0x35);
    spif_add_cmd(spifilter, 0xec);
    spif_add_cmd(spifilter, 0xeb);
    spif_add_cmd(spifilter, 0x06);
    spif_add_cmd(spifilter, 0x21);
    spif_add_cmd(spifilter, 0x02);
    spif_add_cmd(spifilter, 0x05);
    spif_add_cmd(spifilter, 0xff);
    spif_add_cmd(spifilter, 0x66);
    spif_add_cmd(spifilter, 0x99);
    spif_memset_addr_whitelist(spifilter, 1);
    spif_flash_size_set(spifilter, MB(32));
    spif_address_privilege_config(spifilter, FLAG_ADDR_PRIV_READ_SELECT, FLAG_ADDR_PRIV_DISABLE, TEST_START_ADDR, TEST_TOTAL_SIZE);
    spif_address_privilege_config(spifilter, FLAG_ADDR_PRIV_WRITE_SELECT, FLAG_ADDR_PRIV_DISABLE, TEST_START_ADDR, TEST_TOTAL_SIZE);
    spif_dump_cmd_table(spifilter);
    spif_dump_rw_addr_privilege_table(spifilter);

    // spi_filter_dma_thread_init(spifilter);
    spif_dma_start(spifilter);
    spif_filter_enable(spifilter, true);
    check = spif_pinctrl_filter_mode_check(spifilter);
    __ASSERT_NO_MSG(check);
    spif_passthrough_analog_mux_enable(spifilter, true);
    check = spif_pinctrl_passthrough_mode_check(spifilter);
    __ASSERT_NO_MSG(check);

    do {
        const struct device *const spifilter = DEVICE_DT_GET(DT_ALIAS(spifilter));
        // linkedsemi_spi_filter_cold_reset(spifilter);
        spif_switch_to_master(spifilter);
        check = spif_pinctrl_master_mode_check(spifilter);
        __ASSERT_NO_MSG(check);
#if 0
#if 1
        spif_switch_to_filter(spifilter);
        check = spif_pinctrl_filter_mode_check(spifilter);
        __ASSERT_NO_MSG(check);
#endif

        const struct device *const spifilter2 = DEVICE_DT_GET(DT_NODELABEL(spif2));
        spif_switch_to_master(spifilter2);
        check = spif_pinctrl_master_mode_check(spifilter2);
        __ASSERT_NO_MSG(check);

        spif_switch_to_filter(spifilter2);
        check = spif_pinctrl_filter_mode_check(spifilter2);
        __ASSERT_NO_MSG(check);

        spif_switch_to_master(spifilter);
        check = spif_pinctrl_master_mode_check(spifilter);
        __ASSERT_NO_MSG(check);
#endif
    } while (0);

    // const struct device *const flash_dev = DEVICE_DT_GET(DT_ALIAS(spiflash));
    bool flag = false;
#if 0
    spi_nor_set_line_width(flash_dev, 4);
    spi_nor_set_freq(flash_dev, 50000000);
    spi_nor_re_init(flash_dev);
#endif
#if 0
    for (int i = 0; i < 1; i ++) {
        test();

        spif_operation_mode_config(spifilter, flag);
        flag = !flag;
    }
#endif
#if 0
    spi_nor_set_line_width(flash_dev, 2);
    spi_nor_set_freq(flash_dev, 5000000);
    spi_nor_re_init(flash_dev);
    for (int i = 0; i < 1; i ++) {
        test();

#if 0
        spif_operation_mode_config(spifilter, flag);
        flag = !flag;
#endif
    }
#endif
#if 0
    spi_nor_set_line_width(flash_dev, 1);
    spi_nor_set_freq(flash_dev, 10000000);
    int per_func_get_pi08 = per_func_get(PI08);
    int is_per_func_valid_pi08 = is_per_func_valid(PI08);
    printk("%d\n", per_func_get_pi08);
    printk("%d\n", is_per_func_valid_pi08);
    spi_nor_re_init(flash_dev);
    for (int i = 0; i < 1; i ++) {
        test();

#if 0
        spif_operation_mode_config(spifilter, flag);
        flag = !flag;
#endif
    }
#endif

    for (int i = 0; i < (SPIF_LOG_RAM_MAX_SIZE_U32 < 8 ? SPIF_LOG_RAM_MAX_SIZE_U32 : 8); i ++) {
        test_spif();
    }
    test_spif();
// __asm("ebreak"::);
//     io_cfg_output(PN14);
//     while(1) {
//         io_toggle_pin(PN14);
//     }
#if 0
    k_msleep(500);
    struct spim_log_info info;
	spim_get_log_info(spifilter, &info);
    uint32_t cur_off = info.log_idx_reg;
    uint32_t pre_off = 0;
	if (cur_off < pre_off) {
		for (int i = pre_off; i < info.log_max_sz / 4; i++) {
			spim_log_parser(spifilter, i,
				sys_read32(info.log_ram_addr + i * 4));
		}
		pre_off = 0;
	}
	for (int i = pre_off; i < cur_off; i++) {
		spim_log_parser(spifilter, i,
			sys_read32(info.log_ram_addr + i * 4));
	}
#endif
    return 0;
}
