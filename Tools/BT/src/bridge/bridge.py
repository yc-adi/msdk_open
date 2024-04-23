# ******************************************************************************
# Copyright (C) Maxim Integrated Products, Inc., All Rights Reserved.
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
#******************************************************************************
#
#
import sys


class SwitchBridgeCommands:
    MAIN_CMD = ord('B')
    # sub cmds
    I2C = b'I2C'
    SPI = b'SPI'
    UART = b'UART'
    UART0 = b'UART0'
    UART2 = b'UART2'
    IDLE = b'IDLE'


class GPIOCommands:
    MAIN_CMD = ord('G')
    # sub cmds
    CONFIGURE = 0x00
    SET_CLR = 0x01
    GET = 0x02

class OtherCommands:
    MAIN_CMD = ord('O')
    # sub cmds
    ONBOARD_I2C_PULLUP_DISABLE = 0x00
    ONBOARD_I2C_PULLUP_ENABLE = 0x01


class Bridge:
    def __init__(self, comport, printf):
        self.cmd_main_prefix = b'\xDE\xEF\xAA\x55\x23\x41\x16\xDC'
        #
        self.comport = comport
        self.print = printf

    def _send_rcv(self, packet, debug=False):
        try:
            retVal = 0xff
            req = bytearray()
            rsp = bytearray()

            self.comport.open()

            req.extend(self.cmd_main_prefix)
            req.append(len(packet))
            req.extend(packet)

            self.comport.writePacket(req)
            retVal, rsp = self.comport.readPacket(dump=debug)
        except:
            pass
        finally:
            self.comport.close()

        return retVal, rsp

    def switch_bridge_mode(self, mode='I2C'):
        packet = bytearray()
        packet.append(SwitchBridgeCommands.MAIN_CMD)
        packet.extend(mode.encode())
        #
        retVal, rsp = self._send_rcv(packet)

        self.print(rsp.decode('utf-8'))
        return retVal

    def gpio_config(self, gpio0, gpio1):
        packet = bytearray()
        packet.append(GPIOCommands.MAIN_CMD)
        packet.append(GPIOCommands.CONFIGURE)
        packet.append( ((gpio1<<4) | gpio0) & 0xff)
        #
        retVal, rsp = self._send_rcv(packet)

        return retVal

    def gpio_set(self, gpio_num, state):
        packet = bytearray()
        packet.append(GPIOCommands.MAIN_CMD)
        packet.append(GPIOCommands.SET_CLR)
        if gpio_num:
            packet.append(0x1F if state else 0x0F)
        else:
            packet.append(0xF1 if state else 0xF0)
        #
        retVal, rsp = self._send_rcv(packet)

        return retVal

    def gpio_get(self):
        packet = bytearray()
        packet.append(GPIOCommands.MAIN_CMD)
        packet.append(GPIOCommands.GET)
        #
        retVal, rsp = self._send_rcv(packet)

        gpio_0 = 0
        gpio_1 = 0

        if len(rsp) != 1:
            self.print('Invalid Response (hex):' + "-".join("%02x" % b for b in rsp), flush=True)
        else:
            gpio_0 = (rsp[0]>>0) & 0x0f
            gpio_1 = (rsp[0]>>4) & 0x0f

        return retVal, gpio_0, gpio_1

    def on_board_i2c_pullup_status(self, state):
        packet = bytearray()
        packet.append(OtherCommands.MAIN_CMD)
        if state.upper() == 'ENABLE':
            packet.append(OtherCommands.ONBOARD_I2C_PULLUP_ENABLE)
        else:
            packet.append(OtherCommands.ONBOARD_I2C_PULLUP_DISABLE)
        #
        retVal, rsp = self._send_rcv(packet)

        return retVal
