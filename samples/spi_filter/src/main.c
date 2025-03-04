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
#include <reg_spi_filter.h>
#include <spi_filter.h>

int main(void)
{
    const struct device *const spifilter = DEVICE_DT_GET(DT_ALIAS(spif));
    linkedsemi_spi_filter_cold_reset(spifilter);
    spif_dump_cmd_table(spifilter);
    spif_dump_rw_addr_privilege_table(spifilter);
    spif_add_cmd(spifilter, CMD_READ, 0);
    spif_dump_cmd_table(spifilter);
    spif_dump_rw_addr_privilege_table(spifilter);

    return 0;
}
