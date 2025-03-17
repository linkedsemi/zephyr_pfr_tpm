# pfr and tpm module
<!-- TOC -->

- [pfr and tpm module](#pfr-and-tpm-module)
    - [usage](#usage)
    - [Zephyr BSP](#zephyr-bsp)
            - [spid(tpm spi slave)](#spidtpm-spi-slave)
            - [i2c filter](#i2c-filter)
            - [spi filter(spi monitor)](#spi-filterspi-monitor)

<!-- /TOC -->
## usage
1. git clone project with name `pfr_tpm`
```
cd /data/work/modules/lib/
git clone https://github.com/linkedsemi/zephyr_pfr_tpm.git pfr_tpm
```
2. modify zephyr/west.yml
```
htliu@sr:zephyr$ git diff west.yml
diff --git a/west.yml b/west.yml
index 7b26860a91c..c1be0ddb8fd 100644
--- a/west.yml
+++ b/west.yml
@@ -271,6 +271,8 @@ manifest:
       revision: v1.4.18-stable-fix-build-error
       url: https://github.com/linkedsemi/wolfssh.git
       path: modules/lib/wolfssh
+    - name: pfr_tpm
+      path: modules/lib/pfr_tpm
     - name: hostap
       path: modules/lib/hostap
       revision: 0f7b166487b1ac08e1c6c492383f5c103320b2be
```

3. install *.yaml to zephyr/dts/bindings/misc
```
cd modules/lib/pfr_tpm
bash install.sh /data/work/modules/lib/pfr_tpm /data/work/zephyr
```

4. build
```
cd /data/work
west build -b lsqsh_evb@2os/lsqsh/cpu1 -p always modules/lib/pfr_tpm/samples/spid
```
## Zephyr BSP
#### spid(tpm spi slave)
- driver

| item          | content                                               | description |
|---------------|-------------------------------------------------------|-------------|
| interface     | modules/lib/pfr_tpm/drivers/spid/spid_linkedsemi.h    |             |
| source file   | modules/lib/pfr_tpm/drivers/spid/spid_linkedsemi.c    |             |
| Kconfig file  | modules/lib/pfr_tpm/zephyr/Kconfig.spid               |             |
| configuration | CONFIG_SPID_LINKEDSEMI                                |             |
| yaml file     | modules/lib/pfr_tpm/drivers/spid/linkedsemi,spid.yaml |             |
| sample        | modules/lib/pfr_tpm/samples/spid                      |             |
|               |                                                       |             |
- test plan
```
启动slave程序等待master下发指令
```
- board
lsqsh_evb_cpu1: security zephyr
lsqsh_evb_cpu2: application zephyr
- build sample
```
west build -b lsqsh_evb@2os/lsqsh/cpu1 -p always modules/lib/pfr_tpm/samples/spid/
```
#### i2c filter
- driver

| item          | content                                                        | description                                   |
|---------------|----------------------------------------------------------------|-----------------------------------------------|
| interface     | modules/lib/pfr_tpm/drivers/pfr/i2c/i2c_filter.h               |                                               |
| source file   | modules/lib/pfr_tpm/drivers/pfr/i2c/i2c_filter.c               |                                               |
| Kconfig file  | modules/lib/pfr_tpm/zephyr/Kconfig.i2cfilter                   |                                               |
| configuration | CONFIG_I2C_FILTER_LINKEDSEMI                                   |                                               |
| yaml file     | modules/lib/pfr_tpm/drivers/pfr/i2c/linkedsemi,i2c-filter.yaml |                                               |
| sample 1      | modules/lib/pfr_tpm/samples/i2c_filter                         | 仅开启i2c filter                              |
| sample 2      | modules/lib/pfr_tpm/samples/i2c_filter_loopback                | i2c master + i2c filter + i2c slave的环回测试 |
|               |                                                                |                                               |
- test plan 1
modules/lib/pfr_tpm/samples/i2c_filter
```
启动slave程序等待master下发指令
```
- test plan 2
modules/lib/pfr_tpm/samples/i2c_filter_loopback
```
i2c master对i2c filter发送波形，i2c slave负责ack让master流程走下去，观察波形和i2c filter上报中断
```
- board
lsqsh_evb_cpu1: security zephyr
lsqsh_evb_cpu2: application zephyr
- build sample 1
```
west build -b lsqsh_evb@2os/lsqsh/cpu1 -p always modules/lib/pfr_tpm/samples/i2c_filter/
```
- build sample 2
```
west build -b lsqsh_evb@2os/lsqsh/cpu1 -p always modules/lib/pfr_tpm/samples/i2c_filter_loopback/
```
#### spi filter(spi monitor)
- driver

| item            | content                                                        | description                                   |
|-----------------|----------------------------------------------------------------|-----------------------------------------------|
| interface       | modules/lib/pfr_tpm/drivers/pfr/spi/spi_filter.h               |                                               |
| source file 1   | modules/lib/pfr_tpm/drivers/pfr/spi/spi_filter.c               |                                               |
| source file 2   | modules/lib/pfr_tpm/drivers/pfr/spi/spi_filter_shell.c         |                                               |
| Kconfig file    | modules/lib/pfr_tpm/zephyr/Kconfig.spifilter                   |                                               |
| configuration 1 | CONFIG_SPI_FILTER_LINKEDSEMI                                   |                                               |
| configuration 2 | CONFIG_SPI_FILTER_LINKEDSEMI_SHELL                             |                                               |
| yaml file       | modules/lib/pfr_tpm/drivers/pfr/spi/linkedsemi,spi-filter.yaml |                                               |
| sample 1        | modules/lib/pfr_tpm/samples/spi_filter                         | 仅开启spi filter                              |
| sample 2        | modules/lib/pfr_tpm/samples/spi_filter_with_master             | spi master + spi filter + spi slave的环回测试 |
|                 |                                                                |                                               |
- test plan 1
modules/lib/pfr_tpm/samples/spi_filter
```
启动slave程序等待master下发指令
```
- test plan 2
modules/lib/pfr_tpm/samples/spi_filter_with_master
```
spi master对spi filter发送波形，观察波形和spi filter上报中断
```
- ip lock/unlock
```
spif_reg_lock()
```
```
spif_reg_unlock()
```
- dump command table
```
spif_dump_cmd_table()
```
- dump read/write address table
```
spif_dump_rw_addr_privilege_table()
```
- dump bitmap log
```
spif_dump_cmd_bitmap_log()
```
- write

| | spi | qpi |
|-|-|-|
| cmd only | `test_general_cmd()`<br>(except ENQPI) | `test_general_cmd_qpi()` {<br> test_enqpi()<br>test_general_cmd_qpi_main()<br>test_exqpi() <br>}<br>(except EXQPI) |
| cmd + 3byte single line addr<br>`CMD_PAGE_PROGRAM` `02h` | `test_w_spicmd_saddr3b()`<br>(except EN4B) |
| cmd + 4byte single line addr<br>`CMD_4BYTE_PAGE_PROGRAM` `12h` | `test_w_spicmd_saddr4b()` {<br> test_w_spi_en4b()<br>test_w_spicmd_saddr4b_main()<br>test_w_spi_ex4b() <br>}<br>(except EX4B) |
| cmd + 3byte dual line addr | no write cmd support
| cmd + 4byte dual line addr | no write cmd support
| cmd + 3byte quad line addr<br>`CMD_PAGE_PROGRAM_QUAD_ADDRESS_QUAD_DATA` `38h` | `test_w_spicmd_qaddr3b()`<br>(except EN4B) | `test_w_qpi_3b` {<br> test_enqpi()<br>test_w_qpi_3b_main()<br>test_exqpi() <br>}<br>(except EN4B) |
| cmd + 4byte quad line addr<br>`CMD_4BYTE_PAGE_PROGRAM_QUAD_ADDRESS_QUAD_DATA` `3eh` | `test_w_spicmd_qaddr4b()` {<br> test_w_spi_en4b()<br>test_w_spicmd_qaddr4b_main()<br>test_w_spi_ex4b() <br>}<br>(except EX4B) | `test_w_qpi_4b()` {<br> test_enqpi()<br>test_w_qpi_en4b()<br>test_w_qpi_4b_main()<br>test_w_qpi_ex4b()<br>test_exqpi() <br>}<br>(except EX4B) |
|

- read

| | spi | qpi |
|-|-|-|
| cmd only(like write) | `test_general_cmd()`<br>(except ENQPI) | `test_general_cmd_qpi()` {<br> test_enqpi()<br>test_general_cmd_qpi_main()<br>test_exqpi() <br>}<br>(except EXQPI) |
| cmd + 3byte single line addr<br>`CMD_FAST_READ` `0bh` | `test_r_spicmd_saddr3b()`<br>(except EN4B) |
| cmd + 4byte single line addr<br>`CMD_4BYTE_FAST_READ` `0ch` | `test_r_spicmd_saddr4b()` {<br> test_r_spi_en4b()<br>test_r_spicmd_saddr4b_main()<br>test_r_spi_ex4b() <br>}<br>(except EX4B) |
| cmd + 3byte dual line addr<br>`CMD_READ_DUAL_ADDR_DUAL_DATA` `bbh` | `test_r_spicmd_daddr3b()`<br>(except EN4B) |
| cmd + 4byte dual line addr<br>`CMD_4BYTE_READ_DUAL_ADDR_DUAL_DATA` `bch` | `test_r_spicmd_daddr4b()` {<br> test_r_spi_en4b()<br>test_r_spicmd_daddr4b_main()<br>test_r_spi_ex4b() <br>}<br>(except EX4B) |
| cmd + 3byte quad line addr<br>`CMD_READ_QUAD_ADDRESS_QUAD_DATA` `ebh` | `test_r_spicmd_qaddr3b()`<br>(except EN4B) | `test_r_qpi_3b` {<br> test_enqpi()<br>test_r_qpi_3b_main()<br>test_exqpi() <br>}<br>(except EN4B) |
| cmd + 4byte quad line addr<br>`CMD_4BYTE_READ_QUAD_ADDRESS_QUAD_DATA` `ech` | `test_r_spicmd_qaddr4b()` {<br> test_r_spi_en4b()<br>test_r_spicmd_qaddr4b_main()<br>test_r_spi_ex4b() <br>}<br>(except EX4B) | `test_r_qpi_4b()` {<br> test_enqpi()<br>test_r_qpi_en4b()<br>test_r_qpi_4b_main()<br>test_r_qpi_ex4b()<br>test_exqpi() <br>}<br>(except EX4B) |
|
- dma log
```
spif_dma_config()
```
- dump dma log
```
spif_log_dma_buf()
```
- sck check
```
spif_clk_check_config()
```
- target address detect
```
spif_target_addr_config()
```

- board
lsqsh_evb_cpu1: security zephyr
lsqsh_evb_cpu2: application zephyr
- build sample 1
```
west build -b lsqsh_evb@2os/lsqsh/cpu1 -p always modules/lib/pfr_tpm/samples/spi_filter/
```
- build sample 2
```
west build -b lsqsh_evb@2os/lsqsh/cpu1 -p always modules/lib/pfr_tpm/samples/spi_filter_with_master/
```
