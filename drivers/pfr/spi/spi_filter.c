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

#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/cache.h>
#include <zephyr/drivers/dma.h>
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
#include <zephyr/drivers/dma/dma_dw.h>
#include <soc_dma.h>

#define LOG_LEVEL CONFIG_SPI_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_pfr_filter);

#include <spi_filter.h>
#include <reg_spi_filter.h>
#include <lsqsh-pinctrl_pfr_tpm_func_pinctrl.h>
#include <ls_soc_gpio.h>

#define PINCTRL_STATE_PASSTHROUGH PINCTRL_STATE_PRIV_START
#define PINCTRL_STATE_MASTER      (PINCTRL_STATE_PRIV_START + 1U)

void spim_dump_allow_command_table(const struct device *dev)
{
    spif_dump_cmd_table(dev);
}

int spim_add_allow_command(const struct device *dev, uint8_t cmd, uint32_t flag)
{
    if (FLAG_CMD_TABLE_VALID_ONCE == flag) {
        LOG_ERR("FLAG_CMD_TABLE_VALID_ONCE is not support");
        return -ENOTSUP;
    }

    return spif_add_cmd(dev, cmd);
}

int spim_remove_allow_command(const struct device *dev, uint8_t cmd)
{
    return spif_remove_cmd(dev, cmd);
}

void spim_dump_rw_addr_privilege_table(const struct device *dev)
{
    spif_dump_rw_addr_privilege_table(dev);
}

int spim_address_privilege_config(const struct device *dev, enum addr_priv_rw_select rw_select, enum addr_priv_op priv_op, mm_reg_t addr, uint32_t len)
{
    return spif_address_privilege_config(dev, rw_select, priv_op, addr, len);
}

void spim_lock_common(const struct device *dev)
{
    spif_reg_lock(dev);
}

void spim_monitor_enable(const struct device *dev, bool enable)
{
    spif_enable(dev, enable);
}

void spim_isr_callback_install(const struct device *dev, spim_callback_t isr_callback)
{
    linkedsemi_spif_register_callback(dev, isr_callback);
}

void spim_get_log_info(const struct device *dev, struct spim_log_info *info)
{
    spif_get_log_info(dev, info);
}

void spim_log_parser(const struct device *dev, uint32_t idx, uint32_t log_val)
{
    __ASSERT_NO_MSG(dev);
    spif_dma_data_t *log = (spif_dma_data_t *)&log_val;
    if (log->CMD_ERR) {
        /* block command */
        LOG_ERR("[%s][b][%03d][cmd] %02xh", dev->name, idx, log->ERROR_CMD);
    } else if (log->ADDR_ERR) {
        if (log->POR_ADDR) {
            /* block read command */
            LOG_ERR("[%s][b][%03d][r_addr] 0x%08x", dev->name, idx, log->ERROR_ADDR << 11);
        } else {
            /* block write command */
            LOG_ERR("[%s][b][%03d][w_addr] 0x%08x", dev->name, idx, log->ERROR_ADDR << 11);
        }
    } else {
        LOG_ERR("[%s][%03d]invalid ctx: 0x%08x", dev->name, idx, log_val);
    }
}

uint32_t spim_get_ctrl_idx(const struct device *dev)
{
    return spif_get_ctrl_idx(dev);
}

void spim_allow_command_get(const struct device *dev, uint8_t cmd[SPIF_CMD_TABLE_NUM], uint32_t *cmd_num)
{
    spif_get_cmd_table(dev, cmd, cmd_num);
}

struct spif_dma_config {
    int32_t state;
    void (*irq_call_back)(void);
    struct dma_config dma_cfg;
    struct dma_block_config dma_block;
};

struct linkedsemi_spi_filter_config {
    mm_reg_t base;
    const struct device *spi;
    const struct gpio_dt_spec cs;
    void (*irq_config_func)(const struct device *dev);
    bool blacklist_en;
    uint8_t index;
    const struct device *dev_dma;
    uint32_t dma_channel;
    uint8_t dma_handshake;
    uint8_t fixed_cmd_tab[SPIF_FIXED_CMD_TABLE_NUM];
    uint8_t fixed_cmd_dummy_tab[SPIF_FIXED_CMD_TABLE_NUM];
    struct spif_log_info *log_info;
    IF_ENABLED(CONFIG_PINCTRL, (const struct pinctrl_dev_config *pcfg;))
    IF_ENABLED(CONFIG_CLOCK_CONTROL, (struct ls_clk_cfg ccfg;))
    IF_ENABLED(CONFIG_RESET, (struct reset_dt_spec reset;))
};

struct linkedsemi_spi_filter_data {
#if defined(CONFIG_SPI_FILTER_ADDR_WHITELIST_BUF)
    uint32_t read_addr_whitelist[SPIF_ADDR_PRIV_REG_NUN];
    uint32_t write_addr_whitelist[SPIF_ADDR_PRIV_REG_NUN];
#endif
    const struct device *dev;
    struct k_sem sem_spif;
    spif_callback_t cb;
    void *user_data;
    struct spif_dma_config spif_dma_config;
    struct k_work dma_work;
};

uint32_t spif_get_ctrl_idx(const struct device *dev)
{
    __ASSERT_NO_MSG(dev);

    const struct linkedsemi_spi_filter_config *dev_config = dev->config;

    return dev_config->index;
}

void spif_get_log_info(const struct device *dev, struct spif_log_info *info)
{
    __ASSERT_NO_MSG(dev);
    __ASSERT_NO_MSG(info);

    const struct linkedsemi_spi_filter_config *dev_config = dev->config;

    memcpy(info, dev_config->log_info, sizeof(struct spif_log_info));

    return;
}

int linkedsemi_spif_register_callback(const struct device *dev, spif_callback_t cb)
{
    __ASSERT_NO_MSG(dev);

    struct linkedsemi_spi_filter_data *dev_data = dev->data;

    dev_data->cb = cb;

    return 0;
}

