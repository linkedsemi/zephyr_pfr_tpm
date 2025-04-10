void test_enqpi(const struct device *const spifilter)
{
    g_cmd = CMD_QUAD_SPI_MODE_ENTER;

    g_cnt_last = g_cnt;
    do {
        printf("spif_add_cmd_with_dummy: %#x\n", g_cmd);
        spif_add_cmd_with_dummy(spifilter, g_cmd, 0);
        if (HAL_SSI_Transmit(&SsiHandle, &g_cmd, 1) != HAL_OK) {
            while(1);
        }
    } while(0);
    __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == g_cnt_last));
}

void test_exqpi(const struct device *const spifilter)
{
    g_cmd = CMD_QUAD_SPI_MODE_EXIT;

    g_cnt_last = g_cnt;
    do {
        printf("spif_add_cmd_with_dummy: %#x\n", g_cmd);
        spif_add_cmd_with_dummy(spifilter, g_cmd, 0);
        if (HAL_SSI_QPI_Transmit(&SsiHandle, &g_cmd, 1) != HAL_OK) {
            while(1);
        }
    } while(0);
    __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == g_cnt_last));
}

void test_spi_en4b(const struct device *const spifilter)
{
    g_cmd = CMD_4BYTE_MODE_ENTER;

    g_cnt_last = g_cnt;
    do {
        printf("spif_add_cmd_with_dummy: %#x\n", g_cmd);
        spif_add_cmd_with_dummy(spifilter, g_cmd, 0);
        if (HAL_SSI_Transmit(&SsiHandle, &g_cmd, 1) != HAL_OK) {
            while(1);
        }
    } while(0);
    __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == g_cnt_last));
}

void test_spi_ex4b(const struct device *const spifilter)
{
    g_cmd = CMD_4BYTE_MODE_EXIT;

    g_cnt_last = g_cnt;
    do {
        printf("spif_add_cmd_with_dummy: %#x\n", g_cmd);
        spif_add_cmd_with_dummy(spifilter, g_cmd, 0);
        if (HAL_SSI_Transmit(&SsiHandle, &g_cmd, 1) != HAL_OK) {
            while(1);
        }
    } while(0);
    __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == g_cnt_last));
}

void test_qpi_en4b(const struct device *const spifilter)
{
    g_cmd = CMD_4BYTE_MODE_ENTER;

    g_cnt_last = g_cnt;
    do {
        printf("spif_add_cmd_with_dummy: %#x\n", g_cmd);
        spif_add_cmd_with_dummy(spifilter, g_cmd, 0);
        if (HAL_SSI_QPI_Transmit(&SsiHandle, &g_cmd, 1) != HAL_OK) {
            while(1);
        }
    } while(0);
    __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == g_cnt_last));
}

void test_qpi_ex4b(const struct device *const spifilter)
{
    g_cmd = CMD_4BYTE_MODE_EXIT;

    g_cnt_last = g_cnt;
    do {
        printf("spif_add_cmd_with_dummy: %#x\n", g_cmd);
        spif_add_cmd_with_dummy(spifilter, g_cmd, 0);
        if (HAL_SSI_QPI_Transmit(&SsiHandle, &g_cmd, 1) != HAL_OK) {
            while(1);
        }
    } while(0);
    __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == g_cnt_last));
}

void print_bits(const uint8_t *array, size_t size) {
    for (size_t i = 0; i < size; i++) {
        uint8_t byte = array[i];
        for (int j = 0; j <= 7; j++) {
            printf("%c", ((byte >> j) & 1) ? '1' : '0');
        }
        printf(" ");
    }
    printf("\n");
}

void test_general_cmd(const struct device *const spifilter)
{
    for (/*uint8_t*/ g_test_cmd = 0x0; g_test_cmd <= 0xff; g_test_cmd++) {
        spif_remove_cmd(spifilter, g_test_cmd);
        if (g_test_cmd == 0xff) {
            break;
        }
    }
    uint32_t bitmap[SPI_CMD_BITMAPF_LOG_SIZE_U32];
    uint8_t cmds[16] = {};
    for (/*uint8_t*/ g_cmd = 0; g_cmd <= 0; g_cmd++) {
        printf("----------------------------------------------------\n");
        printf("spif_add_general_cmd: %#x ", g_cmd);
        spif_add_general_cmd(spifilter, g_cmd);
        for (uint32_t i = 0; i < 16; i++) {
            cmds[i] = rand() & 0xff;
            spif_add_general_cmd(spifilter, cmds[i]);
            printf("%#x ", cmds[i]);
        }
        printf("\n");
        spif_dump_cmd_table(spifilter);
        // spif_dump_rw_addr_privilege_table(spifilter);
        for (/*uint8_t*/ g_test_cmd = 0x0; g_test_cmd <= 0xff; g_test_cmd++) {
            if ((g_test_cmd == CMD_QUAD_SPI_MODE_ENTER)
               || (g_test_cmd == CMD_QUAD_SPI_MODE_EXIT)) {
                continue;
            }
            bool is_in_white_list = false;
            if (g_test_cmd == g_cmd) {
                is_in_white_list = true;
            } else {
                for (uint32_t j = 0; j < 16; j++) {
                    if (g_test_cmd == cmds[j]) {
                        is_in_white_list = true;
                        break;
                    }
                }
            }
            printf("\n");
            printf("g_test_cmd: %#x\n", g_test_cmd);
            uint8_t tx_data[] = {g_test_cmd, 0, 0, 0, 0, 0x4, 0x5a, 0xf7};
            g_cnt_last = g_cnt;
            if (HAL_SSI_Transmit(&SsiHandle, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
                while(1);
            }
            if (is_in_white_list) {
                __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == g_cnt_last));
            } else {
                g_cnt_expect++;
                __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == (g_cnt_last + 1)));
            }
            spif_dump_cmd_bitmap_log(spifilter, (uint8_t *)bitmap);
            bool hasbit = sys_bitfield_test_bit((mem_addr_t)bitmap, g_test_cmd);
            print_bits((uint8_t *)bitmap, 32);
            __ASSERT_NO_MSG(hasbit == true);
            spif_clear_cmd_bitmap_log(spifilter);
            if (g_test_cmd == 0xff) {
                break;
            }
        }
        uint8_t tx_data[] = {g_cmd, 0, 0, 0, 0, 0x4, 0x5a, 0xf7};
        g_cnt_last = g_cnt;
        if (HAL_SSI_Transmit(&SsiHandle, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
        __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == g_cnt_last));
        spif_remove_cmd(spifilter, g_cmd);
        for (uint32_t i = 0; i < 16; i++) {
            spif_remove_cmd(spifilter, cmds[i]);
        }
        printf("----------------------------------------------------\n");
        if (g_cmd == 0xff) {
            break;
        }
        spif_dump_cmd_table(spifilter);
        // spif_dump_rw_addr_privilege_table(spifilter);
    }
}

