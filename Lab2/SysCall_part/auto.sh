#!/bin/sh
sh auto_compile.sh
echo plz check if any problem
echo Enter to continue
echo Ctrl and C to quit
read dummy_variable
sh auto_copycode.sh
sh runqemu.sh
