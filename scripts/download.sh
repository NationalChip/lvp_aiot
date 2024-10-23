#!/bin/bash

if [ $# -eq 0 ]; then
    echo "Please enter the serial port number!!!"
    echo "eg1: ./download.sh 0"
    echo "eg2: ./download.sh 0 -r 1000000"
    exit
fi

# partition start address
MCU_OFFSET=0x0

# bin file path
MCU_PATH="mcu_nor.bin"

# download command
BOOTXCMD="download ${MCU_OFFSET} ${MCU_PATH};"
BOOTXCMD="$BOOTXCMD reboot;"

if [ $# == 1 ]; then
sudo ./bootx -m grus -c $BOOTXCMD -d /dev/ttyUSB$1
else
sudo ./bootx -m grus -c $BOOTXCMD -d /dev/ttyUSB$1 $2 $3
fi