void test_general_cmd_qpi_main(const struct device *const spifilter)
{
    for (/*uint8_t*/ g_test_cmd = 0x0; g_test_cmd <= 0xff; g_test_cmd++) {
        spif_remove_cmd(spifilter, g_test_cmd);
        if (g_test_cmd == 0xff) {
            break;
        }
    }
    uint32_t bitmap[SPI_CMD_BITMAPF_LOG_SIZE_U32];
    uint8_t cmds[16] = {};
    for (/*uint8_t*/ g_cmd = 0; g_cmd <= 0; g_cmd++) {
        printf("----------------------------------------------------\n");
        printf("spif_add_general_cmd: %#x ", g_cmd);
        spif_add_general_cmd(spifilter, g_cmd);
        for (uint32_t i = 0; i < 16; i++) {
            cmds[i] = rand() & 0xff;
            spif_add_general_cmd(spifilter, cmds[i]);
            printf("%#x ", cmds[i]);
        }
        printf("\n");
        spif_dump_cmd_table(spifilter);
        // spif_dump_rw_addr_privilege_table(spifilter);
        for (/*uint8_t*/ g_test_cmd = 0x0; g_test_cmd <= 0xff; g_test_cmd++) {
            if ((g_test_cmd == CMD_QUAD_SPI_MODE_ENTER)
               || (g_test_cmd == CMD_QUAD_SPI_MODE_EXIT)) {
                continue;
            }
            bool is_in_white_list = false;
            if (g_test_cmd == g_cmd) {
                is_in_white_list = true;
            } else {
                for (uint32_t j = 0; j < 16; j++) {
                    if (g_test_cmd == cmds[j]) {
                        is_in_white_list = true;
                        break;
                    }
                }
            }
            printf("\n");
            printf("g_test_cmd: %#x\n", g_test_cmd);
            uint8_t tx_data[] = {g_test_cmd, 0, 0, 0, 0, 0x4, 0x5a, 0xf7};
            g_cnt_last = g_cnt;
            if (HAL_SSI_QPI_Transmit(&SsiHandle, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
                while(1);
            }
            if (is_in_white_list) {
                __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == g_cnt_last));
            } else {
                g_cnt_expect++;
                __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == (g_cnt_last + 1)));
            }
            spif_dump_cmd_bitmap_log(spifilter, (uint8_t *)bitmap);
            bool hasbit = sys_bitfield_test_bit((mem_addr_t)bitmap, g_test_cmd);
            print_bits((uint8_t *)bitmap, 32);
            __ASSERT_NO_MSG(hasbit == true);
            spif_clear_cmd_bitmap_log(spifilter);
            if (g_test_cmd == 0xff) {
                break;
            }
        }
        uint8_t tx_data[] = {g_cmd, 0, 0, 0, 0, 0x4, 0x5a, 0xf7};
        g_cnt_last = g_cnt;
        if (HAL_SSI_QPI_Transmit(&SsiHandle, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
        __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == g_cnt_last));
        spif_remove_cmd(spifilter, g_cmd);
        for (uint32_t i = 0; i < 16; i++) {
            spif_remove_cmd(spifilter, cmds[i]);
        }
        printf("----------------------------------------------------\n");
        if (g_cmd == 0xff) {
            break;
        }
        spif_dump_cmd_table(spifilter);
        // spif_dump_rw_addr_privilege_table(spifilter);
    }
}

void test_general_cmd_qpi(const struct device *const spifilter)
{
    test_enqpi(spifilter);
    test_general_cmd_qpi_main(spifilter);
    test_exqpi(spifilter);
}

