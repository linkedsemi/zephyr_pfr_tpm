/*
 * Copyright (c) 2024 Linkedsemi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if !defined(CONFIG_SPI_FILTER_LINKEDSEMI)
    #error no CONFIG_SPI_FILTER_LINKEDSEMI define
#endif
#if !defined(CONFIG_SPI)
    #error no CONFIG_SPI define
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
    const struct device *const spifilter = DEVICE_DT_GET(DT_ALIAS(spifilter));

    spif_add_cmd(spifilter, CMD_READ);

    return 0;
}
