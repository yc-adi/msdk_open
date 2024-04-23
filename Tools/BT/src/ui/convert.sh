#!/bin/bash
#
#

file_name=$1
file_name=${file_name%.*}

pyuic5.exe -x ${file_name}.ui -o ui_${file_name}.py

