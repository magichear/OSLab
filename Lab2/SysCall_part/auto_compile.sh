#!/bin/sh
gcc -static -o my_get_p_c my_get_p_c.c
gcc -static -o flab2 flab2.c
echo plz check if any problem
echo Enter to continue
echo Ctrl and C to quit
read dummy_variable
sudo cp -f ~/oslab/OSLab/Lab2/SysCall_part/my_get_p_c ~/oslab/busybox-1.32.1/_install/
sudo cp -f ~/oslab/OSLab/Lab2/SysCall_part/flab2 ~/oslab/busybox-1.32.1/_install/
cd ~/oslab/busybox-1.32.1/_install/
find . -print0 | cpio --null -ov --format=newc | gzip -9 > ~/oslab/initramfs-busybox-x64.cpio.gz
cd ~/oslab/linux-4.9.263/
make -j $((`nproc`-1))
