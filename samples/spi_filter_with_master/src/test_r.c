void test_r_spicmd_saddr3b(const struct device *const spifilter)
{

    g_cnt_last = g_cnt;
    const uint32_t flash_size = MB(16);

    g_cnt_last = g_cnt;
    do { /* forbidden */
        g_cmd = CMD_FAST_READ;
        uint8_t tx_data[] = {g_cmd, 0x1, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        spif_address_privilege_config(spifilter, FLAG_ADDR_PRIV_READ_SELECT, FLAG_ADDR_PRIV_DISABLE, MB(0), flash_size);
        printf("spif_add_cmd: %#x\n", g_cmd);
        spif_add_cmd(spifilter, g_cmd, 8);
        if (HAL_SSI_Transmit(&SsiHandle, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);
    g_cnt_expect++;
    __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == (g_cnt_last + 1)));

    g_cnt_last = g_cnt;
    do { /* pass */
        g_cmd = CMD_FAST_READ;
        uint8_t tx_data[] = {g_cmd, 0x1, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        spif_address_privilege_config(spifilter,
                                    FLAG_ADDR_PRIV_READ_SELECT,
                                    FLAG_ADDR_PRIV_ENABLE,
                                    MB(0),
                                    flash_size);
        printf("spif_add_cmd: %#x\n", g_cmd);
        spif_add_cmd(spifilter, g_cmd, 8);
        if (HAL_SSI_Transmit(&SsiHandle, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);
    __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == g_cnt_last));

    g_cnt_last = g_cnt;
    do { /* forbidden */
        g_cmd = CMD_FAST_READ;
        uint8_t tx_data[] = {g_cmd, 0x1, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        spif_address_privilege_config(spifilter, FLAG_ADDR_PRIV_READ_SELECT, FLAG_ADDR_PRIV_DISABLE, MB(0), flash_size);
        spif_address_privilege_config(spifilter,
                                    FLAG_ADDR_PRIV_READ_SELECT,
                                    FLAG_ADDR_PRIV_ENABLE,
                                    flash_size >> 1,
                                    flash_size);
        printf("spif_add_cmd: %#x\n", g_cmd);
        spif_add_cmd(spifilter, g_cmd, 8);
        if (HAL_SSI_Transmit(&SsiHandle, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);
    g_cnt_expect++;
    __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == (g_cnt_last + 1)));

    g_cnt_last = g_cnt;
    do { /* pass */
        g_cmd = CMD_FAST_READ;
        uint8_t tx_data[] = {g_cmd, 0x1, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        spif_address_privilege_config(spifilter, FLAG_ADDR_PRIV_READ_SELECT, FLAG_ADDR_PRIV_DISABLE, MB(0), flash_size);
        spif_address_privilege_config(spifilter,
                                    FLAG_ADDR_PRIV_READ_SELECT,
                                    FLAG_ADDR_PRIV_ENABLE,
                                    MB(0),
                                    flash_size >> 1);
        printf("spif_add_cmd: %#x\n", g_cmd);
        spif_add_cmd(spifilter, g_cmd, 8);
        if (HAL_SSI_Transmit(&SsiHandle, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);
    __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == g_cnt_last));

    g_cnt_last = g_cnt;
    do { /* forbidden */
        g_cmd = CMD_FAST_READ;
        uint8_t tx_data[] = {g_cmd, 0x7, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        spif_address_privilege_config(spifilter, FLAG_ADDR_PRIV_READ_SELECT, FLAG_ADDR_PRIV_DISABLE, MB(0), flash_size);
        spif_address_privilege_config(spifilter,
                                    FLAG_ADDR_PRIV_READ_SELECT,
                                    FLAG_ADDR_PRIV_ENABLE,
                                    flash_size >> 1,
                                    flash_size);
        printf("spif_add_cmd: %#x\n", g_cmd);
        spif_add_cmd(spifilter, g_cmd, 8);
        if (HAL_SSI_Transmit(&SsiHandle, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);
    g_cnt_expect++;
    __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == (g_cnt_last + 1)));

    g_cnt_last = g_cnt;
    do { /* pass */
        g_cmd = CMD_FAST_READ;
        uint8_t tx_data[] = {g_cmd, 0x7, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        spif_address_privilege_config(spifilter, FLAG_ADDR_PRIV_READ_SELECT, FLAG_ADDR_PRIV_DISABLE, MB(0), flash_size);
        spif_address_privilege_config(spifilter,
                                    FLAG_ADDR_PRIV_READ_SELECT,
                                    FLAG_ADDR_PRIV_ENABLE,
                                    MB(0),
                                    flash_size >> 1);
        printf("spif_add_cmd: %#x\n", g_cmd);
        spif_add_cmd(spifilter, g_cmd, 8);
        if (HAL_SSI_Transmit(&SsiHandle, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);
    __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == g_cnt_last));
}

void test_r_spicmd_saddr4b_main(const struct device *const spifilter)
{
    const uint32_t flash_size = MB(256);

    g_cnt_last = g_cnt;
    do { /* forbidden */
        g_cmd = CMD_4BYTE_FAST_READ;
        uint8_t tx_data[] = {g_cmd, 0x1, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        spif_address_privilege_config(spifilter, FLAG_ADDR_PRIV_READ_SELECT, FLAG_ADDR_PRIV_DISABLE, MB(0), flash_size);
        printf("spif_add_cmd: %#x\n", g_cmd);
        spif_add_cmd(spifilter, g_cmd, 8);
        if (HAL_SSI_Transmit(&SsiHandle, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);
    g_cnt_expect++;
    __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == (g_cnt_last + 1)));

    g_cnt_last = g_cnt;
    do { /* pass */
        g_cmd = CMD_4BYTE_FAST_READ;
        uint8_t tx_data[] = {g_cmd, 0x1, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        spif_address_privilege_config(spifilter,
                                    FLAG_ADDR_PRIV_READ_SELECT,
                                    FLAG_ADDR_PRIV_ENABLE,
                                    MB(0),
                                    flash_size);
        printf("spif_add_cmd: %#x\n", g_cmd);
        spif_add_cmd(spifilter, g_cmd, 8);
        if (HAL_SSI_Transmit(&SsiHandle, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);
    __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == g_cnt_last));

    g_cnt_last = g_cnt;
    do { /* forbidden */
        g_cmd = CMD_4BYTE_FAST_READ;
        uint8_t tx_data[] = {g_cmd, 0x1, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        spif_address_privilege_config(spifilter, FLAG_ADDR_PRIV_READ_SELECT, FLAG_ADDR_PRIV_DISABLE, MB(0), flash_size);
        spif_address_privilege_config(spifilter,
                                    FLAG_ADDR_PRIV_READ_SELECT,
                                    FLAG_ADDR_PRIV_ENABLE,
                                    MB(128),
                                    flash_size);
        printf("spif_add_cmd: %#x\n", g_cmd);
        spif_add_cmd(spifilter, g_cmd, 8);
        if (HAL_SSI_Transmit(&SsiHandle, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);
    g_cnt_expect++;
    __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == (g_cnt_last + 1)));

    g_cnt_last = g_cnt;
    do { /* pass */
        g_cmd = CMD_4BYTE_FAST_READ;
        uint8_t tx_data[] = {g_cmd, 0x1, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        spif_address_privilege_config(spifilter, FLAG_ADDR_PRIV_READ_SELECT, FLAG_ADDR_PRIV_DISABLE, MB(0), flash_size);
        spif_address_privilege_config(spifilter,
                                    FLAG_ADDR_PRIV_READ_SELECT,
                                    FLAG_ADDR_PRIV_ENABLE,
                                    MB(0),
                                    MB(128));
        printf("spif_add_cmd: %#x\n", g_cmd);
        spif_add_cmd(spifilter, g_cmd, 8);
        if (HAL_SSI_Transmit(&SsiHandle, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);
    __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == g_cnt_last));

    g_cnt_last = g_cnt;
    do { /* forbidden */
        g_cmd = CMD_4BYTE_FAST_READ;
        uint8_t tx_data[] = {g_cmd, 0x7, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        spif_address_privilege_config(spifilter, FLAG_ADDR_PRIV_READ_SELECT, FLAG_ADDR_PRIV_DISABLE, MB(0), flash_size);
        spif_address_privilege_config(spifilter,
                                    FLAG_ADDR_PRIV_READ_SELECT,
                                    FLAG_ADDR_PRIV_ENABLE,
                                    MB(128),
                                    flash_size);
        printf("spif_add_cmd: %#x\n", g_cmd);
        spif_add_cmd(spifilter, g_cmd, 8);
        if (HAL_SSI_Transmit(&SsiHandle, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);
    g_cnt_expect++;
    __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == (g_cnt_last + 1)));

    g_cnt_last = g_cnt;
    do { /* pass */
        g_cmd = CMD_4BYTE_FAST_READ;
        uint8_t tx_data[] = {g_cmd, 0x7, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        spif_address_privilege_config(spifilter, FLAG_ADDR_PRIV_READ_SELECT, FLAG_ADDR_PRIV_DISABLE, MB(0), flash_size);
        spif_address_privilege_config(spifilter,
                                    FLAG_ADDR_PRIV_READ_SELECT,
                                    FLAG_ADDR_PRIV_ENABLE,
                                    MB(0),
                                    MB(128));
        printf("spif_add_cmd: %#x\n", g_cmd);
        spif_add_cmd(spifilter, g_cmd, 8);
        if (HAL_SSI_Transmit(&SsiHandle, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);
    __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == g_cnt_last));
}

void test_r_spicmd_saddr4b(const struct device *const spifilter)
{
    test_spi_en4b(spifilter);
    test_r_spicmd_saddr4b_main(spifilter);
    test_spi_ex4b(spifilter);
}

#if 0
void test_r_spicmd_saddr3b_ddata(const struct device *const spifilter)
{

    g_cnt_last = g_cnt;
    do {
        g_cmd = CMD_PAGE_PROGRAM;
        // uint8_t tx_data[] = {g_cmd, 0x1, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        uint8_t tx_data[] = {0x4, 0x5a, 0xf7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd};
        uint64_t addr = 0x010203;
        spif_address_privilege_config(spifilter, FLAG_ADDR_PRIV_READ_SELECT, FLAG_ADDR_PRIV_DISABLE, MB(0), flash_size);
        printf("spif_add_cmd: %#x\n", g_cmd);
        spif_add_cmd(spifilter, g_cmd, 8);
        if (HAL_SSI_Dual_Ddata_Transmit(&SsiHandle, addr, Addr_Width_24_bits, g_cmd, 0, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);

    g_cnt_last = g_cnt;
    do {
        g_cmd = CMD_PAGE_PROGRAM;
        // uint8_t tx_data[] = {g_cmd, 0x1, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        uint8_t tx_data[] = {0x4, 0x5a, 0xf7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd};
        uint64_t addr = 0x010203;
        spif_address_privilege_config(spifilter,
                                    FLAG_ADDR_PRIV_READ_SELECT,
                                    FLAG_ADDR_PRIV_ENABLE,
                                    MB(0),
                                    flash_size);
        printf("spif_add_cmd: %#x\n", g_cmd);
        spif_add_cmd(spifilter, g_cmd, 8);
        if (HAL_SSI_Dual_Ddata_Transmit(&SsiHandle, addr, Addr_Width_24_bits, g_cmd, 0, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);
}

void test_r_spicmd_saddr4b_ddata(const struct device *const spifilter)
{

    g_cnt_last = g_cnt;
    do {
        g_cmd = CMD_PAGE_PROGRAM;
        // uint8_t tx_data[] = {g_cmd, 0x1, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        uint8_t tx_data[] = {0x4, 0x5a, 0xf7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd};
        uint64_t addr = 0x01020304;
        spif_address_privilege_config(spifilter, FLAG_ADDR_PRIV_READ_SELECT, FLAG_ADDR_PRIV_DISABLE, MB(0), flash_size);
        printf("spif_add_cmd: %#x\n", g_cmd);
        spif_add_cmd(spifilter, g_cmd, 8);
        if (HAL_SSI_Dual_Ddata_Transmit(&SsiHandle, addr, Addr_Width_32_bits, g_cmd, 0, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);

    g_cnt_last = g_cnt;
    do {
        g_cmd = CMD_PAGE_PROGRAM;
        // uint8_t tx_data[] = {g_cmd, 0x1, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        uint8_t tx_data[] = {0x4, 0x5a, 0xf7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd};
        uint64_t addr = 0x01020304;
        spif_address_privilege_config(spifilter,
                                    FLAG_ADDR_PRIV_READ_SELECT,
                                    FLAG_ADDR_PRIV_ENABLE,
                                    MB(0),
                                    flash_size);
        printf("spif_add_cmd: %#x\n", g_cmd);
        spif_add_cmd(spifilter, g_cmd, 8);
        if (HAL_SSI_Dual_Ddata_Transmit(&SsiHandle, addr, Addr_Width_32_bits, g_cmd, 0, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);
}
#endif

//maybe test
void test_r_addr_dump(const struct device *const spifilter)
{
    const uint32_t flash_size = MB(16);
    for(uint32_t i = 0; i < 3; i++) {
        do {
            spif_address_privilege_config(spifilter,
                                        FLAG_ADDR_PRIV_READ_SELECT,
                                        FLAG_ADDR_PRIV_ENABLE,
                                        MB(0),
                                        flash_size);
            spif_dump_rw_addr_privilege_table(spifilter);io_toggle_pin(PA01);
        } while(0);
        do {
            spif_address_privilege_config(spifilter,
                                        FLAG_ADDR_PRIV_READ_SELECT,
                                        FLAG_ADDR_PRIV_DISABLE,
                                        MB(0),
                                        flash_size);
            spif_dump_rw_addr_privilege_table(spifilter);io_toggle_pin(PA01);
        } while(0);
    }
//     do {
//         spif_address_privilege_config(spifilter,
//                                     FLAG_ADDR_PRIV_READ_SELECT,
//                                     FLAG_ADDR_PRIV_ENABLE,
//                                     MB(0),
//                                     flash_size);
//         spif_dump_rw_addr_privilege_table(spifilter);io_toggle_pin(PA01);
//     } while(0);
// #if 1
//     do {
//         spif_address_privilege_config(spifilter,
//                                     FLAG_ADDR_PRIV_READ_SELECT,
//                                     FLAG_ADDR_PRIV_ENABLE,
//                                     MB(0),
//                                     flash_size >> 1);
//         spif_dump_rw_addr_privilege_table(spifilter);io_toggle_pin(PA01);
//     } while(0);
// #endif

// #if 1
//     do {
//         spif_address_privilege_config(spifilter,
//                                     FLAG_ADDR_PRIV_READ_SELECT,
//                                     FLAG_ADDR_PRIV_ENABLE,
//                                     flash_size >> 1,
//                                     flash_size);
//         spif_dump_rw_addr_privilege_table(spifilter);io_toggle_pin(PA01);
//     } while(0);
// #endif
}

//maybe test
void test_r_spicmd_daddr3b(const struct device *const spifilter)
{
    const uint32_t flash_size = MB(16);
    // spif_dump_cmd_table(spifilter);
    spif_dump_rw_addr_privilege_table(spifilter);io_toggle_pin(PA00);

    g_cnt_last = g_cnt;
    do {/* forbidden */
        g_cmd = CMD_READ_DUAL_ADDR_DUAL_DATA;
        // uint8_t tx_data[] = {g_cmd, 0x1, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        uint8_t tx_data[] = {0x4, 0x5a, 0xf7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd};
        uint64_t addr = 0x010203;
        spif_address_privilege_config(spifilter, FLAG_ADDR_PRIV_READ_SELECT, FLAG_ADDR_PRIV_DISABLE, MB(0), flash_size);
        printf("spif_add_cmd: %#x\n", g_cmd);
        spif_add_cmd(spifilter, g_cmd, 8);
        // spif_dump_cmd_table(spifilter);io_toggle_pin(PA01);
        spif_dump_rw_addr_privilege_table(spifilter);
        if (HAL_SSI_Dual_Daddr_Transmit(&SsiHandle, addr, Addr_Width_24_bits, g_cmd, 0, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);
    g_cnt_expect++;
    __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == (g_cnt_last + 1)));

    g_cnt_last = g_cnt;
    do {/*pass*/
        g_cmd = CMD_READ_DUAL_ADDR_DUAL_DATA;
        // uint8_t tx_data[] = {g_cmd, 0x1, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        uint8_t tx_data[] = {0x4, 0x5a, 0xf7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd};
        uint64_t addr = 0x010203;
        spif_address_privilege_config(spifilter,
                                    FLAG_ADDR_PRIV_READ_SELECT,
                                    FLAG_ADDR_PRIV_ENABLE,
                                    MB(0),
                                    flash_size);
        printf("spif_add_cmd: %#x\n", g_cmd);
        spif_add_cmd(spifilter, g_cmd, 8);
        // spif_dump_cmd_table(spifilter);
        spif_dump_rw_addr_privilege_table(spifilter);io_toggle_pin(PA01);
        if (HAL_SSI_Dual_Daddr_Transmit(&SsiHandle, addr, Addr_Width_24_bits, g_cmd, 0, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);
    __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == g_cnt_last));
#if 1

    g_cnt_last = g_cnt;
    do {/*pass*/
        g_cmd = CMD_READ_DUAL_ADDR_DUAL_DATA;
        // uint8_t tx_data[] = {g_cmd, 0x1, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        uint8_t tx_data[] = {0x4, 0x5a, 0xf7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd};
        uint64_t addr = 0x010203;
        spif_address_privilege_config(spifilter, FLAG_ADDR_PRIV_READ_SELECT, FLAG_ADDR_PRIV_DISABLE, MB(0), flash_size);
        spif_address_privilege_config(spifilter,
                                    FLAG_ADDR_PRIV_READ_SELECT,
                                    FLAG_ADDR_PRIV_ENABLE,
                                    MB(0),
                                    flash_size >> 1);
        printf("spif_add_cmd: %#x\n", g_cmd);
        spif_add_cmd(spifilter, g_cmd, 8);
        // spif_dump_cmd_table(spifilter);
        spif_dump_rw_addr_privilege_table(spifilter);io_toggle_pin(PA01);
        if (HAL_SSI_Dual_Daddr_Transmit(&SsiHandle, addr, Addr_Width_24_bits, g_cmd, 0, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);
    __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == g_cnt_last));
#endif

#if 1

    g_cnt_last = g_cnt;
    do {/* forbidden */
        g_cmd = CMD_READ_DUAL_ADDR_DUAL_DATA;
        // uint8_t tx_data[] = {g_cmd, 0x1, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        uint8_t tx_data[] = {0x4, 0x5a, 0xf7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd};
        uint64_t addr = 0x010203;
        spif_address_privilege_config(spifilter, FLAG_ADDR_PRIV_READ_SELECT, FLAG_ADDR_PRIV_DISABLE, MB(0), flash_size);
        spif_address_privilege_config(spifilter,
                                    FLAG_ADDR_PRIV_READ_SELECT,
                                    FLAG_ADDR_PRIV_ENABLE,
                                    flash_size >> 1,
                                    flash_size);
        printf("spif_add_cmd: %#x\n", g_cmd);
        spif_add_cmd(spifilter, g_cmd, 8);
        // spif_dump_cmd_table(spifilter);
        spif_dump_rw_addr_privilege_table(spifilter);io_toggle_pin(PA01);
        if (HAL_SSI_Dual_Daddr_Transmit(&SsiHandle, addr, Addr_Width_24_bits, g_cmd, 0, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);
    g_cnt_expect++;
    __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == (g_cnt_last + 1)));
#endif
}

void test_r_spicmd_daddr4b(const struct device *const spifilter)
{
    const uint32_t flash_size = MB(256);

    g_cnt_last = g_cnt;
    do {/* forbidden */
        g_cmd = CMD_4BYTE_READ_DUAL_ADDR_DUAL_DATA;
        // uint8_t tx_data[] = {g_cmd, 0x1, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        uint8_t tx_data[] = {0x4, 0x5a, 0xf7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd};
        uint64_t addr = 0x01020304;
        spif_address_privilege_config(spifilter, FLAG_ADDR_PRIV_READ_SELECT, FLAG_ADDR_PRIV_DISABLE, MB(0), flash_size);
        printf("spif_add_cmd: %#x\n", g_cmd);
        spif_add_cmd(spifilter, g_cmd, 8);
        if (HAL_SSI_Dual_Daddr_Transmit(&SsiHandle, addr, Addr_Width_32_bits, g_cmd, 0, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);
    g_cnt_expect++;
    __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == (g_cnt_last + 1)));

    g_cnt_last = g_cnt;
    do {/* pass */
        g_cmd = CMD_4BYTE_READ_DUAL_ADDR_DUAL_DATA;
        // uint8_t tx_data[] = {g_cmd, 0x1, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        uint8_t tx_data[] = {0x4, 0x5a, 0xf7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd};
        uint64_t addr = 0x01020304;
        spif_address_privilege_config(spifilter,
                                    FLAG_ADDR_PRIV_READ_SELECT,
                                    FLAG_ADDR_PRIV_ENABLE,
                                    MB(0),
                                    flash_size);
        printf("spif_add_cmd: %#x\n", g_cmd);
        spif_add_cmd(spifilter, g_cmd, 8);
        if (HAL_SSI_Dual_Daddr_Transmit(&SsiHandle, addr, Addr_Width_32_bits, g_cmd, 0, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);
    __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == g_cnt_last));

}

#if 0
void test_r_spicmd_saddr3b_qdata(const struct device *const spifilter)
{

    g_cnt_last = g_cnt;
    do {
        g_cmd = CMD_PAGE_PROGRAM;
        // uint8_t tx_data[] = {g_cmd, 0x1, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        uint8_t tx_data[] = {0x4, 0x5a, 0xf7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd};
        uint64_t addr = 0x010203;
        spif_address_privilege_config(spifilter, FLAG_ADDR_PRIV_READ_SELECT, FLAG_ADDR_PRIV_DISABLE, MB(0), flash_size);
        printf("spif_add_cmd: %#x\n", g_cmd);
        spif_add_cmd(spifilter, g_cmd, 8);
        if (HAL_SSI_Quad_Qdata_Transmit(&SsiHandle, addr, Addr_Width_24_bits, g_cmd, 0, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);

    g_cnt_last = g_cnt;
    do {
        g_cmd = CMD_PAGE_PROGRAM;
        // uint8_t tx_data[] = {g_cmd, 0x1, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        uint8_t tx_data[] = {0x4, 0x5a, 0xf7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd};
        uint64_t addr = 0x010203;
        spif_address_privilege_config(spifilter,
                                    FLAG_ADDR_PRIV_READ_SELECT,
                                    FLAG_ADDR_PRIV_ENABLE,
                                    MB(0),
                                    flash_size);
        printf("spif_add_cmd: %#x\n", g_cmd);
        spif_add_cmd(spifilter, g_cmd, 8);
        if (HAL_SSI_Quad_Qdata_Transmit(&SsiHandle, addr, Addr_Width_24_bits, g_cmd, 0, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);
}

void test_r_spicmd_saddr4b_qdata(const struct device *const spifilter)
{

    g_cnt_last = g_cnt;
    do {
        g_cmd = CMD_PAGE_PROGRAM;
        // uint8_t tx_data[] = {g_cmd, 0x1, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        uint8_t tx_data[] = {0x4, 0x5a, 0xf7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd};
        uint64_t addr = 0x01020304;
        spif_address_privilege_config(spifilter, FLAG_ADDR_PRIV_READ_SELECT, FLAG_ADDR_PRIV_DISABLE, MB(0), flash_size);
        printf("spif_add_cmd: %#x\n", g_cmd);
        spif_add_cmd(spifilter, g_cmd, 8);
        if (HAL_SSI_Quad_Qdata_Transmit(&SsiHandle, addr, Addr_Width_32_bits, g_cmd, 0, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);

    g_cnt_last = g_cnt;
    do {
        g_cmd = CMD_PAGE_PROGRAM;
        // uint8_t tx_data[] = {g_cmd, 0x1, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        uint8_t tx_data[] = {0x4, 0x5a, 0xf7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd};
        uint64_t addr = 0x01020304;
        spif_address_privilege_config(spifilter,
                                    FLAG_ADDR_PRIV_READ_SELECT,
                                    FLAG_ADDR_PRIV_ENABLE,
                                    MB(0),
                                    flash_size);
        printf("spif_add_cmd: %#x\n", g_cmd);
        spif_add_cmd(spifilter, g_cmd, 8);
        if (HAL_SSI_Quad_Qdata_Transmit(&SsiHandle, addr, Addr_Width_32_bits, g_cmd, 0, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);
}
#endif

void test_r_spicmd_qaddr3b(const struct device *const spifilter)
{
    const uint32_t flash_size = MB(16);

    g_cnt_last = g_cnt;
    do { /* forbidden */
        g_cmd = CMD_READ_DUAL_ADDR_DUAL_DATA;
        // uint8_t tx_data[] = {g_cmd, 0x1, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        uint8_t tx_data[] = {0x4, 0x5a, 0xf7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd};
        uint64_t addr = 0x010203;
        spif_address_privilege_config(spifilter, FLAG_ADDR_PRIV_READ_SELECT, FLAG_ADDR_PRIV_DISABLE, MB(0), flash_size);
        printf("spif_add_cmd: %#x\n", g_cmd);
        spif_add_cmd(spifilter, g_cmd, 8);
        if (HAL_SSI_Quad_Qaddr_Transmit(&SsiHandle, addr, Addr_Width_24_bits, g_cmd, 0,
                                        tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);
    g_cnt_expect++;
    __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == (g_cnt_last + 1)));

    g_cnt_last = g_cnt;
    do { /* pass */
        g_cmd = CMD_READ_DUAL_ADDR_DUAL_DATA;
        // uint8_t tx_data[] = {g_cmd, 0x1, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        uint8_t tx_data[] = {0x4, 0x5a, 0xf7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd};
        uint64_t addr = 0x010203;
        spif_address_privilege_config(spifilter,
                                    FLAG_ADDR_PRIV_READ_SELECT,
                                    FLAG_ADDR_PRIV_ENABLE,
                                    MB(0),
                                    flash_size);
        printf("spif_add_cmd: %#x\n", g_cmd);
        spif_add_cmd(spifilter, g_cmd, 8);
        if (HAL_SSI_Quad_Qaddr_Transmit(&SsiHandle, addr, Addr_Width_24_bits, g_cmd, 0,
                                        tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);
    __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == g_cnt_last));
}

void test_r_spicmd_qaddr4b_main(const struct device *const spifilter)
{
    const uint32_t flash_size = MB(256);

    g_cnt_last = g_cnt;
    do { /* forbidden */
        g_cmd = CMD_4BYTE_READ_QUAD_ADDR_QUAD_DATA;
        // uint8_t tx_data[] = {g_cmd, 0x1, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        uint8_t tx_data[] = {0x4, 0x5a, 0xf7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd};
        uint64_t addr = 0x01020304;
        spif_address_privilege_config(spifilter, FLAG_ADDR_PRIV_READ_SELECT, FLAG_ADDR_PRIV_DISABLE, MB(0), flash_size);
        printf("spif_add_cmd: %#x\n", g_cmd);
        spif_add_cmd(spifilter, g_cmd, 8);
        // spif_dump_rw_addr_privilege_table(spifilter);
        if (HAL_SSI_Quad_Qaddr_Transmit(&SsiHandle, addr, Addr_Width_32_bits, g_cmd, 0,
                                            tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);
    g_cnt_expect++;
    __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == (g_cnt_last + 1)));

    g_cnt_last = g_cnt;
    do { /* pass */
        g_cmd = CMD_4BYTE_READ_QUAD_ADDR_QUAD_DATA;
        // uint8_t tx_data[] = {g_cmd, 0x1, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        uint8_t tx_data[] = {0x4, 0x5a, 0xf7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd};
        uint64_t addr = 0x01020304;
        spif_address_privilege_config(spifilter,
                                    FLAG_ADDR_PRIV_READ_SELECT,
                                    FLAG_ADDR_PRIV_ENABLE,
                                    MB(0),
                                    flash_size);
        printf("spif_add_cmd: %#x\n", g_cmd);
        spif_add_cmd(spifilter, g_cmd, 8);
        // spif_dump_rw_addr_privilege_table(spifilter);
        if (HAL_SSI_Quad_Qaddr_Transmit(&SsiHandle, addr, Addr_Width_32_bits, g_cmd, 0,
                                            tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);
    __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == g_cnt_last));
}

void test_r_spicmd_qaddr4b(const struct device *const spifilter)
{
    test_spi_en4b(spifilter);
    test_r_spicmd_qaddr4b_main(spifilter);
    test_spi_ex4b(spifilter);
}

#if 1
void test_r_qpi_3b_main(const struct device *const spifilter)
{
    const uint32_t flash_size = MB(16);

    g_cnt_last = g_cnt;
    do { /* forbidden */
        g_cmd = CMD_READ_QUAD_ADDR_QUAD_DATA;
        uint8_t tx_data[] = {g_cmd, 0x1, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        spif_address_privilege_config(spifilter, FLAG_ADDR_PRIV_READ_SELECT, FLAG_ADDR_PRIV_DISABLE, MB(0), flash_size);
        printf("spif_add_cmd: %#x\n", g_cmd);
        spif_add_cmd(spifilter, g_cmd, 8);
        if (HAL_SSI_QPI_Transmit(&SsiHandle, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);
    g_cnt_expect++;
    __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == (g_cnt_last + 1)));

    g_cnt_last = g_cnt;
    do { /* pass */
        g_cmd = CMD_READ_QUAD_ADDR_QUAD_DATA;
        uint8_t tx_data[] = {g_cmd, 0x1, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        spif_address_privilege_config(spifilter,
                                    FLAG_ADDR_PRIV_READ_SELECT,
                                    FLAG_ADDR_PRIV_ENABLE,
                                    MB(0),
                                    flash_size);
        printf("spif_add_cmd: %#x\n", g_cmd);
        spif_add_cmd(spifilter, g_cmd, 8);
        if (HAL_SSI_QPI_Transmit(&SsiHandle, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);
    __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == g_cnt_last));
}

void test_r_qpi_4b_main(const struct device *const spifilter)
{
    const uint32_t flash_size = MB(256);

    g_cnt_last = g_cnt;
    do { /* forbidden */
        g_cmd = CMD_4BYTE_READ_QUAD_ADDR_QUAD_DATA;
        uint8_t tx_data[] = {g_cmd, 0x1, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        // uint8_t tx_data[] = {0x4, 0x5a, 0xf7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd};
        // uint64_t addr = 0x01020304;
        spif_address_privilege_config(spifilter, FLAG_ADDR_PRIV_READ_SELECT, FLAG_ADDR_PRIV_DISABLE, MB(0), flash_size);
        printf("spif_add_cmd: %#x\n", g_cmd);
        spif_add_cmd(spifilter, g_cmd, 8);
        if (HAL_SSI_QPI_Transmit(&SsiHandle, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);
    g_cnt_expect++;
    __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == (g_cnt_last + 1)));

    g_cnt_last = g_cnt;
    do { /* pass */
        g_cmd = CMD_4BYTE_READ_QUAD_ADDR_QUAD_DATA;
        uint8_t tx_data[] = {g_cmd, 0x1, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        // uint8_t tx_data[] = {0x4, 0x5a, 0xf7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd};
        // uint64_t addr = 0x01020304;
        spif_address_privilege_config(spifilter,
                                    FLAG_ADDR_PRIV_READ_SELECT,
                                    FLAG_ADDR_PRIV_ENABLE,
                                    MB(0),
                                    flash_size);
        printf("spif_add_cmd: %#x\n", g_cmd);
        spif_add_cmd(spifilter, g_cmd, 8);
        if (HAL_SSI_QPI_Transmit(&SsiHandle, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);
    __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == g_cnt_last));
}

void test_r_qpi_3b(const struct device *const spifilter)
{
    test_enqpi(spifilter);
    test_r_qpi_3b_main(spifilter);
    test_exqpi(spifilter);
}

void test_r_qpi_4b(const struct device *const spifilter)
{
    test_enqpi(spifilter);
    test_qpi_en4b(spifilter);
    test_r_qpi_4b_main(spifilter);
    test_qpi_ex4b(spifilter);
    test_exqpi(spifilter);
}
#endif

void test_cmd_rd_addr_overflow(const struct device *const spifilter)
{
    const uint32_t flash_size = MB(16);
    for(uint32_t i = 0; i < 3; i++) {
        g_cnt_last = g_cnt;
        do { /* forbidden */
            g_cmd = CMD_FAST_READ;
            uint8_t tx_data[1 + 3 + 1 + 1 + 4] = {g_cmd, 0x7f, 0xff, 0xff, 0x0, 0x5a, 0xf7};
            // uint8_t tx_data[1 + 3 + 1 + 7 + 10] = {g_cmd, 0x7f, 0xff, 0xf7, 0x0, 0x5a, 0xf7};
            spif_address_privilege_config(spifilter, FLAG_ADDR_PRIV_READ_SELECT, FLAG_ADDR_PRIV_DISABLE, MB(0), flash_size);
            spif_address_privilege_config(spifilter,
                                        FLAG_ADDR_PRIV_READ_SELECT,
                                        FLAG_ADDR_PRIV_ENABLE,
                                        MB(0),
                                        // (flash_size >> 1) - KB(16));
                                        flash_size >> 1);
            printf("spif_add_cmd: %#x\n", g_cmd);
            spif_add_cmd(spifilter, g_cmd, 8);
            // spif_dump_rw_addr_privilege_table(spifilter);
            if (HAL_SSI_Transmit(&SsiHandle, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
                while(1);
            }
        } while(0);
        g_cnt_expect++;
        __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == (g_cnt_last + 1)));
    }
}

void test_target_addr(const struct device *const spifilter)
{
    const uint32_t flash_size = MB(16);
    spif_target_addr_config(spifilter, 0, FLAG_TARGET_ADDR_32BIT, true);

    spif_3byte_mode_config(spifilter);
    for(uint32_t i = 0; i < 24; i++) {
        uint32_t addr = BIT(i);
        uint8_t *addr_arr = (uint8_t *)&addr;
        printf("i: %d\n", i);
        printf("addr: %#x\n", addr);
        spif_target_addr_config(spifilter, addr, FLAG_TARGET_ADDR_32BIT, true);
        g_cnt_last = g_cnt;
        do {
            g_cmd = CMD_FAST_READ;
            uint8_t tx_data[] = {g_cmd, addr_arr[2], addr_arr[1], addr_arr[0], 0x4, 0x5a, 0xf7};
            printf("spif_add_cmd: %#x\n", g_cmd);
            spif_add_cmd(spifilter, g_cmd, 8);
            // spif_address_privilege_config(spifilter, FLAG_ADDR_PRIV_READ_SELECT, FLAG_ADDR_PRIV_ENABLE, MB(0), flash_size);
            if (HAL_SSI_Transmit(&SsiHandle, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
                while(1);
            }
        } while(0);
        g_cnt_expect++;
        __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == (g_cnt_last + 1)));
    }
    spif_4byte_mode_config(spifilter);
    for(uint32_t i = 0; i < 32; i++) {
        uint32_t addr = BIT(i);
        uint8_t *addr_arr = (uint8_t *)&addr;
        printf("i: %d\n", i);
        printf("addr: %#x\n", addr);
        spif_target_addr_config(spifilter, addr, FLAG_TARGET_ADDR_32BIT, true);
        g_cnt_last = g_cnt;
        do {
            g_cmd = CMD_FAST_READ;
            uint8_t tx_data[] = {g_cmd, addr_arr[3], addr_arr[2], addr_arr[1], addr_arr[0], 0x4, 0x5a, 0xf7};
            printf("spif_add_cmd: %#x\n", g_cmd);
            spif_add_cmd(spifilter, g_cmd, 8);
            // spif_address_privilege_config(spifilter, FLAG_ADDR_PRIV_READ_SELECT, FLAG_ADDR_PRIV_ENABLE, MB(0), flash_size);
            if (HAL_SSI_Transmit(&SsiHandle, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
                while(1);
            }
        } while(0);
        g_cnt_expect++;
        __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == (g_cnt_last + 1)));
    }
}
