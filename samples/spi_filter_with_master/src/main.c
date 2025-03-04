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
    pinmux_cfg_pin_func_alt(SPI1_MST_CLK_FUNC3_PG00_PIN, SPI1_MST_CLK_FUNC3_PG00_FUNC, 0);
    pinmux_cfg_pin_func_alt(SPI1_MST_CSN_FUNC3_PI08_PIN, SPI1_MST_CSN_FUNC3_PI08_FUNC, 0);
    pinmux_cfg_pin_func_alt(SPI1_MST_IO0_FUNC3_PF15_PIN, SPI1_MST_IO0_FUNC3_PF15_FUNC, 0);
    pinmux_cfg_pin_func_alt(SPI1_MST_IO1_FUNC3_PF13_PIN, SPI1_MST_IO1_FUNC3_PF13_FUNC, 0);
    pinmux_cfg_pin_func_alt(SPI1_MST_IO2_FUNC3_PH01_PIN, SPI1_MST_IO2_FUNC3_PH01_FUNC, 0);
    pinmux_cfg_pin_func_alt(SPI1_MST_IO3_FUNC3_PH03_PIN, SPI1_MST_IO3_FUNC3_PH03_FUNC, 0);

    io_cfg_input(SPI1_MST_IO0_FUNC3_PF15_PIN);
    io_cfg_input(SPI1_MST_IO1_FUNC3_PF13_PIN);
    io_cfg_input(SPI1_MST_IO2_FUNC3_PH01_PIN);
    io_cfg_input(SPI1_MST_IO3_FUNC3_PH03_PIN);
}

