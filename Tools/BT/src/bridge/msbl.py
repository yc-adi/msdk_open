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
from ctypes import *
from copy import deepcopy


class MsblHeader(Structure):
    _fields_ = [('magic', 4 * c_char),
                ('formatVersion', c_uint),
                ('target', 16 * c_char),
                ('enc_type', 16 * c_char),
                ('nonce', 11 * c_ubyte),
                ('resv0', c_ubyte),
                ('auth', 16 * c_ubyte),
                ('numPages', c_ushort),
                ('pageSize', c_ushort),
                ('crcSize', c_ubyte),
                ('resv1', 3 * c_ubyte)]


class AppHeader(Structure):
    _fields_ = [('crc32', c_uint),
                ('length', c_uint),
                ('validMark', c_uint),
                ('boot_mode', c_uint)]


class Page(Structure):
    _fields_ = [('data', (8192 + 16) * c_ubyte)]


class RawPage(Structure):
    _fields_ = [('data', (8192) * c_ubyte)]


class CRC32(Structure):
    _fields_ = [('val', c_uint)]


class MaximSBL:
    def __init__(self, printf):
        self.file_name = None
        self.page_size = 0
        self.img = bytearray()
        self.img_size = 0
        self.header = MsblHeader()
        self.pages = {}
        self.print = printf

    def print_as_hex(self, label, arr):
        if len(arr) <= 16:
            self.print('{:<10}: {}'.format(label, "-".join("%02X" % b for b in arr)))
        else:
            self.print('{:<10}: '.format(label), end='')
            for i in range(len(arr)):
                if (i % 32) == 0:
                    self.print('')
                    self.print('\t\t', end='')
                self.print('{:02X} '.format(arr[i]), end='')
            self.print('')

    def _load_msbl_file(self):
        with open(self.file_name, 'rb') as self.f:
            header = MsblHeader()

            if self.f.readinto(header) == sizeof(header):
                self.print('MSBL Info')
                self.print('-'*48)
                self.print('{:<15}: {}'.format("magic", str(header.magic, encoding='utf-8')))
                self.print('{:<15}: {}'.format("formatVersion", str(header.formatVersion)))
                self.print('{:<15}: {}'.format("target", str(header.target, encoding='utf-8')))
                self.print('{:<15}: {}'.format("EncType", str(header.enc_type, encoding='utf-8')))
                self.print('{:<15}: {}'.format("numPages", header.numPages))
                self.print('{:<15}: {}'.format("pageSize", header.pageSize))
                self.print('{:<15}: {}'.format("crcSize", header.crcSize))
                self.print('{:<15}: {}'.format("Header Size", sizeof(header)))
                self.print('{:<15}: {}'.format("resv0", header.resv0))

                self.print_as_hex('nonce', header.nonce)
                self.print_as_hex('auth', header.auth)
                self.print_as_hex('resv1', header.resv1)
            else:
                return 1

            self.header = header

            i = 0
            self.page = {}
            tmp_page = Page()
            total_size = sizeof(header)
            while self.f.readinto(tmp_page) == sizeof(tmp_page):
                self.page[i] = deepcopy(tmp_page.data)
                total_size += sizeof(tmp_page)
                i += 1

            self.crc32 = CRC32()
            self.f.seek(-4, 2)

            self.f.readinto(self.crc32)
            total_size = total_size + sizeof(self.crc32)
            self.print('Total file size: {} bytes'.format(total_size))
            self.print('CRC32: 0x{:X}'.format(self.crc32.val))
            self.print('MSBL file read succeeded.')

        return 0

    def load_file(self, file_name):
        self.file_name = file_name

        f, ext = os.path.splitext(self.file_name)
        if ext == '.msbl':
            return self._load_msbl_file()

        self.print('Invalid file extension: ' + ext)
        return 1

    @staticmethod
    def generate_msbl(bin_file, target, pagesize, key=''):
        if sys.platform == 'linux':
            msblGen = 'msblGenLinux'
        else: # 'win32'
            msblGen = 'msblGenWin32.exe'

        sep = os.path.sep
        possible_paths = [f'.{sep}', f'..{sep}', f'..{sep}bin{sep}']
        for path in possible_paths:
            if os.path.exists(path + msblGen):
                retVal = os.system(f'{path}{msblGen} {bin_file} {target} {pagesize} {key}')
                return retVal

        print(f'{msblGen} not found in')
        for path in possible_paths:
            print(path)
        return 2