void test_w_spicmd_saddr3b(const struct device *const spifilter)
{
    const uint32_t flash_size = MB(16);

    g_cnt_last = g_cnt;
    do { /* forbidden */
        g_cmd = CMD_PAGE_PROGRAM;
        uint8_t tx_data[] = {g_cmd, 0x1, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        spif_memset_addr_whitelist(spifilter, 0);
        printf("spif_add_cmd_with_dummy: %#x\n", g_cmd);
        spif_add_cmd_with_dummy(spifilter, g_cmd, 0);
        if (HAL_SSI_Transmit(&SsiHandle, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);
    g_cnt_expect++;
    __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == (g_cnt_last + 1)));

    g_cnt_last = g_cnt;
    do { /* pass */
        g_cmd = CMD_PAGE_PROGRAM;
        uint8_t tx_data[] = {g_cmd, 0x1, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        spif_memset_addr_whitelist(spifilter, 0);
        spif_address_privilege_config(spifilter,
                                    FLAG_ADDR_PRIV_WRITE_SELECT,
                                    FLAG_ADDR_PRIV_ENABLE,
                                    MB(0),
                                    flash_size);
        printf("spif_add_cmd_with_dummy: %#x\n", g_cmd);
        spif_add_cmd_with_dummy(spifilter, g_cmd, 0);
        if (HAL_SSI_Transmit(&SsiHandle, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);
    __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == g_cnt_last));

    g_cnt_last = g_cnt;
    do { /* forbidden */
        g_cmd = CMD_PAGE_PROGRAM;
        uint8_t tx_data[] = {g_cmd, 0x1, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        spif_memset_addr_whitelist(spifilter, 0);
        spif_address_privilege_config(spifilter,
                                    FLAG_ADDR_PRIV_WRITE_SELECT,
                                    FLAG_ADDR_PRIV_ENABLE,
                                    flash_size >> 1,
                                    flash_size);
        printf("spif_add_cmd_with_dummy: %#x\n", g_cmd);
        spif_add_cmd_with_dummy(spifilter, g_cmd, 0);
        if (HAL_SSI_Transmit(&SsiHandle, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);
    g_cnt_expect++;
    __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == (g_cnt_last + 1)));

    g_cnt_last = g_cnt;
    do { /* pass */
        g_cmd = CMD_PAGE_PROGRAM;
        uint8_t tx_data[] = {g_cmd, 0x1, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        spif_memset_addr_whitelist(spifilter, 0);
        spif_address_privilege_config(spifilter,
                                    FLAG_ADDR_PRIV_WRITE_SELECT,
                                    FLAG_ADDR_PRIV_ENABLE,
                                    MB(0),
                                    flash_size >> 1);
        printf("spif_add_cmd_with_dummy: %#x\n", g_cmd);
        spif_add_cmd_with_dummy(spifilter, g_cmd, 0);
        if (HAL_SSI_Transmit(&SsiHandle, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);
    __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == g_cnt_last));

    g_cnt_last = g_cnt;
    do { /* forbidden */
        g_cmd = CMD_PAGE_PROGRAM;
        uint8_t tx_data[] = {g_cmd, 0x7, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        spif_memset_addr_whitelist(spifilter, 0);
        spif_address_privilege_config(spifilter,
                                    FLAG_ADDR_PRIV_WRITE_SELECT,
                                    FLAG_ADDR_PRIV_ENABLE,
                                    flash_size >> 1,
                                    flash_size);
        printf("spif_add_cmd_with_dummy: %#x\n", g_cmd);
        spif_add_cmd_with_dummy(spifilter, g_cmd, 0);
        if (HAL_SSI_Transmit(&SsiHandle, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);
    g_cnt_expect++;
    __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == (g_cnt_last + 1)));

    g_cnt_last = g_cnt;
    do { /* pass */
        g_cmd = CMD_PAGE_PROGRAM;
        uint8_t tx_data[] = {g_cmd, 0x7, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        spif_memset_addr_whitelist(spifilter, 0);
        spif_address_privilege_config(spifilter,
                                    FLAG_ADDR_PRIV_WRITE_SELECT,
                                    FLAG_ADDR_PRIV_ENABLE,
                                    MB(0),
                                    flash_size >> 1);
        printf("spif_add_cmd_with_dummy: %#x\n", g_cmd);
        spif_add_cmd_with_dummy(spifilter, g_cmd, 0);
        if (HAL_SSI_Transmit(&SsiHandle, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);
    __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == g_cnt_last));
}

void test_w_spicmd_saddr4b_by_cmd(const struct device *const spifilter)
{
    const uint32_t flash_size = MB(256);

    g_cnt_last = g_cnt;
    do { /* forbidden */
        g_cmd = CMD_4BYTE_PAGE_PROGRAM;
        uint8_t tx_data[] = {g_cmd, 0x1, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        spif_memset_addr_whitelist(spifilter, 0);
        printf("spif_add_cmd_with_dummy: %#x\n", g_cmd);
        spif_add_cmd_with_dummy(spifilter, g_cmd, 0);
        if (HAL_SSI_Transmit(&SsiHandle, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);
    g_cnt_expect++;
    __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == (g_cnt_last + 1)));

    g_cnt_last = g_cnt;
    do { /* pass */
        g_cmd = CMD_4BYTE_PAGE_PROGRAM;
        uint8_t tx_data[] = {g_cmd, 0x1, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        spif_memset_addr_whitelist(spifilter, 0);
        spif_address_privilege_config(spifilter,
                                    FLAG_ADDR_PRIV_WRITE_SELECT,
                                    FLAG_ADDR_PRIV_ENABLE,
                                    MB(0),
                                    flash_size);
        printf("spif_add_cmd_with_dummy: %#x\n", g_cmd);
        spif_add_cmd_with_dummy(spifilter, g_cmd, 0);
        if (HAL_SSI_Transmit(&SsiHandle, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);
    __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == g_cnt_last));

    g_cnt_last = g_cnt;
    do { /* forbidden */
        g_cmd = CMD_4BYTE_PAGE_PROGRAM;
        uint8_t tx_data[] = {g_cmd, 0x1, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        spif_memset_addr_whitelist(spifilter, 0);
        spif_address_privilege_config(spifilter,
                                    FLAG_ADDR_PRIV_WRITE_SELECT,
                                    FLAG_ADDR_PRIV_ENABLE,
                                    MB(128),
                                    flash_size);
        printf("spif_add_cmd_with_dummy: %#x\n", g_cmd);
        spif_add_cmd_with_dummy(spifilter, g_cmd, 0);
        if (HAL_SSI_Transmit(&SsiHandle, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);
    g_cnt_expect++;
    __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == (g_cnt_last + 1)));

    g_cnt_last = g_cnt;
    do { /* pass */
        g_cmd = CMD_4BYTE_PAGE_PROGRAM;
        uint8_t tx_data[] = {g_cmd, 0x1, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        spif_memset_addr_whitelist(spifilter, 0);
        spif_address_privilege_config(spifilter,
                                    FLAG_ADDR_PRIV_WRITE_SELECT,
                                    FLAG_ADDR_PRIV_ENABLE,
                                    MB(0),
                                    MB(128));
        printf("spif_add_cmd_with_dummy: %#x\n", g_cmd);
        spif_add_cmd_with_dummy(spifilter, g_cmd, 0);
        if (HAL_SSI_Transmit(&SsiHandle, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);
    __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == g_cnt_last));

    g_cnt_last = g_cnt;
    do { /* forbidden */
        g_cmd = CMD_4BYTE_PAGE_PROGRAM;
        uint8_t tx_data[] = {g_cmd, 0x7, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        spif_memset_addr_whitelist(spifilter, 0);
        spif_address_privilege_config(spifilter,
                                    FLAG_ADDR_PRIV_WRITE_SELECT,
                                    FLAG_ADDR_PRIV_ENABLE,
                                    MB(128),
                                    flash_size);
        printf("spif_add_cmd_with_dummy: %#x\n", g_cmd);
        spif_add_cmd_with_dummy(spifilter, g_cmd, 0);
        if (HAL_SSI_Transmit(&SsiHandle, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);
    g_cnt_expect++;
    __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == (g_cnt_last + 1)));

    g_cnt_last = g_cnt;
    do { /* pass */
        g_cmd = CMD_4BYTE_PAGE_PROGRAM;
        uint8_t tx_data[] = {g_cmd, 0x7, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        spif_memset_addr_whitelist(spifilter, 0);
        spif_address_privilege_config(spifilter,
                                    FLAG_ADDR_PRIV_WRITE_SELECT,
                                    FLAG_ADDR_PRIV_ENABLE,
                                    MB(0),
                                    MB(128));
        printf("spif_add_cmd_with_dummy: %#x\n", g_cmd);
        spif_add_cmd_with_dummy(spifilter, g_cmd, 0);
        if (HAL_SSI_Transmit(&SsiHandle, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);
    __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == g_cnt_last));
}

void test_w_spicmd_saddr4b_main(const struct device *const spifilter)
{
    const uint32_t flash_size = MB(256);

    g_cnt_last = g_cnt;
    do { /* forbidden */
        g_cmd = CMD_PAGE_PROGRAM;
        uint8_t tx_data[] = {g_cmd, 0x1, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        spif_memset_addr_whitelist(spifilter, 0);
        printf("spif_add_cmd_with_dummy: %#x\n", g_cmd);
        spif_add_cmd_with_dummy(spifilter, g_cmd, 0);
        if (HAL_SSI_Transmit(&SsiHandle, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);
    g_cnt_expect++;
    __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == (g_cnt_last + 1)));

    g_cnt_last = g_cnt;
    do { /* pass */
        g_cmd = CMD_PAGE_PROGRAM;
        uint8_t tx_data[] = {g_cmd, 0x1, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        spif_memset_addr_whitelist(spifilter, 0);
        spif_address_privilege_config(spifilter,
                                    FLAG_ADDR_PRIV_WRITE_SELECT,
                                    FLAG_ADDR_PRIV_ENABLE,
                                    MB(0),
                                    flash_size);
        printf("spif_add_cmd_with_dummy: %#x\n", g_cmd);
        spif_add_cmd_with_dummy(spifilter, g_cmd, 0);
        if (HAL_SSI_Transmit(&SsiHandle, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);
    __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == g_cnt_last));

    g_cnt_last = g_cnt;
    do { /* forbidden */
        g_cmd = CMD_PAGE_PROGRAM;
        uint8_t tx_data[] = {g_cmd, 0x1, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        spif_memset_addr_whitelist(spifilter, 0);
        spif_address_privilege_config(spifilter,
                                    FLAG_ADDR_PRIV_WRITE_SELECT,
                                    FLAG_ADDR_PRIV_ENABLE,
                                    MB(128),
                                    flash_size);
        printf("spif_add_cmd_with_dummy: %#x\n", g_cmd);
        spif_add_cmd_with_dummy(spifilter, g_cmd, 0);
        if (HAL_SSI_Transmit(&SsiHandle, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);
    g_cnt_expect++;
    __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == (g_cnt_last + 1)));

    g_cnt_last = g_cnt;
    do { /* pass */
        g_cmd = CMD_PAGE_PROGRAM;
        uint8_t tx_data[] = {g_cmd, 0x1, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        spif_memset_addr_whitelist(spifilter, 0);
        spif_address_privilege_config(spifilter,
                                    FLAG_ADDR_PRIV_WRITE_SELECT,
                                    FLAG_ADDR_PRIV_ENABLE,
                                    MB(0),
                                    MB(128));
        printf("spif_add_cmd_with_dummy: %#x\n", g_cmd);
        spif_add_cmd_with_dummy(spifilter, g_cmd, 0);
        if (HAL_SSI_Transmit(&SsiHandle, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);
    __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == g_cnt_last));

    g_cnt_last = g_cnt;
    do { /* forbidden */
        g_cmd = CMD_PAGE_PROGRAM;
        uint8_t tx_data[] = {g_cmd, 0x7, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        spif_memset_addr_whitelist(spifilter, 0);
        spif_address_privilege_config(spifilter,
                                    FLAG_ADDR_PRIV_WRITE_SELECT,
                                    FLAG_ADDR_PRIV_ENABLE,
                                    MB(128),
                                    flash_size);
        printf("spif_add_cmd_with_dummy: %#x\n", g_cmd);
        spif_add_cmd_with_dummy(spifilter, g_cmd, 0);
        if (HAL_SSI_Transmit(&SsiHandle, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);
    g_cnt_expect++;
    __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == (g_cnt_last + 1)));

    g_cnt_last = g_cnt;
    do { /* pass */
        g_cmd = CMD_PAGE_PROGRAM;
        uint8_t tx_data[] = {g_cmd, 0x7, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        spif_memset_addr_whitelist(spifilter, 0);
        spif_address_privilege_config(spifilter,
                                    FLAG_ADDR_PRIV_WRITE_SELECT,
                                    FLAG_ADDR_PRIV_ENABLE,
                                    MB(0),
                                    MB(128));
        printf("spif_add_cmd_with_dummy: %#x\n", g_cmd);
        spif_add_cmd_with_dummy(spifilter, g_cmd, 0);
        if (HAL_SSI_Transmit(&SsiHandle, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);
    __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == g_cnt_last));
}

void test_w_spicmd_saddr4b_by_en4b(const struct device *const spifilter)
{
    test_spi_en4b(spifilter);
    test_w_spicmd_saddr4b_main(spifilter);
    test_spi_ex4b(spifilter);
}

void test_w_spicmd_qaddr3b(const struct device *const spifilter)
{
    const uint32_t flash_size = MB(16);

    g_cnt_last = g_cnt;
    do { /* forbidden */
        g_cmd = CMD_PAGE_PROGRAM_QUAD_ADDR_QUAD_DATA;
        // uint8_t tx_data[] = {g_cmd, 0x1, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        uint8_t tx_data[] = {0x4, 0x5a, 0xf7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd};
        uint64_t addr = 0x010203;
        spif_memset_addr_whitelist(spifilter, 0);
        printf("spif_add_cmd_with_dummy: %#x\n", g_cmd);
        spif_add_cmd_with_dummy(spifilter, g_cmd, 0);
        if (HAL_SSI_Quad_Qaddr_Transmit(&SsiHandle, addr, Addr_Width_24_bits, g_cmd, 0,
                                        tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);
    g_cnt_expect++;
    __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == (g_cnt_last + 1)));

    g_cnt_last = g_cnt;
    do { /* pass */
        g_cmd = CMD_PAGE_PROGRAM_QUAD_ADDR_QUAD_DATA;
        // uint8_t tx_data[] = {g_cmd, 0x1, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        uint8_t tx_data[] = {0x4, 0x5a, 0xf7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd};
        uint64_t addr = 0x010203;
        spif_memset_addr_whitelist(spifilter, 0);
        spif_address_privilege_config(spifilter,
                                    FLAG_ADDR_PRIV_WRITE_SELECT,
                                    FLAG_ADDR_PRIV_ENABLE,
                                    MB(0),
                                    flash_size);
        printf("spif_add_cmd_with_dummy: %#x\n", g_cmd);
        spif_add_cmd_with_dummy(spifilter, g_cmd, 0);
        if (HAL_SSI_Quad_Qaddr_Transmit(&SsiHandle, addr, Addr_Width_24_bits, g_cmd, 0,
                                        tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);
    __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == g_cnt_last));
}

void test_w_spicmd_qaddr4b_by_cmd(const struct device *const spifilter)
{
    const uint32_t flash_size = MB(256);

    g_cnt_last = g_cnt;
    do { /* forbidden */
        g_cmd = CMD_4BYTE_PAGE_PROGRAM_QUAD_ADDR_QUAD_DATA;
        // uint8_t tx_data[] = {g_cmd, 0x1, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        uint8_t tx_data[] = {0x4, 0x5a, 0xf7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd};
        uint64_t addr = 0x01020304;
        spif_memset_addr_whitelist(spifilter, 0);
        printf("spif_add_cmd_with_dummy: %#x\n", g_cmd);
        spif_add_cmd_with_dummy(spifilter, g_cmd, 0);
        if (HAL_SSI_Quad_Qaddr_Transmit(&SsiHandle, addr, Addr_Width_32_bits, g_cmd, 0,
                                            tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);
    g_cnt_expect++;
    __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == (g_cnt_last + 1)));

    g_cnt_last = g_cnt;
    do { /* pass */
        g_cmd = CMD_4BYTE_PAGE_PROGRAM_QUAD_ADDR_QUAD_DATA;
        // uint8_t tx_data[] = {g_cmd, 0x1, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        uint8_t tx_data[] = {0x4, 0x5a, 0xf7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd};
        uint64_t addr = 0x01020304;
        spif_memset_addr_whitelist(spifilter, 0);
        spif_address_privilege_config(spifilter,
                                    FLAG_ADDR_PRIV_WRITE_SELECT,
                                    FLAG_ADDR_PRIV_ENABLE,
                                    MB(0),
                                    flash_size);
        printf("spif_add_cmd_with_dummy: %#x\n", g_cmd);
        spif_add_cmd_with_dummy(spifilter, g_cmd, 0);
        if (HAL_SSI_Quad_Qaddr_Transmit(&SsiHandle, addr, Addr_Width_32_bits, g_cmd, 0,
                                            tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);
    __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == g_cnt_last));
}

void test_w_spicmd_qaddr4b_main(const struct device *const spifilter)
{
    const uint32_t flash_size = MB(256);

    g_cnt_last = g_cnt;
    do { /* forbidden */
        g_cmd = CMD_PAGE_PROGRAM_QUAD_ADDR_QUAD_DATA;
        // uint8_t tx_data[] = {g_cmd, 0x1, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        uint8_t tx_data[] = {0x4, 0x5a, 0xf7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd};
        uint64_t addr = 0x01020304;
        spif_memset_addr_whitelist(spifilter, 0);
        printf("spif_add_cmd_with_dummy: %#x\n", g_cmd);
        spif_add_cmd_with_dummy(spifilter, g_cmd, 0);
        if (HAL_SSI_Quad_Qaddr_Transmit(&SsiHandle, addr, Addr_Width_32_bits, g_cmd, 0,
                                            tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);
    g_cnt_expect++;
    __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == (g_cnt_last + 1)));

    g_cnt_last = g_cnt;
    do { /* pass */
        g_cmd = CMD_PAGE_PROGRAM_QUAD_ADDR_QUAD_DATA;
        // uint8_t tx_data[] = {g_cmd, 0x1, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        uint8_t tx_data[] = {0x4, 0x5a, 0xf7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd};
        uint64_t addr = 0x01020304;
        spif_memset_addr_whitelist(spifilter, 0);
        spif_address_privilege_config(spifilter,
                                    FLAG_ADDR_PRIV_WRITE_SELECT,
                                    FLAG_ADDR_PRIV_ENABLE,
                                    MB(0),
                                    flash_size);
        printf("spif_add_cmd_with_dummy: %#x\n", g_cmd);
        spif_add_cmd_with_dummy(spifilter, g_cmd, 0);
        if (HAL_SSI_Quad_Qaddr_Transmit(&SsiHandle, addr, Addr_Width_32_bits, g_cmd, 0,
                                            tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);
    __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == g_cnt_last));
}

void test_w_spicmd_qaddr4b_by_en4b(const struct device *const spifilter)
{
    test_spi_en4b(spifilter);
    test_w_spicmd_qaddr4b_main(spifilter);
    test_spi_ex4b(spifilter);
}

void test_w_qpi_3b_main(const struct device *const spifilter)
{
    const uint32_t flash_size = MB(16);

    g_cnt_last = g_cnt;
    do { /* forbidden */
        g_cmd = CMD_PAGE_PROGRAM;
        uint8_t tx_data[] = {g_cmd, 0x1, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        spif_memset_addr_whitelist(spifilter, 0);
        printf("spif_add_cmd_with_dummy: %#x\n", g_cmd);
        spif_add_cmd_with_dummy(spifilter, g_cmd, 0);
        if (HAL_SSI_QPI_Transmit(&SsiHandle, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);
    g_cnt_expect++;
    __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == (g_cnt_last + 1)));

    g_cnt_last = g_cnt;
    do { /* pass */
        g_cmd = CMD_PAGE_PROGRAM;
        uint8_t tx_data[] = {g_cmd, 0x1, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        spif_memset_addr_whitelist(spifilter, 0);
        spif_address_privilege_config(spifilter,
                                    FLAG_ADDR_PRIV_WRITE_SELECT,
                                    FLAG_ADDR_PRIV_ENABLE,
                                    MB(0),
                                    flash_size);
        printf("spif_add_cmd_with_dummy: %#x\n", g_cmd);
        spif_add_cmd_with_dummy(spifilter, g_cmd, 0);
        if (HAL_SSI_QPI_Transmit(&SsiHandle, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);
    __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == g_cnt_last));
}

void test_w_qpi_4b_by_cmd_main(const struct device *const spifilter)
{
    const uint32_t flash_size = MB(256);

    g_cnt_last = g_cnt;
    do { /* forbidden */
        g_cmd = CMD_4BYTE_PAGE_PROGRAM;
        uint8_t tx_data[] = {g_cmd, 0x1, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        // uint8_t tx_data[] = {0x4, 0x5a, 0xf7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd};
        // uint64_t addr = 0x01020304;
        spif_memset_addr_whitelist(spifilter, 0);
        printf("spif_add_cmd_with_dummy: %#x\n", g_cmd);
        spif_add_cmd_with_dummy(spifilter, g_cmd, 0);
        if (HAL_SSI_QPI_Transmit(&SsiHandle, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);
    g_cnt_expect++;
    __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == (g_cnt_last + 1)));

    g_cnt_last = g_cnt;
    do { /* pass */
        g_cmd = CMD_4BYTE_PAGE_PROGRAM;
        uint8_t tx_data[] = {g_cmd, 0x1, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        // uint8_t tx_data[] = {0x4, 0x5a, 0xf7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd};
        // uint64_t addr = 0x01020304;
        spif_memset_addr_whitelist(spifilter, 0);
        spif_address_privilege_config(spifilter,
                                    FLAG_ADDR_PRIV_WRITE_SELECT,
                                    FLAG_ADDR_PRIV_ENABLE,
                                    MB(0),
                                    flash_size);
        printf("spif_add_cmd_with_dummy: %#x\n", g_cmd);
        spif_add_cmd_with_dummy(spifilter, g_cmd, 0);
        if (HAL_SSI_QPI_Transmit(&SsiHandle, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);
    __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == g_cnt_last));
}

void test_w_qpi_4b_main(const struct device *const spifilter)
{
    const uint32_t flash_size = MB(256);

    g_cnt_last = g_cnt;
    do { /* forbidden */
        g_cmd = CMD_PAGE_PROGRAM;
        uint8_t tx_data[] = {g_cmd, 0x1, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        // uint8_t tx_data[] = {0x4, 0x5a, 0xf7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd};
        // uint64_t addr = 0x01020304;
        spif_memset_addr_whitelist(spifilter, 0);
        printf("spif_add_cmd_with_dummy: %#x\n", g_cmd);
        spif_add_cmd_with_dummy(spifilter, g_cmd, 0);
        if (HAL_SSI_QPI_Transmit(&SsiHandle, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);
    g_cnt_expect++;
    __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == (g_cnt_last + 1)));

    g_cnt_last = g_cnt;
    do { /* pass */
        g_cmd = CMD_PAGE_PROGRAM;
        uint8_t tx_data[] = {g_cmd, 0x1, 0x2, 0x3, 0x4, 0x5a, 0xf7};
        // uint8_t tx_data[] = {0x4, 0x5a, 0xf7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd};
        // uint64_t addr = 0x01020304;
        spif_memset_addr_whitelist(spifilter, 0);
        spif_address_privilege_config(spifilter,
                                    FLAG_ADDR_PRIV_WRITE_SELECT,
                                    FLAG_ADDR_PRIV_ENABLE,
                                    MB(0),
                                    flash_size);
        printf("spif_add_cmd_with_dummy: %#x\n", g_cmd);
        spif_add_cmd_with_dummy(spifilter, g_cmd, 0);
        if (HAL_SSI_QPI_Transmit(&SsiHandle, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);
    __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == g_cnt_last));
}

void test_w_qpi_3b(const struct device *const spifilter)
{
    test_enqpi(spifilter);
    test_w_qpi_3b_main(spifilter);
    test_exqpi(spifilter);
}

void test_w_qpi_4b_by_en4b(const struct device *const spifilter)
{
    test_enqpi(spifilter);
    test_qpi_en4b(spifilter);
    test_w_qpi_4b_main(spifilter);
    test_qpi_ex4b(spifilter);
    test_exqpi(spifilter);
}

void test_w_qpi_4b_by_cmd(const struct device *const spifilter)
{
    test_enqpi(spifilter);
    test_w_qpi_4b_by_cmd_main(spifilter);
    test_exqpi(spifilter);
}

void test_cmd_erase_addr_3b_overflow(const struct device *const spifilter)
{
    const uint32_t flash_size = MB(16);

    g_cnt_last = g_cnt;
    do { /* forbidden */
        g_cmd = CMD_ERASE_32KB;
        uint8_t tx_data[] = {g_cmd, 0x7f, 0xff, 0xf0, 0x4, 0x5a, 0xf7};
        spif_memset_addr_whitelist(spifilter, 0);
        spif_address_privilege_config(spifilter,
                                    FLAG_ADDR_PRIV_WRITE_SELECT,
                                    FLAG_ADDR_PRIV_ENABLE,
                                    MB(0),
                                    (flash_size >> 1) - KB(16));
        printf("spif_add_cmd_with_dummy: %#x\n", g_cmd);
        spif_add_cmd_with_dummy(spifilter, g_cmd, 0);
        // spif_dump_rw_addr_privilege_table(spifilter);
        if (HAL_SSI_Transmit(&SsiHandle, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);
    g_cnt_expect++;
    __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == (g_cnt_last + 1)));

    g_cnt_last = g_cnt;
    do { /* forbidden */
        g_cmd = CMD_ERASE_64KB;
        uint8_t tx_data[] = {g_cmd, 0x7f, 0xff, 0xf0, 0x4, 0x5a, 0xf7};
        spif_memset_addr_whitelist(spifilter, 0);
        spif_address_privilege_config(spifilter,
                                    FLAG_ADDR_PRIV_WRITE_SELECT,
                                    FLAG_ADDR_PRIV_ENABLE,
                                    MB(0),
                                    (flash_size >> 1) - KB(16));
        printf("spif_add_cmd_with_dummy: %#x\n", g_cmd);
        spif_add_cmd_with_dummy(spifilter, g_cmd, 0);
        // spif_dump_rw_addr_privilege_table(spifilter);
        if (HAL_SSI_Transmit(&SsiHandle, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);
    g_cnt_expect++;
    __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == (g_cnt_last + 1)));
}

void test_cmd_erase_addr_4b_by_cmd_overflow(const struct device *const spifilter)
{
    const uint32_t flash_size = MB(256);

    g_cnt_last = g_cnt;
    do { /* forbidden */
        g_cmd = CMD_4BYTE_ERASE_32KB;
        uint8_t tx_data[] = {g_cmd, 0x7, 0xff, 0xff, 0xf0, 0x4, 0x5a, 0xf7};
        spif_memset_addr_whitelist(spifilter, 0);
        spif_address_privilege_config(spifilter,
                                    FLAG_ADDR_PRIV_WRITE_SELECT,
                                    FLAG_ADDR_PRIV_ENABLE,
                                    MB(0),
                                    (flash_size >> 1) - KB(16));
        printf("spif_add_cmd_with_dummy: %#x\n", g_cmd);
        spif_add_cmd_with_dummy(spifilter, g_cmd, 0);
        // spif_dump_rw_addr_privilege_table(spifilter);
        if (HAL_SSI_Transmit(&SsiHandle, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);
    g_cnt_expect++;
    __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == (g_cnt_last + 1)));

    g_cnt_last = g_cnt;
    do { /* forbidden */
        g_cmd = CMD_4BYTE_ERASE_64KB;
        uint8_t tx_data[] = {g_cmd, 0x7, 0xff, 0xff, 0xf0, 0x4, 0x5a, 0xf7};
        spif_memset_addr_whitelist(spifilter, 0);
        spif_address_privilege_config(spifilter,
                                    FLAG_ADDR_PRIV_WRITE_SELECT,
                                    FLAG_ADDR_PRIV_ENABLE,
                                    MB(0),
                                    (flash_size >> 1) - KB(16));
        printf("spif_add_cmd_with_dummy: %#x\n", g_cmd);
        spif_add_cmd_with_dummy(spifilter, g_cmd, 0);
        // spif_dump_rw_addr_privilege_table(spifilter);
        if (HAL_SSI_Transmit(&SsiHandle, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);
    g_cnt_expect++;
    __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == (g_cnt_last + 1)));
}

void test_cmd_page_program_addr_3b_overflow(const struct device *const spifilter)
{
    const uint32_t flash_size = MB(16);

    g_cnt_last = g_cnt;
    do { /* pass */
        g_cmd = CMD_PAGE_PROGRAM;
        uint8_t tx_data[1 + 3 + 1 + 1] = {g_cmd, 0x7f, 0xff, 0xff, 0x5a, 0xf7};
        spif_memset_addr_whitelist(spifilter, 0);
        spif_address_privilege_config(spifilter,
                                    FLAG_ADDR_PRIV_WRITE_SELECT,
                                    FLAG_ADDR_PRIV_ENABLE,
                                    MB(0),
                                    flash_size >> 1);
        printf("spif_add_cmd_with_dummy: %#x\n", g_cmd);
        spif_add_cmd_with_dummy(spifilter, g_cmd, 0);
        // spif_dump_rw_addr_privilege_table(spifilter);
        if (HAL_SSI_Transmit(&SsiHandle, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);
    __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == g_cnt_last));

    g_cnt_last = g_cnt;
    do { /* forbidden */
        g_cmd = CMD_PAGE_PROGRAM;
        uint8_t tx_data[1 + 3 + 1 + 1] = {g_cmd, 0x80, 0x0, 0x0, 0x5a, 0xf7};
        spif_memset_addr_whitelist(spifilter, 0);
        spif_address_privilege_config(spifilter,
                                    FLAG_ADDR_PRIV_WRITE_SELECT,
                                    FLAG_ADDR_PRIV_ENABLE,
                                    MB(0),
                                    flash_size >> 1);
        printf("spif_add_cmd_with_dummy: %#x\n", g_cmd);
        spif_add_cmd_with_dummy(spifilter, g_cmd, 0);
        // spif_dump_rw_addr_privilege_table(spifilter);
        if (HAL_SSI_Transmit(&SsiHandle, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);
    g_cnt_expect++;
    __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == (g_cnt_last + 1)));
}

void test_cmd_page_program_addr_4b_by_cmd_overflow(const struct device *const spifilter)
{
    const uint32_t flash_size = MB(256);

    g_cnt_last = g_cnt;
    do { /* pass */
        g_cmd = CMD_4BYTE_PAGE_PROGRAM;
        uint8_t tx_data[1 + 4 + 1 + 1] = {g_cmd, 0x7, 0xff, 0xff, 0xff, 0x5a, 0xf7};
        spif_memset_addr_whitelist(spifilter, 0);
        spif_address_privilege_config(spifilter,
                                    FLAG_ADDR_PRIV_WRITE_SELECT,
                                    FLAG_ADDR_PRIV_ENABLE,
                                    MB(0),
                                    flash_size >> 1);
        printf("spif_add_cmd_with_dummy: %#x\n", g_cmd);
        spif_add_cmd_with_dummy(spifilter, g_cmd, 0);
        // spif_dump_rw_addr_privilege_table(spifilter);
        if (HAL_SSI_Transmit(&SsiHandle, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);
    __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == g_cnt_last));
}

void test_w_spicmd_qaddr3b_overflow(const struct device *const spifilter)
{
    const uint32_t flash_size = MB(16);

    g_cnt_last = g_cnt;
    do { /* pass */
        g_cmd = CMD_PAGE_PROGRAM_QUAD_ADDR_QUAD_DATA;
        uint8_t tx_data[] = {0x4, 0x5a, 0xf7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd};
        uint64_t addr = 0x7fffff;
        spif_memset_addr_whitelist(spifilter, 0);
        spif_address_privilege_config(spifilter,
                                    FLAG_ADDR_PRIV_WRITE_SELECT,
                                    FLAG_ADDR_PRIV_ENABLE,
                                    MB(0),
                                    flash_size >> 1);
        printf("spif_add_cmd_with_dummy: %#x\n", g_cmd);
        spif_add_cmd_with_dummy(spifilter, g_cmd, 0);
        if (HAL_SSI_Quad_Qaddr_Transmit(&SsiHandle, addr, Addr_Width_24_bits, g_cmd, 0,
                                        tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);
    __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == g_cnt_last));
}

void test_w_spicmd_qaddr4b_by_cmd_overflow(const struct device *const spifilter)
{
    const uint32_t flash_size = MB(256);

    g_cnt_last = g_cnt;
    do { /* pass */
        g_cmd = CMD_4BYTE_PAGE_PROGRAM_QUAD_ADDR_QUAD_DATA;
        uint8_t tx_data[] = {0x4, 0x5a, 0xf7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd};
        uint64_t addr = 0x7ffffff;
        spif_memset_addr_whitelist(spifilter, 0);
        spif_address_privilege_config(spifilter,
                                    FLAG_ADDR_PRIV_WRITE_SELECT,
                                    FLAG_ADDR_PRIV_ENABLE,
                                    MB(0),
                                    flash_size >> 1);
        printf("spif_add_cmd_with_dummy: %#x\n", g_cmd);
        spif_add_cmd_with_dummy(spifilter, g_cmd, 0);
        if (HAL_SSI_Quad_Qaddr_Transmit(&SsiHandle, addr, Addr_Width_32_bits, g_cmd, 0,
                                            tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);
    __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == g_cnt_last));
}

void test_w_qpi_3b_overflow_main(const struct device *const spifilter)
{
    const uint32_t flash_size = MB(16);

    g_cnt_last = g_cnt;
    do { /* pass */
        g_cmd = CMD_PAGE_PROGRAM;
        uint8_t tx_data[1 + 3 + 1 + 1] = {g_cmd, 0x7f, 0xff, 0xff, 0x5a, 0xf7};
        spif_memset_addr_whitelist(spifilter, 0);
        spif_address_privilege_config(spifilter,
                                    FLAG_ADDR_PRIV_WRITE_SELECT,
                                    FLAG_ADDR_PRIV_ENABLE,
                                    MB(0),
                                    flash_size >> 1);
        printf("spif_add_cmd_with_dummy: %#x\n", g_cmd);
        spif_add_cmd_with_dummy(spifilter, g_cmd, 0);
        // spif_dump_rw_addr_privilege_table(spifilter);
        if (HAL_SSI_QPI_Transmit(&SsiHandle, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);
    __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == g_cnt_last));

    g_cnt_last = g_cnt;
    do { /* forbidden */
        g_cmd = CMD_PAGE_PROGRAM;
        uint8_t tx_data[1 + 3 + 1 + 1] = {g_cmd, 0x80, 0x0, 0x0, 0x5a, 0xf7};
        spif_memset_addr_whitelist(spifilter, 0);
        spif_address_privilege_config(spifilter,
                                    FLAG_ADDR_PRIV_WRITE_SELECT,
                                    FLAG_ADDR_PRIV_ENABLE,
                                    MB(0),
                                    flash_size >> 1);
        printf("spif_add_cmd_with_dummy: %#x\n", g_cmd);
        spif_add_cmd_with_dummy(spifilter, g_cmd, 0);
        // spif_dump_rw_addr_privilege_table(spifilter);
        if (HAL_SSI_QPI_Transmit(&SsiHandle, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);
    g_cnt_expect++;
    __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == (g_cnt_last + 1)));
}

void test_w_qpi_4b_by_cmd_overflow_main(const struct device *const spifilter)
{
    const uint32_t flash_size = MB(256);

    g_cnt_last = g_cnt;
    do { /* pass */
        g_cmd = CMD_4BYTE_PAGE_PROGRAM;
        uint8_t tx_data[1 + 4 + 1 + 1] = {g_cmd, 0x7, 0xff, 0xff, 0xff, 0x5a, 0xf7};
        spif_memset_addr_whitelist(spifilter, 0);
        spif_address_privilege_config(spifilter,
                                    FLAG_ADDR_PRIV_WRITE_SELECT,
                                    FLAG_ADDR_PRIV_ENABLE,
                                    MB(0),
                                    flash_size >> 1);
        printf("spif_add_cmd_with_dummy: %#x\n", g_cmd);
        spif_add_cmd_with_dummy(spifilter, g_cmd, 0);
        // spif_dump_rw_addr_privilege_table(spifilter);
        if (HAL_SSI_QPI_Transmit(&SsiHandle, tx_data, ARRAY_SIZE(tx_data)) != HAL_OK) {
            while(1);
        }
    } while(0);
    __ASSERT_NO_MSG((g_cnt_expect == g_cnt) && (g_cnt == g_cnt_last));
}

void test_w_qpi_3b_overflow(const struct device *const spifilter)
{
    test_enqpi(spifilter);
    test_w_qpi_3b_overflow_main(spifilter);
    test_exqpi(spifilter);
}

void test_w_qpi_4b_overflow(const struct device *const spifilter)
{
    test_enqpi(spifilter);
    test_w_qpi_4b_by_cmd_overflow_main(spifilter);
    test_exqpi(spifilter);
}

void test_cmd_wr_addr_overflow(const struct device *const spifilter)
{
    test_cmd_erase_addr_3b_overflow(spifilter);
    test_cmd_erase_addr_4b_by_cmd_overflow(spifilter);
    test_cmd_page_program_addr_3b_overflow(spifilter);
    test_cmd_page_program_addr_4b_by_cmd_overflow(spifilter);
    test_w_spicmd_qaddr3b_overflow(spifilter);
    test_w_spicmd_qaddr4b_by_cmd_overflow(spifilter);
    test_w_qpi_3b_overflow(spifilter);
    test_w_qpi_4b_overflow(spifilter);
}
