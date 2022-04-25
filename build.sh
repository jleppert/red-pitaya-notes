#!/bin/bash

# vivado -source scripts/project.tcl -tclargs adc_recorder_122_88 xc7z020clg400-1 

make PART=xc7z020clg400-1 boot.bin
make PART=xc7z020clg400-1 uImage
make PART=xc7z020clg400-1 devicetree.dtb
sudo sh scripts/image.sh scripts/ubuntu.sh red-pitaya-ubuntu-20.04-armhf.img 1024
make NAME=adc_recorder_122_88 PART=xc7z020clg400-1 bit
