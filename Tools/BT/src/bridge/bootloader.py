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
import os
import struct
import time
import math

from bridge.bootloader_cmd import *
from bridge.bridge_settings import *


class BootloaderProtocol:
    START_CMD = ord('S')
    STOP_CMD = ord('P')

    BL_WRITE = 0
    BL_READ = 1

    def __init__(self, comport, printf):
        #
        self.comport = comport
        self.print = printf

    def _bl_read(self, length, debug=False):
        packet = bytearray()
        packet.append(self.START_CMD)
        packet.append((BridgeSettings.i2c_addr<<1) + self.BL_READ)   # addr
        packet.extend(length.to_bytes(2, byteorder='big'))  # add len
        packet.append(self.STOP_CMD)
        # send
        if debug:
            print('BL Read :', end=' ')
        self.comport.writePacket(packet)
        retVal, rsp = self.comport.readPacket(dump=debug)

        return retVal, rsp

    def _bl_write(self, cmd, debug=False):
        packet = bytearray()
        packet.append(self.START_CMD)
        packet.append((BridgeSettings.i2c_addr<<1) + self.BL_WRITE)   # addr
        packet.extend(len(cmd).to_bytes(2, byteorder='big'))  # add len
        packet.extend(cmd)
        packet.append(self.STOP_CMD)
        # send
        if debug:
            print('BL Write:', end=' ')
        self.comport.writePacket(packet)
        if len(cmd) > 1024: # wait a little before reading response
            time.sleep(0.3)
        retVal, rsp = self.comport.readPacket(dump=debug)

        return retVal, rsp

    def sleep_ms(self, ms):
        time.sleep(ms/1000)

    def bl_send_rcv(self, cmd, rxLen, delay_ms=0, delay_step_ms=100, debug=False):
        try:
            retVal = 0
            rsp = bytearray(b'0xff')

            self.comport.open()

            if BridgeSettings.bridge_mode.startswith("UART"):
                retVal = self.comport.write(cmd)
                self.comport.flush()
            else:
                for i in range(2):
                    retVal, rsp = self._bl_write(cmd, debug)
                    if retVal == len(cmd):
                        break
                    self.sleep_ms(100)
                    if debug:
                        print("Write Retry:{}, retVal:{}".format(i+1, retVal), flush=True)

            if retVal == len(cmd):
                nb_of_retry = int(delay_ms / delay_step_ms) + 1
                for i in range(nb_of_retry):
                    if delay_ms:
                        self.sleep_ms(delay_step_ms)

                    if BridgeSettings.bridge_mode.startswith("UART"):
                        retVal, rsp = rxLen, self.comport.read(rxLen)
                    else:
                        retVal, rsp = self._bl_read(rxLen, debug)

                    if (retVal == rxLen) and (rsp[0] != BLRetVal.ERR_TRY_AGAIN):
                        break
                    if debug:
                        print("Read Retry:{}, retVal:{}, rsp[0]:{}".format(i+1, retVal, rsp[0]), flush=True)
        except:
            pass
        finally:
            self.comport.close()

        return retVal, rsp


