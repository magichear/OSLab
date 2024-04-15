#!/bin/sh
gcc -static -o my_get_p_c my_get_p_c.c
echo plz check if any problem
echo Enter to continue
echo Ctrl and C to quit
read dummy_variable
sudo cp -f ~/oslab/OSlab/Lab2/SysCall_part/my_get_p_c ~/oslab/busybox-1.32.1/_install/
cd ~/oslab/busybox-1.32.1/_install/
find . -print0 | cpio --null -ov --format=newc | gzip -9 > ~/oslab/initramfs-busybox-x64.cpio.gz
cd ~/oslab/linux-4.9.263/
make -j $((`nproc`-1))
