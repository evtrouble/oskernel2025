#!/bin/bash 

# 通常情况下该文件应当放在项目的根目录下

RUNENV_PREFIX=/home/gyl/qemu/
KERNEL_PREFIX=`pwd`

cd $RUNENV_PREFIX

./bin/qemu-system-loongarch64 \
    -M ls2k \
    -serial stdio \
    -k ./share/qemu/keymaps/en-us \
    -kernel ${KERNEL_PREFIX}/kernel-la \
    -serial vc \
    -m 1G \
    -net nic \
    -net user,net=10.0.2.0/24,tftp=/srv/tftp \
    -vnc :0 \
    -hda ${KERNEL_PREFIX}/sdcard-la-final.img \
    # -D ./qemu.log
    -S -s
    # -hdb your_sdcard_img 

    
    # -D ./tmp/qemu.log \
    # -D /dev/null \

    # -device usb-kbd,bus=usb-bus.0 \
    # -device usb-tablet,bus=usb-bus.0 \
    # -device usb-storage,drive=udisk \
    # -drive if=none,id=udisk,file=./tmp/disk \



    #-initrd ${KERNEL_PREFIX}/fs.img \     
# init ram file system 
# echo $RUN_CMD
# $RUN_CMD