#!/bin/sh
cp -f ~/oslab/OSLab/Lab2/SysCall_part/tmp/syscall_64.tbl ~/oslab/linux-4.9.263/arch/x86/entry/syscalls/
cp -f ~/oslab/OSLab/Lab2/SysCall_part/tmp/syscalls.h ~/oslab/linux-4.9.263/include/linux/
cp -f ~/oslab/OSLab/Lab2/SysCall_part/tmp/sys.c ~/oslab/linux-4.9.263/kernel/
cp -rf ~/oslab/OSLab/Lab2/SysCall_part/tmp/myconfig ~/oslab/
