/*
 * Copyright (c) 2021 - 2023 Chin-Ting Kuo <chin-ting_kuo@aspeedtech.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/util.h>
#include <stdlib.h>
#include <string.h>
#include <spi_filter.h>
#include <soc.h>
#include <zephyr/kernel.h>

#define LOG_LEVEL CONFIG_SPI_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_filter_shell);

static const struct device *spif_device;

static int probe_parse_helper(const struct shell *shell, size_t *argc,
        char **argv[], const struct device **spif_dev)
{
    *spif_dev = device_get_binding((*argv)[1]);
    if (!*spif_dev) {
        shell_error(shell, "SPI filter device/driver is not found!");
        return -ENODEV;
    }

    return 0;
}

static int cmd_parse_helper(const struct shell *shell, size_t *argc,
        char **argv[], uint8_t *cmd, uint8_t *dummy_cycle)
{
    char *endptr;

    if (*argc < 2) {
        shell_error(shell, "Missing cmd.");
        return -EINVAL;
    }

    *cmd = strtoul((*argv)[1], &endptr, 16);
    if (*argc == 3) {
        *dummy_cycle = strtoul((*argv)[2], &endptr, 16);
    }

    return 0;
}

static int addr_parse_helper(const struct shell *shell, size_t *argc,
        char **argv[], bool *enable, mm_reg_t *addr, uint32_t *len)
{
    char *endptr;

    if (*argc < 4) {
        shell_error(shell, "Missing address or length parameter.");
        return -EINVAL;
    }

    *enable = false;
    if (strncmp((*argv)[1], "enable", 6) == 0)
        *enable = true;

    *addr = strtoul((*argv)[2], &endptr, 16);
    *len = strtoul((*argv)[3], &endptr, 16);

    return 0;
}

static int cmd_probe(const struct shell *shell, size_t argc, char *argv[])
{
    int ret;

    ret = probe_parse_helper(shell, &argc, &argv, &spif_device);
    if (ret)
        goto end;

    shell_print(shell, "SPI filter device, %s, is found!", spif_device->name);

end:
    return ret;
}

static int dump_cmd_table(const struct shell *shell, size_t argc, char *argv[])
{
    if (!spif_device) {
        shell_error(shell, "Please set the device first.");
        return -ENODEV;
    }

    spif_dump_cmd_table(spif_device);

    return 0;
}

static int add_cmd(const struct shell *shell, size_t argc, char *argv[])
{
    int ret;
    uint8_t cmd = 0;
    uint8_t dummy_cycle = 0;

    if (!spif_device) {
        shell_error(shell, "Please set the device first.");
        return -ENODEV;
    }

    ret = cmd_parse_helper(shell, &argc, &argv, &cmd, &dummy_cycle);
    if (ret)
        goto end;

    ret = spif_add_cmd(spif_device, cmd, dummy_cycle);

end:
    return ret;
}

static int remove_cmd(const struct shell *shell, size_t argc, char *argv[])
{
    int ret;
    uint8_t cmd = 0;

    if (!spif_device) {
        shell_error(shell, "Please set the device first.");
        return -ENODEV;
    }

    ret = cmd_parse_helper(shell, &argc, &argv, &cmd, NULL);
    if (ret)
        goto end;

    ret = spif_remove_cmd(spif_device, cmd);
    if (ret)
        goto end;

end:
    return ret;
}

static int dump_rw_addr_priv_table(const struct shell *shell, size_t argc, char *argv[])
{
    if (!spif_device) {
        shell_error(shell, "Please set the device first.");
        return -ENODEV;
    }

    spif_dump_rw_addr_privilege_table(spif_device);

    return 0;
}

static int dump_cmd_bitmap_log(const struct shell *shell, size_t argc, char *argv[])
{
    if (!spif_device) {
        shell_error(shell, "Please set the device first.");
        return -ENODEV;
    }

    uint8_t bitmap[SPIF_CMD_BITMAP_LOG_SIZE_BYTE];
    spif_dump_cmd_bitmap_log(spif_device, bitmap);
    shell_hexdump(shell, bitmap, SPIF_CMD_BITMAP_LOG_SIZE_BYTE);

    return 0;
}

static int read_addr_priv_table_config(const struct shell *shell, size_t argc, char *argv[])
{
    int ret;
    mm_reg_t addr = 0;
    uint32_t len = 0;
    bool enable = false;
    enum addr_priv_op op;

    if (!spif_device) {
        shell_error(shell, "Please set the device first.");
        return -ENODEV;
    }

    ret = addr_parse_helper(shell, &argc, &argv, &enable, &addr, &len);
    if (ret)
        goto end;

    LOG_DBG("read: %s, addr: 0x%08lx, len: 0x%08x\n",
        enable ? "enable" : "disable", addr, len);

    if (enable)
        op = FLAG_ADDR_PRIV_ENABLE;
    else
        op = FLAG_ADDR_PRIV_DISABLE;

    ret = spif_address_privilege_config(spif_device,
                        FLAG_ADDR_PRIV_READ_SELECT,
                        op, addr, len);

end:
    return ret;
}

static int write_addr_priv_table_config(const struct shell *shell, size_t argc, char *argv[])
{
    int ret;
    mm_reg_t addr = 0;
    uint32_t len = 0;
    bool enable = false;
    enum addr_priv_op op;

    if (!spif_device) {
        shell_error(shell, "Please set the device first.");
        return -ENODEV;
    }

    ret = addr_parse_helper(shell, &argc, &argv, &enable, &addr, &len);
    if (ret)
        goto end;

    LOG_DBG("write: %s, addr: 0x%08lx, len: 0x%08x\n",
        enable ? "enable" : "disable", addr, len);

    if (enable)
        op = FLAG_ADDR_PRIV_ENABLE;
    else
        op = FLAG_ADDR_PRIV_DISABLE;

    ret = spif_address_privilege_config(spif_device,
                        FLAG_ADDR_PRIV_WRITE_SELECT,
                        op, addr, len);

end:
    return ret;
}

static int spi_filter_enabled(const struct shell *shell, size_t argc, char *argv[])
{

    if (!spif_device) {
        shell_error(shell, "Please set the device first.");
        return -ENODEV;
    }

    spif_filter_enable(spif_device, true);

    return 0;
}

static int spi_filter_disabled(const struct shell *shell, size_t argc, char *argv[])
{

    if (!spif_device) {
        shell_error(shell, "Please set the device first.");
        return -ENODEV;
    }

    spif_filter_enable(spif_device, false);

    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_spif_cmds,
    SHELL_CMD_ARG(dump, NULL, "\"dump\"", dump_cmd_table, 1, 0),
    SHELL_CMD_ARG(add, NULL, "<cmd>", add_cmd, 2, 1),
    SHELL_CMD_ARG(rm, NULL, "<cmd>", remove_cmd, 2, 0),

    SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_spif_addr,
    SHELL_CMD_ARG(dump, NULL, "\"dump\"", dump_rw_addr_priv_table, 1, 0),
    SHELL_CMD_ARG(log, NULL, "\"log\"", dump_cmd_bitmap_log, 1, 0),
    SHELL_CMD_ARG(read, NULL, "<enable/disable> <addr> <len>",
        read_addr_priv_table_config, 4, 0),
    SHELL_CMD_ARG(write, NULL, "<enable/disable> <addr> <len>",
        write_addr_priv_table_config, 4, 0),

    SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_spif_config,
    SHELL_CMD_ARG(enable, NULL, "\"enable\"", spi_filter_enabled, 1, 0),
    SHELL_CMD_ARG(disable, NULL, "\"disable\"", spi_filter_disabled, 1, 0),

    SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(spif_cmds,
    SHELL_CMD_ARG(set_dev, NULL, "<device>", cmd_probe, 2, 0),
    SHELL_CMD(cmd, &sub_spif_cmds, "cmd table related operations", NULL),
    SHELL_CMD(addr, &sub_spif_addr, "address privilege table related operations", NULL),
    SHELL_CMD(config, &sub_spif_config, "SPI filter configuration", NULL),

    SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(spif, &spif_cmds, "SPI filter shell cmds", NULL);

