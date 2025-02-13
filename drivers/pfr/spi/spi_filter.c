/*
 * Copyright (c) 2024 Linkedsemi
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#if 0
    #if !defined(CONFIG_NOCACHE_MEMORY)
        #error "missing memory attribute for descriptors"
    #endif
#endif

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
    uint8_t dma_chan;
    uint8_t dma_handshake;
};

struct linkedsemi_spi_filter_data {
    struct k_sem sem_spif;
    uint8_t fixed_cmd_tab[SPIF_FIXED_CMD_TABLE_NUM];
    spif_callback_t cb;
    void *user_data;
    uint32_t *dma_mem;
    uint16_t dma_log_cnt;
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

    intr_status.value = sys_read32(dev_config->base + SPIF_INTR_STT);
    sys_write32(intr_status.value, dev_config->base + SPIF_INTR_CLR);
    if (intr_status.ERROR_OVERFLOW) {
        LOG_DBG("ERROR_OVERFLOW\n");
    }
    if (intr_status.ERROR) {
        LOG_DBG("ERROR\n");
    }
    if (intr_status.TARGET_ADDR) {
        LOG_DBG("TARGET_ADDR\n");
    }
    if (intr_status.SCK_CHECK) {
        spif_sck_fqc_hi_t spif_sck_fqc_hi;
        spif_sck_fqc_lo_t spif_sck_fqc_lo;
        spif_sck_set_t spif_sck_set;
        spif_sck_fqc_hi.value = sys_read32(dev_config->base + SPIF_SCK_FQC_HI);
        spif_sck_fqc_lo.value = sys_read32(dev_config->base + SPIF_SCK_FQC_LO);
        spif_sck_set.value = sys_read32(dev_config->base + SPIF_SCK_SET);
        LOG_DBG("SCK_CHECK: expect [%#x, %#x] but got %#x\n",
               spif_sck_fqc_lo.SCK_FQC_LO,
               spif_sck_fqc_hi.SCK_FQC_HI,
               spif_sck_set.SCK_FQC);
    }

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
        if ((spif_cmd.CMD) == cmd)
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
        if ((spif_cmd.CMD) == cmd)
            return idx;
    }

    return -ENOSR;
}

static const uint8_t *fix_cmd_desc[] = {
    "cmd-page-program",
    "cmd-page-program-quad-address-quad-data",
    "cmd-erase-4kb",
    "cmd-erase-32kb",
    "cmd-erase-64kb",
    "cmd-read",
    "cmd-fast-read",
    "cmd-read-quad-data",
    "cmd-read-quad-address-quad-data",
    "cmd-quad-spi-mode-enter",
    "cmd-quad-spi-mode-exit",
    "cmd-4byte-mode-enter",
    "cmd-4byte-mode-exit",
    "cmd-4byte-read-extended-address",
    "cmd-4byte-write-extended-address",
    "cmd-4byte-page-program",
    "cmd-4byte-page-program-quad-address-quad-data",
    "cmd-4byte-erase-4kb",
    "cmd-4byte-erase-32kb",
    "cmd-4byte-erase-64kb",
    "cmd-4byte-read",
    "cmd-4byte-fast-read",
    "cmd-4byte-read-quad-data",
    "cmd-4byte-read-quad-address-quad-data",
    "cmd-read-dual-data",
    "cmd-read-dual-addr-dual-data",
    "cmd-4byte-read-dual-data",
    "cmd-4byte-read-dual-addr-dual-data",
    "cmd-program-quad-data",
    "cmd-4byte-program-quad-data",
    "cmd-4byte-program-quad-data",
};

static const uint8_t *general_cmd_desc = "cmd-general";

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
        if (i < SPIF_FIXED_CMD_TABLE_NUM) {
            LOG_DBG("[%s]idx %02d: %-46.46s: 0x%02x: %s\n", dev->name, i, fix_cmd_desc[i], spif_cmd.CMD, spif_cmd.EN == 1 ? "enabled" : "disabled");
        } else {
            LOG_DBG("[%s]idx %02d: %-46.46s: 0x%02x: %s\n", dev->name, i, general_cmd_desc, spif_cmd.CMD, spif_cmd.EN == 1 ? "enabled" : "disabled");
        }
    }

    release_spif_device(dev);
}

static void spif_peek_rw_area(const struct device *dev, enum addr_priv_rw_select rw_select, uint32_t addr, uint32_t *data)
{
    __unused const struct linkedsemi_spi_filter_config *dev_config = dev->config;
    __unused struct linkedsemi_spi_filter_data *dev_data = dev->data;
    spif_read_addr_req_t spif_read_addr_req = {};

    if (rw_select == FLAG_ADDR_PRIV_READ_SELECT) {
        spif_read_addr_req.READ_RADDR_REQ = 1;
        spif_read_addr_req.READ_WADDR_REQ = 0;
    } else {
        spif_read_addr_req.READ_RADDR_REQ = 0;
        spif_read_addr_req.READ_WADDR_REQ = 1;
    }

    sys_write32(addr, dev_config->base + SPIF_READ_SRAM_ADDR);
    sys_write32(spif_read_addr_req.value, dev_config->base + SPIF_READ_ADDR_REQ);
    while (sys_read32(dev_config->base + SPIF_READ_ADDR_REQ)); /* wait for cs line idle */
    *data = sys_read32(dev_config->base + SPIF_READ_SRAM_DATA);
}

