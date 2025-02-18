/*
 * Copyright (c) 2024 Linkedsemi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT linkedsemi_i2c_filter

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <i2c_filter.h>
#include <reg_i2c_filter.h>
#include <zephyr/drivers/pinctrl.h>

#define LOG_LEVEL CONFIG_I2C_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i2c_pfr_filter);

struct linkedsemi_i2c_filter_config {
    mm_reg_t base;
    const struct pinctrl_dev_config *pcfg;
    void (*irq_config_func)(const struct device *dev);
};

struct linkedsemi_i2c_filter_data {
    struct k_mutex lock;
    uint16_t scl_hold_time;
    i2c_filter_callback_t cb;
    void *user_data;
};

int linkedsemi_i2c_filter_config_scl_hold_time(const struct device *dev, uint16_t scl_hold_time)
{
    __unused const struct linkedsemi_i2c_filter_config *dev_config = dev->config;
    __unused struct linkedsemi_i2c_filter_data *dev_data = dev->data;
    smbf_control0_reg_t smbf_control0_reg;

    k_mutex_lock(&dev_data->lock, K_FOREVER);

    smbf_control0_reg.value = sys_read32(dev_config->base + SMBF_CONTROL0_REG);
    smbf_control0_reg.SCL_HOLD_TIME = scl_hold_time;
    sys_write32(smbf_control0_reg.value, dev_config->base + SMBF_CONTROL0_REG);

    k_mutex_unlock(&dev_data->lock);

    return 0;
}

int linkedsemi_i2c_filter_register_callback(const struct device *dev,
                                      uint32_t callback_idx,
                                      i2c_filter_callback_t cb,
                                      void *user_data)
{
    struct linkedsemi_i2c_filter_data *dev_data = dev->data;

    dev_data->cb = cb;
    dev_data->user_data = user_data;

    return 0;
}

static void linkedsemi_i2c_filter_isr(const struct device *dev)
{
    __unused const struct linkedsemi_i2c_filter_config *dev_config = dev->config;
    __unused struct linkedsemi_i2c_filter_data *dev_data = dev->data;
    intr_t intr_status;
    smbf_nonwhitelist_t smbf_nonwhitelist;

    intr_status.value = sys_read32(dev_config->base + INTR_STT);
    sys_write32(intr_status.value, dev_config->base + INTR_CLR);
    smbf_nonwhitelist.value = sys_read32(dev_config->base + SMBF_NONWHITELIST);

    if (intr_status.ADDRESS_BEYOND_WHITELIST) {
        printk("address beyond whitelist: i2c@%#x\n", smbf_nonwhitelist.ERROR_ADDRESS);
    }
    if (intr_status.COMMAND_BEYOND_WHITELIST) {
        printk("command beyond whitelist: i2c@%#x cmd@%#x\n",
                                                    smbf_nonwhitelist.ERROR_ADDRESS,
                                                    smbf_nonwhitelist.ERROR_COMMAND);
    }

    if (dev_data->cb) {
        dev_data->cb(dev, 0, dev_data->user_data, NULL);
    }
}

int linkedsemi_i2c_filter_fill_bitmap(const struct device *dev,
                                    uint8_t idx,
                                    uint8_t addr,
                                    uint32_t bitmap[LINKEDSEMI_I2C_F_REMAP_SIZE_U32])
{
    __unused const struct linkedsemi_i2c_filter_config *dev_config = dev->config;
    __unused struct linkedsemi_i2c_filter_data *dev_data = dev->data;

    if (idx > (LINKEDSEMI_I2C_F_ADDR_NUM - 1)) {
        LOG_ERR("i2c filter index invalid");
        return -EINVAL;
    } else if (!bitmap) {
        LOG_ERR("i2c filter bitmap is NULL");
        return -EINVAL;
    }

    k_mutex_lock(&dev_data->lock, K_FOREVER);

    uint32_t offset = (idx >> 2) << 2;
    whitelist_address_t wht_addr;
    wht_addr.value = sys_read32(dev_config->base + WHITELIST_ADDRESS_3_0 + offset);
    wht_addr.field[idx % 4].WHITELIST_ADDRESS = addr;
    sys_write32(wht_addr.value, dev_config->base + WHITELIST_ADDRESS_3_0 + offset);

    sys_write32(idx, dev_config->base + SMBF_ADDRESS_INDEX);
    for (uint8_t i = 0; i < LINKEDSEMI_I2C_F_REMAP_SIZE_U32; i++) {
        uint32_t val = bitmap[i];
        mm_reg_t reg = (mm_reg_t)(WHITELIST_COMMAND_0 + i * 4);
        sys_write32(val, dev_config->base + reg);
    }

    uint32_t smbf_address_enable = sys_read32(dev_config->base + SMBF_ADDRESS_ENABLE);
    smbf_address_enable |= BIT(idx);
    sys_write32(smbf_address_enable, dev_config->base + SMBF_ADDRESS_ENABLE);

    k_mutex_unlock(&dev_data->lock);

    return 0;
}

int linkedsemi_i2c_filter_dump_bitmap(const struct device *dev,
                                    uint8_t idx,
                                    uint8_t *addr,
                                    uint32_t bitmap[LINKEDSEMI_I2C_F_REMAP_SIZE_U32])
{
    __unused const struct linkedsemi_i2c_filter_config *dev_config = dev->config;
    __unused struct linkedsemi_i2c_filter_data *dev_data = dev->data;

    if (idx > (LINKEDSEMI_I2C_F_ADDR_NUM - 1)) {
        LOG_ERR("i2c filter index invalid");
        return -EINVAL;
    } else if (!bitmap) {
        LOG_ERR("i2c filter bitmap is NULL");
        return -EINVAL;
    }

    k_mutex_lock(&dev_data->lock, K_FOREVER);

    uint32_t offset = (idx >> 2) << 2;
    whitelist_address_t wht_addr;
    wht_addr.value = sys_read32(dev_config->base + WHITELIST_ADDRESS_3_0 + offset);
    *addr = wht_addr.field[idx % 4].WHITELIST_ADDRESS;

    sys_write32(idx, dev_config->base + SMBF_ADDRESS_INDEX);
    for (uint8_t i = 0; i < LINKEDSEMI_I2C_F_REMAP_SIZE_U32; i++) {
        mm_reg_t reg = (mm_reg_t)(WHITELIST_COMMAND_0 + i * 4);
        bitmap[i] = sys_read32(dev_config->base + reg);
    }

    k_mutex_unlock(&dev_data->lock);

    return 0;
}

int linkedsemi_i2c_filter_en(const struct device *dev,
                             bool filter_en,
                             bool wlist_en,
                             bool clr_tbl)
{
    __unused const struct linkedsemi_i2c_filter_config *dev_config = dev->config;
    __unused struct linkedsemi_i2c_filter_data *dev_data = dev->data;
    smbf_set_t smbf_set = {
        .BLOCK_DISABLE = filter_en ? 0 : 1,
        .FILTER_DISABLE = wlist_en ? 0 : 1,
        .MASTER_WRITE_MODE = 1,
        .reserve0 = 0,
    };

    k_mutex_lock(&dev_data->lock, K_FOREVER);

    sys_write32(smbf_set.value, dev_config->base + SMBUS_FILTER_SET);

    if (clr_tbl) {
        for (uint8_t idx = 0; idx < LINKEDSEMI_I2C_F_ADDR_NUM; idx++) {
            sys_write32(idx, dev_config->base + SMBF_ADDRESS_INDEX);
            for (uint8_t whtlst = 0; whtlst < LINKEDSEMI_I2C_F_REMAP_SIZE_U32; whtlst++) {
                sys_write32(0, dev_config->base + (mm_reg_t)(WHITELIST_COMMAND_0 + whtlst * 4));
            }
        }
    }

    k_mutex_unlock(&dev_data->lock);

    return 0;
}

int linkedsemi_i2c_filter_cold_reset(const struct device *dev)
{
    __unused const struct linkedsemi_i2c_filter_config *dev_config = dev->config;
    __unused struct linkedsemi_i2c_filter_data *dev_data = dev->data;

#if defined(CONFIG_PINCTRL)
    if (dev_config->pcfg != NULL) {
        int ret;
        ret = pinctrl_apply_state(dev_config->pcfg, PINCTRL_STATE_DEFAULT);
        if (ret < 0) {
            LOG_WRN("Could not configure pins");
        }
    }
#endif

    intr_t intr_mask = {
        .COMMAND_BEYOND_WHITELIST = 1,
        .ADDRESS_BEYOND_WHITELIST = 1,
    };
    sys_write32(0x1, dev_config->base + SMBF_REG_ENABLE);
    sys_write32(intr_mask.value, dev_config->base + INTR_MSK);

    linkedsemi_i2c_filter_en(dev, false, false, true);
    linkedsemi_i2c_filter_config_scl_hold_time(dev, dev_data->scl_hold_time);

    return 0;
}

static int linkedsemi_i2c_filter_init(const struct device *dev)
{
    __unused const struct linkedsemi_i2c_filter_config *dev_config = dev->config;
    __unused struct linkedsemi_i2c_filter_data *dev_data = dev->data;

    k_mutex_init(&dev_data->lock);

    dev_config->irq_config_func(dev);

    return 0;
}

#define I2C_FILTER_INIT(inst)                                                             \
    static void linkedsemi_i2c_filter_irq_config_func_##inst(const struct device *dev)    \
    {                                                                                     \
        ARG_UNUSED(dev);                                                                  \
        IRQ_CONNECT(DT_INST_IRQN(inst),                                                   \
                    DT_INST_IRQ(inst, priority),                                          \
                    linkedsemi_i2c_filter_isr,                                            \
                    DEVICE_DT_INST_GET(inst),                                             \
                    0);                                                                   \
        irq_enable(DT_INST_IRQN(inst));                                                   \
    }                                                                                     \
    PINCTRL_DT_INST_DEFINE(inst);                                                         \
    static const struct linkedsemi_i2c_filter_config linkedsemi_i2c_filter_cfg_##inst = { \
        .base = (mm_reg_t)DT_INST_REG_ADDR(inst),                                       \
        .irq_config_func = linkedsemi_i2c_filter_irq_config_func_##inst,                  \
        IF_ENABLED(CONFIG_PINCTRL, (.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst), ))      \
    };                                                                                    \
    static struct linkedsemi_i2c_filter_data linkedsemi_i2c_filter_data_##inst = {        \
        .scl_hold_time = DT_INST_PROP(inst, scl_hold_time),                               \
    };                                                                                    \
                                                                                          \
    DEVICE_DT_INST_DEFINE(inst,                                                           \
                          linkedsemi_i2c_filter_init,                                     \
                          NULL,                                                           \
                          &linkedsemi_i2c_filter_data_##inst,                             \
                          &linkedsemi_i2c_filter_cfg_##inst,                              \
                          POST_KERNEL,                                                    \
                          CONFIG_KERNEL_INIT_PRIORITY_DEVICE,                             \
                          NULL);

DT_INST_FOREACH_STATUS_OKAY(I2C_FILTER_INIT)
