/*
 * Copyright (c) 2024 Linkedsemi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT linkedsemi_spi_filter

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <spi_filter.h>
#include <reg_spi_filter.h>
#include <zephyr/drivers/pinctrl.h>

#define LOG_LEVEL CONFIG_SPI_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_pfr_filter);

struct linkedsemi_spi_filter_config {
    mm_reg_t base;
    const struct pinctrl_dev_config *pcfg;
    void (*irq_config_func)(const struct device *dev);
    bool blacklist_en;
};

struct linkedsemi_spi_filter_data {
    struct k_sem sem_spif;
    uint8_t fixed_cmd_tab[SPIF_FIXED_CMD_TABLE_NUM];
};

static void linkedsemi_spi_filter_isr(const struct device *dev)
{
    __unused const struct linkedsemi_spi_filter_config *dev_config = dev->config;
    __unused struct linkedsemi_spi_filter_data *dev_data = dev->data;
    spif_intr_t intr_status;

    intr_status.value = sys_read32(dev_config->base + SPIF_INTR_STT);
    sys_write32(intr_status.value, dev_config->base + SPIF_INTR_CLR);

    printk("linkedsemi_spi_filter_isr: %#x\n", intr_status.value);
}

static void acquire_spif_device(const struct device *dev)
{
    if (IS_ENABLED(CONFIG_MULTITHREADING)) {
        struct linkedsemi_spi_filter_data *dev_data = dev->data;

        k_sem_take(&dev_data->sem_spif, K_FOREVER);
    }
}

static void release_spif_device(const struct device *dev)
{
    if (IS_ENABLED(CONFIG_MULTITHREADING)) {
        struct linkedsemi_spi_filter_data *dev_data = dev->data;

        k_sem_give(&dev_data->sem_spif);
    }
}

static int spif_get_empty_cmd_slot(const struct device *dev)
{
    __unused const struct linkedsemi_spi_filter_config *dev_config = dev->config;
    __unused struct linkedsemi_spi_filter_data *dev_data = dev->data;
    int idx;
    spif_cmd_t spif_cmd;

    for (idx = SPIF_FIXED_CMD_TABLE_NUM; idx < SPIF_CMD_TABLE_NUM; idx++) {
        spif_cmd.value = sys_read32(dev_config->base + SPIF_CMD_BASE + idx * 4);
        if ((spif_cmd.value) == 0)
            return idx;
    }

    return -ENOSR;
}

int spif_get_cmd_slot(const struct device *dev, uint8_t cmd, uint32_t start_off)
{
    __unused const struct linkedsemi_spi_filter_config *dev_config = dev->config;
    __unused struct linkedsemi_spi_filter_data *dev_data = dev->data;
    int idx;
    spif_cmd_t spif_cmd;

    for (idx = start_off; idx < SPIF_CMD_TABLE_NUM; idx++) {
        spif_cmd.value = sys_read32(dev_config->base + SPIF_CMD_BASE + idx * 4);
        if ((spif_cmd.field.CMD) == cmd)
            return idx;
    }

    return -ENOSR;
}

int spif_add_cmd(const struct device *dev, uint8_t cmd)
{
    __unused const struct linkedsemi_spi_filter_config *dev_config = dev->config;
    __unused struct linkedsemi_spi_filter_data *dev_data = dev->data;
    int ret = 0;
    mm_reg_t table_base = dev_config->base + SPIF_CMD_BASE;
    int idx;

    acquire_spif_device(dev);

    for (uint8_t off = 0; off < SPIF_CMD_TABLE_NUM; off++) {
        idx = spif_get_cmd_slot(dev, cmd, off);
        if (idx >= 0) {
            spif_cmd_t spif_cmd;
            spif_cmd.value = sys_read32(table_base + idx * 4);
            spif_cmd.field.EN = 1;
            sys_write32(spif_cmd.value, table_base + idx * 4);
        } else {
            break;
        }
    }

    for (uint8_t off = 0; off < SPIF_CMD_TABLE_NUM; off++) {
        if (dev_data->fixed_cmd_tab[off] == cmd) {
            spif_cmd_t spif_cmd;
            spif_cmd.field.CMD = cmd;
            spif_cmd.field.EN = 1;
            sys_write32(spif_cmd.value, table_base + idx * 4);
            goto end;
        }
    }

    idx = spif_get_empty_cmd_slot(dev);
    if (idx < 0) {
        LOG_ERR("No more space for new cmd");
        ret = -ENOSR;
        goto end;
    }
    spif_cmd_t spif_cmd;
    spif_cmd.field.CMD = cmd;
    spif_cmd.field.EN = 1;
    sys_write32(spif_cmd.value, table_base + idx * 4);

end:
    release_spif_device(dev);

    return ret;
}

int spif_remove_cmd(const struct device *dev, uint8_t cmd)
{
    __unused const struct linkedsemi_spi_filter_config *dev_config = dev->config;
    __unused struct linkedsemi_spi_filter_data *dev_data = dev->data;
    mm_reg_t table_base = dev_config->base + SPIF_CMD_BASE;
    int ret = 0;
    int idx;
    uint32_t off;
    bool found = false;

    acquire_spif_device(dev);

    for (off = 0; off < SPIF_CMD_TABLE_NUM; off++) {
        idx = spif_get_cmd_slot(dev, cmd, off);
        if (idx >= 0) {
            found = true;
            sys_write32(0, table_base + idx * 4);
            /* break; */ /* do not break for remove all */
        }
    }

    if (!found) {
        LOG_ERR("cmd %02x is not found in allow cmd table", cmd);
        ret = -EINVAL;
        goto end;
    }

