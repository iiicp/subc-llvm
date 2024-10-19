#!/bin/bash
## input 汇编
input="$1"
## output 目标文件
output="$2"
## 汇编
as $input -o tmp.o
## 链接
ld tmp.o -lSystem -L $(xcrun --show-sdk-path -sdk macosx)/usr/lib -o "$output"
## 删除临时文件
rm -rf tmp.o