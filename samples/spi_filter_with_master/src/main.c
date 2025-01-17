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

int main(void)
{
    const struct device *const spifilter = DEVICE_DT_GET(DT_ALIAS(spifilter));
    uint8_t test_cmd = 0x5a; //0xce
    uint8_t chip_erase = 0xce;
    uint8_t cmd_read = CMD_READ;

    // spif_dump_cmd_table(spifilter);
    // spif_dump_rw_addr_privilege_table(spifilter);
    spif_add_cmd(spifilter, CMD_READ);
    // spif_dump_cmd_table(spifilter);
    // spif_dump_rw_addr_privilege_table(spifilter);

    spi_init();
    if (HAL_SSI_Transmit(&SsiHandle, &test_cmd, 1) != HAL_OK) {
        /* Transfer error in transmission process */
        while(1);
    }
    if (HAL_SSI_Transmit(&SsiHandle, &chip_erase, 1) != HAL_OK) {
        /* Transfer error in transmission process */
        while(1);
    }
    if (HAL_SSI_Transmit(&SsiHandle, &cmd_read, 1) != HAL_OK) {
        /* Transfer error in transmission process */
        while(1);
    }
    while(1);

    return 0;
}
