if(CONFIG_SPID_LINKEDSEMI)
  zephyr_include_directories(
    ${ZEPHYR_CURRENT_MODULE_DIR}/include
    ${ZEPHYR_CURRENT_MODULE_DIR}/drivers/spid/
  )

  zephyr_library_include_directories(
    ${ZEPHYR_CURRENT_MODULE_DIR}/include
    ${ZEPHYR_CURRENT_MODULE_DIR}/drivers/spid/
  )

  zephyr_library_sources(
    ${ZEPHYR_CURRENT_MODULE_DIR}/drivers/spid/spid_linkedsemi.c
  )

  # riscv64-unknown-elf-objdump -d -x -z  zephyr.elf > zephyr.elf.asm
endif()

if(CONFIG_I2C_FILTER_LINKEDSEMI)
  zephyr_include_directories(
    ${ZEPHYR_CURRENT_MODULE_DIR}/include
    ${ZEPHYR_CURRENT_MODULE_DIR}/drivers/pfr/i2c/
  )

  zephyr_library_include_directories(
    ${ZEPHYR_CURRENT_MODULE_DIR}/include
    ${ZEPHYR_CURRENT_MODULE_DIR}/drivers/pfr/i2c/
  )

  zephyr_library_sources(
    ${ZEPHYR_CURRENT_MODULE_DIR}/drivers/pfr/i2c/i2c_filter.c
  )

  # riscv64-unknown-elf-objdump -d -x -z  zephyr.elf > zephyr.elf.asm
endif()

if(CONFIG_SPI_FILTER_LINKEDSEMI)
  zephyr_include_directories(
    ${ZEPHYR_CURRENT_MODULE_DIR}/include
    ${ZEPHYR_CURRENT_MODULE_DIR}/drivers/pfr/spi/
  )

  zephyr_library_include_directories(
    ${ZEPHYR_CURRENT_MODULE_DIR}/include
    ${ZEPHYR_CURRENT_MODULE_DIR}/drivers/pfr/spi/
  )

  zephyr_library_sources(
    ${ZEPHYR_CURRENT_MODULE_DIR}/drivers/pfr/spi/spi_filter.c
  )

  # riscv64-unknown-elf-objdump -d -x -z  zephyr.elf > zephyr.elf.asm
endif()
