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
    spif_callback_t cb;
    void *user_data;
};

int linkedsemi_spif_register_callback(const struct device *dev,
                                      uint32_t callback_idx,
                                      spif_callback_t cb,
                                      void *user_data)
{
    struct linkedsemi_spi_filter_data *dev_data = dev->data;

    dev_data->cb = cb;
    dev_data->user_data = user_data;

    return 0;
}

static void linkedsemi_spi_filter_isr(const struct device *dev)
{
    __unused const struct linkedsemi_spi_filter_config *dev_config = dev->config;
    __unused struct linkedsemi_spi_filter_data *dev_data = dev->data;
    spif_intr_t intr_status;
    uint32_t illegal_cmd;
    uint32_t illegal_addr;

    intr_status.value = sys_read32(dev_config->base + SPIF_INTR_STT);
    sys_write32(intr_status.value, dev_config->base + SPIF_INTR_CLR);
    illegal_cmd = sys_read32(dev_config->base + SPIF_ILLEGAL_CMD);
    illegal_addr = sys_read32(dev_config->base + SPIF_ILLEGAL_ADDR);
    LOG_DBG("linkedsemi_spi_filter_isr: %#x\n", intr_status.value);
    LOG_DBG("SPIF_ILLEGAL_CMD: %#x\n", illegal_cmd);
    LOG_DBG("SPIF_ILLEGAL_ADDR: %#x\n", illegal_addr);

    if (dev_data->cb) {
        dev_data->cb(dev, 0, dev_data->user_data, NULL);
    }
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

static int spif_get_empty_general_cmd_slot(const struct device *dev)
{
    __unused const struct linkedsemi_spi_filter_config *dev_config = dev->config;
    __unused struct linkedsemi_spi_filter_data *dev_data = dev->data;
    int idx;
    spif_cmd_t spif_cmd;

    for (idx = 0; idx < SPIF_GENERAL_CMD_TABLE_NUM; idx++) {
        spif_cmd.value = sys_read32(dev_config->base + SPIF_GENERAL_CMD_BASE + idx * 4);
        if ((spif_cmd.value) == 0)
            return idx;
    }

    return -ENOSR;
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

int spif_get_general_cmd_slot(const struct device *dev, uint8_t cmd, uint32_t start_off)
{
    __unused const struct linkedsemi_spi_filter_config *dev_config = dev->config;
    __unused struct linkedsemi_spi_filter_data *dev_data = dev->data;
    int idx;
    spif_cmd_t spif_cmd;

    for (idx = start_off; idx < SPIF_GENERAL_CMD_TABLE_NUM; idx++) {
        spif_cmd.value = sys_read32(dev_config->base + SPIF_GENERAL_CMD_BASE + idx * 4);
        if ((spif_cmd.field.CMD) == cmd)
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
void spif_dump_cmd_table(const struct device *dev)
{
    __unused const struct linkedsemi_spi_filter_config *dev_config = dev->config;
    __unused struct linkedsemi_spi_filter_data *dev_data = dev->data;

    acquire_spif_device(dev);

    for (uint32_t i = 0; i < SPIF_CMD_TABLE_NUM; i++) {
        spif_cmd_t spif_cmd;
        spif_cmd.value = sys_read32(dev_config->base + SPIF_CMD_BASE + i * 4);
        if (spif_cmd.value == 0)
            continue;
        LOG_DBG("[%s]idx %02d: 0x%02x: %s\n", dev->name, i,
            spif_cmd.field.CMD, spif_cmd.field.EN == 1 ? "enabled" : "disabled");
    }

    release_spif_device(dev);
}

static void spif_mgnt_area_parser(const struct device *dev,
                                      struct priv_reg_info start,
                                      struct priv_reg_info *res,
                                      uint32_t *num_forbidden_blk,
                                      enum addr_priv_rw_select region)
{
    __unused const struct linkedsemi_spi_filter_config *dev_config = dev->config;
    __unused struct linkedsemi_spi_filter_data *dev_data = dev->data;
    mm_reg_t priv_table_base;
    uint32_t reg_off = start.start_reg_off;
    uint32_t bit_off = start.start_bit_off;
    uint32_t reg_val;
    uint32_t i;

    if (region == FLAG_ADDR_PRIV_READ_SELECT) {
        priv_table_base = dev_config->base + SPIF_ADDR_PRIV_TABLE_BASE + SPIF_ADDR_SIZE;
    } else {
        priv_table_base = dev_config->base + SPIF_ADDR_PRIV_TABLE_BASE;
    }

    /* init search result */
    *num_forbidden_blk = 0;

    while (reg_off < SPIF_ADDR_PRIV_REG_NUN) {
        reg_val = sys_read32(priv_table_base + reg_off * 4);
        reg_val >>= bit_off;
        for (i = bit_off; i < 32; i++) {
            if ((reg_val & 1) == 0) {
                if (*num_forbidden_blk == 0) {
                    /* get the first forbidden block */
                    res->start_reg_off = reg_off;
                    res->start_bit_off = i;
                }

                (*num_forbidden_blk)++;
            } else if ((reg_val & 1) == 1 && *num_forbidden_blk != 0) {
                res->end_reg_off = reg_off;
                res->end_bit_off = i;
                return;
            }

            reg_val >>= 1;
        }

        bit_off = 0;
        reg_off++;
    }

    res->end_reg_off = SPIF_ADDR_PRIV_REG_NUN - 1;
    res->end_bit_off = 32;
}

void spif_dump_rw_addr_privilege_table(const struct device *dev)
{
    __unused const struct linkedsemi_spi_filter_config *dev_config = dev->config;
    __unused struct linkedsemi_spi_filter_data *dev_data = dev->data;
    uint32_t num_forbidden_blk = 0;
    struct priv_reg_info start;
    struct priv_reg_info res;
    bool mgnt_en = false;
    uint32_t rw;

    acquire_spif_device(dev);

    for (rw = 0; rw < 2; rw++) {
        memset(&start, 0x0, sizeof(struct priv_reg_info));
        memset(&res, 0x0, sizeof(struct priv_reg_info));
        LOG_DBG("%s mgnt regions:\n", rw == 0 ? "read" : "write");
        do {
            spif_mgnt_area_parser(dev, start, &res, &num_forbidden_blk, rw);
            if (num_forbidden_blk != 0) {
                mgnt_en = true;
                LOG_DBG("[0x%08x - 0x%08x]\n",
                       SPIF_ABS_ADDR(res.start_reg_off, res.start_bit_off),
                       SPIF_ABS_ADDR(res.end_reg_off, res.end_bit_off));
                start.start_reg_off = res.end_reg_off;
                start.start_bit_off = res.end_bit_off;
            }
        } while (num_forbidden_blk != 0);

        if (!mgnt_en)
            LOG_DBG("all regions are %s!\n", rw == 0 ? "readable" : "writable");
        LOG_DBG("======END======\n\n");
    }

    release_spif_device(dev);
}

static uint32_t spif_get_cross_block_num(uint32_t addr, uint32_t len)
{
    if (len == 0)
        return 0;

    if (addr % KB(16) != 0)
        len += (addr % KB(16));

    /* 16KB aligned */
    len = (len + KB(16) - 1) / KB(16) * KB(16);

    return len / KB(16);
}

int spif_address_privilege_config(const struct device *dev,
                                  enum addr_priv_rw_select rw_select,
                                  enum addr_priv_op priv_op,
                                  mm_reg_t addr,
                                  uint32_t len)
{
    __unused const struct linkedsemi_spi_filter_config *dev_config = dev->config;
    __unused struct linkedsemi_spi_filter_data *dev_data = dev->data;
    mm_reg_t priv_table_base;
    int ret = 0;
    uint32_t reg_off;
    uint32_t bit_off;
    uint32_t total_bit_num;
    uint32_t reg_val;

    if (addr >= MB(256) || len == 0) {
        LOG_WRN("invalid address or zero length!");
        ret = -EINVAL;
        goto end;
    }

    if (addr + len > MB(256)) {
        LOG_WRN("invalid protected regions, change the protected length...");
        len -= (addr + len - MB(256));
        LOG_WRN("the new length: 0x%08x", len);
    }

    if ((addr % KB(16)) != 0 || (len % KB(16)) != 0) {
        LOG_WRN("protected address(0x%08lx) and length(0x%08x) should be 16KB aligned",
                addr,
                len);
        LOG_WRN("stricter mgnt regions will be applied. (force 16KB aligned)");
        /* protect more region in order to align 16KB boundary */
        len = addr + len - (addr / KB(16)) * KB(16);
        addr = (addr / KB(16)) * KB(16);
        len = ((len + KB(16) - 1) / KB(16)) * KB(16);
    }

    reg_off = addr / KB(512);            /* 512K per register; */
    bit_off = (addr % KB(512)) / KB(16); /* (512K / 16K); */
    total_bit_num = spif_get_cross_block_num(addr, len);
    LOG_DBG("addr: 0x%08lx, len: 0x%08x\n", addr, len);
    LOG_DBG("reg_off: 0x%08x, bit_off: 0x%08x, total_bit_num: 0x%08x\n",
            reg_off,
            bit_off,
            total_bit_num);

    acquire_spif_device(dev);

    if (rw_select == FLAG_ADDR_PRIV_READ_SELECT) {
        priv_table_base = dev_config->base + SPIF_ADDR_PRIV_TABLE_BASE + SPIF_ADDR_SIZE;
    } else {
        priv_table_base = dev_config->base + SPIF_ADDR_PRIV_TABLE_BASE;
    }

    do {
        if (bit_off > 31) {
            bit_off = 0;
            reg_off++;
        }

        if (bit_off == 0 && total_bit_num >= 32) {
            /* speed up for large area configuration */
            if (priv_op == FLAG_ADDR_PRIV_ENABLE)
                sys_write32(0xffffffff, priv_table_base + reg_off * 4);
            else
                sys_write32(0x0, priv_table_base + reg_off * 4);

            reg_off++;
            total_bit_num -= 32;
        } else {
            reg_val = sys_read32(priv_table_base + reg_off * 4);
            if (priv_op == FLAG_ADDR_PRIV_ENABLE) {
                sys_write32(reg_val | BIT(bit_off),
                            priv_table_base + reg_off * 4);
            } else {
                sys_write32(reg_val & (~BIT(bit_off)),
                            priv_table_base + reg_off * 4);
            }

            LOG_DBG("reg: 0x%08lx, val: 0x%08x\n",
                    priv_table_base + reg_off * 4,
                    sys_read32(priv_table_base + reg_off * 4));

            bit_off++;
            total_bit_num--;
        }
    } while (total_bit_num > 0);

end:
    release_spif_device(dev);

    return ret;
}

int spif_add_general_cmd(const struct device *dev, uint8_t cmd)
{
    __unused const struct linkedsemi_spi_filter_config *dev_config = dev->config;
    __unused struct linkedsemi_spi_filter_data *dev_data = dev->data;
    int ret = 0;
    mm_reg_t table_base = dev_config->base + SPIF_GENERAL_CMD_BASE;
    int idx;

    acquire_spif_device(dev);

    for (uint8_t off = 0; off < SPIF_GENERAL_CMD_TABLE_NUM; off++) {
        idx = spif_get_general_cmd_slot(dev, cmd, off);
        if (idx >= 0) {
            spif_cmd_t spif_cmd;
            spif_cmd.value = sys_read32(table_base + idx * 4);
            spif_cmd.field.EN = 1;
            sys_write32(spif_cmd.value, table_base + idx * 4);
            goto end;
        } else {
            break;
        }
    }

    idx = spif_get_empty_general_cmd_slot(dev);
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
            goto end;
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

int spif_remove_general_cmd(const struct device *dev, uint8_t cmd)
{
    __unused const struct linkedsemi_spi_filter_config *dev_config = dev->config;
    __unused struct linkedsemi_spi_filter_data *dev_data = dev->data;
    mm_reg_t table_base = dev_config->base + SPIF_GENERAL_CMD_BASE;
    int ret = 0;
    int idx;
    uint32_t off;
    bool found = false;

    acquire_spif_device(dev);

    for (off = 0; off < SPIF_GENERAL_CMD_TABLE_NUM; off++) {
        idx = spif_get_general_cmd_slot(dev, cmd, off);
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

void spif_monitor_enable(const struct device *dev, bool enable)
{
    __unused const struct linkedsemi_spi_filter_config *dev_config = dev->config;
    __unused struct linkedsemi_spi_filter_data *dev_data = dev->data;

    acquire_spif_device(dev);

    spif_cfg_t spif_cfg;
    spif_cfg.value = sys_read32(dev_config->base + SPIF_CFG);
    spif_cfg.field.EN = enable ? 1 : 0;
    sys_write32(spif_cfg.value, dev_config->base + SPIF_CFG);

    release_spif_device(dev);
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
    for (uint8_t i = 0; i < SPIF_FIXED_CMD_TABLE_NUM; i++) {
        mm_reg_t table_base = dev_config->base + SPIF_CMD_BASE;
        spif_cmd_t spif_cmd;
        spif_cmd.field.CMD = dev_data->fixed_cmd_tab[i];
        spif_cmd.field.EN = 0;
        sys_write32(spif_cmd.value, table_base + i * 4);
    }
    sys_write32(0xffffffff, dev_config->base + SPIF_TARGET_ADDR);
    sys_write32(0x7, dev_config->base + SPIF_BCMD_RANGE);
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

#define SPI_FILTER_INIT(inst)                                                                                                                                                                                                        \
    static void linkedsemi_spi_filter_irq_config_func_##inst(const struct device *dev)                                                                                                                                               \
    {                                                                                                                                                                                                                                \
        ARG_UNUSED(dev);                                                                                                                                                                                                             \
        IRQ_CONNECT(DT_INST_IRQN(inst),                                                                                                                                                                                              \
                    DT_INST_IRQ(inst, priority),                                                                                                                                                                                     \
                    linkedsemi_spi_filter_isr,                                                                                                                                                                                       \
                    DEVICE_DT_INST_GET(inst),                                                                                                                                                                                        \
                    0);                                                                                                                                                                                                              \
        irq_enable(DT_INST_IRQN(inst));                                                                                                                                                                                              \
    }                                                                                                                                                                                                                                \
    PINCTRL_DT_INST_DEFINE(inst);                                                                                                                                                                                                    \
    static const struct linkedsemi_spi_filter_config linkedsemi_spi_filter_cfg_##inst = {                                                                                                                                            \
        .base = (mm_reg_t)DT_INST_REG_ADDR(inst),                                                                                                                                                                                    \
        .irq_config_func = linkedsemi_spi_filter_irq_config_func_##inst,                                                                                                                                                             \
        IF_ENABLED(CONFIG_PINCTRL, (.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst), ))                                                                                                                                                 \
    };                                                                                                                                                                                                                               \
    static struct linkedsemi_spi_filter_data linkedsemi_spi_filter_data_##inst = {                                                                                                                                                   \
        .fixed_cmd_tab = {                                                                                                                                                                                                           \
            IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, cmd_page_program), ([IDX_CMD_PAGE_PROGRAM] = DT_INST_PROP(inst, cmd_page_program), ))                                                                                             \
            IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, cmd_page_program_quad_address_quad_data), ([IDX_CMD_PAGE_PROGRAM_QUAD_ADDRESS_QUAD_DATA] = DT_INST_PROP(inst, cmd_page_program_quad_address_quad_data), ))                        \
            IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, cmd_erase_4kb), ([IDX_CMD_ERASE_4KB] = DT_INST_PROP(inst, cmd_erase_4kb), ))                                                                                                      \
            IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, cmd_erase_32kb), ([IDX_CMD_ERASE_32KB] = DT_INST_PROP(inst, cmd_erase_32kb), ))                                                                                                   \
            IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, cmd_erase_64kb), ([IDX_CMD_ERASE_64KB] = DT_INST_PROP(inst, cmd_erase_64kb), ))                                                                                                   \
            IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, cmd_read), ([IDX_CMD_READ] = DT_INST_PROP(inst, cmd_read), ))                                                                                                                     \
            IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, cmd_fast_read), ([IDX_CMD_FAST_READ] = DT_INST_PROP(inst, cmd_fast_read), ))                                                                                                      \
            IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, cmd_read_quad_data), ([IDX_CMD_READ_QUAD_DATA] = DT_INST_PROP(inst, cmd_read_quad_data), ))                                                                                       \
            IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, cmd_read_quad_address_quad_data), ([IDX_CMD_READ_QUAD_ADDRESS_QUAD_DATA] = DT_INST_PROP(inst, cmd_read_quad_address_quad_data), ))                                                \
            IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, cmd_quad_spi_mode_enter), ([IDX_CMD_QUAD_SPI_MODE_ENTER] = DT_INST_PROP(inst, cmd_quad_spi_mode_enter), ))                                                                        \
            IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, cmd_quad_spi_mode_exit), ([IDX_CMD_QUAD_SPI_MODE_EXIT] = DT_INST_PROP(inst, cmd_quad_spi_mode_exit), ))                                                                           \
            IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, cmd_4byte_mode_enter), ([IDX_CMD_4BYTE_MODE_ENTER] = DT_INST_PROP(inst, cmd_4byte_mode_enter), ))                                                                                 \
            IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, cmd_4byte_mode_exit), ([IDX_CMD_4BYTE_MODE_EXIT] = DT_INST_PROP(inst, cmd_4byte_mode_exit), ))                                                                                    \
            IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, cmd_4byte_read_extended_address), ([IDX_CMD_4BYTE_READ_EXTENDED_ADDRESS] = DT_INST_PROP(inst, cmd_4byte_read_extended_address), ))                                                \
            IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, cmd_4byte_write_extended_address), ([IDX_CMD_4BYTE_WRITE_EXTENDED_ADDRESS] = DT_INST_PROP(inst, cmd_4byte_write_extended_address), ))                                             \
            IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, cmd_4byte_page_program), ([IDX_CMD_4BYTE_PAGE_PROGRAM] = DT_INST_PROP(inst, cmd_4byte_page_program), ))                                                                           \
            IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, cmd_4byte_page_program_quad_address_quad_data), ([IDX_CMD_4BYTE_PAGE_PROGRAM_QUAD_ADDRESS_QUAD_DATA] = DT_INST_PROP(inst, cmd_4byte_page_program_quad_address_quad_data), ))      \
            IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, cmd_4byte_erase_4kb), ([IDX_CMD_4BYTE_ERASE_4KB] = DT_INST_PROP(inst, cmd_4byte_erase_4kb), ))                                                                                    \
            IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, cmd_4byte_erase_32kb), ([IDX_CMD_4BYTE_ERASE_32KB] = DT_INST_PROP(inst, cmd_4byte_erase_32kb), ))                                                                                 \
            IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, cmd_4byte_erase_64kb), ([IDX_CMD_4BYTE_ERASE_64KB] = DT_INST_PROP(inst, cmd_4byte_erase_64kb), ))                                                                                 \
            IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, cmd_4byte_read), ([IDX_CMD_4BYTE_READ] = DT_INST_PROP(inst, cmd_4byte_read), ))                                                                                                   \
            IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, cmd_4byte_fast_read), ([IDX_CMD_4BYTE_FAST_READ] = DT_INST_PROP(inst, cmd_4byte_fast_read), ))                                                                                    \
            IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, cmd_4byte_read_quad_data), ([IDX_CMD_4BYTE_READ_QUAD_DATA] = DT_INST_PROP(inst, cmd_4byte_read_quad_data), ))                                                                     \
            IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, cmd_4byte_read_quad_address_quad_data), ([IDX_CMD_4BYTE_READ_QUAD_ADDRESS_QUAD_DATA] = DT_INST_PROP(inst, cmd_4byte_read_quad_address_quad_data), ))                              \
            IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, cmd_read_dual_data ), ([IDX_CMD_READ_DUAL_DATA] = DT_INST_PROP(inst, cmd_read_dual_data ), ))                                                                                     \
            IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, cmd_read_dual_addr_dual_data ), ([IDX_CMD_READ_DUAL_ADDR_DUAL_DATA] = DT_INST_PROP(inst, cmd_read_dual_addr_dual_data ), ))                                                       \
            IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, cmd_4byte_read_dual_data ), ([IDX_CMD_4BYTE_READ_DUAL_DATA] = DT_INST_PROP(inst, cmd_4byte_read_dual_data ), ))                                                                   \
            IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, cmd_4byte_read_dual_addr_dual_data ), ([IDX_CMD_4BYTE_READ_DUAL_ADDR_DUAL_DATA] = DT_INST_PROP(inst, cmd_4byte_read_dual_addr_dual_data ), ))                                     \
            IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, cmd_program_quad_data ), ([IDX_CMD_PROGRAM_QUAD_DATA] = DT_INST_PROP(inst, cmd_program_quad_data ), ))                                                                            \
            IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, cmd_4byte_program_quad_data ), ([IDX_CMD_4BYTE_PROGRAM_QUAD_DATA] = DT_INST_PROP(inst, cmd_4byte_program_quad_data ), ))                                                          \
        },                                                                                                                                                                                                                           \
    };                                                                                                                                                                                                                               \
    DEVICE_DT_INST_DEFINE(inst,                                                                                                                                                                                                      \
                          linkedsemi_spi_filter_init,                                                                                                                                                                                \
                          NULL,                                                                                                                                                                                                      \
                          &linkedsemi_spi_filter_data_##inst,                                                                                                                                                                        \
                          &linkedsemi_spi_filter_cfg_##inst,                                                                                                                                                                         \
                          POST_KERNEL,                                                                                                                                                                                               \
                          CONFIG_KERNEL_INIT_PRIORITY_DEVICE,                                                                                                                                                                        \
                          NULL);

DT_INST_FOREACH_STATUS_OKAY(SPI_FILTER_INIT)
