#!/bin/bash

help(){
    echo "help:"
    echo "$ bash install.sh /data/work/modules/lib/pfr_tpm /data/work/zephyr"
}

if [ -z "$1" ];then
    help
    exit
fi
if [ -z "$2" ];then
    help
    exit
fi
if [ ! -d "$1" ];then
    help
    exit
fi
if [ ! -d "$2" ];then
    help
    exit
fi

ZEPHYR_CURRENT_MODULE_DIR=$1
ZEPHYR_BASE=$2
find ${ZEPHYR_CURRENT_MODULE_DIR}/drivers -name "*.yaml" -exec cp {} ${ZEPHYR_BASE}/dts/bindings/misc/ \;
