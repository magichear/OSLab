#!/bin/sh
cp -f ~/oslab/linux-4.9.263/arch/x86/entry/syscalls/syscall_64.tbl ~/oslab/OSLab/Lab2/SysCall_part/tmp/
cp -f ~/oslab/linux-4.9.263/include/linux/syscalls.h ~/oslab/OSLab/Lab2/SysCall_part/tmp/
cp -f ~/oslab/linux-4.9.263/kernel/sys.c ~/oslab/OSLab/Lab2/SysCall_part/tmp/
cp -rf ~/oslab/myconfig ~/oslab/OSLab/Lab2/SysCall_part/tmp/
cp -f ~/oslab/OSLab/Lab2/SysCall_part/my_get_p_c.c ~/oslab/OSLab/Lab2/SysCall_part/tmp/
