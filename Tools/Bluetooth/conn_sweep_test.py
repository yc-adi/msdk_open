#! /usr/bin/env python3

###############################################################################
 #
 # Copyright (C) 2022-2023 Maxim Integrated Products, Inc., All Rights Reserved.
 # (now owned by Analog Devices, Inc.)
 #
 # Permission is hereby granted, free of charge, to any person obtaining a
 # copy of this software and associated documentation files (the "Software"),
 # to deal in the Software without restriction, including without limitation
 # the rights to use, copy, modify, merge, publish, distribute, sublicense,
 # and/or sell copies of the Software, and to permit persons to whom the
 # Software is furnished to do so, subject to the following conditions:
 #
 # The above copyright notice and this permission notice shall be included
 # in all copies or substantial portions of the Software.
 #
 # THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 # OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 # MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 # IN NO EVENT SHALL MAXIM INTEGRATED BE LIABLE FOR ANY CLAIM, DAMAGES
 # OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 # ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 # OTHER DEALINGS IN THE SOFTWARE.
 #
 # Except as contained in this notice, the name of Maxim Integrated
 # Products, Inc. shall not be used except as stated in the Maxim Integrated
 # Products, Inc. Branding Policy.
 #
 # The mere transfer of this software does not imply any licenses
 # of trade secrets, proprietary technology, copyrights, patents,
 # trademarks, maskwork rights, or any other form of intellectual
 # property whatsoever. Maxim Integrated Products, Inc. retains all
 # ownership rights.
 #
 ##############################################################################
 #
 # Copyright 2023 Analog Devices, Inc.
 #
 # Licensed under the Apache License, Version 2.0 (the "License");
 # you may not use this file except in compliance with the License.
 # You may obtain a copy of the License at
 #
 #     http://www.apache.org/licenses/LICENSE-2.0
 #
 # Unless required by applicable law or agreed to in writing, software
 # distributed under the License is distributed on an "AS IS" BASIS,
 # WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 # See the License for the specific language governing permissions and
 # limitations under the License.
 #
 ##############################################################################

## conn_sweep.py
 #
 # Sweep connection parameters.
 #
 # Ensure that both targets are built with BT_VER := 9
 #


from datetime import datetime as dt
import sys
import argparse
from argparse import RawTextHelpFormatter
from time import sleep
import itertools
import json
from mini_RCDAT_USB import mini_RCDAT_USB
from BLE_hci import BLE_hci
from BLE_hci import Namespace
import os
from pprint import pprint
import socket
from subprocess import call, Popen, PIPE, CalledProcessError, STDOUT
import time


# Setup the command line description text
descText = """
Connection sweep.

This tool uses a Mini Circuits RCDAT to control attenuation between two devices
running DTM software. A connection is created and PER data is gathered based on a 
combination of parameters.
"""

# Parse the command line arguments
parser = argparse.ArgumentParser(description=descText, formatter_class=RawTextHelpFormatter)
parser.add_argument('slaveSerial',help='Serial port for slave device')
parser.add_argument('masterSerial',help='Serial port for master device')
parser.add_argument('--mtp', default="", help="master TRACE serial port")
parser.add_argument('--stp', default="", help="slave TRACE serial port")
 
args = parser.parse_args()

print("--------------------------------------------------------------------------------------------")
pprint(vars(args))

# Create the BLE_hci objects
mst = BLE_hci(Namespace(serialPort=args.masterSerial, monPort=args.mtp, baud=115200, id=1))
slv = BLE_hci(Namespace(serialPort=args.slaveSerial,  monPort=args.stp, baud=115200, id=2))

# start the test                
print("\nslv reset")
slv.resetFunc(None)
print("\nmst reset")
mst.resetFunc(None)

sleep(0.2)

slv_addr = "11:22:33:44:55:02"
print(f"\nslv addr {slv_addr}")
slv.addrFunc(Namespace(addr=slv_addr))

mst_addr = "11:22:33:44:55:01"
print(f"\nmst addr {mst_addr}")
mst.addrFunc(Namespace(addr=mst_addr))

sleep(0.2)

print("\nslv start advertising.")
slv.advFunc(Namespace(interval="60", stats="False", connect="True", maintain=False, listen="False"))

print("\nmst start connection.")
mst.initFunc(Namespace(interval="6", timeout="64", addr=slv_addr, stats="False", maintain=False, listen="False"))

print("\nslv and mst wait for HCI events")
slv.listenFunc(Namespace(time=1, stats="False"))
mst.listenFunc(Namespace(time=1, stats="False"))

print("\nSlave and master sinkAclFunc")
slv.sinkAclFunc(None)
mst.sinkAclFunc(None)

print("\nslave listenFunc, 1 sec")
slv.listenFunc(Namespace(time=1, stats="False"))
#mst.listenFunc(Namespace(time=1, stats="False"))

print("\nSlave and master sendAclFunc, slave listenFunc")
slv.sendAclFunc(Namespace(packetLen=str(250), numPackets=str(0)))
mst.sendAclFunc(Namespace(packetLen=str(250), numPackets=str(0)))
slv.listenFunc(Namespace(time=1, stats="False"))

start_secs = time.time()

print("\nReset the packet stats.")
slv.cmdFunc(Namespace(cmd="0102FF00"), timeout=10.0)
mst.cmdFunc(Namespace(cmd="0102FF00"), timeout=10.0)

print("sleep 5")
sleep(5)

print("\nslave read any pending events")
slv.listenFunc(Namespace(time=1, stats="False"))
print("\nmaster read any pending events")
mst.listenFunc(Namespace(time=1, stats="False"))

print("\nMaster collects results.")
perMaster = mst.connStatsFunc(None)
print(f'perMst = {perMaster}')

print("\nSlave collects results.")
perSlave = slv.connStatsFunc(None)                
print(f'perSlv = {perSlave}')

print('--------------------------------------------------------------------------------------------')
print("Reset the devices.")
slv.resetFunc(None)
mst.resetFunc(None)

print("DONE!")
sys.exit(0)
