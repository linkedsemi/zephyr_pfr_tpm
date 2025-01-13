/*
 * Copyright (c) 2024 Linkedsemi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
1. set bitmap
2. for i in data_len:
        xxx
        for j in command_code:
            yyy
            gen rand data,
            expect ret val
3. gen waveform
   compare ret val
*/

#if !defined(CONFIG_I2C_FILTER_LINKEDSEMI)
    #error no CONFIG_I2C_FILTER_LINKEDSEMI define
#endif
#if !defined(CONFIG_I2C)
    #error no CONFIG_I2C define
#endif

#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <stdlib.h>
#include <stdio.h>
#include <zephyr/kernel.h>
#include <i2c_filter.h>
// #include "ls_soc_gpio.h"

#define WHT_ADDR_0 0x50
#define BUF_SIZE   256
uint8_t wdata_buf[BUF_SIZE];
uint8_t rdata_buf[BUF_SIZE];

/* set whitelist mode on/off */
static bool is_whitelist_on = true;

static void gen_rand_data(uint8_t *buf, uint16_t len)
{
    while (len--) {
        *buf++ = rand();
    }
}

static int write_read_compare(const struct device *const i2c, uint16_t dev_addr, uint8_t start_addr, const uint8_t *wdata, uint8_t *rdata, uint16_t len)
{
    i2c_burst_write(i2c, dev_addr, start_addr, wdata, len);
#if 0
    io_toggle_pin(PA02);
#endif
    i2c_burst_read(i2c, dev_addr, start_addr, rdata, len);
    if (memcmp(wdata, rdata, len)) {
        printf("len: %d\n", len);
        printf("start_addr: %#x\n", start_addr);
        printf("wdata: ");
        for (uint32_t i = 0; i < len; i++) {
            printf("%x ", wdata[i]);
        }
        printf("\n\n");
        printf("len: %d\n", len);
        printf("start_addr: %#x\n", start_addr);
        printf("rdata: ");
        for (uint32_t i = 0; i < len; i++) {
            printf("%x ", rdata[i]);
        }
        printf("\n\n");
        // __ASSERT(0, "wdata rdata not match\n");
        printf("wdata rdata not match\n");
        return -1;
    }
#if 1
    else {
        printf("len: %d\n", len);
        printf("start_addr: %#x\n", start_addr);
        printf("data: ");
        for (uint32_t i = 0; i < len; i++) {
            printf("%x ", wdata[i]);
        }
        printf("\n\n");
    }
#endif
    return 0;
}