class Bootloader(BootloaderProtocol):
    def __init__(self, comport, printf):
        #
        super(Bootloader, self).__init__(comport, printf)
        # default i2c
        self.bl_config = None
        self.print = printf

    def enter_bootloader(self):
        cmd = bytearray()
        cmd.append(BLCmdDevSetMode.MAIN_CMD)
        cmd.append(BLCmdDevSetMode.SET_MODE)
        cmd.append(0x08)
        #
        retVal, rsp = self.bl_send_rcv(cmd, 1)
        return 0 if rsp[0] == BLRetVal.BTLDR_SUCCESS else rsp[0]

    def exit_bootloader(self):
        cmd = bytearray()
        cmd.append(BLCmdDevSetMode.MAIN_CMD)
        cmd.append(BLCmdDevSetMode.SET_MODE)
        cmd.append(0x00)
        #
        retVal, rsp = self.bl_send_rcv(cmd, 1)
        return 0 if rsp[0] == BLRetVal.BTLDR_SUCCESS else rsp[0]

    def get_page_size(self):
        cmd = bytearray()
        cmd.append(BLCmdInfo.MAIN_CMD)
        cmd.append(BLCmdInfo.GET_PAGE_SIZE)
        #
        retVal, rsp = self.bl_send_rcv(cmd, 3)
        retVal = 0 if rsp[0] == BLRetVal.BTLDR_SUCCESS else rsp[0]

        page_size = 0
        if len(rsp) == 3:
            bl_ret, page_size = struct.unpack('>BH', rsp)

        return retVal, page_size

    def set_num_pages(self, page_count):
        cmd = bytearray()
        cmd.append(BLCmdFlash.MAIN_CMD)
        cmd.append(BLCmdFlash.SET_NUM_PAGES)
        #
        cmd = cmd + page_count.to_bytes(2, byteorder='big')
        retVal, rsp = self.bl_send_rcv(cmd, 1)
        return 0 if rsp[0] == BLRetVal.BTLDR_SUCCESS else rsp[0]

    def get_usn(self):
        cmd = bytearray()
        cmd.append(BLCmdInfo.MAIN_CMD)
        cmd.append(BLCmdInfo.GET_USN)
        #
        retVal, rsp = self.bl_send_rcv(cmd, 25)
        retVal = 0 if rsp[0] == BLRetVal.BTLDR_SUCCESS else rsp[0]

        usn = rsp[1:]
        return retVal, usn

    def get_target_type(self):
        cmd = bytearray()
        cmd.append(BLCmdDeviceInfo.MAIN_CMD)
        cmd.append(BLCmdDeviceInfo.GET_PLATFORM_TYPE)
        #
        retVal, rsp = self.bl_send_rcv(cmd, 2)
        retVal = 0 if rsp[0] == BLRetVal.BTLDR_SUCCESS else rsp[0]

        target = 'Error'
        if len(rsp) == 2:
            if rsp[1] == 1:
                target = 'MAX32660'
            elif rsp[1] == 3:
                target = 'MAX32670'
            elif rsp[1] == 5:
                target = 'MAX78000'
            elif rsp[1] == 6:
                target = 'MAX32655'
            elif rsp[1] == 9:
                target = 'MAX32672'
            else:
                target = f'{rsp[1]}'

        BridgeSettings.target = target
        return retVal, target

    def get_target_version(self):
        cmd = bytearray()
        cmd.append(BLCmdInfo.MAIN_CMD)
        cmd.append(BLCmdInfo.GET_VERSION)
        #
        retVal, rsp = self.bl_send_rcv(cmd, 4)
        retVal = 0 if rsp[0] == BLRetVal.BTLDR_SUCCESS else rsp[0]

        version = 'Error'
        if len(rsp) == 4:
            version = f'v{rsp[1]}.{rsp[2]}.{rsp[3]}'

        BridgeSettings.target_bl_version = version
        return retVal, version

    def set_iv(self, iv):
        cmd = bytearray()
        cmd.append(BLCmdFlash.MAIN_CMD)
        cmd.append(BLCmdFlash.SET_IV)
        #
        retVal, rsp = self.bl_send_rcv(cmd + iv, 1)
        return 0 if rsp[0] == BLRetVal.BTLDR_SUCCESS else rsp[0]

    def set_auth_bytes(self, auth):
        cmd = bytearray()
        cmd.append(BLCmdFlash.MAIN_CMD)
        cmd.append(BLCmdFlash.SET_AUTH)
        #
        retVal, rsp = self.bl_send_rcv(cmd + auth, 1)
        return 0 if rsp[0] == BLRetVal.BTLDR_SUCCESS else rsp[0]

    def erase_app(self):
        cmd = bytearray()
        cmd.append(BLCmdFlash.MAIN_CMD)
        cmd.append(BLCmdFlash.ERASE_APP_MEMORY)
        #
        retVal, rsp = self.bl_send_rcv(cmd, 1, delay_ms=2000, delay_step_ms=1000)
        return 0 if rsp[0] == BLRetVal.BTLDR_SUCCESS else rsp[0]

    def write_page(self, page_data):
        # page data have to include checsum too
        cmd = bytearray()
        cmd.append(BLCmdFlash.MAIN_CMD)
        cmd.append(BLCmdFlash.WRITE_PAGE)
        #
        retVal, rsp = self.bl_send_rcv(cmd + page_data, 1, delay_ms=1000)
        return 0 if rsp[0] == BLRetVal.BTLDR_SUCCESS else rsp[0]

    def load_aes_key(self, key, aad):
        if len(key) not in (16, 24, 32):
            self.print('Wrong Key Length {}'.format(len(key)))
            return -1  # wrong key len

        if len(aad) not in (16, 24, 32):
            self.print('Wrong AAD Length {}'.format(len(aad)))
            return -1  # wrong key len

        # Add command
        cmd = bytearray()
        cmd.append(BLCmdFlash.MAIN_CMD)
        cmd.append(BLCmdFlash.SET_KEY_AAD)
        # Add key
        cmd.append(len(key))
        cmd.extend(key)
        for i in range(len(key), 32):
            cmd.append(0)
        # Add aad
        cmd.append(len(aad))
        cmd.extend(aad)
        for i in range(len(aad), 32):
            cmd.append(0)

        retVal, rsp = self.bl_send_rcv(cmd, 1, delay_ms=2000, delay_step_ms=500)
        return 0 if rsp[0] == BLRetVal.BTLDR_SUCCESS else rsp[0]

    def get_bl_configure(self, target, bl_version):
        cmd = bytearray()
        cmd.append(BLCmdConfigRead.MAIN_CMD)
        cmd.append(BLCmdConfigRead.CMDIDX_R_ALL_CFG)
        cmd.append(0x00) # dummy byte
        #
        if bl_version >= 'v3.4.2':
            self.bl_config = BLConfig()
            retVal, rsp = self.bl_send_rcv(cmd, 9)
            if len(rsp) == 9:
                self.bl_config.cfg, = struct.unpack('>Q', rsp[1:])
        else: #elif bl_version <= 'v3.4.1':
            self.bl_config = BLConfig_v341()
            retVal, rsp = self.bl_send_rcv(cmd, 5)
            if len(rsp) == 5:
                self.bl_config.cfg, = struct.unpack('>I', rsp[1:])

        retVal = 0 if rsp[0] == BLRetVal.BTLDR_SUCCESS else rsp[0]
        return retVal, self.bl_config.b

    def _update_bl_cfg_item(self, item, req):
        cmd = bytearray()
        cmd.append(BLCmdConfigWrite.MAIN_CMD)
        cmd.append(BLCmdConfigWrite.ENTRY_CONFIG)
        cmd.append(item)
        cmd.append(req)
        #
        retVal, rsp = self.bl_send_rcv(cmd, 1)
        return 0 if rsp[0] == BLRetVal.BTLDR_SUCCESS else rsp[0]

    def update_cfg_enter_bl_check(self, enable):
        return self._update_bl_cfg_item(BLConfigItems.BL_CFG_EBL_CHECK, 1 if enable == True else 0)

    def update_cfg_set_ebl_pin(self, port, pin):
        return self._update_bl_cfg_item(BLConfigItems.BL_CFG_EBL_PIN, (port<<6) | pin)

    def update_cfg_set_ebl_pin_polarity(self, polarity):
        return self._update_bl_cfg_item(BLConfigItems.BL_CFG_EBL_POL, 1 if polarity == 1 else 0)

    def update_cfg_set_valid_mark_check(self, enable):
        return self._update_bl_cfg_item(BLConfigItems.BL_CFG_VALID_MARK_CHK, 1 if enable == True else 0)

    def update_cfg_enable_uart(self, enable):
        return self._update_bl_cfg_item(BLConfigItems.BL_CFG_UART_ENABLE, 1 if enable == True else 0)

    def update_cfg_enable_i2c(self, enable):
        return self._update_bl_cfg_item(BLConfigItems.BL_CFG_I2C_ENABLE, 1 if enable == True else 0)

    def update_cfg_enable_spi(self, enable):
        return self._update_bl_cfg_item(BLConfigItems.BL_CFG_SPI_ENABLE, 1 if enable == True else 0)

    def update_cfg_set_i2c_addr(self, addr):
        if BridgeSettings.target_bl_version <= 'v3.4.1':
            if   addr == 0x58: return self._update_bl_cfg_item(BLConfigItems.BL_CFG_I2C_SLAVE_ADDR, 0)
            elif addr == 0x5A: return self._update_bl_cfg_item(BLConfigItems.BL_CFG_I2C_SLAVE_ADDR, 1)
            elif addr == 0x5C: return self._update_bl_cfg_item(BLConfigItems.BL_CFG_I2C_SLAVE_ADDR, 2)
            elif addr == 0xAA: return self._update_bl_cfg_item(BLConfigItems.BL_CFG_I2C_SLAVE_ADDR, 3)
            else:
                self.print(f'{BridgeSettings.target_bl_version} version bootloader does not support 0x{addr:X} I2C addr')
                return 0xff
        else:
            return self._update_bl_cfg_item(BLConfigItems.BL_CFG_I2C_SLAVE_ADDR, addr)

    def update_cfg_set_crc_check(self, enable):
        return self._update_bl_cfg_item(BLConfigItems.BL_CFG_CRC_CHECK, 1 if enable == True else 0)

    def update_cfg_lock_swd(self, enable):
        return self._update_bl_cfg_item(BLConfigItems.BL_CFG_LOCK_SWD, 1 if enable == True else 0)

    def update_cfg_set_bl_exit_mode(self, mode):
        """
        Possible modes:
            BL_EXIT_IMMEDIATE 0
            BL_EXIT_TIMEOUT 1
            BL_EXIT_INDEFINITE 2
        """
        cmd = bytearray()
        cmd.append(BLCmdConfigWrite.MAIN_CMD)
        cmd.append(BLCmdConfigWrite.EXIT_CONFIG)
        cmd.append(0x00)   # means exit mode
        cmd.append(mode)
        #
        retVal, rsp = self.bl_send_rcv(cmd, 1)

        return 0 if rsp[0] == BLRetVal.BTLDR_SUCCESS else rsp[0]

    def update_cfg_set_bl_exit_timeout(self, to):
        """
            Timeout is ms
        """
        cmd = bytearray()
        cmd.append(BLCmdConfigWrite.MAIN_CMD)
        cmd.append(BLCmdConfigWrite.EXIT_CONFIG)
        cmd.append(0x01)   # means timeout mode
        cmd.append(round(math.log(to,2)))
        #
        retVal, rsp = self.bl_send_rcv(cmd, 1)

        return 0 if rsp[0] == BLRetVal.BTLDR_SUCCESS else rsp[0]

    def flash_bl_cfg(self):
        cmd = bytearray()
        cmd.append(BLCmdConfigWrite.MAIN_CMD)
        cmd.append(BLCmdConfigWrite.SAVE_SETTINGS)
        #
        retVal, rsp = self.bl_send_rcv(cmd, 1, delay_ms=1000, delay_step_ms=500)

        return 0 if rsp[0] == BLRetVal.BTLDR_SUCCESS else rsp[0]