static void linkedsemi_spi_filter_isr(const struct device *dev)
{
    const struct linkedsemi_spi_filter_config *dev_config = dev->config;
    struct linkedsemi_spi_filter_data *dev_data = dev->data;
    spif_intr_t intr_status;

    intr_status.value = sys_read32(dev_config->base + SPIF_INTR_STT);
    sys_write32(intr_status.value, dev_config->base + SPIF_INTR_CLR);
#if defined(CONFIG_SPI_FILTER_INTERRUPT_LOG)
    if (intr_status.ERROR_OVERFLOW) {
        LOG_ERR("ERROR_OVERFLOW");
    }
    if (intr_status.ERROR) {
        LOG_ERR("ERROR");
    }
    if (intr_status.TARGET_ADDR) {
        uint32_t addr = sys_read32(dev_config->base + SPIF_TARGET_ADDR);
        LOG_ERR("TARGET_ADDR: %#x", addr);
    }
    if (intr_status.SCK_CHECK) {
        spif_sck_fqc_hi_t spif_sck_fqc_hi;
        spif_sck_fqc_lo_t spif_sck_fqc_lo;
        spif_sck_set_t spif_sck_set;
        spif_sck_fqc_hi.value = sys_read32(dev_config->base + SPIF_SCK_FQC_HI);
        spif_sck_fqc_lo.value = sys_read32(dev_config->base + SPIF_SCK_FQC_LO);
        spif_sck_set.value = sys_read32(dev_config->base + SPIF_SCK_SET);
        LOG_ERR("SCK_CHECK: expect [%#x, %#x] but got %#x",
               spif_sck_fqc_lo.SCK_FQC_LO,
               spif_sck_fqc_hi.SCK_FQC_HI,
               spif_sck_set.SCK_FQC);
    }
#endif
    if (dev_data->cb) {
        dev_data->cb(dev);
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
    __ASSERT_NO_MSG(dev);

    const struct linkedsemi_spi_filter_config *dev_config = dev->config;
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
    __ASSERT_NO_MSG(dev);

    const struct linkedsemi_spi_filter_config *dev_config = dev->config;
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
    __ASSERT_NO_MSG(dev);

    const struct linkedsemi_spi_filter_config *dev_config = dev->config;
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
    __ASSERT_NO_MSG(dev);

    const struct linkedsemi_spi_filter_config *dev_config = dev->config;
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
    __ASSERT_NO_MSG(dev);

    const struct linkedsemi_spi_filter_config *dev_config = dev->config;

    acquire_spif_device(dev);

    for (uint32_t i = 0; i < SPIF_CMD_TABLE_NUM; i++) {
        spif_cmd_t spif_cmd;
        spif_cmd.value = sys_read32(dev_config->base + SPIF_CMD_BASE + i * 4);
        if (spif_cmd.value == 0)
            continue;
        if (i < SPIF_FIXED_CMD_TABLE_NUM) {
            LOG_INF("[%s]idx %02d: %-46.46s: 0x%02x: %s", dev->name, i, fix_cmd_desc[i], spif_cmd.CMD, spif_cmd.EN == 1 ? "enabled" : "disabled");
        } else {
            LOG_INF("[%s]idx %02d: %-46.46s: 0x%02x: %s", dev->name, i, general_cmd_desc, spif_cmd.CMD, spif_cmd.EN == 1 ? "enabled" : "disabled");
        }
    }

    release_spif_device(dev);
}

void spif_get_cmd_table(const struct device *dev, uint8_t cmd[SPIF_CMD_TABLE_NUM], uint32_t *cmd_num)
{
    __ASSERT_NO_MSG(dev);
    __ASSERT_NO_MSG(cmd);
    __ASSERT_NO_MSG(cmd_num);

    const struct linkedsemi_spi_filter_config *dev_config = dev->config;
    acquire_spif_device(dev);

    *cmd_num = 0;
    for (uint32_t i = 0; i < SPIF_CMD_TABLE_NUM; i++) {
        spif_cmd_t spif_cmd;
        spif_cmd.value = sys_read32(dev_config->base + SPIF_CMD_BASE + i * 4);
        if (1 == spif_cmd.EN) {
            cmd[i] = spif_cmd.CMD;
            (*cmd_num)++;
        }
    }

    release_spif_device(dev);
}

#if defined(CONFIG_SPI_FILTER_ADDR_WHITELIST_BUF)
static inline int spif_peek_rw_area(const struct device *dev, enum addr_priv_rw_select rw_select, uint32_t addr, uint32_t *data)
{
    __ASSERT_NO_MSG(addr < SPIF_ADDR_PRIV_REG_NUN);

    struct linkedsemi_spi_filter_data *dev_data = dev->data;
    int ret = 0;

    if (rw_select == FLAG_ADDR_PRIV_READ_SELECT) {
        *data = dev_data->read_addr_whitelist[addr];
    } else {
        *data = dev_data->write_addr_whitelist[addr];
    }

    return ret;
}
#else
static int spif_peek_rw_area(const struct device *dev, enum addr_priv_rw_select rw_select, uint32_t addr, uint32_t *data)
{
    __ASSERT_NO_MSG(dev);

    const struct linkedsemi_spi_filter_config *dev_config = dev->config;
    spif_read_addr_req_t spif_read_addr_req = {};
    k_timepoint_t timeout;
    int ret = 0;

    if (rw_select == FLAG_ADDR_PRIV_READ_SELECT) {
        spif_read_addr_req.READ_RADDR_REQ = 1;
        spif_read_addr_req.READ_WADDR_REQ = 0;
    } else {
        spif_read_addr_req.READ_RADDR_REQ = 0;
        spif_read_addr_req.READ_WADDR_REQ = 1;
    }

    sys_write32(addr, dev_config->base + SPIF_READ_SRAM_ADDR);
    sys_write32(spif_read_addr_req.value, dev_config->base + SPIF_READ_ADDR_REQ);
    timeout = sys_timepoint_calc(K_MSEC(100));
    while (sys_read32(dev_config->base + SPIF_READ_ADDR_REQ)) { /* wait for cs line idle */
        if (sys_timepoint_expired(timeout)) {
            LOG_ERR( "cs line busy");
            ret = -EIO;
            goto err;
        }
    }
    *data = sys_read32(dev_config->base + SPIF_READ_SRAM_DATA);

err:
    return ret;
}
#endif

static int spif_protect_area_parser(const struct device *dev,
                                       struct priv_reg_info start,
                                       struct priv_reg_info *res,
                                       uint32_t *num_protect_blk,
                                       enum addr_priv_rw_select region)
{
    __ASSERT_NO_MSG(dev);

    uint32_t reg_off = start.start_reg_off;
    uint32_t bit_off = start.start_bit_off;
    uint32_t reg_val;
    uint32_t i;
    int ret = 0;

    /* init search result */
    *num_protect_blk = 0;

    while (reg_off < SPIF_ADDR_PRIV_REG_NUN) {
        ret = spif_peek_rw_area(dev, region, reg_off, &reg_val);
        if (ret) {
            goto end;
        }
        reg_val >>= bit_off;
        for (i = bit_off; i < 32; i++) {
            if ((reg_val & 1) == 0) {
                if (*num_protect_blk == 0) {
                    /* get the first protect block */
                    res->start_reg_off = reg_off;
                    res->start_bit_off = i;
                }

                (*num_protect_blk)++;
            } else if ((reg_val & 1) == 1 && *num_protect_blk != 0) {
                res->end_reg_off = reg_off;
                res->end_bit_off = i;
                goto end;
            }

            reg_val >>= 1;
        }

        bit_off = 0;
        reg_off++;
    }

    res->end_reg_off = SPIF_ADDR_PRIV_REG_NUN - 1;
    res->end_bit_off = 32;

end:
    return ret;
}

void spif_dump_cmd_bitmap_log(const struct device *dev, uint8_t bitmap[SPIF_CMD_BITMAP_LOG_SIZE_BYTE])
{
    __ASSERT_NO_MSG(dev);

    const struct linkedsemi_spi_filter_config *dev_config = dev->config;

    for (uint32_t i = 0; i < SPI_CMD_BITMAPF_LOG_SIZE_U32; i++) {
        uint32_t val = sys_read32(dev_config->base + SPIF_BIT_MAP0 - (i * 4));
        UNALIGNED_PUT(val, (uint32_t *)(bitmap + (i * 4)));
    }
}

void spif_clear_cmd_bitmap_log(const struct device *dev)
{
    __ASSERT_NO_MSG(dev);

    const struct linkedsemi_spi_filter_config *dev_config = dev->config;

    spif_intr_clr_t spif_intr_clr = { .BITMAP = 1 };
    sys_write32(spif_intr_clr.value, dev_config->base + SPIF_INTR_CLR);
}

int spif_dump_rw_addr_privilege_table(const struct device *dev)
{
    __ASSERT_NO_MSG(dev);

    uint32_t num_protect_blk = 0;
    struct priv_reg_info start;
    struct priv_reg_info res;
    bool protect_en = false;
    uint32_t rw;
    int ret = 0;

    acquire_spif_device(dev);

    for (rw = 0; rw < 2; rw++) {
        memset(&start, 0x0, sizeof(struct priv_reg_info));
        memset(&res, 0x0, sizeof(struct priv_reg_info));
        LOG_INF("%s protect regions:", rw == 0 ? "read" : "write");
        do {
            spif_protect_area_parser(dev, start, &res, &num_protect_blk, rw);
            if (ret) {
                goto end;
            }
            if (num_protect_blk != 0) {
                protect_en = true;
                LOG_INF("[0x%08x - 0x%08x]",
                       SPIF_ABS_ADDR(res.start_reg_off, res.start_bit_off),
                       SPIF_ABS_ADDR(res.end_reg_off, res.end_bit_off));
                start.start_reg_off = res.end_reg_off;
                start.start_bit_off = res.end_bit_off;
            }
        } while (num_protect_blk != 0);

        if (!protect_en) {
            LOG_INF("all regions are free!");
        }
        LOG_INF("======END======");
    }

end:
    release_spif_device(dev);
    return ret;
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
    __ASSERT_NO_MSG(dev);

    const struct linkedsemi_spi_filter_config *dev_config = dev->config;
    struct linkedsemi_spi_filter_data *dev_data = dev->data;
    mm_reg_t priv_table_base;
    uint32_t *addr_whitelist;
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
        LOG_WRN("stricter protect regions will be applied. (force 16KB aligned)");
        /* protect more region in order to align 16KB boundary */
        len = addr + len - (addr / KB(16)) * KB(16);
        addr = (addr / KB(16)) * KB(16);
        len = ((len + KB(16) - 1) / KB(16)) * KB(16);
    }

    reg_off = addr / KB(512);            /* 512K per register; */
    bit_off = (addr % KB(512)) / KB(16); /* (512K / 16K); */
    total_bit_num = spif_get_cross_block_num(addr, len);
    LOG_DBG("addr: 0x%08lx, len: 0x%08x", addr, len);
    LOG_DBG("reg_off: 0x%08x, bit_off: 0x%08x, total_bit_num: 0x%08x",
            reg_off,
            bit_off,
            total_bit_num);

    acquire_spif_device(dev);

    if (rw_select == FLAG_ADDR_PRIV_READ_SELECT) {
        priv_table_base = dev_config->base + SPIF_READ_ADDR_VALID_EN_ADDR;
        addr_whitelist = dev_data->read_addr_whitelist;
    } else {
        priv_table_base = dev_config->base + SPIF_WRITE_ADDR_VALID_EN_ADDR;
        addr_whitelist = dev_data->write_addr_whitelist;
    }

    do {
        if (bit_off > 31) {
            bit_off = 0;
            reg_off++;
        }

        if (bit_off == 0 && total_bit_num >= 32) {
            /* speed up for large area configuration */
            if (priv_op == FLAG_ADDR_PRIV_ENABLE) {
                addr_whitelist[reg_off] = 0xffffffff;
                sys_write32(0xffffffff, priv_table_base + reg_off * 4);
            } else {
                addr_whitelist[reg_off] = 0x0;
                sys_write32(0x0, priv_table_base + reg_off * 4);
            }

            reg_off++;
            total_bit_num -= 32;
        } else {
            ret = spif_peek_rw_area(dev, rw_select, reg_off, &reg_val);
            if (ret) {
                goto end;
            }
            if (priv_op == FLAG_ADDR_PRIV_ENABLE) {
                reg_val |= BIT(bit_off);
            } else {
                reg_val &= ~BIT(bit_off);
            }
            addr_whitelist[reg_off] = reg_val;
            sys_write32(reg_val, priv_table_base + reg_off * 4);
            LOG_DBG("reg: 0x%08lx, val: 0x%08x", priv_table_base + reg_off * 4, reg_val);

            bit_off++;
            total_bit_num--;
        }
    } while (total_bit_num > 0);

end:
    release_spif_device(dev);

    return ret;
}

