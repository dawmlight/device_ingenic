#!/bin/bash

set -e

date
echo "build uboot x2000"
echo "sh param:$1,$2,$3"

uboot_src=$1
out_path=$2

make -C ${uboot_src} O=${out_path}/uboot-intermediate smartpen_xImage_msc2

cp ${out_path}/uboot-intermediate/u-boot-with-spl-mbr-gpt.bin ${out_path}/uboot

#exit $?
