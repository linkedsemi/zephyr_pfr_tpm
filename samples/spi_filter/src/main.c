/*
 * Copyright (c) 2024 Linkedsemi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if !defined(CONFIG_SPI_FILTER_LINKEDSEMI)
    #error no CONFIG_SPI_FILTER_LINKEDSEMI define
#endif

#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <stdlib.h>
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>
#include <reg_spi_filter.h>
#include <spi_filter.h>

int main(void)
{
    const struct device *const spifilter = DEVICE_DT_GET(DT_ALIAS(spif));
    linkedsemi_spi_filter_cold_reset(spifilter);
    spif_dump_cmd_table(spifilter);
    spif_dump_rw_addr_privilege_table(spifilter);
    // spif_add_cmd(spifilter, CMD_READ);
    spif_dump_cmd_table(spifilter);
    spif_dump_rw_addr_privilege_table(spifilter);

    int flag = 1;
    while(1) {
        switch (flag) {
        case 0:
            flag++;
            break;
        case 1:
            spif_switch_to_master(spifilter);
            const struct device *spi_dev = spif_spi_dev(spifilter);
            static const uint8_t tx_buffer[] = {0x9f};
            static uint8_t rx_buffer[3];
            const struct spi_config spi_dev_cfg = {
                .operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_TRANSFER_MSB,
                .frequency = MHZ(1),
                .cs = {
                    .gpio = *(spif_spi_cs(spifilter)),
                },
            };
            static const struct spi_buf tx_buf = {
                .buf = (void *)tx_buffer,
                .len = sizeof(tx_buffer)
            };
            static const struct spi_buf_set tx = {
                .buffers = &tx_buf,
                .count = 1
            };
            static const struct spi_buf rx_buf = {
                .buf = rx_buffer,
                .len = sizeof(rx_buffer),
            };
            static const struct spi_buf_set rx = {
                .buffers = &rx_buf,
                .count = 1
            };
            int ret;

            ret = spi_transceive(spi_dev, &spi_dev_cfg, &tx, &rx);
            if (ret < 0) {
                printk("transfer failed: %d\n", ret);
                return false;
            }

            spif_switch_to_filter(spifilter);

            flag=0;
            break;
        }

        k_msleep(50);
    }

    return 0;
}
