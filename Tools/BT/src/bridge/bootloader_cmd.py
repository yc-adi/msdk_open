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
from ctypes import *


class BLRetVal:
    # SUCCESS
    SUCCESS                  = 0x00
    BTLDR_SUCCESS            = 0xAA
    # FAILURES
    ERR_COMMAND              = 0x01
    ERR_UNAVAILABLE          = 0x02
    ERR_DATA_FORMAT          = 0x03
    ERR_INPUT_VALUE          = 0x04
    ERR_BTLDR_GENERAL        = 0x80
    ERR_BTLDR_CHECKSUM       = 0x81
    ERR_BTLDR_AUTH           = 0x82
    ERR_BTLDR_INVALID_APP    = 0x83
    ERR_BTLDR_APP_NOT_ERASED = 0x84
    ERR_BTLDR_DECRYPTION	 = 0x85
    ERR_KEY_EXIST			 = 0x86
    ERR_NO_KEY_MEM			 = 0x87
    ERR_PARTIAL_ACK          = 0xAB
    ERR_TRY_AGAIN            = 0xFE
    ERR_UNKNOWN              = 0xFF


class BLCmdDevSetMode:
     MAIN_CMD = 0x01
     # sub cmds
     SET_MODE = 0x00


class BLCmdDevGetMode:
    MAIN_CMD = 0x02
    # sub cmds
    GET_MODE = 0x00


class BLCmdFlash:
    MAIN_CMD = 0x80
    # sub cmds
    SET_IV = 0x00
    SET_AUTH = 0x01
    SET_NUM_PAGES = 0x02
    ERASE_APP_MEMORY = 0x03
    WRITE_PAGE = 0x04
    ERASE_PAGE_MEMORY = 0x05
    SET_PARTIAL_PAGE_SIZE = 0x06
    SET_KEY_AAD = 0x07


class BLCmdInfo:
    MAIN_CMD = 0x81
    # sub cmds
    GET_VERSION = 0x00
    GET_PAGE_SIZE = 0x01
    GET_USN = 0x02


class BLCmdConfigWrite:
    MAIN_CMD = 0x82
    # sub cmds
    SAVE_SETTINGS = 0x00
    ENTRY_CONFIG = 0x01
    EXIT_CONFIG = 0x02


class BLCmdConfigRead:
    MAIN_CMD = 0x83
    # sub cmds
    RD_ENTRY_CONFIG = 0x01
    RD_EXIT_CONFIG = 0x02
    CMDIDX_R_ALL_CFG = 0xFF


class BLCmdDeviceInfo:
     MAIN_CMD = 0xFF
     # sub cmds
     GET_PLATFORM_TYPE = 0x00

class BLConfigItems:
    BL_CFG_EBL_CHECK        = 0x00
    BL_CFG_EBL_PIN          = 0x01
    BL_CFG_EBL_POL          = 0x02
    BL_CFG_VALID_MARK_CHK   = 0x03
    BL_CFG_UART_ENABLE      = 0x04
    BL_CFG_I2C_ENABLE       = 0x05
    BL_CFG_SPI_ENABLE       = 0x06
    BL_CFG_I2C_SLAVE_ADDR   = 0x07
    BL_CFG_CRC_CHECK        = 0x08
    BL_CFG_LOCK_SWD         = 0x09


class BLConfigBits_v341(LittleEndianStructure):
    _fields_ = [
            ("enter_bl_check", c_uint32, 1),
            ("ebl_pin", c_uint32, 4),
            ("ebl_polarity", c_uint32, 1),
            ("ebl_port", c_uint32, 2),
            ("uart_enable", c_uint32, 1),
            ("i2c_enable", c_uint32, 1),
            ("spi_enable", c_uint32, 1),
            ("i2c_addr", c_uint32, 2),
            ("res1", c_uint32, 3),
            ("ebl_timeout", c_uint32, 4),
            ("exit_bl_mode", c_uint32, 2),
            ("res2", c_uint32, 2),
            ("crc_check", c_uint32, 1),
            ("valid_mark_check", c_uint32, 1),
            ("lock_swd", c_uint32, 1),
            ("res3", c_uint32, 5),
        ]


class BLConfig_v341(Union):
    _fields_ = [("b", BLConfigBits_v341),
                ("cfg", c_uint32)]


class BLConfigBits(LittleEndianStructure):
    _fields_ = [
            ("enter_bl_check", c_uint32, 1),
            ("ebl_pin", c_uint32, 4),
            ("ebl_polarity", c_uint32, 2),
            ("res0", c_uint32, 1),
            ("uart_enable", c_uint32, 1),
            ("i2c_enable", c_uint32, 1),
            ("spi_enable", c_uint32, 1),
            ("res1", c_uint32, 5),
            ("ebl_timeout", c_uint32, 4),
            ("exit_bl_mode", c_uint32, 2),
            ("res2", c_uint32, 2),
            ("crc_check", c_uint32, 1),
            ("valid_mark_check", c_uint32, 1),
            ("lock_swd", c_uint32, 1),
            ("res3", c_uint32, 5),
            ("i2c_addr", c_uint32, 7),
            ("res4", c_uint32, 25),
        ]


class BLConfig(Union):
    _fields_ = [("b", BLConfigBits),
                ("cfg", c_uint64)]