void spif_memset_read_addr_whitelist(const struct device *dev, uint8_t num)
{
    __ASSERT_NO_MSG(dev);

    const struct linkedsemi_spi_filter_config *dev_config = dev->config;
    struct linkedsemi_spi_filter_data *dev_data = dev->data;

    if (num == 0) {
        for (uint32_t off = 0; off < SPIF_ADDR_SIZE; off += 4) {
            memset(dev_data->read_addr_whitelist, 0, SPIF_ADDR_PRIV_REG_NUN * sizeof(uint32_t));
            sys_write32(0, dev_config->base + SPIF_READ_ADDR_VALID_EN_ADDR + off);  /* memset READ_ADDR */
        }
    } else if (num == 1) {
        for (uint32_t off = 0; off < SPIF_ADDR_SIZE; off += 4) {
            memset(dev_data->read_addr_whitelist, 0xff, SPIF_ADDR_PRIV_REG_NUN * sizeof(uint32_t));
            sys_write32(0xffffffff, dev_config->base + SPIF_READ_ADDR_VALID_EN_ADDR + off);  /* memset READ_ADDR */
        }
    } else {
        LOG_ERR("num should be 0 or 1");
    }
}

void spif_memset_write_addr_whitelist(const struct device *dev, uint8_t num)
{
    __ASSERT_NO_MSG(dev);

    const struct linkedsemi_spi_filter_config *dev_config = dev->config;
    struct linkedsemi_spi_filter_data *dev_data = dev->data;

    if (num == 0) {
        for (uint32_t off = 0; off < SPIF_ADDR_SIZE; off += 4) {
            memset(dev_data->write_addr_whitelist, 0, SPIF_ADDR_PRIV_REG_NUN * sizeof(uint32_t));
            sys_write32(0, dev_config->base + SPIF_WRITE_ADDR_VALID_EN_ADDR + off); /* memset WRITE_ADDR */
        }
    } else if (num == 1) {
        for (uint32_t off = 0; off < SPIF_ADDR_SIZE; off += 4) {
            memset(dev_data->write_addr_whitelist, 0xff, SPIF_ADDR_PRIV_REG_NUN * sizeof(uint32_t));
            sys_write32(0xffffffff, dev_config->base + SPIF_WRITE_ADDR_VALID_EN_ADDR + off); /* memset WRITE_ADDR */
        }
    } else {
        LOG_ERR("num should be 0 or 1");
    }
}

void spif_memset_addr_whitelist(const struct device *dev, uint8_t num)
{
    __ASSERT_NO_MSG(dev);

    if (num == 0) {
        spif_memset_read_addr_whitelist(dev, 0);
        spif_memset_write_addr_whitelist(dev, 0);
    } else if (num == 1) {
        spif_memset_read_addr_whitelist(dev, 1);
        spif_memset_write_addr_whitelist(dev, 1);
    } else {
        LOG_ERR("num should be 0 or 1");
    }
}

