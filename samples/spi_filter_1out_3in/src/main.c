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

uint8_t buf_r[4096];

int test(void)
{
    do {
        uint8_t id[3] = { 0 };
        const struct device *const flash_dev = DEVICE_DT_GET(DT_NODELABEL(flash3_0));
        // spi_nor_re_init(flash_dev);
        int rc = flash_read_jedec_id(flash_dev, id);
        if (rc == 0) {
            printk("jedec-id = [%02x %02x %02x];\n", id[0], id[1], id[2]);
        } else {
            printk("JEDEC ID read failed: %d\n", rc);
        }

        // flash_read(flash_dev, MB(16), buf_r, sizeof(buf_r));
    } while(0);

    return 0;
}

int main(void)
{
    const struct device *const spifilter = DEVICE_DT_GET(DT_ALIAS(spif));
    bool check;

    linkedsemi_spi_filter_cold_reset(spifilter);
    spif_dump_cmd_table(spifilter);
    spif_dump_rw_addr_privilege_table(spifilter);
    // spif_add_cmd(spifilter, CMD_READ);
    spif_dump_cmd_table(spifilter);
    spif_dump_rw_addr_privilege_table(spifilter);

    spi_filter_dma_thread_init(spifilter);
    spif_dma_start(spifilter);
    spif_filter_enable(spifilter, true);
    check = spif_pinctrl_filter_mode_check(spifilter);
    __ASSERT_NO_MSG(check);
    spif_passthrough_analog_mux_enable(spifilter, true);
    check = spif_pinctrl_passthrough_mode_check(spifilter);
    __ASSERT_NO_MSG(check);

    do {
        const struct device *const spifilter = DEVICE_DT_GET(DT_NODELABEL(spif1));
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

    bool flag = false;
    for (int i = 0; i < 5; i ++) {
        test();

        spif_operation_mode_config(spifilter, flag);
        flag = !flag;
    }

    return 0;
}
