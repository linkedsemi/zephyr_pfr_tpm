/*
 * Copyright (c) 2024 Linkedsemi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if !defined(CONFIG_SPI_FILTER_LINKEDSEMI)
    #error no CONFIG_SPI_FILTER_LINKEDSEMI define
#endif
// #if !defined(CONFIG_SPI)
//     #error no CONFIG_SPI define
// #endif

#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <stdlib.h>
#include <stdio.h>
#include <zephyr/kernel.h>
#include <reg_spi_filter.h>
#include <spi_filter.h>

#include "ls_soc_gpio.h"
#include "per_func_mux.h"
#include "ls_hal_ssi.h"

void pinmux_spi1_mst_init(void)
{
    pinmux_cfg_pin_func_alt(SPI1_MST_CLK_FUNC3_PG01_PIN, SPI1_MST_CLK_FUNC3_PG01_FUNC, 0);
    pinmux_cfg_pin_func_alt(SPI1_MST_CSN_FUNC3_PG03_PIN, SPI1_MST_CSN_FUNC3_PG03_FUNC, 0);
    pinmux_cfg_pin_func_alt(SPI1_MST_IO0_FUNC3_PF15_PIN, SPI1_MST_IO0_FUNC3_PF15_FUNC, 0);
    pinmux_cfg_pin_func_alt(SPI1_MST_IO1_FUNC3_PF13_PIN, SPI1_MST_IO1_FUNC3_PF13_FUNC, 0);
    pinmux_cfg_pin_func_alt(SPI1_MST_IO2_FUNC3_PG07_PIN, SPI1_MST_IO2_FUNC3_PG07_FUNC, 0);
    pinmux_cfg_pin_func_alt(SPI1_MST_IO3_FUNC3_PG05_PIN, SPI1_MST_IO3_FUNC3_PG05_FUNC, 0);

    io_cfg_input(SPI1_MST_IO0_FUNC3_PF15_PIN);
    io_cfg_input(SPI1_MST_IO1_FUNC3_PF13_PIN);
    io_cfg_input(SPI1_MST_IO2_FUNC3_PG07_PIN);
    io_cfg_input(SPI1_MST_IO3_FUNC3_PG05_PIN);
}

SSI_HandleTypeDef SsiHandle = {0};
void spi_init(void)
{
    pinmux_spi1_mst_init();
    SsiHandle.REG = (reg_ssi_t *)APP_DWSPI3_ADDR;
    SsiHandle.Init.clk_div = 128;
    SsiHandle.Init.rxsample_dly = 0;
    SsiHandle.Init.ctrl.cph = SCLK_Toggle_In_Middle;
    SsiHandle.Init.ctrl.cpol = Inactive_Low;
    SsiHandle.Init.ctrl.data_frame_size = DFS_32_8_bits;

    if (HAL_SSI_Init(&SsiHandle) != HAL_OK) {
        /* Initialization Error */
        while(1);
    }
}

uint32_t g_cnt = 0;
uint8_t g_cmd = 0;
uint8_t g_test_cmd = 0;
static void spif_callback(const struct device *dev,
                          uint32_t callback_idx,
                          void *user_data,
                          void *drv_data)
{
    ARG_UNUSED(dev);
    ARG_UNUSED(callback_idx);
    ARG_UNUSED(user_data);
    ARG_UNUSED(drv_data);

    printk("g_cnt: %d  g_cmd: %#x  g_test_cmd: %#x\n", g_cnt++, g_cmd, g_test_cmd);
}

void test_general_cmd(const struct device *const spifilter)
{
    for (/*uint8_t*/ g_cmd = 0; g_cmd <= 0xff; g_cmd++) {
        printf("----------------------------------------------------\n");
        printf("spif_add_general_cmd: %#x\n", g_cmd);
        spif_add_general_cmd(spifilter, g_cmd);
        for (/*uint8_t*/ g_test_cmd = 0; g_test_cmd <= 0xff; g_test_cmd++) {
            if (HAL_SSI_Transmit(&SsiHandle, &g_test_cmd, 1) != HAL_OK) {
                /* Transfer error in transmission process */
                while(1);
            }
            if (g_test_cmd == 0xff) {
                break;
            }
        }
        if (HAL_SSI_Transmit(&SsiHandle, &g_cmd, 1) != HAL_OK) {
            /* Transfer error in transmission process */
            while(1);
        }
        spif_remove_general_cmd(spifilter, g_cmd);
        printf("----------------------------------------------------\n");
        if (g_cmd == 0xff) {
            break;
        }
    }
}

