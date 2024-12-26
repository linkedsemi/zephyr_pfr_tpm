# pfr and tpm module
1. git clone project with name `pfr_tpm`
```
git clone https://github.com/linkedsemi/zephyr_pfr_tpm.git pfr_tpm
```
2. modify zephyr/west.yml
```
htliu@sr:/data/hunter/zephyr/zephyr_linkedsemi_rgmii_new_fpga_tpm/zephyr$ git diff west.yml
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

2. install *.yaml to zephyr/dts/bindings/misc
```
cd /data/hunter/zephyr/zephyr_linkedsemi_rgmii_new_fpga_tpm/modules/lib/pfr_tpm
bash install.sh /data/hunter/zephyr/zephyr_linkedsemi_rgmii_new_fpga_tpm/modules/lib/pfr_tpm /data/hunter/zephyr/zephyr_linkedsemi_rgmii_new_fpga_tpm/zephyr
```

3. build
```
cd /data/hunter/zephyr/zephyr_linkedsemi_rgmii_new_fpga_tpm
west build -b lsqsh_evb@runbmc_v3_2os/lsqsh/cpu0 -p always modules/lib/pfr_tpm/samples/spid
```
