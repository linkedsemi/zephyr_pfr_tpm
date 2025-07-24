/*
 * Copyright (c) 2024 Linkedsemi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT linkedsemi_i2c_filter

#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#if defined(CONFIG_PINCTRL)
    #include <zephyr/drivers/pinctrl.h>
#endif
#if defined(CONFIG_RESET)
    #include <zephyr/drivers/reset.h>
#endif
#if defined(CONFIG_CLOCK_CONTROL)
    #include <zephyr/drivers/clock_control.h>
    #include <soc_clock.h>
#endif

#define LOG_LEVEL CONFIG_I2C_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i2c_pfr_filter);

#include <i2c_filter.h>
#include <reg_i2c_filter.h>

struct linkedsemi_i2c_filter_config {
    mm_reg_t base;
    const struct device *i2c;
    void (*irq_config_func)(const struct device *dev);
    IF_ENABLED(CONFIG_PINCTRL, (const struct pinctrl_dev_config *pcfg;))
    IF_ENABLED(CONFIG_CLOCK_CONTROL, (struct ls_clk_cfg ccfg;))
    IF_ENABLED(CONFIG_RESET, (struct reset_dt_spec reset;))
};

struct linkedsemi_i2c_filter_data {
    struct k_mutex lock;
    uint16_t scl_hold_time;
    i2c_filter_callback_t cb;
    void *user_data;
};

extern int i2c_ls_pinctrl(const struct device *dev, uint32_t pinctrl_state);

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
        LOG_DBG("address beyond whitelist: i2c@%#x\n", smbf_nonwhitelist.ERROR_ADDRESS);
    }
    if (intr_status.COMMAND_BEYOND_WHITELIST) {
        LOG_DBG("command beyond whitelist: i2c@%#x cmd@%#x\n",
                                                    smbf_nonwhitelist.ERROR_ADDRESS,
                                                    smbf_nonwhitelist.ERROR_COMMAND);
    }

    if (dev_data->cb) {
        dev_data->cb(dev, 0, dev_data->user_data, NULL);
    }
}

int linkedsemi_i2c_filter_enable_channel(const struct device *dev,
                                        uint8_t idx,
                                        bool enable)
{
    __unused const struct linkedsemi_i2c_filter_config *dev_config = dev->config;
    __unused struct linkedsemi_i2c_filter_data *dev_data = dev->data;

    if (idx > (LINKEDSEMI_I2C_F_ADDR_NUM - 1)) {
        LOG_ERR("i2c filter index invalid");
        return -EINVAL;
    }

    k_mutex_lock(&dev_data->lock, K_FOREVER);

    uint32_t smbf_address_enable = sys_read32(dev_config->base + SMBF_ADDRESS_ENABLE);
    if (enable) {
        smbf_address_enable |= BIT(idx);
    } else {
        smbf_address_enable &= ~(BIT(idx));
    }
    sys_write32(smbf_address_enable, dev_config->base + SMBF_ADDRESS_ENABLE);

    k_mutex_unlock(&dev_data->lock);

    return 0;
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

    for (uint8_t reg_idx = 0; reg_idx < 4; reg_idx++) {
        whitelist_address_t wht_addr;
        wht_addr.value = sys_read32(dev_config->base + WHITELIST_ADDRESS_3_0 + (reg_idx * 0x4));
        for (uint8_t addr_idx = 0; addr_idx < 4; addr_idx++) {
            uint8_t it_idx = (reg_idx * 0x4) + addr_idx;
            if ((wht_addr.field[addr_idx].WHITELIST_ADDRESS == addr) && (it_idx != idx)) {
                uint32_t smbf_address_enable = sys_read32(dev_config->base + SMBF_ADDRESS_ENABLE);
                if (smbf_address_enable & BIT(it_idx)) {
                    LOG_ERR("operation fail: addr[%#x] is duplicated with channel %d", addr, it_idx);
                    k_mutex_unlock(&dev_data->lock);
                    return -EINVAL;
                }
            }
        }
    }

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

const struct device *i2cf_i2c_dev(const struct device *dev)
{
    const struct linkedsemi_i2c_filter_config *dev_config = dev->config;

    return dev_config->i2c;
}

int linkedsemi_i2c_filter_switch_to_master(const struct device *dev)
{
    __unused const struct linkedsemi_i2c_filter_config *dev_config = dev->config;
    __unused struct linkedsemi_i2c_filter_data *dev_data = dev->data;
    const struct device *master = i2cf_i2c_dev(dev);

    k_mutex_lock(&dev_data->lock, K_FOREVER);

    i2c_ls_pinctrl(master, PINCTRL_STATE_DEFAULT);

    k_mutex_unlock(&dev_data->lock);

    return 0;
}

int linkedsemi_i2c_filter_switch_to_filter(const struct device *dev)
{
    __unused const struct linkedsemi_i2c_filter_config *dev_config = dev->config;
    __unused struct linkedsemi_i2c_filter_data *dev_data = dev->data;

    k_mutex_lock(&dev_data->lock, K_FOREVER);

    pinctrl_apply_state(dev_config->pcfg, PINCTRL_STATE_DEFAULT);

    k_mutex_unlock(&dev_data->lock);

    return 0;
}

int linkedsemi_i2c_filter_cold_reset(const struct device *dev)
{
    __unused const struct linkedsemi_i2c_filter_config *dev_config = dev->config;
    __unused struct linkedsemi_i2c_filter_data *dev_data = dev->data;
    __unused int ret;

#if defined(CONFIG_CLOCK_CONTROL)
    if (dev_config->ccfg.cctl_dev) {
        const struct device *clk_dev = dev_config->ccfg.cctl_dev;
        if (!device_is_ready(clk_dev)) {
            LOG_DBG("%s device not ready", clk_dev->name);
            return -ENODEV;
        }
        clock_control_off(clk_dev, (clock_control_subsys_t)&dev_config->ccfg);
    }
#endif

#if defined(CONFIG_RESET)
    if (dev_config->reset.dev != NULL) {
        if (!device_is_ready(dev_config->reset.dev)) {
            LOG_ERR("Reset controller device is not ready");
            return -ENODEV;
        }

        ret = reset_line_toggle(dev_config->reset.dev, dev_config->reset.id);
        if (ret != 0) {
            LOG_ERR("toggle reset line failed");
            return ret;
        }
    }
#endif

#if defined(CONFIG_CLOCK_CONTROL)
    if (dev_config->ccfg.cctl_dev) {
        const struct device *clk_dev = dev_config->ccfg.cctl_dev;
        clock_control_on(clk_dev, (clock_control_subsys_t)&dev_config->ccfg);
    }
#endif

#if defined(CONFIG_PINCTRL)
    ret = pinctrl_apply_state(dev_config->pcfg, PINCTRL_STATE_DEFAULT);
    if (ret < 0) {
        LOG_ERR("Could not configure pins");
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

#define I2C_FILTER_INIT(inst)                                                                      \
    static void linkedsemi_i2c_filter_irq_config_func_##inst(const struct device *dev)             \
    {                                                                                              \
        ARG_UNUSED(dev);                                                                           \
        IRQ_CONNECT(DT_INST_IRQN(inst),                                                            \
                    DT_INST_IRQ(inst, priority),                                                   \
                    linkedsemi_i2c_filter_isr,                                                     \
                    DEVICE_DT_INST_GET(inst),                                                      \
                    0);                                                                            \
        irq_enable(DT_INST_IRQN(inst));                                                            \
    }                                                                                              \
    PINCTRL_DT_INST_DEFINE(inst);                                                                  \
    static const struct linkedsemi_i2c_filter_config linkedsemi_i2c_filter_cfg_##inst = {          \
        .base = (mm_reg_t)DT_INST_REG_ADDR(inst),                                                  \
        .irq_config_func = linkedsemi_i2c_filter_irq_config_func_##inst,                           \
        .i2c = DEVICE_DT_GET(DT_INST_PHANDLE(inst, i2c)),                                          \
        IF_ENABLED(CONFIG_PINCTRL, (.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst), ))               \
        IF_ENABLED(DT_HAS_CLOCKS(inst), (.ccfg = LS_DT_CLK_CFG_ITEM(inst), ))                      \
        IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, resets), (.reset = RESET_DT_SPEC_INST_GET(inst), )) \
    };                                                                                             \
    static struct linkedsemi_i2c_filter_data linkedsemi_i2c_filter_data_##inst = {                 \
        .scl_hold_time = DT_INST_PROP(inst, scl_hold_time),                                        \
    };                                                                                             \
                                                                                                   \
    DEVICE_DT_INST_DEFINE(inst,                                                                    \
                          linkedsemi_i2c_filter_init,                                              \
                          NULL,                                                                    \
                          &linkedsemi_i2c_filter_data_##inst,                                      \
                          &linkedsemi_i2c_filter_cfg_##inst,                                       \
                          POST_KERNEL,                                                             \
                          CONFIG_KERNEL_INIT_PRIORITY_DEVICE,                                      \
                          NULL);

DT_INST_FOREACH_STATUS_OKAY(I2C_FILTER_INIT)
