#!/bin/bash

# vivado -source scripts/project.tcl -tclargs adc_recorder_122_88 xc7z020clg400-1 

make NAME=adc_recorder_122_88 PART=xc7z020clg400-1 bit
