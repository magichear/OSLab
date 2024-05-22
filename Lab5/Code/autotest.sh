#!/bin/bash

# 自动测试
# 每次执行都会自动编译
# 包括 make clean 与删除重建 /test/fat16 目录
sh ./test/run_test.sh

echo plz check if any problem
echo Enter to continue
echo Ctrl and C to quit
read dummy_variable0

make clean


# 调试
# 第一个终端执行   ./test/run_mount.sh
# 另开一个终端执行 ./test/run_test_alone.sh