void test_spicmd_spi3b(const struct device *const spifilter)
{
    do {
        g_cmd = CMD_PAGE_PROGRAM;
        uint8_t tx_data[] = {g_cmd, 0x1, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        spif_address_privilege_config(spifilter,
                                    FLAG_ADDR_PRIV_WRITE_SELECT,
                                    FLAG_ADDR_PRIV_DISABLE,
                                    MB(0),
                                    MB(256));
        printf("spif_add_cmd: %#x\n", g_cmd);
        spif_add_cmd(spifilter, g_cmd);
        if (HAL_SSI_Transmit(&SsiHandle, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            /* Transfer error in transmission process */
            while(1);
        }
    } while(0);
    do {
        g_cmd = CMD_PAGE_PROGRAM;
        uint8_t tx_data[] = {g_cmd, 0x1, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        spif_address_privilege_config(spifilter,
                                    FLAG_ADDR_PRIV_WRITE_SELECT,
                                    FLAG_ADDR_PRIV_ENABLE,
                                    MB(0),
                                    MB(256));
        printf("spif_add_cmd: %#x\n", g_cmd);
        spif_add_cmd(spifilter, g_cmd);
        if (HAL_SSI_Transmit(&SsiHandle, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            /* Transfer error in transmission process */
            while(1);
        }
    } while(0);
    do {
        g_cmd = CMD_PAGE_PROGRAM;
        uint8_t tx_data[] = {g_cmd, 0x1, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        spif_address_privilege_config(spifilter,
                                    FLAG_ADDR_PRIV_WRITE_SELECT,
                                    FLAG_ADDR_PRIV_DISABLE,
                                    MB(0),
                                    MB(8));
        spif_address_privilege_config(spifilter,
                                    FLAG_ADDR_PRIV_WRITE_SELECT,
                                    FLAG_ADDR_PRIV_ENABLE,
                                    MB(8),
                                    MB(16));
        printf("spif_add_cmd: %#x\n", g_cmd);
        spif_add_cmd(spifilter, g_cmd);
        if (HAL_SSI_Transmit(&SsiHandle, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            /* Transfer error in transmission process */
            while(1);
        }
    } while(0);
    do {
        g_cmd = CMD_PAGE_PROGRAM;
        uint8_t tx_data[] = {g_cmd, 0x1, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        spif_address_privilege_config(spifilter,
                                    FLAG_ADDR_PRIV_WRITE_SELECT,
                                    FLAG_ADDR_PRIV_ENABLE,
                                    MB(0),
                                    MB(8));
        spif_address_privilege_config(spifilter,
                                    FLAG_ADDR_PRIV_WRITE_SELECT,
                                    FLAG_ADDR_PRIV_DISABLE,
                                    MB(8),
                                    MB(16));
        printf("spif_add_cmd: %#x\n", g_cmd);
        spif_add_cmd(spifilter, g_cmd);
        if (HAL_SSI_Transmit(&SsiHandle, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            /* Transfer error in transmission process */
            while(1);
        }
    } while(0);
    do {
        g_cmd = CMD_PAGE_PROGRAM;
        uint8_t tx_data[] = {g_cmd, 0x7, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        spif_address_privilege_config(spifilter,
                                    FLAG_ADDR_PRIV_WRITE_SELECT,
                                    FLAG_ADDR_PRIV_DISABLE,
                                    MB(0),
                                    MB(8));
        spif_address_privilege_config(spifilter,
                                    FLAG_ADDR_PRIV_WRITE_SELECT,
                                    FLAG_ADDR_PRIV_ENABLE,
                                    MB(8),
                                    MB(16));
        printf("spif_add_cmd: %#x\n", g_cmd);
        spif_add_cmd(spifilter, g_cmd);
        if (HAL_SSI_Transmit(&SsiHandle, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            /* Transfer error in transmission process */
            while(1);
        }
    } while(0);
    do {
        g_cmd = CMD_PAGE_PROGRAM;
        uint8_t tx_data[] = {g_cmd, 0x7, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        spif_address_privilege_config(spifilter,
                                    FLAG_ADDR_PRIV_WRITE_SELECT,
                                    FLAG_ADDR_PRIV_ENABLE,
                                    MB(0),
                                    MB(8));
        spif_address_privilege_config(spifilter,
                                    FLAG_ADDR_PRIV_WRITE_SELECT,
                                    FLAG_ADDR_PRIV_DISABLE,
                                    MB(8),
                                    MB(16));
        printf("spif_add_cmd: %#x\n", g_cmd);
        spif_add_cmd(spifilter, g_cmd);
        if (HAL_SSI_Transmit(&SsiHandle, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            /* Transfer error in transmission process */
            while(1);
        }
    } while(0);
}

void test_spicmd_spi4b(const struct device *const spifilter)
{
    do {
        g_cmd = CMD_4BYTE_MODE_ENTER;
        printf("spif_add_cmd: %#x\n", g_cmd);
        spif_add_cmd(spifilter, g_cmd);
        if (HAL_SSI_Transmit(&SsiHandle, &g_cmd, 1) != HAL_OK) {
            /* Transfer error in transmission process */
            while(1);
        }
    } while(0);
    do {
        g_cmd = CMD_4BYTE_PAGE_PROGRAM;
        uint8_t tx_data[] = {g_cmd, 0x1, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        spif_address_privilege_config(spifilter,
                                    FLAG_ADDR_PRIV_WRITE_SELECT,
                                    FLAG_ADDR_PRIV_DISABLE,
                                    MB(0),
                                    MB(256));
        printf("spif_add_cmd: %#x\n", g_cmd);
        spif_add_cmd(spifilter, g_cmd);
        if (HAL_SSI_Transmit(&SsiHandle, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            /* Transfer error in transmission process */
            while(1);
        }
    } while(0);
    do {
        g_cmd = CMD_4BYTE_PAGE_PROGRAM;
        uint8_t tx_data[] = {g_cmd, 0x1, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        spif_address_privilege_config(spifilter,
                                    FLAG_ADDR_PRIV_WRITE_SELECT,
                                    FLAG_ADDR_PRIV_ENABLE,
                                    MB(0),
                                    MB(256));
        printf("spif_add_cmd: %#x\n", g_cmd);
        spif_add_cmd(spifilter, g_cmd);
        if (HAL_SSI_Transmit(&SsiHandle, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            /* Transfer error in transmission process */
            while(1);
        }
    } while(0);
    do {
        g_cmd = CMD_4BYTE_PAGE_PROGRAM;
        uint8_t tx_data[] = {g_cmd, 0x1, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        spif_address_privilege_config(spifilter,
                                    FLAG_ADDR_PRIV_WRITE_SELECT,
                                    FLAG_ADDR_PRIV_DISABLE,
                                    MB(0),
                                    MB(128));
        spif_address_privilege_config(spifilter,
                                    FLAG_ADDR_PRIV_WRITE_SELECT,
                                    FLAG_ADDR_PRIV_ENABLE,
                                    MB(128),
                                    MB(256));
        printf("spif_add_cmd: %#x\n", g_cmd);
        spif_add_cmd(spifilter, g_cmd);
        if (HAL_SSI_Transmit(&SsiHandle, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            /* Transfer error in transmission process */
            while(1);
        }
    } while(0);
    do {
        g_cmd = CMD_4BYTE_PAGE_PROGRAM;
        uint8_t tx_data[] = {g_cmd, 0x1, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        spif_address_privilege_config(spifilter,
                                    FLAG_ADDR_PRIV_WRITE_SELECT,
                                    FLAG_ADDR_PRIV_ENABLE,
                                    MB(0),
                                    MB(128));
        spif_address_privilege_config(spifilter,
                                    FLAG_ADDR_PRIV_WRITE_SELECT,
                                    FLAG_ADDR_PRIV_DISABLE,
                                    MB(128),
                                    MB(256));
        printf("spif_add_cmd: %#x\n", g_cmd);
        spif_add_cmd(spifilter, g_cmd);
        if (HAL_SSI_Transmit(&SsiHandle, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            /* Transfer error in transmission process */
            while(1);
        }
    } while(0);
    do {
        g_cmd = CMD_4BYTE_PAGE_PROGRAM;
        uint8_t tx_data[] = {g_cmd, 0x7, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        spif_address_privilege_config(spifilter,
                                    FLAG_ADDR_PRIV_WRITE_SELECT,
                                    FLAG_ADDR_PRIV_DISABLE,
                                    MB(0),
                                    MB(128));
        spif_address_privilege_config(spifilter,
                                    FLAG_ADDR_PRIV_WRITE_SELECT,
                                    FLAG_ADDR_PRIV_ENABLE,
                                    MB(128),
                                    MB(256));
        printf("spif_add_cmd: %#x\n", g_cmd);
        spif_add_cmd(spifilter, g_cmd);
        if (HAL_SSI_Transmit(&SsiHandle, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            /* Transfer error in transmission process */
            while(1);
        }
    } while(0);
    do {
        g_cmd = CMD_4BYTE_PAGE_PROGRAM;
        uint8_t tx_data[] = {g_cmd, 0x7, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        spif_address_privilege_config(spifilter,
                                    FLAG_ADDR_PRIV_WRITE_SELECT,
                                    FLAG_ADDR_PRIV_ENABLE,
                                    MB(0),
                                    MB(128));
        spif_address_privilege_config(spifilter,
                                    FLAG_ADDR_PRIV_WRITE_SELECT,
                                    FLAG_ADDR_PRIV_DISABLE,
                                    MB(128),
                                    MB(256));
        printf("spif_add_cmd: %#x\n", g_cmd);
        spif_add_cmd(spifilter, g_cmd);
        if (HAL_SSI_Transmit(&SsiHandle, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            /* Transfer error in transmission process */
            while(1);
        }
    } while(0);
}

int main(void)
{
    const struct device *const spifilter = DEVICE_DT_GET(DT_ALIAS(spifilter));
    int ret = linkedsemi_spif_register_callback(spifilter, 0, spif_callback, NULL);
    if (ret) {
        return -1;
    }
    spi_init();

    test_spicmd_spi3b(spifilter);
    test_spicmd_spi4b(spifilter);
#if 0
    test_spicmd_qpi3b(spifilter); //TBD
    test_spicmd_qpi4b(spifilter); //TBD
    test_qpi_3b(spifilter); //TBD
    test_qpi_4b(spifilter); //TBD
#endif
    test_general_cmd(spifilter);

    printf("done\n");
    while(1);

    return 0;
}
