/*
 * Copyright (c) 2024 linkedsemi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SPID_LINKEDSEMI_H_
#define _SPID_LINKEDSEMI_H_

#include <zephyr/device.h>
#include <stdint.h>

enum {
    INTF_FIFO_MODE,
    INTF_CRB_MODE,
};

typedef void (*spid_callback_t)(const struct device *dev,
                uint32_t callback_idx, void *user_data,
                void *drv_data);

int spid_linkedsemi_cold_reset(const struct device *dev);

void init_spid_registers(const struct device *dev, int mode);

int spid_linkedsemi_register_callback(const struct device *dev,
        uint32_t callback_idx, spid_callback_t cb, void *user_data);

#endif /* _SPID_LINKEDSEMI_H_ */
