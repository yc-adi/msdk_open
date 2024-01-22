#!/usr/bin/env python

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

from datetime import datetime
import os
import serial
import sys
import threading
import time

cmd_data = [[] for _ in range(9)]


def get_curr_time_str():
    curr = datetime.now()
    time_str = curr.strftime("%H:%M:%S.%f")
    return time_str[:-3]


class ComClass:
    def __init__(self, id, port, baud, name):
        self.id = id
        self.port = port
        self.baud = baud
        self.name = name
        self.comSerial = None
        self.enable = False

        self.open()

        self.run()

    def run(self):
        self.thread1 = threading.Thread(target=self.receiveData)
        self.thread2 = threading.Thread(target=self.sendData)

        self.thread1.daemon = True
        self.thread2.daemon = True

        self.thread1.start()
        self.thread2.start()

    def open(self):
        try:
            self.comSerial = serial.Serial(self.port, self.baud)
            self.enable = True
            print(f"{get_curr_time_str()} --- open serial port {self.name}")
        except:
            print(f"{get_curr_time_str()} --- fail to open serial port {self.name}")
            self.close()

    def close(self):
        self.comSerial.close()
        print(f"{get_curr_time_str()} --- close serial port {self.name}")

    def sendData(self):
        global cmd_data
        while self.enable:
            if len(cmd_data[self.id - 1]) > 0:
                print(f'{get_curr_time_str()} {self.name}< {cmd_data[self.id - 1]}')
                # self.comSerial.write(str(cmd_data[self.id - 1]).encode('UTF-8'))
                encoded = cmd_data[self.id - 1].encode('UTF-8')
                array = bytearray(encoded)
                self.comSerial.write(array)
                cmd_data[self.id - 1] = list()

            time.sleep(0.005)

    def receiveData(self):
        rcvd = list()
        last_data = []

        while self.enable:
            cnt = self.comSerial.inWaiting()
            if cnt != None and cnt > 0:
                try:
                    rcvd = self.comSerial.read(cnt).decode()
                except:
                    pass
                self.comSerial.flushInput()

                if (len(last_data) > 0):
                    last_data.append(rcvd)
                    rcvd = ''.join(last_data)

                if len(rcvd) > 1 and ((rcvd[-2] == '\r' and rcvd[-1] == '\n') or
                                      (len(rcvd) > 3 and rcvd[-3] == '\n' and rcvd[-2] == '>' and rcvd[-1] == ' ')):
                    lines = rcvd.split('\r\n')
                    i = 0
                    for line in lines:
                        i = i + 1
                        print(f'{get_curr_time_str()} {self.name}> {line}')
                    last_data.clear()
                else:
                    last_data.append(rcvd)

            time.sleep(0.002)


def monitor_console_input():
    global cmd_data
    while True:
        input_str = input()

        try:
            if input_str.find('exit') != -1 or input_str.find('quit') != -1:
                print(f"{get_curr_time_str()} --- DONE!")
                os._exit(1)

            temp = input_str.lower()
            input_str = ' '.join(temp.split())  # combine multiple whitespaces

            [id, cmd] = input_str.split(':')
            cmd_data[int(id) - 1] = cmd + '\r\n'
        except:
            pass


if __name__ == '__main__':
    monitor_input_thd = threading.Thread(target=monitor_console_input)
    monitor_input_thd.daemon = True
    monitor_input_thd.start()

    #ser1 = ComClass(1, '/dev/serial/by-id/usb-FTDI_FT230X_Basic_UART_D3074PPH-if00-port0', 115200, " C")  # dats MAX32655 y9
    #ser1 = ComClass(1, '/dev/serial/by-id/usb-FTDI_FT230X_Basic_UART_D3073IDG-if00-port0', 115200, " C")  # datc MAX32655 y1
    #ser2 = ComClass(2, '/dev/serial/by-id/usb-FTDI_FT230X_Basic_UART_D309ZDE9-if00-port0', 115200, "P1")  # datc MAX32655 y2
    #ser2 = ComClass(2, '/dev/serial/by-id/usb-FTDI_FT230X_Basic_UART_D307NU7N-if00-port0', 115200, "P1")  # datc MAX32655 y3
    ser2 = ComClass(2, 'COM5',  115200, "P1")  # dats MAX32655 y2
    #ser1 = ComClass(1, '/dev/serial/by-id/usb-FTDI_FT230X_Basic_UART_D30BLH58-if00-port0', 115200, " C")  # datc MAX32655 y4
    ser1 = ComClass(1, 'COM10', 115200, " C")  # datc MAX32655 y1
    #ser6 = ComClass(6, '/dev/serial/by-id/usb-FTDI_FT230X_Basic_UART_D307NU7M-if00-port0', 115200, "P5")  # datc MAX32655 y5
    #ser7 = ComClass(7, '/dev/serial/by-id/usb-FTDI_FT230X_Basic_UART_D30A1B4P-if00-port0', 115200, "P6")  # datc MAX32655 y6
    #ser8 = ComClass(8, '/dev/serial/by-id/usb-FTDI_FT230X_Basic_UART_D307NU76-if00-port0', 115200, "P7")  # datc MAX32655 y7
    #ser9 = ComClass(9, '/dev/serial/by-id/usb-FTDI_FT230X_Basic_UART_D307NU7L-if00-port0', 115200, "P8")  # datc MAX32655 y8
    
    while True:
        pass

