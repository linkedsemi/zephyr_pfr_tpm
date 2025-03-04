/*
 * Copyright (c) 2024 Linkedsemi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if !defined(CONFIG_SPI_FILTER_LINKEDSEMI)
    #error no CONFIG_SPI_FILTER_LINKEDSEMI define
#endif

#if !defined(CONFIG_SPI_FILTER_LINKEDSEMI_SHELL)
    #error no CONFIG_SPI_FILTER_LINKEDSEMI_SHELL define
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

    return 0;
}