end:
    release_spif_device(dev);

    return ret;
}

static int linkedsemi_spi_filter_init(const struct device *dev)
{
    __unused const struct linkedsemi_spi_filter_config *dev_config = dev->config;
    __unused struct linkedsemi_spi_filter_data *dev_data = dev->data;

    if (IS_ENABLED(CONFIG_MULTITHREADING))
        k_sem_init(&dev_data->sem_spif, 1, 1);

#if defined(CONFIG_PINCTRL)
    if (dev_config->pcfg != NULL) {
        int ret;
        ret = pinctrl_apply_state(dev_config->pcfg, PINCTRL_STATE_DEFAULT);
        if (ret < 0) {
            LOG_ERR("Could not configure ethernet pins");
            return ret;
        }
    }
#endif
    spif_cfg_t spif_cfg = {
        .field = {
            .EN = 1,
            .OPERATION_MODE = 1,
            .ALLOW_4BYTE_ADDR = 1,
            .TARGET_ADDR_MODE_SEL = dev_config->blacklist_en ? 1 : 0,
            .BOW_CMD_SEL = 0,
        },
    };
    sys_write32(spif_cfg.value, dev_config->base + SPIF_CFG);
    spif_intr_t intr_mask = {
        .field = {
            .ERROR_OVERFLOW = 1,
            .ERROR = 1,
            .TARGET_ADDR = 1,
        },
    };
    sys_write32(intr_mask.value, dev_config->base + SPIF_INTR_MASK);

    dev_config->irq_config_func(dev);

    return 0;
}

#define SPI_FILTER_INIT(inst)                                                             \
    static void linkedsemi_spi_filter_irq_config_func_##inst(const struct device *dev)    \
    {                                                                                     \
        ARG_UNUSED(dev);                                                                  \
        IRQ_CONNECT(DT_INST_IRQN(inst),                                                   \
                    DT_INST_IRQ(inst, priority),                                          \
                    linkedsemi_spi_filter_isr,                                            \
                    DEVICE_DT_INST_GET(inst),                                             \
                    0);                                                                   \
        irq_enable(DT_INST_IRQN(inst));                                                   \
    }                                                                                     \
    PINCTRL_DT_INST_DEFINE(inst);                                                         \
    static const struct linkedsemi_spi_filter_config linkedsemi_spi_filter_cfg_##inst = { \
        .base = (mm_reg_t)DT_INST_REG_ADDR(inst),                                         \
        .irq_config_func = linkedsemi_spi_filter_irq_config_func_##inst,                  \
        IF_ENABLED(CONFIG_PINCTRL, (.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst), ))      \
        IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, blacklist), (.blacklist_en = true,))       \
    };                                                                                    \
    static struct linkedsemi_spi_filter_data linkedsemi_spi_filter_data_##inst = {        \
        .fixed_cmd_tab = {                                                                \
            CMD_PAGE_PROGRAM,                                                             \
            CMD_PAGE_PROGRAM_QUAD_ADDRESS_QUAD_DATA,                                      \
            CMD_ERASE_4KB,                                                                \
            CMD_ERASE_32KB,                                                               \
            CMD_ERASE_64KB,                                                               \
            CMD_READ,                                                                     \
            CMD_FAST_READ,                                                                \
            CMD_READ_QUAD_DATA,                                                           \
            CMD_READ_QUAD_ADDRESS_QUAD_DATA,                                              \
            CMD_QUAD_SPI_MODE_ENTER,                                                      \
            CMD_QUAD_SPI_MODE_EXIT,                                                       \
            CMD_4BYTE_MODE_ENTER,                                                         \
            CMD_4BYTE_MODE_EXIT,                                                          \
            CMD_4BYTE_READ_EXTENDED_ADDRESS,                                              \
            CMD_4BYTE_WRITE_EXTENDED_ADDRESS,                                             \
            CMD_4BYTE_PAGE_PROGRAM,                                                       \
            CMD_4BYTE_PAGE_PROGRAM_QUAD_ADDRESS_QUAD_DATA,                                \
            CMD_4BYTE_ERASE_4KB,                                                          \
            CMD_4BYTE_ERASE_32KB,                                                         \
            CMD_4BYTE_ERASE_64KB,                                                         \
            CMD_4BYTE_READ,                                                               \
            CMD_4BYTE_FAST_READ,                                                          \
            CMD_4BYTE_READ_QUAD_DATA,                                                     \
            CMD_4BYTE_READ_QUAD_ADDRESS_QUAD_DATA,                                        \
        },                                                                                \
    };                                                                                    \
                                                                                          \
    DEVICE_DT_INST_DEFINE(inst,                                                           \
                          linkedsemi_spi_filter_init,                                     \
                          NULL,                                                           \
                          &linkedsemi_spi_filter_data_##inst,                             \
                          &linkedsemi_spi_filter_cfg_##inst,                              \
                          POST_KERNEL,                                                    \
                          CONFIG_KERNEL_INIT_PRIORITY_DEVICE,                             \
                          NULL);

DT_INST_FOREACH_STATUS_OKAY(SPI_FILTER_INIT)