SSI_HandleTypeDef SsiHandle = {0};
// DEF_DMA_CONTROLLER(dmac1_inst,DMAC1);
void spi_init(void)
{
    pinmux_spi1_mst_init();
    SsiHandle.REG = (reg_ssi_t *)APP_DWSPI3_ADDR;
    SsiHandle.Init.clk_div = 500;
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
uint32_t g_cnt_expect = 0;
uint32_t g_cnt_last = 0;
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
    const struct device *const spifilter = DEVICE_DT_GET(DT_ALIAS(spif));
    int ret = linkedsemi_spif_register_callback(spifilter, 0, spif_callback, NULL);
    if (ret) {
        return -1;
    }
    spi_init();

    linkedsemi_spi_filter_cold_reset(spifilter);

//sck check
    spif_clk_check_config(spifilter, 0, 2, BIT(12) - 1, true);

//dma log
#if 1
    DMA_CONTROLLER_INIT(hdma_inst);
    spif_dma_config(spifilter, &hdma_inst);
#endif
#if 1
//stand spi
// send FFh to disabled qpi
    test_w_spicmd_saddr3b(spifilter);// Page Program PP 02h 3 0 1+ program selected page
    test_w_spicmd_saddr4b_by_en4b(spifilter);// Page Program 4byte address PP4B 12h 4 0 1+ program selected page
    test_w_spicmd_saddr4b_by_cmd(spifilter);// Page Program 4byte address PP4B 12h 4 0 1+ program selected page

#if DUPLICATED
//dual mode
// send FFh to disabled qpi
    // test_w_spicmd_saddr3b_ddata(spifilter); // duplicated
    // test_w_spicmd_saddr4b_ddata(spifilter); // duplicated

    // test_w_spicmd_daddr3b(spifilter); //TBD RO CMD xxx
    // test_w_spicmd_daddr4b(spifilter); //TBD RO CMD xxx
#endif

//quad mode
// send QPIEN
// EN4B
    test_w_spicmd_qaddr3b(spifilter); // Quad-in page program QIPP C2h 3 0 1+ quad input to program selected page
    test_w_spicmd_qaddr4b_by_en4b(spifilter); // Quad-in page program 4byte address QIPP4B 3Eh 4 0 1+ quad input to program selected page
    test_w_spicmd_qaddr4b_by_cmd(spifilter); // Quad-in page program 4byte address QIPP4B 3Eh 4 0 1+ quad input to program selected page

    test_w_qpi_3b(spifilter); // Page Program PP 02h 3 0 1+ program selected page
    test_w_qpi_4b_by_en4b(spifilter); // Page Program 4byte address PP4B 12h 4 0 1+ program selected page
    test_w_qpi_4b_by_cmd(spifilter); // Page Program 4byte address PP4B 12h 4 0 1+ program selected page
#endif

#if 1
    test_r_spicmd_saddr3b(spifilter);
    test_r_spicmd_saddr4b_by_en4b(spifilter);
    test_r_spicmd_saddr4b_by_cmd(spifilter);

#if DUPLICATED
    // test_r_spicmd_saddr3b_ddata(spifilter); // duplicated
    // test_r_spicmd_saddr4b_ddata(spifilter); // duplicated
#endif

    test_r_spicmd_daddr3b(spifilter);
    test_r_spicmd_daddr4b_by_en4b(spifilter);
    test_r_spicmd_daddr4b_by_cmd(spifilter);

    test_r_spicmd_qaddr3b(spifilter);
    test_r_spicmd_qaddr4b_by_en4b(spifilter);
    test_r_spicmd_qaddr4b_by_cmd(spifilter);

    test_r_qpi_3b(spifilter);
    test_r_qpi_4b_by_en4b(spifilter);
    test_r_qpi_4b_by_cmd(spifilter);
#endif

//addr overflow
#if 1
    test_cmd_rd_addr_overflow(spifilter);
    test_cmd_wr_addr_overflow(spifilter);
#endif

// target_addr
#if 1
    test_target_addr(spifilter);
#endif

#if 1
    test_general_cmd(spifilter);
    spif_dma_data_t *log_dma_buf = spif_log_dma_buf(spifilter);
    printf("log_dma_buf: %#8.8x  ADDR_ERR: %#x  CMD_ERR: %#x  POR_ADDR: %#x  ERROR_ADDR: %#x  ERROR_CMD: %#x\n", log_dma_buf[0].value, log_dma_buf[0].ADDR_ERR, log_dma_buf[0].CMD_ERR, log_dma_buf[0].POR_ADDR, log_dma_buf[0].ERROR_ADDR << 11, log_dma_buf[0].ERROR_CMD);
    printf("log_dma_buf: %#8.8x  ADDR_ERR: %#x  CMD_ERR: %#x  POR_ADDR: %#x  ERROR_ADDR: %#x  ERROR_CMD: %#x\n", log_dma_buf[1].value, log_dma_buf[1].ADDR_ERR, log_dma_buf[1].CMD_ERR, log_dma_buf[1].POR_ADDR, log_dma_buf[1].ERROR_ADDR << 11, log_dma_buf[1].ERROR_CMD);
    printf("log_dma_buf: %#8.8x  ADDR_ERR: %#x  CMD_ERR: %#x  POR_ADDR: %#x  ERROR_ADDR: %#x  ERROR_CMD: %#x\n", log_dma_buf[2].value, log_dma_buf[2].ADDR_ERR, log_dma_buf[2].CMD_ERR, log_dma_buf[2].POR_ADDR, log_dma_buf[2].ERROR_ADDR << 11, log_dma_buf[2].ERROR_CMD);
    printf("log_dma_buf: %#8.8x  ADDR_ERR: %#x  CMD_ERR: %#x  POR_ADDR: %#x  ERROR_ADDR: %#x  ERROR_CMD: %#x\n", log_dma_buf[3].value, log_dma_buf[3].ADDR_ERR, log_dma_buf[3].CMD_ERR, log_dma_buf[3].POR_ADDR, log_dma_buf[3].ERROR_ADDR << 11, log_dma_buf[3].ERROR_CMD);
    //...

    test_general_cmd_qpi(spifilter);
#endif

    printf("g_cnt: %d\n", g_cnt);
    printf("done\n");
    while(1);

    return 0;
}