static void spif_allocated_area_parser(const struct device *dev,
                                       struct priv_reg_info start,
                                       struct priv_reg_info *res,
                                       uint32_t *num_allocated_blk,
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
    *num_allocated_blk = 0;

    while (reg_off < SPIF_ADDR_PRIV_REG_NUN) {
        spif_peek_rw_area(dev, region, reg_off, &reg_val);
        reg_val >>= bit_off;
        for (i = bit_off; i < 32; i++) {
            if ((reg_val & 1) == 1) {
                if (*num_allocated_blk == 0) {
                    /* get the first allocated block */
                    res->start_reg_off = reg_off;
                    res->start_bit_off = i;
                }

                (*num_allocated_blk)++;
            } else if ((reg_val & 1) == 0 && *num_allocated_blk != 0) {
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

void spif_dump_cmd_bitmap_log(const struct device *dev, uint8_t bitmap[SPIF_CMD_BITMAP_LOG_SIZE_BYTE])
{
    __unused const struct linkedsemi_spi_filter_config *dev_config = dev->config;
    __unused struct linkedsemi_spi_filter_data *dev_data = dev->data;

    for (uint32_t i = 0; i < SPI_CMD_BITMAPF_LOG_SIZE_U32; i++) {
        uint32_t val = sys_read32(dev_config->base + SPIF_BIT_MAP0 - (i * 4));
        UNALIGNED_PUT(val, (uint32_t *)(bitmap + (i * 4)));
    }
}

void spif_clear_cmd_bitmap_log(const struct device *dev)
{
    __unused const struct linkedsemi_spi_filter_config *dev_config = dev->config;
    __unused struct linkedsemi_spi_filter_data *dev_data = dev->data;

    spif_intr_clr_t spif_intr_clr = { .BITMAP = 1 };
    sys_write32(spif_intr_clr.value, dev_config->base + SPIF_INTR_CLR);
}

void spif_dump_rw_addr_privilege_table(const struct device *dev)
{
    __unused const struct linkedsemi_spi_filter_config *dev_config = dev->config;
    __unused struct linkedsemi_spi_filter_data *dev_data = dev->data;
    uint32_t num_allocated_blk = 0;
    struct priv_reg_info start;
    struct priv_reg_info res;
    bool allocated_en = false;
    uint32_t rw;

    acquire_spif_device(dev);

    for (rw = 0; rw < 2; rw++) {
        memset(&start, 0x0, sizeof(struct priv_reg_info));
        memset(&res, 0x0, sizeof(struct priv_reg_info));
        LOG_DBG("%s allocated regions:\n", rw == 0 ? "read" : "write");
        do {
            spif_allocated_area_parser(dev, start, &res, &num_allocated_blk, rw);
            if (num_allocated_blk != 0) {
                allocated_en = true;
                LOG_DBG("[0x%08x - 0x%08x]\n",
                       SPIF_ABS_ADDR(res.start_reg_off, res.start_bit_off),
                       SPIF_ABS_ADDR(res.end_reg_off, res.end_bit_off));
                start.start_reg_off = res.end_reg_off;
                start.start_bit_off = res.end_bit_off;
            }
        } while (num_allocated_blk != 0);

        if (!allocated_en) {
            LOG_DBG("all regions are free!\n");
        }
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
        LOG_WRN("stricter allocated regions will be applied. (force 16KB aligned)");
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
        priv_table_base = dev_config->base + SPIF_READ_ADDR_VALID_EN_ADDR;
    } else {
        priv_table_base = dev_config->base + SPIF_WRITE_ADDR_VALID_EN_ADDR;
    }

    do {
        if (bit_off > 31) {
            bit_off = 0;
            reg_off++;
        }

        if (bit_off == 0 && total_bit_num >= 32) {
            /* speed up for large area configuration */
            if (priv_op == FLAG_ADDR_PRIV_ENABLE) {
                sys_write32(0xffffffff, priv_table_base + reg_off * 4);
            } else {
                sys_write32(0x0, priv_table_base + reg_off * 4);
            }

            reg_off++;
            total_bit_num -= 32;
        } else {
            spif_peek_rw_area(dev, rw_select, reg_off, &reg_val);
            if (priv_op == FLAG_ADDR_PRIV_ENABLE) {
                reg_val |= BIT(bit_off);
            } else {
                reg_val &= ~BIT(bit_off);
            }
            sys_write32(reg_val, priv_table_base + reg_off * 4);
            LOG_DBG("reg: 0x%08lx, val: 0x%08x\n", priv_table_base + reg_off * 4, reg_val);

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
            spif_cmd.EN = 1;
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
    spif_cmd.CMD = cmd;
    spif_cmd.EN = 1;
    sys_write32(spif_cmd.value, table_base + idx * 4);

end:
    release_spif_device(dev);

    return ret;
}

int spif_add_cmd(const struct device *dev, uint8_t cmd, uint8_t dummy_cycle)
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
            spif_cmd.EN = 1;
            sys_write32(spif_cmd.value, table_base + idx * 4);
            goto end;
        } else {
            break;
        }
    }

    for (uint8_t off = 0; off < SPIF_FIXED_CMD_TABLE_NUM; off++) {
        if (dev_data->fixed_cmd_tab[off] == cmd) {
            spif_cmd_t spif_cmd;
            spif_cmd.CMD = cmd;
            spif_cmd.DUMMY_CYCLE = dummy_cycle;
            spif_cmd.EN = 1;
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
    spif_cmd.CMD = cmd;
    spif_cmd.EN = 1;
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
            if (idx < SPIF_FIXED_CMD_TABLE_NUM) {
                spif_cmd_t spif_cmd;
                spif_cmd.value = sys_read32(table_base + idx * 4);
                spif_cmd.EN = 0;
                sys_write32(spif_cmd.value, table_base + idx * 4);
            } else {
                sys_write32(0, table_base + idx * 4);
            }
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

void spif_filter_enable(const struct device *dev, bool enable)
{
    __unused const struct linkedsemi_spi_filter_config *dev_config = dev->config;
    __unused struct linkedsemi_spi_filter_data *dev_data = dev->data;

    acquire_spif_device(dev);

    spif_cfg_t spif_cfg;
    spif_cfg.value = sys_read32(dev_config->base + SPIF_CFG);
    spif_cfg.EN = enable ? 1 : 0;
    sys_write32(spif_cfg.value, dev_config->base + SPIF_CFG);

    release_spif_device(dev);
}

void spif_reg_unlock(const struct device *dev)
{
    __unused const struct linkedsemi_spi_filter_config *dev_config = dev->config;
    __unused struct linkedsemi_spi_filter_data *dev_data = dev->data;

    spif_cfg_t spif_cfg = { .IP_LOCK = 1, };
    sys_write32(spif_cfg.value, dev_config->base + SPIF_CFG);
}

void spif_reg_lock(const struct device *dev)
{
    __unused const struct linkedsemi_spi_filter_config *dev_config = dev->config;
    __unused struct linkedsemi_spi_filter_data *dev_data = dev->data;

    spif_cfg_t spif_cfg = { .IP_LOCK = 0, };
    sys_write32(spif_cfg.value, dev_config->base + SPIF_CFG);
}

void spif_clk_check_config(const struct device *dev,
                           uint8_t div,
                           uint16_t threshold_high_cycle,
                           uint16_t threshold_low_cycle,
                           bool enable_intr)
{
    __ASSERT_NO_MSG(threshold_high_cycle < threshold_low_cycle);
    __unused const struct linkedsemi_spi_filter_config *dev_config = dev->config;
    __unused struct linkedsemi_spi_filter_data *dev_data = dev->data;
    spif_sck_set_t spif_sck_set;
    spif_sck_fqc_hi_t spif_sck_fqc_hi;
    spif_sck_fqc_lo_t spif_sck_fqc_lo;
    spif_intr_t intr_mask;

    spif_sck_set.value = sys_read32(dev_config->base + SPIF_SCK_SET);
    spif_sck_set.SCK_DIV = div;
    sys_write32(spif_sck_set.value, dev_config->base + SPIF_SCK_SET);

    spif_sck_fqc_hi.value = sys_read32(dev_config->base + SPIF_SCK_FQC_HI);
    spif_sck_fqc_hi.SCK_FQC_HI = threshold_high_cycle;
    sys_write32(spif_sck_fqc_hi.value, dev_config->base + SPIF_SCK_FQC_HI);

    spif_sck_fqc_lo.value = sys_read32(dev_config->base + SPIF_SCK_FQC_LO);
    spif_sck_fqc_lo.SCK_FQC_LO = threshold_low_cycle;
    sys_write32(spif_sck_fqc_lo.value, dev_config->base + SPIF_SCK_FQC_LO);

    intr_mask.value = sys_read32(dev_config->base + SPIF_INTR_MASK);
    intr_mask.SCK_CHECK = enable_intr ? 1 : 0;
    sys_write32(intr_mask.value, dev_config->base + SPIF_INTR_MASK);
}

uint16_t spif_clk_check_peek(const struct device *dev)
{
    __unused const struct linkedsemi_spi_filter_config *dev_config = dev->config;
    __unused struct linkedsemi_spi_filter_data *dev_data = dev->data;
    spif_sck_set_t spif_sck_set;
    spif_sck_set.value = sys_read32(dev_config->base + SPIF_SCK_SET);

    return spif_sck_set.SCK_FQC;
}

uint32_t *spif_log_dma_buf(const struct device *dev)
{
    __unused const struct linkedsemi_spi_filter_config *dev_config = dev->config;
    __unused struct linkedsemi_spi_filter_data *dev_data = dev->data;

    return dev_data->dma_mem;
}

uint16_t HAL_DMA_Controller_Peek_BLOCK_TS(DMA_Controller_HandleTypeDef *hdma, uint8_t ch_idx)
{
    volatile uint64_t *CTL = (void *)hdma->Instance->CH[ch_idx].CTL;
    return (*CTL >> 32) & DMAC_BLOCK_TS_MASK;
}

static void spif_dma_callback(DMA_Controller_HandleTypeDef *hdma, uint32_t param, uint8_t ch_idx, uint32_t *lli, bool tfr_end)
{
    const struct device *dev = (const struct device *)param;
    __unused const struct linkedsemi_spi_filter_config *dev_config = dev->config;
    __unused struct linkedsemi_spi_filter_data *dev_data = dev->data;
    uint16_t block_ts = HAL_DMA_Controller_Peek_BLOCK_TS(hdma, ch_idx);

    if (tfr_end == false) {
        if (dev_data->dma_log_cnt < block_ts) {
            for (uint16_t i = dev_data->dma_log_cnt; i < block_ts; i++) {
                spif_dma_data_t *spif_dma_data = (spif_dma_data_t *)dev_data->dma_mem;
                printk("dma log idx: %d\n", i);
                printk("SPIF_ADDR_ERR: %#x\n", spif_dma_data[i].SPIF_ADDR_ERR);
                printk("SPIF_CMD_ERR: %#x\n", spif_dma_data[i].SPIF_CMD_ERR);
                printk("SPIF_POR_ADDR: %#x\n", spif_dma_data[i].SPIF_POR_ADDR);
                printk("SPIF_ERROR_ADDR: %#x\n", spif_dma_data[i].SPIF_ERROR_ADDR);
                printk("SPIF_ERROR_CMD: %#x\n", spif_dma_data[i].SPIF_ERROR_CMD);
            }
        } else {
            for (uint16_t i = dev_data->dma_log_cnt; i < SPIF_LOG_RAM_MAX_SIZE_U32; i++) {
                spif_dma_data_t *spif_dma_data = (spif_dma_data_t *)dev_data->dma_mem;
                printk("dma log idx: %d\n", i);
                printk("SPIF_ADDR_ERR: %#x\n", spif_dma_data[i].SPIF_ADDR_ERR);
                printk("SPIF_CMD_ERR: %#x\n", spif_dma_data[i].SPIF_CMD_ERR);
                printk("SPIF_POR_ADDR: %#x\n", spif_dma_data[i].SPIF_POR_ADDR);
                printk("SPIF_ERROR_ADDR: %#x\n", spif_dma_data[i].SPIF_ERROR_ADDR);
                printk("SPIF_ERROR_CMD: %#x\n", spif_dma_data[i].SPIF_ERROR_CMD);
            }
            for (uint16_t i = 0; i < block_ts; i++) {
                spif_dma_data_t *spif_dma_data = (spif_dma_data_t *)dev_data->dma_mem;
                printk("dma log idx: %d\n", i);
                printk("SPIF_ADDR_ERR: %#x\n", spif_dma_data[i].SPIF_ADDR_ERR);
                printk("SPIF_CMD_ERR: %#x\n", spif_dma_data[i].SPIF_CMD_ERR);
                printk("SPIF_POR_ADDR: %#x\n", spif_dma_data[i].SPIF_POR_ADDR);
                printk("SPIF_ERROR_ADDR: %#x\n", spif_dma_data[i].SPIF_ERROR_ADDR);
                printk("SPIF_ERROR_CMD: %#x\n", spif_dma_data[i].SPIF_ERROR_CMD);
            }
        }
        dev_data->dma_log_cnt = block_ts;
    } else {
        printk("log buffer full. reload.\n");
    }
}

int spif_dma_handshake_get(const struct device *dev)
{
    __unused const struct linkedsemi_spi_filter_config *dev_config = dev->config;
    __unused struct linkedsemi_spi_filter_data *dev_data = dev->data;

    int ret = 0;
    switch ((uint32_t)dev_config->base) {
    case SPIFILTER1: ret = DMA_SPIFILTER1; break;
    case SPIFILTER2: ret = DMA_SPIFILTER2; break;
    case SPIFILTER3: ret = DMA_SPIFILTER3; break;
    case SPIFILTER4: ret = DMA_SPIFILTER4; break;
    default: ret = -1; break;
    }

    return ret;
}

#define DMA_CHANNEL_CFG_RELOAD(reg_struct, idx, src, dst, data_width, size, type, lli, hs, scatter_count, scatter_interval, gather_count, gather_interval)                 \
    do {                                                                                                                                                                   \
        _DMA_SAR_DAR_LLP_CTL_SET(reg_struct, idx, src, dst, data_width, BUSRT_TRANSACTION_1_ITEM, BUSRT_TRANSACTION_1_ITEM, size, type, lli, scatter_count, gather_count); \
        reg_struct.cfg.reserved0 = 0;                                                                                                                                      \
        reg_struct.cfg.ch_prior = idx;                                                                                                                                     \
        reg_struct.cfg.ch_susp = 0;                                                                                                                                        \
        reg_struct.cfg.fifo_empty = 0;                                                                                                                                     \
        reg_struct.cfg.hs_sel_dst = 0;                                                                                                                                     \
        reg_struct.cfg.hs_sel_src = 0;                                                                                                                                     \
        reg_struct.cfg.lock_ch_l = 0;                                                                                                                                      \
        reg_struct.cfg.lock_b_l = 0;                                                                                                                                       \
        reg_struct.cfg.lock_ch = 0;                                                                                                                                        \
        reg_struct.cfg.dst_hs_pol = 0;                                                                                                                                     \
        reg_struct.cfg.src_hs_pol = 0;                                                                                                                                     \
        reg_struct.cfg.max_abrst = 0;                                                                                                                                      \
        reg_struct.cfg.reload_src = 1;                                                                                                                                     \
        reg_struct.cfg.reload_dst = 1;                                                                                                                                     \
        reg_struct.cfg.fcmode = 0;                                                                                                                                         \
        reg_struct.cfg.fifo_mode = 1;                                                                                                                                      \
        reg_struct.cfg.protctl = 1;                                                                                                                                        \
        reg_struct.cfg.ds_upd_en = 0;                                                                                                                                      \
        reg_struct.cfg.ss_upd_en = 0;                                                                                                                                      \
        reg_struct.cfg.src_per = idx;                                                                                                                                      \
        reg_struct.cfg.dst_per = idx;                                                                                                                                      \
        reg_struct.cfg.reserved1 = 0;                                                                                                                                      \
        reg_struct.sgr.interval = gather_interval;                                                                                                                         \
        reg_struct.sgr.count = gather_count;                                                                                                                               \
        reg_struct.dsr.interval = scatter_interval;                                                                                                                        \
        reg_struct.dsr.count = scatter_count;                                                                                                                              \
        reg_struct.ch_idx = idx;                                                                                                                                           \
        reg_struct.handshake = hs;                                                                                                                                         \
    } while (0)

void spif_dma_channel_start_it(DMA_Controller_HandleTypeDef *hdma, struct ch_reg *reg_cfg, void (*callback)(DMA_Controller_HandleTypeDef *, uint32_t, uint8_t, uint32_t *, bool), uint32_t param)
{
    uint8_t ch_idx = reg_cfg->ch_idx;
    hdma->channel_callback[ch_idx] = callback;
    hdma->param[ch_idx] = param;
    hdma->Instance->CH[ch_idx].SAR = reg_cfg->sar;
    hdma->Instance->CH[ch_idx].DAR = reg_cfg->dar;
    hdma->Instance->CH[ch_idx].LLP = reg_cfg->llp;
    uint64_t *ctl = (uint64_t *)&reg_cfg->ctl;
    volatile uint64_t *CTL = (void *)hdma->Instance->CH[ch_idx].CTL;
    *CTL = *ctl;
    uint64_t *cfg = (uint64_t *)&reg_cfg->cfg;
    volatile uint64_t *CFG = (void *)hdma->Instance->CH[ch_idx].CFG;
    *CFG = *cfg;
    uint32_t *sgr = (uint32_t *)&reg_cfg->sgr;
    hdma->Instance->CH[ch_idx].SGR = *sgr;
    uint32_t *dsr = (uint32_t *)&reg_cfg->dsr;
    hdma->Instance->CH[ch_idx].DSR = *dsr;
    HAL_DMA_Channel_Handshake_Set(hdma, ch_idx, reg_cfg->handshake);
    hdma->Instance->CLEARBLOCK = 1 << ch_idx;
    hdma->Instance->CLEARTFR = 1 << ch_idx;
    hdma->Instance->MASKBLOCK = 1 << 8 << ch_idx | 1 << ch_idx;
    hdma->Instance->MASKTFR = 1 << 8 << ch_idx | 1 << ch_idx;
    hdma->Instance->MASKDSTTRAN = 1 << 8 << ch_idx | 1 << ch_idx;
    hdma->Instance->CHEN = 1 << 8 << ch_idx | 1 << ch_idx;
}

void spif_dma_config(const struct device *dev, DMA_Controller_HandleTypeDef *hdma_inst)
{
    __unused const struct linkedsemi_spi_filter_config *dev_config = dev->config;
    __unused struct linkedsemi_spi_filter_data *dev_data = dev->data;

    memset(dev_data->dma_mem, 0, SPIF_LOG_RAM_MAX_SIZE_U32 * sizeof(uint32_t));

    spif_cfg_t spif_cfg;
    spif_cfg.value = sys_read32(dev_config->base + SPIF_CFG);
    spif_cfg.DMA_EN = 1;
    sys_write32(spif_cfg.value, dev_config->base + SPIF_CFG);

    uint8_t data_width = TRANSFER_WIDTH_32BITS;
    struct ch_reg cfg;
    DMA_CHANNEL_CFG_RELOAD(cfg,
                           dev_config->dma_chan,
                           dev_config->base + SPIF_DMA_DATA,
                           (uint32_t)dev_data->dma_mem,
                           data_width,
                           SPIF_LOG_RAM_MAX_SIZE_U32,
                           P2M,
                           0,
                           spif_dma_handshake_get(dev),
                           0,
                           0,
                           0,
                           0);
    spif_dma_channel_start_it(hdma_inst, &cfg, spif_dma_callback, (uint32_t)dev);
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
            LOG_WRN("Could not configure pins");
        }
    }
#endif
    spif_reg_unlock(dev);

    for (uint32_t off = 0; off < SPIF_ADDR_SIZE; off += 4) {
        sys_write32(0, dev_config->base + SPIF_WRITE_ADDR_VALID_EN_ADDR + off); /* memset WRITE_ADDR */
        sys_write32(0, dev_config->base + SPIF_READ_ADDR_VALID_EN_ADDR + off);  /* memset READ_ADDR */
    }
    for (uint8_t i = 0; i < SPIF_FIXED_CMD_TABLE_NUM; i++) {
        mm_reg_t table_base = dev_config->base + SPIF_CMD_BASE;
        spif_cmd_t spif_cmd;
        spif_cmd.CMD = dev_data->fixed_cmd_tab[i];
        spif_cmd.DUMMY_CYCLE = 0;
        spif_cmd.EN = 0;
        sys_write32(spif_cmd.value, table_base + i * 4); /* init fixed table */
    }
    sys_write32(0xffffffff, dev_config->base + SPIF_TARGET_ADDR);
    sys_write32(0x7, dev_config->base + SPIF_BCMD_RANGE);
    spif_cfg_t spif_cfg = {
        .EN = 1,
        .OPERATION_MODE = 1,
        .ADDR_3B_4B_SEL = 1,
        .TARGET_ADDR_MODE_SEL = 0,
        .BOW_CMD_SEL = dev_config->blacklist_en ? 1 : 0,
        .IP_LOCK = 1,
        .DMA_EN = 0,
    };
    sys_write32(spif_cfg.value, dev_config->base + SPIF_CFG);
    spif_address_privilege_config(dev, FLAG_ADDR_PRIV_WRITE_SELECT, FLAG_ADDR_PRIV_DISABLE, MB(0), MB(256));
    spif_address_privilege_config(dev, FLAG_ADDR_PRIV_READ_SELECT, FLAG_ADDR_PRIV_DISABLE, MB(0), MB(256));
    spif_intr_t intr_mask;
    intr_mask.value = sys_read32(dev_config->base + SPIF_INTR_MASK);
    intr_mask.ERROR_OVERFLOW = 1;
    intr_mask.ERROR = 1;
    intr_mask.TARGET_ADDR = 1;
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
    __nocache uint32_t dma_mem_##inst[SPIF_LOG_RAM_MAX_SIZE_U32];                                                                                                                                                                    \
    static const struct linkedsemi_spi_filter_config linkedsemi_spi_filter_cfg_##inst = {                                                                                                                                            \
        .base = (mm_reg_t)DT_INST_REG_ADDR(inst),                                                                                                                                                                                    \
        .irq_config_func = linkedsemi_spi_filter_irq_config_func_##inst,                                                                                                                                                             \
        .dma_chan = 0,                                                                                                                                                                                                               \
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
        .dma_mem = dma_mem_##inst,                                                                                                                                                                                                   \
        .dma_log_cnt = 0,                                                                                                                                                                                                            \
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
