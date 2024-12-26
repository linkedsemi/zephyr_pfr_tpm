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
    mem_addr_t base;
    const struct pinctrl_dev_config *pcfg;
    void (*irq_config_func)(const struct device *dev);
};

struct linkedsemi_i2c_filter_data {
    uint8_t master_scl_delay;
    uint8_t master_sda_delay;
    uint16_t scl_hold_time;
    uint8_t slave_scl_delay;
    uint8_t slave_sda_delay;
};

int linkedsemi_i2c_filter_config_master_scl_delay(const struct device *dev, uint8_t master_scl_delay)
{
    __unused const struct linkedsemi_i2c_filter_config *dev_config = dev->config;
    __unused struct linkedsemi_i2c_filter_data *dev_data = dev->data;

    smbf_control0_reg_t smbf_control0_reg;
    smbf_control0_reg.value = sys_read32(dev_config->base + SMBF_CONTROL0_REG);
    smbf_control0_reg.field.MASTER_SCL_DELAY = master_scl_delay,
    sys_write32(smbf_control0_reg.value, dev_config->base + SMBF_CONTROL0_REG);

    return 0;
}

int linkedsemi_i2c_filter_config_master_sda_delay(const struct device *dev, uint8_t master_sda_delay)
{
    __unused const struct linkedsemi_i2c_filter_config *dev_config = dev->config;
    __unused struct linkedsemi_i2c_filter_data *dev_data = dev->data;

    smbf_control0_reg_t smbf_control0_reg;
    smbf_control0_reg.value = sys_read32(dev_config->base + SMBF_CONTROL0_REG);
    smbf_control0_reg.field.MASTER_SDA_DELAY = master_sda_delay,
    sys_write32(smbf_control0_reg.value, dev_config->base + SMBF_CONTROL0_REG);

    return 0;
}

int linkedsemi_i2c_filter_config_scl_hold_time(const struct device *dev, uint16_t scl_hold_time)
{
    __unused const struct linkedsemi_i2c_filter_config *dev_config = dev->config;
    __unused struct linkedsemi_i2c_filter_data *dev_data = dev->data;

    smbf_control0_reg_t smbf_control0_reg;
    smbf_control0_reg.value = sys_read32(dev_config->base + SMBF_CONTROL0_REG);
    smbf_control0_reg.field.SCL_HOLD_TIME = scl_hold_time,
    sys_write32(smbf_control0_reg.value, dev_config->base + SMBF_CONTROL0_REG);

    return 0;
}

int linkedsemi_i2c_filter_config_slave_scl_delay(const struct device *dev, uint8_t slave_scl_delay)
{
    __unused const struct linkedsemi_i2c_filter_config *dev_config = dev->config;
    __unused struct linkedsemi_i2c_filter_data *dev_data = dev->data;

    smbf_control1_reg_t smbf_control1_reg;
    smbf_control1_reg.value = sys_read32(dev_config->base + SMBF_CONTROL1_REG);
    smbf_control1_reg.field.SLAVE_SCL_DELAY = slave_scl_delay,
    sys_write32(smbf_control1_reg.value, dev_config->base + SMBF_CONTROL1_REG);

    return 0;
}

int linkedsemi_i2c_filter_config_slave_sda_delay(const struct device *dev, uint8_t slave_sda_delay)
{
    __unused const struct linkedsemi_i2c_filter_config *dev_config = dev->config;
    __unused struct linkedsemi_i2c_filter_data *dev_data = dev->data;

    smbf_control1_reg_t smbf_control1_reg;
    smbf_control1_reg.value = sys_read32(dev_config->base + SMBF_CONTROL1_REG);
    smbf_control1_reg.field.SLAVE_SDA_DELAY = slave_sda_delay,
    sys_write32(smbf_control1_reg.value, dev_config->base + SMBF_CONTROL1_REG);

    return 0;
}

static void linkedsemi_i2c_filter_isr(const struct device *dev)
{
    __unused const struct linkedsemi_i2c_filter_config *dev_config = dev->config;
    __unused struct linkedsemi_i2c_filter_data *dev_data = dev->data;
    intr_t intr_status;
    intr_status.value = sys_read32(dev_config->base + INTR_STT);
    sys_write32(intr_status.value, dev_config->base + INTR_CLR);

    if (intr_status.field.ADDRESS_BEYOND_WHITELIST) {
        LOG_DBG("address beyond whitelist");
    }
    if (intr_status.field.COMMAND_BEYOND_WHITELIST) {
        LOG_DBG("command beyond whitelist");
    }
    if (intr_status.field.NON_WHITELIST_WARN) {
        LOG_DBG("non whitelist warn");
    }
}

