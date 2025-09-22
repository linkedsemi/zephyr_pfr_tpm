/*
 * Copyright (c) 2024 linkedsemi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/irq.h>
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

#define LOG_LEVEL CONFIG_SPID_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spid_linkedsemi);

#include "spid_linkedsemi.h"
#include "reg_spid.h"

#define DT_DRV_COMPAT linkedsemi_spid

struct spid_linkedsemi_data {
    spid_callback_t cb;
    void *user_data;
    const struct device *dev;
    uint32_t data;
};

typedef void (*irq_cfg_func_t)(const struct device *dev);

struct spid_linkedsemi_config {
    mm_reg_t reg;
    uint32_t data;
    irq_cfg_func_t irq_config_func;
    IF_ENABLED(CONFIG_PINCTRL, (const struct pinctrl_dev_config *pcfg;))
    IF_ENABLED(CONFIG_CLOCK_CONTROL, (struct ls_clk_cfg ccfg;))
    IF_ENABLED(CONFIG_RESET, (struct reset_dt_spec reset;))
};

static void linkedsemi_spid_main_isr(const struct device *dev)
{
    struct spid_linkedsemi_data *data = dev->data;
    struct spid_linkedsemi_config *cfg = (struct spid_linkedsemi_config *)dev->config;

    uint32_t stat = sys_read32(cfg->reg + SPID_INTR_STATE);
    LOG_DBG("%s stat: 0x%x\n", __func__, stat);
    if (stat) {
        if (data->cb) {
            data->cb(dev, 0, data->user_data, NULL);
        }
    }

    sys_write32(0xff, cfg->reg + SPID_INTR_STATE);
}

int spid_linkedsemi_register_callback(const struct device *dev,
                                      uint32_t callback_idx,
                                      spid_callback_t cb,
                                      void *user_data)
{
    struct spid_linkedsemi_data *data = dev->data;

    data->cb = cb;
    data->user_data = user_data;

    return 0;
}

// #define SPID_MODE_REG   0x400120fc
// #define ENABLE_SPID_REG 0x400130a4

void init_spid_registers(const struct device *dev, int mode)
{
    struct spid_linkedsemi_config *cfg = (struct spid_linkedsemi_config *)dev->config;

    // invalid_locality=1
    if (mode == INTF_FIFO_MODE) {
        sys_write32(0x11, cfg->reg + SPID_TPM_CFG);
    } else if (mode == INTF_CRB_MODE) {
        sys_write32(0x13, cfg->reg + SPID_TPM_CFG);
    } else {
        printk("***Fatal error, invalid interface mode %d\n", mode);
    }
    // sys_write32(0x22aaaa, SPID_MODE_REG);  // both spid1 & spid2 enable TPM spid
    // sys_write32(0x0000f000, ENABLE_SPID_REG); // enable spid2 io(MISO)
}

int spid_linkedsemi_cold_reset(const struct device *dev)
{
    const struct spid_linkedsemi_config *dev_config = dev->config;
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
        LOG_DBG("%s: Could not configure pins", dev->name);
    }
#endif

    sys_write32(0xff, dev_config->reg + SPID_INTR_ENABLE);

    return 0;
}

int spid_linkedsemi_interrupt_register(const struct device *dev)
{
    const struct spid_linkedsemi_config *dev_config = dev->config;
    __unused int ret;

    dev_config->irq_config_func(dev);

    return 0;
}

static int spid_linkedsemi_init(const struct device *dev)
{
    return 0;
}

#define SPID_LINKEDSEMI_IRQ_HANDLER(index)                                        \
    static void spid_linkedsemi_irq_config_func_##index(const struct device *dev) \
    {                                                                             \
        IRQ_CONNECT(DT_INST_IRQ_BY_NAME(index, main, irq),                        \
                    DT_INST_IRQ_BY_NAME(index, main, priority),                   \
                    linkedsemi_spid_main_isr,                                     \
                    DEVICE_DT_INST_GET(index),                                    \
                    0);                                                           \
        irq_enable(DT_INST_IRQ_BY_NAME(index, main, irq));                        \
    }

#define SPID_LINKEDSEMI_INIT(index)                                                                          \
    IF_ENABLED(CONFIG_PINCTRL, (PINCTRL_DT_INST_DEFINE(index)));                                             \
    SPID_LINKEDSEMI_IRQ_HANDLER(index)                                                                       \
    static const struct spid_linkedsemi_config spid_linkedsemi_cfg_##index = {                               \
        .reg = (mm_reg_t)DT_INST_REG_ADDR(index),                                                            \
        .irq_config_func = spid_linkedsemi_irq_config_func_##index,                                          \
        IF_ENABLED(CONFIG_PINCTRL, (.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index), ))                        \
        IF_ENABLED(DT_HAS_CLOCKS(index), (.ccfg = LS_DT_CLK_CFG_ITEM(index), ))                              \
        IF_ENABLED(DT_INST_NODE_HAS_PROP(index, resets), (.reset = RESET_DT_SPEC_INST_GET(index), ))         \
    };                                                                                                       \
    static struct spid_linkedsemi_data spid_linkedsemi_dev_data_##index;                                     \
    DEVICE_DT_INST_DEFINE(index,                                                                             \
                          spid_linkedsemi_init,                                                              \
                          NULL,                                                                              \
                          &spid_linkedsemi_dev_data_##index,                                                 \
                          &spid_linkedsemi_cfg_##index,                                                      \
                          POST_KERNEL,                                                                       \
                          CONFIG_KERNEL_INIT_PRIORITY_DEVICE,                                                \
                          NULL);
DT_INST_FOREACH_STATUS_OKAY(SPID_LINKEDSEMI_INIT)