int spif_add_general_cmd(const struct device *dev, uint8_t cmd)
{
    __ASSERT_NO_MSG(dev);

    const struct linkedsemi_spi_filter_config *dev_config = dev->config;
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

int spif_add_cmd(const struct device *dev, uint8_t cmd)
{
    __ASSERT_NO_MSG(dev);

    const struct linkedsemi_spi_filter_config *dev_config = dev->config;
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
        if (dev_config->fixed_cmd_tab[off] == cmd) {
            spif_cmd_t spif_cmd;
            spif_cmd.CMD = cmd;
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

int spif_add_cmd_with_dummy(const struct device *dev, uint8_t cmd, uint8_t dummy_cycle)
{
    __ASSERT_NO_MSG(dev);

    const struct linkedsemi_spi_filter_config *dev_config = dev->config;
    int ret = 0;
    mm_reg_t table_base = dev_config->base + SPIF_CMD_BASE;
    int idx;

    acquire_spif_device(dev);

    for (uint8_t off = 0; off < SPIF_CMD_TABLE_NUM; off++) {
        idx = spif_get_cmd_slot(dev, cmd, off);
        if (idx >= 0) {
            spif_cmd_t spif_cmd;
            spif_cmd.value = sys_read32(table_base + idx * 4);
            spif_cmd.DUMMY_CYCLE = dummy_cycle;
            spif_cmd.EN = 1;
            sys_write32(spif_cmd.value, table_base + idx * 4);
            goto end;
        } else {
            break;
        }
    }

    for (uint8_t off = 0; off < SPIF_FIXED_CMD_TABLE_NUM; off++) {
        if (dev_config->fixed_cmd_tab[off] == cmd) {
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

void spif_set_cmd_by_idx(const struct device *dev, uint8_t cmd, uint8_t idx)
{
    __ASSERT_NO_MSG(dev);

    const struct linkedsemi_spi_filter_config *dev_config = dev->config;
    mm_reg_t table_base = dev_config->base + SPIF_CMD_BASE;

    acquire_spif_device(dev);

    spif_cmd_t spif_cmd;
    spif_cmd.value = sys_read32(table_base + idx * 4);
    spif_cmd.CMD = cmd;
    sys_write32(spif_cmd.value, table_base + idx * 4);

    release_spif_device(dev);

    return;
}

void spif_set_dummy_by_idx(const struct device *dev, uint8_t dummy_cycle, uint8_t idx)
{
    __ASSERT_NO_MSG(dev);

    const struct linkedsemi_spi_filter_config *dev_config = dev->config;
    mm_reg_t table_base = dev_config->base + SPIF_CMD_BASE;

    acquire_spif_device(dev);

    spif_cmd_t spif_cmd;
    spif_cmd.value = sys_read32(table_base + idx * 4);
    spif_cmd.DUMMY_CYCLE = dummy_cycle;
    sys_write32(spif_cmd.value, table_base + idx * 4);

    release_spif_device(dev);

    return;
}

void spif_get_cmd_by_idx(const struct device *dev, uint8_t *cmd, uint8_t idx)
{
    __ASSERT_NO_MSG(dev);

    const struct linkedsemi_spi_filter_config *dev_config = dev->config;
    mm_reg_t table_base = dev_config->base + SPIF_CMD_BASE;

    acquire_spif_device(dev);

    spif_cmd_t spif_cmd;
    spif_cmd.value = sys_read32(table_base + idx * 4);
    *cmd = spif_cmd.CMD;

    release_spif_device(dev);

    return;
}

void spif_get_dummy_by_idx(const struct device *dev, uint8_t *dummy_cycle, uint8_t idx)
{
    __ASSERT_NO_MSG(dev);

    const struct linkedsemi_spi_filter_config *dev_config = dev->config;
    mm_reg_t table_base = dev_config->base + SPIF_CMD_BASE;

    acquire_spif_device(dev);

    spif_cmd_t spif_cmd;
    spif_cmd.value = sys_read32(table_base + idx * 4);
    *dummy_cycle = spif_cmd.DUMMY_CYCLE;

    release_spif_device(dev);

    return;
}

int spif_remove_general_cmd(const struct device *dev, uint8_t cmd)
{
    __ASSERT_NO_MSG(dev);

    const struct linkedsemi_spi_filter_config *dev_config = dev->config;
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
    __ASSERT_NO_MSG(dev);

    const struct linkedsemi_spi_filter_config *dev_config = dev->config;
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

void spif_remove_cmd_by_idx(const struct device *dev, uint8_t cmd, uint8_t idx)
{
    __ASSERT_NO_MSG(dev);

    const struct linkedsemi_spi_filter_config *dev_config = dev->config;
    mm_reg_t table_base = dev_config->base + SPIF_CMD_BASE;

    acquire_spif_device(dev);

    spif_cmd_t spif_cmd;
    spif_cmd.value = sys_read32(table_base + idx * 4);
    spif_cmd.EN = 0;
    sys_write32(spif_cmd.value, table_base + idx * 4);

    release_spif_device(dev);

    return;
}

void spif_filter_enable(const struct device *dev, bool enable)
{
    __ASSERT_NO_MSG(dev);

    const struct linkedsemi_spi_filter_config *dev_config = dev->config;

    acquire_spif_device(dev);

    spif_cfg_t spif_cfg;
    spif_cfg.value = sys_read32(dev_config->base + SPIF_CFG);
    spif_cfg.EN = enable ? 1 : 0;
    sys_write32(spif_cfg.value, dev_config->base + SPIF_CFG);

    release_spif_device(dev);
}

void spif_reg_unlock(const struct device *dev)
{
    __ASSERT_NO_MSG(dev);

    const struct linkedsemi_spi_filter_config *dev_config = dev->config;

    spif_cfg_t spif_cfg = { .IP_LOCK = 1, };
    sys_write32(spif_cfg.value, dev_config->base + SPIF_CFG);
}

void spif_reg_lock(const struct device *dev)
{
    __ASSERT_NO_MSG(dev);

    const struct linkedsemi_spi_filter_config *dev_config = dev->config;

    spif_cfg_t spif_cfg = { .IP_LOCK = 0, };
    sys_write32(spif_cfg.value, dev_config->base + SPIF_CFG);
}

void spif_clk_check_config(const struct device *dev,
                           uint8_t div,
                           uint16_t threshold_high_cycle,
                           uint16_t threshold_low_cycle,
                           bool enable_intr)
{
    __ASSERT_NO_MSG(dev);

    __ASSERT_NO_MSG(threshold_high_cycle < threshold_low_cycle);
    const struct linkedsemi_spi_filter_config *dev_config = dev->config;
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
    __ASSERT_NO_MSG(dev);

    const struct linkedsemi_spi_filter_config *dev_config = dev->config;
    spif_sck_set_t spif_sck_set;
    spif_sck_set.value = sys_read32(dev_config->base + SPIF_SCK_SET);

    return spif_sck_set.SCK_FQC;
}

spif_dma_data_t *spif_log_dma_buf(const struct device *dev)
{
    __ASSERT_NO_MSG(dev);

    const struct linkedsemi_spi_filter_config *dev_config = dev->config;

    return (spif_dma_data_t *)dev_config->log_info->log_ram_addr;
}

#if defined(CONFIG_SPI_FILTER_DMA_LOG)
static void spif_dma_data_print(spif_dma_data_t spif_dma_data)
{
    LOG_ERR("ADDR_ERR: %#x "
            "CMD_ERR: %#x "
            "POR_ADDR: %#x "
            "ERROR_ADDR: %#8.8x "
            "ERROR_CMD: %#2.2x",
            spif_dma_data.ADDR_ERR,
            spif_dma_data.CMD_ERR,
            spif_dma_data.POR_ADDR,
            spif_dma_data.ERROR_ADDR << 11,
            spif_dma_data.ERROR_CMD);
}
#endif

static void spif_dma_callback(const struct device *dev_dma, void *callback_arg,
                 uint32_t channel, int status)
{
    const struct device *dev = (const struct device *)callback_arg;
    struct linkedsemi_spi_filter_data *dev_data = dev->data;

    if(status == DMA_STATUS_TRIGGER) {
        if (!k_work_busy_get(&dev_data->dma_work)) {
            k_work_submit(&dev_data->dma_work);
        }
        LOG_DBG("DMA_STATUS_TRIGGER");
    }
    if (status == DMA_STATUS_BLOCK) {
        LOG_DBG("DMA_STATUS_BLOCK");
    }
    if (status == DMA_STATUS_COMPLETE) {
        LOG_DBG("DMA_STATUS_COMPLETE");
    }
}

int spif_dma_start(const struct device *dev)
{
    __ASSERT_NO_MSG(dev);

    const struct linkedsemi_spi_filter_config *dev_config = dev->config;
    struct linkedsemi_spi_filter_data *dev_data = dev->data;

    memset((void *)dev_config->log_info->log_ram_addr, 0, SPIF_LOG_RAM_MAX_SIZE_U32 * sizeof(uint32_t));

    spif_cfg_t spif_cfg;
    spif_cfg.value = sys_read32(dev_config->base + SPIF_CFG);
    spif_cfg.DMA_EN = 1;
    sys_write32(spif_cfg.value, dev_config->base + SPIF_CFG);

    dev_data->spif_dma_config.dma_block.block_size = SPIF_LOG_RAM_MAX_SIZE_U32;
    dev_data->spif_dma_config.dma_block.source_address = dev_config->base + SPIF_DMA_DATA;
    dev_data->spif_dma_config.dma_block.dest_address = (uint32_t)dev_config->log_info->log_ram_addr;

    dev_data->spif_dma_config.dma_cfg.block_count = 1;
    dev_data->spif_dma_config.dma_cfg.dma_slot = dev_config->dma_handshake;
    dev_data->spif_dma_config.dma_cfg.channel_direction = PERIPHERAL_TO_MEMORY;
    dev_data->spif_dma_config.dma_cfg.source_burst_length = 0;
    dev_data->spif_dma_config.dma_cfg.dest_burst_length = 0;
    dev_data->spif_dma_config.dma_cfg.channel_priority = 0;
    dev_data->spif_dma_config.dma_cfg.complete_callback_en = 1;
    dev_data->spif_dma_config.dma_cfg.dma_callback = spif_dma_callback;
    dev_data->spif_dma_config.dma_cfg.user_data = (void *)dev;
    dev_data->spif_dma_config.dma_cfg.source_data_size = 4;
    dev_data->spif_dma_config.dma_cfg.dest_data_size = 4;
    dev_data->spif_dma_config.dma_cfg.head_block = &(dev_data->spif_dma_config.dma_block);

    if (dev_config->dev_dma == NULL || !device_is_ready(dev_config->dev_dma)) {
        LOG_ERR("dma binding fail");
        return -EINVAL;
    }

    dma_config(dev_config->dev_dma, dev_config->dma_channel, &dev_data->spif_dma_config.dma_cfg);
    dma_start(dev_config->dev_dma, dev_config->dma_channel);

    return 0;
}

void spif_target_addr_config(const struct device *dev, uint32_t addr, enum target_addr_mode mode, bool enable_intr)
{
    __ASSERT_NO_MSG(dev);

    const struct linkedsemi_spi_filter_config *dev_config = dev->config;

    sys_write32(addr, dev_config->base + SPIF_TARGET_ADDR);

    spif_cfg_t spif_cfg;
    spif_cfg.value = sys_read32(dev_config->base + SPIF_CFG);
    spif_cfg.TARGET_ADDR_MODE_SEL = mode,
    sys_write32(spif_cfg.value, dev_config->base + SPIF_CFG);

    spif_intr_t intr_mask;
    intr_mask.value = sys_read32(dev_config->base + SPIF_INTR_MASK);
    intr_mask.TARGET_ADDR = enable_intr;
    sys_write32(intr_mask.value, dev_config->base + SPIF_INTR_MASK);
}

void spif_3byte_mode_config(const struct device *dev)
{
    __ASSERT_NO_MSG(dev);

    const struct linkedsemi_spi_filter_config *dev_config = dev->config;

    spif_cfg_t spif_cfg;
    spif_cfg.value = sys_read32(dev_config->base + SPIF_CFG);
    spif_cfg.ADDR_3B_4B_SEL = 0,
    sys_write32(spif_cfg.value, dev_config->base + SPIF_CFG);
}

void spif_4byte_mode_config(const struct device *dev)
{
    __ASSERT_NO_MSG(dev);

    const struct linkedsemi_spi_filter_config *dev_config = dev->config;

    spif_cfg_t spif_cfg;
    spif_cfg.value = sys_read32(dev_config->base + SPIF_CFG);
    spif_cfg.ADDR_3B_4B_SEL = 1,
    sys_write32(spif_cfg.value, dev_config->base + SPIF_CFG);
}

uint8_t spif_addr_mode_peek(const struct device *dev)
{
    __ASSERT_NO_MSG(dev);

    const struct linkedsemi_spi_filter_config *dev_config = dev->config;

    spif_cfg_t spif_cfg;
    spif_cfg.value = sys_read32(dev_config->base + SPIF_CFG);
    return spif_cfg.ADDR_3B_4B_FLAG;
}

void spif_operation_mode_config(const struct device *dev, bool filter_en)
{
    __ASSERT_NO_MSG(dev);

    const struct linkedsemi_spi_filter_config *dev_config = dev->config;

    spif_cfg_t spif_cfg;
    spif_cfg.value = sys_read32(dev_config->base + SPIF_CFG);
    spif_cfg.OPERATION_MODE = filter_en;
    sys_write32(spif_cfg.value, dev_config->base + SPIF_CFG);
}

const struct device *spif_spi_dev(const struct device *dev)
{
    __ASSERT_NO_MSG(dev);

    const struct linkedsemi_spi_filter_config *dev_config = dev->config;

    return dev_config->spi;
}

const struct gpio_dt_spec *spif_spi_cs(const struct device *dev)
{
    __ASSERT_NO_MSG(dev);

    const struct linkedsemi_spi_filter_config *dev_config = dev->config;

    return &dev_config->cs;
}

static bool spif_pinctrl_mode_check(const struct device *dev, uint8_t mode)
{
    const struct linkedsemi_spi_filter_config *dev_config = dev->config;
    const struct pinctrl_state *state;
    int ret = 0;
    bool check = true;

    __ASSERT_NO_MSG((PINCTRL_STATE_DEFAULT == mode)
                    || (PINCTRL_STATE_PASSTHROUGH == mode)
                    || (PINCTRL_STATE_MASTER == mode));

    ret = pinctrl_lookup_state(dev_config->pcfg, mode, &state);
    if (ret) {
        __ASSERT_NO_MSG(0);
    }

    for(int i = 0; i < state->pin_cnt; i++) {
        const pinctrl_soc_pin_t *pins = &state->pins[i];
        const uint8_t pin = pins->pinmux.pin;
        const uint8_t func = per_func_get(pin);
        bool func_valid = is_per_func_valid(func);
        if ((func_valid != pins->pinmux.func_valid)
            || ((pins->pinmux.func_valid) && (func != pins->pinmux.func))) {
            check = false;
            break;
        }
    }

    return check;
}

bool spif_pinctrl_filter_mode_check(const struct device *dev)
{
    __ASSERT_NO_MSG(dev);
    return spif_pinctrl_mode_check(dev, PINCTRL_STATE_DEFAULT);
}

bool spif_pinctrl_passthrough_mode_check(const struct device *dev)
{
    __ASSERT_NO_MSG(dev);
    return spif_pinctrl_mode_check(dev, PINCTRL_STATE_PASSTHROUGH);
}

bool spif_pinctrl_master_mode_check(const struct device *dev)
{
    __ASSERT_NO_MSG(dev);
    return spif_pinctrl_mode_check(dev, PINCTRL_STATE_MASTER);
}

/*
disable master function
disable anglog function
*/
int spif_switch_to_filter(const struct device *dev)
{
    __ASSERT_NO_MSG(dev);

    const struct linkedsemi_spi_filter_config *dev_config = dev->config;

    pinctrl_apply_state(dev_config->pcfg, PINCTRL_STATE_DEFAULT);
    spif_enable(dev, true);

    return 0;
}

struct spif_spi_check_dev_name {
    char *dev;
    char *check_dev;
};

static const struct spif_spi_check_dev_name spif_spi_check_dev_name_arr[] = {
    { .dev = "spif@40070000", .check_dev = "spif@40072000"},
    { .dev = "spif@40072000", .check_dev = "spif@40070000"},
    { .dev = "spif@40074000", .check_dev = "spif@40076000"},
    { .dev = "spif@40076000", .check_dev = "spif@40074000"},
};

/*
check whether another filter out pin is master function
*/
int spif_switch_to_master_handle(const struct device *dev, bool force)
{
    __ASSERT_NO_MSG(dev);

    const struct linkedsemi_spi_filter_config *dev_config = dev->config;
    int ret = 0;

    const struct device *spif_out_check_dev = NULL;
    for (int i = 0; i < ARRAY_SIZE(spif_spi_check_dev_name_arr); i++) {
        if (0 == strcmp(dev->name, spif_spi_check_dev_name_arr[i].dev)) {
            spif_out_check_dev = device_get_binding(spif_spi_check_dev_name_arr[i].check_dev);
            break;
        }
    }
    if (spif_out_check_dev) {
        if (spif_pinctrl_master_mode_check(spif_out_check_dev)) {
            if (force) {
                LOG_INF("another spi filter out pin is master function, force switch to master mode");
                spif_switch_to_filter(spif_out_check_dev);
            } else {
                LOG_ERR("another spi filter out pin is master function, cannot switch to master mode");
                ret = -EBUSY;
                goto err;
            }
        }
    } else {
        __ASSERT_NO_MSG(0);
    }

    pinctrl_apply_state(dev_config->pcfg, PINCTRL_STATE_MASTER);

err:
    return ret;
}

int spif_switch_to_master(const struct device *dev)
{
    spif_enable(dev, false);
    return spif_switch_to_master_handle(dev, false);
}

int spif_switch_to_master_force(const struct device *dev)
{
    __ASSERT_NO_MSG(dev);
    return spif_switch_to_master_handle(dev, true);
}

int spif_passthrough_analog_mux_enable(const struct device *dev, bool enable)
{
    __ASSERT_NO_MSG(dev);

    const struct linkedsemi_spi_filter_config *dev_config = dev->config;

    pinctrl_apply_state(dev_config->pcfg, PINCTRL_STATE_PASSTHROUGH);

    return 0;
}

static void spif_dma_work(struct k_work *work)
{
    struct linkedsemi_spi_filter_data *dev_data = CONTAINER_OF(work, struct linkedsemi_spi_filter_data, dma_work);
    const struct device *dev = dev_data->dev;
    const struct linkedsemi_spi_filter_config *dev_config = dev->config;

    while (1) {
        struct dma_status stat;
        dma_get_status(dev_config->dev_dma, dev_config->dma_channel, &stat);
        uint16_t block_ts = stat.pending_length >> 2;
        if (dev_config->log_info->log_idx < block_ts) {
            for (uint16_t i = dev_config->log_info->log_idx; i < block_ts; i++) {
                spif_dma_data_t *spif_dma_data = (spif_dma_data_t *)dev_config->log_info->log_ram_addr;
                LOG_DBG("dma log idx: %d", i);
#if defined(CONFIG_SPI_FILTER_DMA_LOG)
                spif_dma_data_print(spif_dma_data[i]);
#endif
            }
            dev_config->log_info->log_idx = block_ts;
        } else if ((dev_config->log_info->log_idx == block_ts) && (SPIF_LOG_RAM_MAX_SIZE_U32 != block_ts)) {
            break;
        }

        if (SPIF_LOG_RAM_MAX_SIZE_U32 == block_ts) {
            dev_config->log_info->log_idx = 0;
            dma_suspend(dev_config->dev_dma, dev_config->dma_channel);
            dma_stop(dev_config->dev_dma, dev_config->dma_channel);
            spif_dma_start(dev);
            break;
        }
    }
}

void spif_enable(const struct device *dev, bool enable)
{
    __ASSERT_NO_MSG(dev);

    const struct linkedsemi_spi_filter_config *dev_config = dev->config;

    spif_cfg_t spif_cfg;
    spif_cfg.value = sys_read32(dev_config->base + SPIF_CFG);
    spif_cfg.EN = enable ? 1 : 0;
    sys_write32(spif_cfg.value, dev_config->base + SPIF_CFG);
}

int linkedsemi_spi_filter_cold_reset(const struct device *dev)
{
    __ASSERT_NO_MSG(dev);

    const struct linkedsemi_spi_filter_config *dev_config = dev->config;
    struct linkedsemi_spi_filter_data *dev_data = dev->data;
    int ret;

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

    spif_reg_unlock(dev);

    spif_memset_addr_whitelist(dev, 1); /* default */
    for (uint8_t i = 0; i < SPIF_FIXED_CMD_TABLE_NUM; i++) {
        mm_reg_t table_base = dev_config->base + SPIF_CMD_BASE;
        spif_cmd_t spif_cmd;
        spif_cmd.CMD = dev_config->fixed_cmd_tab[i];
        spif_cmd.DUMMY_CYCLE = dev_config->fixed_cmd_dummy_tab[i];
        spif_cmd.EN = 0;
        sys_write32(spif_cmd.value, table_base + i * 4); /* init fixed table */
    }
    sys_write32(0xffffffff, dev_config->base + SPIF_TARGET_ADDR);
    sys_write32(0x7, dev_config->base + SPIF_BCMD_RANGE);
    spif_cfg_t spif_cfg = {
        .EN = 0,
        .OPERATION_MODE = 0, /* default: monitor */
        .ADDR_3B_4B_SEL = 0,
        .TARGET_ADDR_MODE_SEL = 0,
        .BOW_CMD_SEL = dev_config->blacklist_en ? 1 : 0,
        .IP_LOCK = 1,
        .DMA_EN = 0,
    };
    sys_write32(spif_cfg.value, dev_config->base + SPIF_CFG);
    spif_clk_check_config(dev, 0, 2, BIT(12) - 1, false);
    spif_intr_t intr_mask;
    intr_mask.value = sys_read32(dev_config->base + SPIF_INTR_MASK);
    intr_mask.ERROR_OVERFLOW = 1;
    intr_mask.ERROR = 1;
    intr_mask.TARGET_ADDR = 0;
    sys_write32(intr_mask.value, dev_config->base + SPIF_INTR_MASK);

    k_work_init(&dev_data->dma_work, spif_dma_work);

    return 0;
}

static int linkedsemi_spi_filter_init(const struct device *dev)
{
    __ASSERT_NO_MSG(dev);

    const struct linkedsemi_spi_filter_config *dev_config = dev->config;
    struct linkedsemi_spi_filter_data *dev_data = dev->data;

    dev_data->dev = dev;

    if (IS_ENABLED(CONFIG_MULTITHREADING))
        k_sem_init(&dev_data->sem_spif, 1, 1);

    dev_config->irq_config_func(dev);

    return 0;
}

#define SPI_FILTER_INIT(inst)                                                                                                                    \
    static void linkedsemi_spi_filter_irq_config_func_##inst(const struct device *dev)                                                           \
    {                                                                                                                                            \
        ARG_UNUSED(dev);                                                                                                                         \
        IRQ_CONNECT(DT_INST_IRQN(inst),                                                                                                          \
                    DT_INST_IRQ(inst, priority),                                                                                                 \
                    linkedsemi_spi_filter_isr,                                                                                                   \
                    DEVICE_DT_INST_GET(inst),                                                                                                    \
                    0);                                                                                                                          \
        irq_enable(DT_INST_IRQN(inst));                                                                                                          \
    }                                                                                                                                            \
    PINCTRL_DT_INST_DEFINE(inst);                                                                                                                \
    __nocache uint32_t log_ram_##inst[SPIF_LOG_RAM_MAX_SIZE_U32];                                                                                \
    struct spif_log_info log_info_##inst = {                                                                                                     \
       .log_ram_addr = (uint32_t)log_ram_##inst,                                                                                                 \
       .log_max_sz = SPIF_LOG_RAM_MAX_SIZE_U32 * sizeof(uint32_t),                                                                               \
       .log_idx = 0,                                                                                                                             \
    };                                                                                                                                           \
    static const struct linkedsemi_spi_filter_config linkedsemi_spi_filter_cfg_##inst = {                                                        \
        .base = (mm_reg_t)DT_INST_REG_ADDR(inst),                                                                                                \
        .spi = DEVICE_DT_GET(DT_INST_PHANDLE(inst, spi)),                                                                                        \
        .cs = GPIO_DT_SPEC_INST_GET(inst, cs_gpios),                                                                                             \
        .irq_config_func = linkedsemi_spi_filter_irq_config_func_##inst,                                                                         \
        .index = DT_INST_PROP(inst, index),                                                                                                      \
        .fixed_cmd_tab = {                                                                                                                       \
            [IDX_CMD_PAGE_PROGRAM] = DT_INST_PROP_OR(inst, cmd_page_program, 0),                                                                 \
            [IDX_CMD_PAGE_PROGRAM_QUAD_ADDR_QUAD_DATA] = DT_INST_PROP_OR(inst, cmd_page_program_quad_addr_quad_data, 0),                         \
            [IDX_CMD_ERASE_4KB] = DT_INST_PROP_OR(inst, cmd_erase_4kb, 0),                                                                       \
            [IDX_CMD_ERASE_32KB] = DT_INST_PROP_OR(inst, cmd_erase_32kb, 0),                                                                     \
            [IDX_CMD_ERASE_64KB] = DT_INST_PROP_OR(inst, cmd_erase_64kb, 0),                                                                     \
            [IDX_CMD_READ] = DT_INST_PROP_OR(inst, cmd_read, 0),                                                                                 \
            [IDX_CMD_FAST_READ] = DT_INST_PROP_OR(inst, cmd_fast_read, 0),                                                                       \
            [IDX_CMD_READ_QUAD_DATA] = DT_INST_PROP_OR(inst, cmd_read_quad_data, 0),                                                             \
            [IDX_CMD_READ_QUAD_ADDR_QUAD_DATA] = DT_INST_PROP_OR(inst, cmd_read_quad_addr_quad_data, 0),                                         \
            [IDX_CMD_QUAD_SPI_MODE_ENTER] = DT_INST_PROP_OR(inst, cmd_quad_spi_mode_enter, 0),                                                   \
            [IDX_CMD_QUAD_SPI_MODE_EXIT] = DT_INST_PROP_OR(inst, cmd_quad_spi_mode_exit, 0),                                                     \
            [IDX_CMD_4BYTE_MODE_ENTER] = DT_INST_PROP_OR(inst, cmd_4byte_mode_enter, 0),                                                         \
            [IDX_CMD_4BYTE_MODE_EXIT] = DT_INST_PROP_OR(inst, cmd_4byte_mode_exit, 0),                                                           \
            [IDX_CMD_4BYTE_READ_EXTENDED_ADDR] = DT_INST_PROP_OR(inst, cmd_4byte_read_extended_addr, 0),                                         \
            [IDX_CMD_4BYTE_WRITE_EXTENDED_ADDR] = DT_INST_PROP_OR(inst, cmd_4byte_write_extended_addr, 0),                                       \
            [IDX_CMD_4BYTE_PAGE_PROGRAM] = DT_INST_PROP_OR(inst, cmd_4byte_page_program, 0),                                                     \
            [IDX_CMD_4BYTE_PAGE_PROGRAM_QUAD_ADDR_QUAD_DATA] = DT_INST_PROP_OR(inst, cmd_4byte_page_program_quad_addr_quad_data, 0),             \
            [IDX_CMD_4BYTE_ERASE_4KB] = DT_INST_PROP_OR(inst, cmd_4byte_erase_4kb, 0),                                                           \
            [IDX_CMD_4BYTE_ERASE_32KB] = DT_INST_PROP_OR(inst, cmd_4byte_erase_32kb, 0),                                                         \
            [IDX_CMD_4BYTE_ERASE_64KB] = DT_INST_PROP_OR(inst, cmd_4byte_erase_64kb, 0),                                                         \
            [IDX_CMD_4BYTE_READ] = DT_INST_PROP_OR(inst, cmd_4byte_read, 0),                                                                     \
            [IDX_CMD_4BYTE_FAST_READ] = DT_INST_PROP_OR(inst, cmd_4byte_fast_read, 0),                                                           \
            [IDX_CMD_4BYTE_READ_QUAD_DATA] = DT_INST_PROP_OR(inst, cmd_4byte_read_quad_data, 0),                                                 \
            [IDX_CMD_4BYTE_READ_QUAD_ADDR_QUAD_DATA] = DT_INST_PROP_OR(inst, cmd_4byte_read_quad_addr_quad_data, 0),                             \
            [IDX_CMD_READ_DUAL_DATA] = DT_INST_PROP_OR(inst, cmd_read_dual_data, 0),                                                             \
            [IDX_CMD_READ_DUAL_ADDR_DUAL_DATA] = DT_INST_PROP_OR(inst, cmd_read_dual_addr_dual_data, 0),                                         \
            [IDX_CMD_4BYTE_READ_DUAL_DATA] = DT_INST_PROP_OR(inst, cmd_4byte_read_dual_data, 0),                                                 \
            [IDX_CMD_4BYTE_READ_DUAL_ADDR_DUAL_DATA] = DT_INST_PROP_OR(inst, cmd_4byte_read_dual_addr_dual_data, 0),                             \
            [IDX_CMD_PROGRAM_QUAD_DATA] = DT_INST_PROP_OR(inst, cmd_program_quad_data, 0),                                                       \
            [IDX_CMD_4BYTE_PROGRAM_QUAD_DATA] = DT_INST_PROP_OR(inst, cmd_4byte_program_quad_data, 0),                                           \
        },                                                                                                                                       \
        .fixed_cmd_dummy_tab = {                                                                                                                 \
            [IDX_CMD_PAGE_PROGRAM_DUMMY] = DT_INST_PROP_OR(inst, cmd_page_program_dummy, 0),                                                     \
            [IDX_CMD_PAGE_PROGRAM_QUAD_ADDR_QUAD_DATA_DUMMY] = DT_INST_PROP_OR(inst, cmd_page_program_quad_addr_quad_data_dummy, 0),             \
            [IDX_CMD_ERASE_4KB_DUMMY] = DT_INST_PROP_OR(inst, cmd_erase_4kb_dummy, 0),                                                           \
            [IDX_CMD_ERASE_32KB_DUMMY] = DT_INST_PROP_OR(inst, cmd_erase_32kb_dummy, 0),                                                         \
            [IDX_CMD_ERASE_64KB_DUMMY] = DT_INST_PROP_OR(inst, cmd_erase_64kb_dummy, 0),                                                         \
            [IDX_CMD_READ_DUMMY] = DT_INST_PROP_OR(inst, cmd_read_dummy, 0),                                                                     \
            [IDX_CMD_FAST_READ_DUMMY] = DT_INST_PROP_OR(inst, cmd_fast_read_dummy, 0),                                                           \
            [IDX_CMD_READ_QUAD_DATA_DUMMY] = DT_INST_PROP_OR(inst, cmd_read_quad_data_dummy, 0),                                                 \
            [IDX_CMD_READ_QUAD_ADDR_QUAD_DATA_DUMMY] = DT_INST_PROP_OR(inst, cmd_read_quad_addr_quad_data_dummy, 0),                             \
            [IDX_CMD_QUAD_SPI_MODE_ENTER_DUMMY] = DT_INST_PROP_OR(inst, cmd_quad_spi_mode_enter_dummy, 0),                                       \
            [IDX_CMD_QUAD_SPI_MODE_EXIT_DUMMY] = DT_INST_PROP_OR(inst, cmd_quad_spi_mode_exit_dummy, 0),                                         \
            [IDX_CMD_4BYTE_MODE_ENTER_DUMMY] = DT_INST_PROP_OR(inst, cmd_4byte_mode_enter_dummy, 0),                                             \
            [IDX_CMD_4BYTE_MODE_EXIT_DUMMY] = DT_INST_PROP_OR(inst, cmd_4byte_mode_exit_dummy, 0),                                               \
            [IDX_CMD_4BYTE_READ_EXTENDED_ADDR_DUMMY] = DT_INST_PROP_OR(inst, cmd_4byte_read_extended_addr_dummy, 0),                             \
            [IDX_CMD_4BYTE_WRITE_EXTENDED_ADDR_DUMMY] = DT_INST_PROP_OR(inst, cmd_4byte_write_extended_addr_dummy, 0),                           \
            [IDX_CMD_4BYTE_PAGE_PROGRAM_DUMMY] = DT_INST_PROP_OR(inst, cmd_4byte_page_program_dummy, 0),                                         \
            [IDX_CMD_4BYTE_PAGE_PROGRAM_QUAD_ADDR_QUAD_DATA_DUMMY] = DT_INST_PROP_OR(inst, cmd_4byte_page_program_quad_addr_quad_data_dummy, 0), \
            [IDX_CMD_4BYTE_ERASE_4KB_DUMMY] = DT_INST_PROP_OR(inst, cmd_4byte_erase_4kb_dummy, 0),                                               \
            [IDX_CMD_4BYTE_ERASE_32KB_DUMMY] = DT_INST_PROP_OR(inst, cmd_4byte_erase_32kb_dummy, 0),                                             \
            [IDX_CMD_4BYTE_ERASE_64KB_DUMMY] = DT_INST_PROP_OR(inst, cmd_4byte_erase_64kb_dummy, 0),                                             \
            [IDX_CMD_4BYTE_READ_DUMMY] = DT_INST_PROP_OR(inst, cmd_4byte_read_dummy, 0),                                                         \
            [IDX_CMD_4BYTE_FAST_READ_DUMMY] = DT_INST_PROP_OR(inst, cmd_4byte_fast_read_dummy, 0),                                               \
            [IDX_CMD_4BYTE_READ_QUAD_DATA_DUMMY] = DT_INST_PROP_OR(inst, cmd_4byte_read_quad_data_dummy, 0),                                     \
            [IDX_CMD_4BYTE_READ_QUAD_ADDR_QUAD_DATA_DUMMY] = DT_INST_PROP_OR(inst, cmd_4byte_read_quad_addr_quad_data_dummy, 0),                 \
            [IDX_CMD_READ_DUAL_DATA_DUMMY] = DT_INST_PROP_OR(inst, cmd_read_dual_data_dummy, 0),                                                 \
            [IDX_CMD_READ_DUAL_ADDR_DUAL_DATA_DUMMY] = DT_INST_PROP_OR(inst, cmd_read_dual_addr_dual_data_dummy, 0),                             \
            [IDX_CMD_4BYTE_READ_DUAL_DATA_DUMMY] = DT_INST_PROP_OR(inst, cmd_4byte_read_dual_data_dummy, 0),                                     \
            [IDX_CMD_4BYTE_READ_DUAL_ADDR_DUAL_DATA_DUMMY] = DT_INST_PROP_OR(inst, cmd_4byte_read_dual_addr_dual_data_dummy, 0),                 \
            [IDX_CMD_PROGRAM_QUAD_DATA_DUMMY] = DT_INST_PROP_OR(inst, cmd_program_quad_data_dummy, 0),                                           \
            [IDX_CMD_4BYTE_PROGRAM_QUAD_DATA_DUMMY] = DT_INST_PROP_OR(inst, cmd_4byte_program_quad_data_dummy, 0),                               \
        },                                                                                                                                       \
        .log_info = &log_info_##inst,                                                                                                            \
        IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, dmas), (.dev_dma = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(inst, rx)),))                          \
        IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, dmas), (.dma_channel = DT_INST_DMAS_CELL_BY_NAME(inst, rx, channel),))                            \
        IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, dmas), (.dma_handshake = DT_INST_DMAS_CELL_BY_NAME(inst, rx, handshake),))                        \
        IF_ENABLED(CONFIG_PINCTRL, (.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst), ))                                                             \
        IF_ENABLED(DT_HAS_CLOCKS(inst), (.ccfg = LS_DT_CLK_CFG_ITEM(inst), ))                                                                    \
        IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, resets), (.reset = RESET_DT_SPEC_INST_GET(inst), ))                                               \
    };                                                                                                                                           \
    static struct linkedsemi_spi_filter_data linkedsemi_spi_filter_data_##inst;                                                                  \
DEVICE_DT_INST_DEFINE(inst,                                                                                                                      \
                      linkedsemi_spi_filter_init,                                                                                                \
                      NULL,                                                                                                                      \
                      &linkedsemi_spi_filter_data_##inst,                                                                                        \
                      &linkedsemi_spi_filter_cfg_##inst,                                                                                         \
                      POST_KERNEL,                                                                                                               \
                      CONFIG_KERNEL_INIT_PRIORITY_DEVICE,                                                                                        \
                      NULL);

DT_INST_FOREACH_STATUS_OKAY(SPI_FILTER_INIT)