int linkedsemi_i2c_filter_fill_bitmap(const struct device *dev,
                                    uint8_t idx,
                                    uint8_t addr,
                                    uint8_t bitmap[LINKEDSEMI_I2C_F_REMAP_SIZE_BYTE])
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

    sys_write32(idx, dev_config->base + SMBF_ADDRESS_INDEX);
    for (uint8_t i = 0; i < LINKEDSEMI_I2C_F_REMAP_SIZE_U32; i++) {
        uint32_t val = UNALIGNED_GET(&((uint32_t *)bitmap)[i]);
        mem_addr_t reg = (mem_addr_t)(WHITELIST_COMMAND_0 + i * 4);
        sys_write32(val, dev_config->base + reg);
    }

    uint32_t offset = (idx >> 2) << 2;
    whitelist_address_t wht_addr;
    wht_addr.value = sys_read32(dev_config->base + WHITELIST_ADDRESS_3_0 + offset);
    wht_addr.field[idx % 4].WHITELIST_ADDRESS = addr;
    sys_write32(wht_addr.value, dev_config->base + WHITELIST_ADDRESS_3_0 + offset);

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
        .field = {
            .BLOCK_DISABLE = filter_en ? 1 : 0,
            .FILTER_DISABLE = wlist_en ? 1 : 0,
            .reserve0 = 0,
        },
    };

    sys_write32(smbf_set.value, dev_config->base + SMBUS_FILTER_SET);

    if (clr_tbl) {
        for (uint8_t idx = 0; idx < LINKEDSEMI_I2C_F_ADDR_NUM; idx++) {
            sys_write32(idx, dev_config->base + SMBF_ADDRESS_INDEX);
            for (uint8_t whtlst = 0; whtlst < LINKEDSEMI_I2C_F_REMAP_SIZE_U32; whtlst++) {
                sys_write32(0, dev_config->base + (mem_addr_t)(WHITELIST_COMMAND_0 + whtlst * 4));
            }
        }
    }

    return 0;
}

static int linkedsemi_i2c_filter_init(const struct device *dev)
{
    __unused const struct linkedsemi_i2c_filter_config *dev_config = dev->config;
    __unused struct linkedsemi_i2c_filter_data *dev_data = dev->data;

    intr_t intr_mask = {
        .field = {
            .COMMAND_BEYOND_WHITELIST = 1,
            .NON_WHITELIST_WARN       = 1,
            .ADDRESS_BEYOND_WHITELIST = 1,
        },
    };
    sys_write32(intr_mask.value, dev_config->base + INTR_STT);

    linkedsemi_i2c_filter_config_master_sda_delay(dev, dev_data->master_sda_delay);
    linkedsemi_i2c_filter_config_scl_hold_time(dev, dev_data->scl_hold_time);
    linkedsemi_i2c_filter_config_slave_scl_delay(dev, dev_data->slave_scl_delay);
    linkedsemi_i2c_filter_config_slave_sda_delay(dev, dev_data->slave_sda_delay);
    linkedsemi_i2c_filter_config_master_scl_delay(dev, dev_data->master_scl_delay);

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
        .base = (mem_addr_t)DT_INST_REG_ADDR(inst),                                       \
        .irq_config_func = linkedsemi_i2c_filter_irq_config_func_##inst,                  \
        IF_ENABLED(CONFIG_PINCTRL, (.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst), ))      \
    };                                                                                    \
    static struct linkedsemi_i2c_filter_data linkedsemi_i2c_filter_data_##inst = {        \
        .master_scl_delay = DT_INST_PROP(inst, master_scl_delay),                         \
        .master_sda_delay = DT_INST_PROP(inst, master_sda_delay),                         \
        .scl_hold_time = DT_INST_PROP(inst, scl_hold_time),                               \
        .slave_scl_delay = DT_INST_PROP(inst, slave_scl_delay),                           \
        .slave_sda_delay = DT_INST_PROP(inst, slave_sda_delay),                           \
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
