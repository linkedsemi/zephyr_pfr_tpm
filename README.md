# pfr and tpm module
1. install *.yaml
```
cd /data/hunter/zephyr/zephyr_linkedsemi_rgmii_new_fpga_tpm/modules/lib/pfr_tpm
bash install.sh /data/hunter/zephyr/zephyr_linkedsemi_rgmii_new_fpga_tpm/modules/lib/pfr_tpm /data/hunter/zephyr/zephyr_linkedsemi_rgmii_new_fpga_tpm/zephyr
```
2. build
```
cd /data/hunter/zephyr/zephyr_linkedsemi_rgmii_new_fpga_tpm
west build -b lsqsh_evb@runbmc_v3_2os/lsqsh/cpu0 -p always modules/lib/pfr_tpm/samples/spid
```