int main(void)
{
    const struct device *const i2cmaster1 = DEVICE_DT_GET(DT_ALIAS(i2cmaster1));
    static const struct device *eeprom1 = DEVICE_DT_GET(DT_ALIAS(eeprom1));
    const struct device *const i2cfilter = DEVICE_DT_GET(DT_ALIAS(i2cfilter));
    uint32_t bitmap[LINKEDSEMI_I2C_F_REMAP_SIZE_U32] = { 0 };
    uint16_t dev_addr = WHT_ADDR_0;

    if (!device_is_ready(i2cmaster1)) {
        __ASSERT(0, "I2C device is not ready\n");
    }
    if (i2c_configure(i2cmaster1, I2C_SPEED_SET(I2C_SPEED_STANDARD) | I2C_MODE_CONTROLLER)) {
        __ASSERT(0, "I2C device config failed\n");
    }
    if (!device_is_ready(eeprom1)) {
        printf("eeprom device not ready\n");
        return 0;
    }
    if (i2c_target_driver_register(eeprom1) < 0) {
        printf("Failed to register i2c target driver\n");
        return 0;
    }
#if 0
uint8_t bit = 0;
uint16_t len = 2;
uint16_t start_addr = 1;
memset(rdata_buf, 0, len);
sys_bitfield_set_bit((mem_addr_t)bitmap, bit);
linkedsemi_i2c_filter_en(i2cfilter, false, true, false); /* close filter */
linkedsemi_i2c_filter_fill_bitmap(i2cfilter, 0, WHT_ADDR_0, bitmap); /* set bitmap */
linkedsemi_i2c_filter_en(i2cfilter, true, true, false); /* reopen filter */
// i2c_burst_read(i2cmaster1, 0x50, 0x0, rdata_buf, len);
wdata_buf[0] = 0xe7;
wdata_buf[1] = 0x2;
write_read_compare(i2cmaster1, dev_addr, start_addr++, wdata_buf, rdata_buf, len);
// write_read_compare(i2cmaster1, dev_addr, start_addr++, wdata_buf, rdata_buf, len);
// write_read_compare(i2cmaster1, dev_addr, start_addr++, wdata_buf, rdata_buf, len);
while(1);
#endif

#if 0
uint8_t bit = 0;
uint16_t len = 2;
uint16_t start_addr = 0;
memset(rdata_buf, 0, len);
sys_bitfield_set_bit((mem_addr_t)bitmap, bit);
linkedsemi_i2c_filter_en(i2cfilter, false, true, false); /* close filter */
linkedsemi_i2c_filter_fill_bitmap(i2cfilter, 0, WHT_ADDR_0, bitmap); /* set bitmap */
linkedsemi_i2c_filter_en(i2cfilter, true, true, false); /* reopen filter */
// i2c_burst_read(i2cmaster1, 0x50, 0x0, rdata_buf, len);
wdata_buf[0] = 0xe7;
wdata_buf[1] = 0x94;
write_read_compare(i2cmaster1, dev_addr, start_addr++, wdata_buf, rdata_buf, len);
write_read_compare(i2cmaster1, dev_addr, start_addr++, wdata_buf, rdata_buf, len);
// write_read_compare(i2cmaster1, dev_addr, start_addr++, wdata_buf, rdata_buf, len);
while(1);
#endif
    // while (1) {;
    for (uint8_t bit = 0; bit <= 0xff; bit++) {
        printf("\n------------------------------------------------------\n");
        sys_bitfield_set_bit((mem_addr_t)bitmap, bit);
        linkedsemi_i2c_filter_en(i2cfilter, false, is_whitelist_on, false); /* disable filter */
        linkedsemi_i2c_filter_fill_bitmap(i2cfilter, 0, WHT_ADDR_0, bitmap); /* set bitmap */
        linkedsemi_i2c_filter_en(i2cfilter, true, is_whitelist_on, false); /* enable filter */
        for (uint16_t len = 1; len <= BUF_SIZE; len++) {
            printf("\n++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
            for (uint16_t start_addr = 0; start_addr <= 0xff; start_addr++) {
                printf("\n******************************************************\n");
                uint16_t page_len = BUF_SIZE - start_addr;
                uint16_t curr_len = len < page_len ? len : page_len;

                memset(rdata_buf, 0, curr_len);
                do {
                    gen_rand_data(wdata_buf, curr_len);
//#if 0
                } while(wdata_buf[0] == 0);
// #else
//                     wdata_buf[0] &= 0x7f;
//                 } while ((wdata_buf[0] == 0) || (wdata_buf[0] & BIT(7)));
// #endif
                    int expect_ret = -1;
                    if (sys_bitfield_test_bit((mem_addr_t)bitmap, start_addr)) {
                        expect_ret = 0;
                    }
                    int ret = write_read_compare(i2cmaster1, dev_addr, start_addr, wdata_buf, rdata_buf, curr_len);
                    if (ret != expect_ret) {
                        if (is_whitelist_on) {
                            __ASSERT(0, "filter not work\n");
                        } else {
                            printf("filter not work\n");
                        }
                    }
                    printf("bit: %d  len: %d  start_addr: %#x\n", bit, len, start_addr);
                    printf("result: pass\n");
                    memset(wdata_buf, 0, curr_len);
                    i2c_burst_write(i2cmaster1, dev_addr, start_addr, wdata_buf, curr_len);
                }
            }
            sys_bitfield_clear_bit((mem_addr_t)bitmap, bit);
        }
    // }

    return 0;
}
