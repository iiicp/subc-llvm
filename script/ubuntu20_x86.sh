#!/bin/bash

## 需要先执行这个命令，安装gcc的环境 
# sudo apt install gcc-multilib

## input 汇编
input="$1"
## output 目标文件
output="$2"
## 汇编
as $input -o tmp.o
## 链接 
# export LIBRARY_PATH=/usr/lib/x86_64-linux-gnu:$LIBRARY_PATH
# ld tmp.o /usr/lib/x86_64-linux-gnu/crt1.o /usr/lib/x86_64-linux-gnu/crti.o /usr/lib/x86_64-linux-gnu/crtn.o /usr/lib/gcc/x86_64-linux-gnu/11/crtbegin.o /usr/lib/gcc/x86_64-linux-gnu/11/crtend.o -L "/usr/lib/gcc/x86_64-linux-gnu/11" "-L/usr/lib/x86_64-linux-gnu"  -lm -lgcc_s -lc -lgcc -o $output
gcc tmp.o -o $output
## 删除临时文件
rm -rf tmp.o