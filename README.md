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
bash install.sh modules/lib/pfr_tpm zephyr
```

4. build
```
cd /data/hunter/zephyr/zephyr_linkedsemi_rgmii_new_fpga_tpm
west build -b lsqsh_evb@runbmc_v3_2os/lsqsh/cpu0 -p always modules/lib/pfr_tpm/samples/spid
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
lsqsh_evb_cpu0: security zephyr
lsqsh_evb_cpu1: application zephyr
- build sample
```
west build -b lsqsh_evb@runbmc_v3_2os/lsqsh/cpu0 -p always modules/lib/pfr_tpm/samples/spid/
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
lsqsh_evb_cpu0: security zephyr
lsqsh_evb_cpu1: application zephyr
- build sample 1
```
west build -b lsqsh_evb@runbmc_v3_2os/lsqsh/cpu0 -p always modules/lib/pfr_tpm/samples/i2c_filter/
```
- build sample 2
```
west build -b lsqsh_evb@runbmc_v3_2os/lsqsh/cpu0 -p always modules/lib/pfr_tpm/samples/i2c_filter_loopback/
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
- board
lsqsh_evb_cpu0: security zephyr
lsqsh_evb_cpu1: application zephyr
- build sample 1
```
west build -b lsqsh_evb@runbmc_v3_2os/lsqsh/cpu0 -p always modules/lib/pfr_tpm/samples/spi_filter/
```
- build sample 2
```
west build -b lsqsh_evb@runbmc_v3_2os/lsqsh/cpu0 -p always modules/lib/pfr_tpm/samples/spi_filter_with_master/
```
