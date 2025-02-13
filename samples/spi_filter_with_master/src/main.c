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
#include "ls_hal_dmacv3.h"

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
// DEF_DMA_CONTROLLER(dmac1_inst,DMAC1);
void spi_init(void)
{
    pinmux_spi1_mst_init();
    SsiHandle.REG = (reg_ssi_t *)APP_DWSPI3_ADDR;
    SsiHandle.Init.clk_div = 128;
    SsiHandle.Init.rxsample_dly = 0;
    SsiHandle.Init.ctrl.cph = SCLK_Toggle_In_Middle;
    SsiHandle.Init.ctrl.cpol = Inactive_Low;
    SsiHandle.Init.ctrl.data_frame_size = DFS_32_8_bits;

    // DMA_CONTROLLER_INIT(dmac1_inst);
    // SsiHandle.hdma_instance = &dmac1_inst;
    // SsiHandle.Tx_Env.DMA.DMA_Channel = 0;
    // SsiHandle.Rx_Env.DMA.DMA_Channel = 1;

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

    g_cnt++;
}

#include "test_w.c"
#include "test_r.c"

DEF_DMA_CONTROLLER(hdma_inst, DMAC1);
int main(void)
{
    const struct device *const spifilter = DEVICE_DT_GET(DT_ALIAS(spifilter));
    int ret = linkedsemi_spif_register_callback(spifilter, 0, spif_callback, NULL);
    if (ret) {
        return -1;
    }
    spi_init();
    // spif_dump_cmd_table(spifilter);
    // spif_dump_rw_addr_privilege_table(spifilter);

//sck check
    spif_clk_check_config(spifilter, 0, 2, BIT(12) - 1, true);

//dma log
#if 0
    DMA_CONTROLLER_INIT(hdma_inst);
    spif_dma_config(spifilter, &hdma_inst);
    test_general_cmd(spifilter);
    uint32_t *log_dma_buf = spif_log_dma_buf(spifilter);
    printf("log_dma_buf: %#x\n", log_dma_buf[0]);
    printf("log_dma_buf: %#x\n", log_dma_buf[1]);
    printf("log_dma_buf: %#x\n", log_dma_buf[2]);
    printf("log_dma_buf: %#x\n", log_dma_buf[3]);
    //...
#endif

#if 0
    test_general_cmd(spifilter);
    test_general_cmd_qpi(spifilter);
#endif

#if 0
//stand spi
// send FFh to disabled qpi
    test_w_spicmd_saddr3b(spifilter);// Page Program PP 02h 3 0 1+ program selected page
    test_w_spicmd_saddr4b(spifilter);// Page Program 4byte address PP4B 12h 4 0 1+ program selected page

//dual mode
// send FFh to disabled qpi
    // test_w_spicmd_saddr3b_ddata(spifilter); // duplicated
    // test_w_spicmd_saddr4b_ddata(spifilter); // duplicated

    // test_w_spicmd_daddr3b(spifilter); //TBD RO CMD xxx
    // test_w_spicmd_daddr4b(spifilter); //TBD RO CMD xxx

//quad mode
// send QPIEN
// EN4B
    test_w_spicmd_qaddr3b(spifilter); //TBD Quad-in page program QIPP C2h 3 0 1+ quad input to program selected page
    test_w_spicmd_qaddr4b(spifilter); //TBD Quad-in page program 4byte address QIPP4B 3Eh 4 0 1+ quad input to program selected page

    test_w_qpi_3b(spifilter); //TBD Page Program PP 02h 3 0 1+ program selected page
    test_w_qpi_4b(spifilter); //TBD Page Program 4byte address PP4B 12h 4 0 1+ program selected page
#endif

    // test_r_addr_dump(spifilter);
    // test_r_spicmd_daddr3b(spifilter);
#if 0
    test_r_spicmd_saddr3b(spifilter);
    test_r_spicmd_saddr4b(spifilter);

    // test_r_spicmd_saddr3b_ddata(spifilter); // duplicated
    // test_r_spicmd_saddr4b_ddata(spifilter); // duplicated

    test_r_spicmd_daddr3b(spifilter);
    test_r_spicmd_daddr4b(spifilter);

    test_r_spicmd_qaddr3b(spifilter);
    test_r_spicmd_qaddr4b(spifilter);

    test_r_qpi_3b(spifilter);
    test_r_qpi_4b(spifilter);
#endif


//cmd bitmap   10.
#if 0
    test_general_cmd(spifilter);
#endif

//addr overflow   10.
#if 0
    test_cmd_rd_addr_overflow(spifilter);
    test_cmd_wr_addr_overflow(spifilter);
#endif
    // spif_dump_cmd_table(spifilter);
    // spif_dump_rw_addr_privilege_table(spifilter);

    printf("g_cnt: %d\n", g_cnt);
    printf("done\n");
    while(1);

    return 0;
}
