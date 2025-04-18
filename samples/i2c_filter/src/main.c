/*
 * Copyright (c) 2024 Linkedsemi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if !defined(CONFIG_I2C_FILTER_LINKEDSEMI)
    #error no CONFIG_I2C_FILTER_LINKEDSEMI define
#endif

#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <i2c_filter.h>
#include "ls_soc_gpio.h"
#include "per_func_mux.h"

int main(void)
{

    const struct device *const i2cfilter = DEVICE_DT_GET(DT_ALIAS(i2cfilter));
    uint32_t dump_bitmap[LINKEDSEMI_I2C_F_REMAP_SIZE_U32] = {};
    uint8_t index = 0;
    uint8_t addr = 0x50;
    uint8_t dump_addr = 0;
    linkedsemi_i2c_filter_cold_reset(i2cfilter);

    // io_cfg_output(PD11);
    // io_clr_pin(PD11);

    int flag = 0;
    while(1) {
        switch (flag) {
        case 0:
            do {
                /* filter */
                // io_clr_pin(PD11);
                // io_set_pin(PD11);
                // io_clr_pin(PD11);
                linkedsemi_i2c_filter_enable_channel(i2cfilter, index, false);
                linkedsemi_i2c_filter_en(i2cfilter, true, true, false);
            } while(0);
            flag++;
            break;
        case 1:
            do {
                /* filter */
                // io_clr_pin(PD11);
                // io_set_pin(PD11);
                // io_clr_pin(PD11);
                // io_set_pin(PD11);
                // io_clr_pin(PD11);
                uint32_t bitmap[LINKEDSEMI_I2C_F_REMAP_SIZE_U32] = {};
                uint8_t bit = 8;
                sys_bitfield_set_bit((mem_addr_t)bitmap, bit);
                linkedsemi_i2c_filter_fill_bitmap(i2cfilter, index, addr, bitmap);
                linkedsemi_i2c_filter_dump_bitmap(i2cfilter, index, &dump_addr, dump_bitmap);
                int ret = memcmp(bitmap, dump_bitmap, LINKEDSEMI_I2C_F_REMAP_SIZE_BYTE);
                __ASSERT_NO_MSG(ret == 0);
                __ASSERT_NO_MSG(addr == dump_addr);
                linkedsemi_i2c_filter_en(i2cfilter, true, true, false);
            } while(0);
            flag++;
            break;
        case 2:
            /* monitor */
            // io_clr_pin(PD11);
            // io_set_pin(PD11);
            // io_clr_pin(PD11);
            // io_set_pin(PD11);
            // io_clr_pin(PD11);
            linkedsemi_i2c_filter_en(i2cfilter, true, false, false);

            flag++;
            break;
        case 3:
            /* master */
            // io_clr_pin(PD11);
            // io_set_pin(PD11);
            // io_clr_pin(PD11);
            // io_set_pin(PD11);
            // io_clr_pin(PD11);
            // io_set_pin(PD11);
            // io_clr_pin(PD11);
            const struct device *const i2cmaster = i2cf_i2c_dev(i2cfilter);
            uint8_t rdata[LINKEDSEMI_I2C_F_REMAP_SIZE_U32] = {};
            uint8_t start_addr = 0;
            uint8_t len = 0x10;
            linkedsemi_i2c_filter_switch_to_master(i2cfilter);
            i2c_burst_read(i2cmaster, addr, start_addr, rdata, len);
            linkedsemi_i2c_filter_switch_to_filter(i2cfilter);
            flag=0;
            break;
        }

        k_msleep(30);
    }

    return 0;
}
