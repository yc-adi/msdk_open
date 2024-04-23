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
import os
from  binascii import unhexlify

from PyQt5 import QtCore, QtGui
from PyQt5.QtWidgets import QFileDialog

from bridge.msbl import MaximSBL
from bridge.bridge_settings import BridgeSettings
from ui import MAXIM_RGB_COLOR_CODE

class BootloaderGUI(QtCore.QObject):
    def __init__(self, ui, bl, bridge, printf):
        super(BootloaderGUI, self).__init__()
        self.ui = ui
        self.bl = bl
        self.bridge = bridge
        self.print = printf

        self.ui.pb_get_bl_conf.clicked.connect(self.on_pb_get_bl_conf_clicked)
        self.ui.pb_save_bl_conf.clicked.connect(self.on_pb_save_bl_conf_clicked)
        # msbl
        self.ui.pb_browse_msbl.clicked.connect(self.pb_browse_msbl_clicked)
        self.ui.pb_load_msbl.clicked.connect(self.pb_load_msbl_clicked)
        # bin
        self.ui.pb_browse_bin.clicked.connect(self.pb_browse_bin_clicked)
        self.ui.pb_browse_key.clicked.connect(self.pb_browse_key_clicked)
        self.ui.pb_load_bin.clicked.connect(self.pb_load_bin_clicked)

        # bl test command
        self.ui.pb_bl_tests_run.clicked.connect(self.on_pb_bl_tests_run_clicked)
        self.set_bl_tests()

        self.ui.tab_fw_update.currentChanged.connect(self.on_tab_fw_update_currentChanged)
        self.on_tab_fw_update_currentChanged(0)

    def on_tab_fw_update_currentChanged(self, index):
        if index == 0:
            self.ui.tab_fw_update.tabBar().setTabTextColor(0, QtGui.QColor(f"#{MAXIM_RGB_COLOR_CODE}"))
            self.ui.tab_fw_update.tabBar().setTabTextColor(1, QtCore.Qt.black)
        elif index == 1:
            self.ui.tab_fw_update.tabBar().setTabTextColor(0, QtCore.Qt.black)
            self.ui.tab_fw_update.tabBar().setTabTextColor(1, QtGui.QColor(f"#{MAXIM_RGB_COLOR_CODE}"))

    def _process_retVal(self, retVal, msg='', raise_except=False):
        if retVal == 0:
            self.print(msg + " --------SUCCESS--------\n", flush=True)
        else:
            self.print(msg + " --------FAIL-------- retVal: {}\n".format(hex(retVal)), flush=True)
            if raise_except:
                raise Exception

    def switch_bl_mode(self):
        """
         First try enter bootloader over enter bl command
         If it fails try over MFIO
        """

        # reset target first
        self.bridge.gpio_set(BridgeSettings.reset_pin, state=0)
        # time.sleep(0.01)
        self.bridge.gpio_set(BridgeSettings.reset_pin, state=1)
        #
        retVal = self.bl.enter_bootloader()

        if retVal:
            # if it fails try with MFIO pin
            self.bridge.gpio_set(BridgeSettings.reset_pin, state=0)
            self.bridge.gpio_set(BridgeSettings.reset_pin, state=1)
            # mfio
            self.bridge.gpio_set(BridgeSettings.mfio_pin, state=BridgeSettings.mfio_pol)
            # send enter bl command
            retVal = self.bl.enter_bootloader()
            # return gpio to org state
            self.bridge.gpio_set(BridgeSettings.mfio_pin, state=not BridgeSettings.mfio_pol)

        if retVal:
            self.print('enter_bootloader failed\n')

        return retVal

    def set_bl_tests(self):
        self.ui.cb_bl_tests.addItem('Hard Reset then Send Enter Bootloader Command')
        self.ui.cb_bl_tests.addItem('Get Target PartNumber')
        self.ui.cb_bl_tests.addItem('Get Target Bootloader Version')
        self.ui.cb_bl_tests.addItem('Enter Bootloader')
        self.ui.cb_bl_tests.addItem('Exit Bootloader')
        self.ui.cb_bl_tests.addItem('Get Page Size')
        self.ui.cb_bl_tests.addItem('Get USN')
        self.ui.cb_bl_tests.addItem('Erase App')
        self.ui.cb_bl_tests.addItem('Load AES Key (Supports by some micros)')

    def on_pb_bl_tests_run_clicked(self):
        retVal = 0xff
        try:
            test = self.ui.cb_bl_tests.currentIndex()
            if test == 0:
                retVal = self.switch_bl_mode()
            elif test == 1:
                retVal, target = self.bl.get_target_type()
                if retVal == 0:
                    self.print(f'Target: {target}')
            elif test == 2:
                retVal, bl_version = self.bl.get_target_version()
                if retVal == 0:
                    self.print(f'Bootloader Version: {bl_version}')
            elif test == 3:
                retVal = self.bl.enter_bootloader()
            elif test == 4:
                retVal = self.bl.exit_bootloader()
            elif test == 5:
                retVal, page_size = self.bl.get_page_size()
                if retVal == 0:
                    self.print(f'\tPageSize: {page_size}')
            elif test == 6:
                retVal, usn = self.bl.get_usn()
                if retVal == 0:
                    self.print('\tUSN (hex):' + "-".join("%02X" % b for b in usn), flush=True)
            elif test == 7:
                retVal = self.bl.erase_app()
            elif test == 8:
                retVal = self.load_aes_key()

        except:
            pass
        self._process_retVal(retVal, msg=self.ui.cb_bl_tests.currentText())

    def on_pb_get_bl_conf_clicked(self):
        retVal = 0xff
        try:
            retVal, target = self.bl.get_target_type()
            self._process_retVal(retVal, msg='Get Target PartNumber', raise_except=True)

            retVal, bl_version = self.bl.get_target_version()
            self._process_retVal(retVal, msg='Get Bootloader Version', raise_except=True)

            retVal, cfg = self.bl.get_bl_configure(target, bl_version)
            self._process_retVal(retVal, msg='Get BL Config')

            if retVal == 0:
                self.print('{:<18}: {}'.format("enter_bl_check", cfg.enter_bl_check))
                self.print('{:<18}: {}'.format("ebl_pin", cfg.ebl_pin))
                if BridgeSettings.target_bl_version <= 'v3.4.1':
                    self.print('{:<18}: {}'.format("ebl_port", cfg.ebl_port))
                else:
                    self.print('{:<18}: {}'.format("ebl_port", 0))
                self.print('{:<18}: {}'.format("ebl_polarity", cfg.ebl_polarity))
                self.print('{:<18}: {}'.format("uart_enable", cfg.uart_enable))
                self.print('{:<18}: {}'.format("i2c_enable", cfg.i2c_enable))
                self.print('{:<18}: {}'.format("spi_enable", cfg.spi_enable))
                self.print('{:<18}: {}'.format("ebl_timeout", cfg.ebl_timeout))
                self.print('{:<18}: {}'.format("exit_bl_mode", cfg.exit_bl_mode))
                self.print('{:<18}: {}'.format("crc_check", cfg.crc_check))
                self.print('{:<18}: {}'.format("valid_mark_check", cfg.valid_mark_check))
                self.print('{:<18}: {}'.format("lock_swd", cfg.lock_swd))
                self.print('{:<18}: 0x{:02X}'.format("i2c_addr", cfg.i2c_addr))
                self.print(' ')

                self.ui.cb_check_mfio.setCurrentIndex(cfg.enter_bl_check)
                self.ui.cb_mfio_pin.setCurrentIndex(cfg.ebl_pin)
                self.ui.cb_mfio_pol.setCurrentIndex(cfg.ebl_polarity)
                #
                self.ui.cb_uart_enable.setCurrentIndex(cfg.uart_enable)
                self.ui.cb_i2c_enable.setCurrentIndex(cfg.i2c_enable)
                self.ui.cb_spi_enable.setCurrentIndex(cfg.spi_enable)
                #
                self.ui.cb_valid_mark_check.setCurrentIndex(cfg.valid_mark_check)
                self.ui.cb_swd_lock.setCurrentIndex(cfg.lock_swd)
                self.ui.cb_crc_check.setCurrentIndex(cfg.crc_check)

                self.ui.lineEdit_bl_timeout.setText(str(1 << cfg.ebl_timeout))

                if BridgeSettings.target_bl_version <= 'v3.4.1':
                    addr_list = ['0x58', '0x5A', '0x5C', '0xAA']
                    self.ui.lineEdit_i2c_addr.setText(addr_list[cfg.i2c_addr & 0x03])
                    self.ui.cb_mfio_port.setCurrentIndex(cfg.ebl_port)
                else:
                    self.ui.lineEdit_i2c_addr.setText(hex(cfg.i2c_addr))
                    self.ui.cb_mfio_port.setCurrentIndex(0)

                if cfg.exit_bl_mode in (0, 1, 2):
                    self.ui.cb_bl_exit_mode.setCurrentIndex(cfg.exit_bl_mode)
                else:
                    self.ui.cb_bl_exit_mode.setCurrentText('INVALID Value')
        except:
            self._process_retVal(0xff, msg='Get BL Config')

    def on_pb_save_bl_conf_clicked(self):
        try:
            mfio_check_status = self.ui.cb_check_mfio.currentIndex()
            mfio_port = self.ui.cb_mfio_port.currentIndex()
            mfio_pin = self.ui.cb_mfio_pin.currentIndex()
            mfio_pol = self.ui.cb_mfio_pol.currentIndex()
            #
            uart_status = self.ui.cb_uart_enable.currentIndex()
            i2c_status = self.ui.cb_i2c_enable.currentIndex()
            spi_status = self.ui.cb_spi_enable.currentIndex()
            #
            valid_mark_check = self.ui.cb_valid_mark_check.currentIndex()
            swd_lock = self.ui.cb_swd_lock.currentIndex()
            exit_bl_mode = self.ui.cb_bl_exit_mode.currentIndex()
            crc_check = self.ui.cb_crc_check.currentIndex()

            ebl_timeout = self.ui.lineEdit_bl_timeout.text()
            i2c_addr = self.ui.lineEdit_i2c_addr.text()

            # update on bl side
            retVal = self.bl.update_cfg_enter_bl_check(mfio_check_status)
            self._process_retVal(retVal, 'MFIO Status Update:', raise_except=True)

            retVal = self.bl.update_cfg_set_ebl_pin(mfio_port, mfio_pin)
            self._process_retVal(retVal, 'MFIO Pin Update:', raise_except=True)

            retVal = self.bl.update_cfg_set_ebl_pin_polarity(mfio_pol)
            self._process_retVal(retVal, 'MFIO Polarity Update:', raise_except=True)

            retVal = self.bl.update_cfg_set_i2c_addr(int(i2c_addr, 16))
            self._process_retVal(retVal, 'I2C Addr Update:', raise_except=True)

            retVal = self.bl.update_cfg_set_valid_mark_check(valid_mark_check)
            self._process_retVal(retVal, 'Valid Mark Check Update:', raise_except=True)

            retVal = self.bl.update_cfg_set_crc_check(crc_check)
            self._process_retVal(retVal, 'CRC Check Update:', raise_except=True)

            retVal = self.bl.update_cfg_enable_uart(bool(uart_status))
            self._process_retVal(retVal, 'UART Enable:', raise_except=True)

            retVal = self.bl.update_cfg_enable_i2c(bool(i2c_status))
            self._process_retVal(retVal, 'I2C Enable:', raise_except=True)

            retVal = self.bl.update_cfg_enable_spi(bool(spi_status))
            self._process_retVal(retVal, 'SPI Enable:', raise_except=True)

            retVal = self.bl.update_cfg_set_bl_exit_mode(exit_bl_mode)
            self._process_retVal(retVal, 'BL Exit Mode Update:', raise_except=True)

            retVal = self.bl.update_cfg_set_bl_exit_timeout(int(ebl_timeout))
            self._process_retVal(retVal, 'BL Exit Timeout Update:', raise_except=True)

            retVal = self.bl.flash_bl_cfg()
            self._process_retVal(retVal, 'Flash Write:', raise_except=True)
        except:
            self._process_retVal(0xff, msg='Save BL Config')

    def pb_browse_msbl_clicked(self):
        filenames, _ = QFileDialog.getOpenFileName(parent=None, caption='Select File', directory='../devices/', filter='MSBL File (*.msbl)')
        if filenames:
            self.ui.lineEdit_image_path.setText(filenames)

    def _update_target_fw(self, msbl_file, progressBar):
        progressBar.setValue(0)

        msbl = MaximSBL(self.print)
        if msbl.load_file(msbl_file):
            self.print('MSBL decode failed')
            return 1
        progressBar.setMaximum(msbl.header.numPages)

        if self.switch_bl_mode():
            self.print('enter_bootloader failed\n')
            return 1

        if self.bl.set_num_pages(msbl.header.numPages):
            self.print('set_num_pages failed\n')
            return 1

        if self.bl.set_iv(iv=msbl.header.nonce):
            self.print('set_iv failed\n')
            return 1

        if self.bl.set_auth_bytes(auth=msbl.header.auth):
            self.print('set_auth_bytes failed\n')
            return 1

        if self.bl.erase_app() != 0:
            self.print('erase_app failed')
            return 1

        for i in range(0, msbl.header.numPages):
            progressBar.setValue(i+1)
            QtCore.QCoreApplication.processEvents()

            ret = self.bl.write_page(msbl.page[i])
            if ret != 0:
                self.print('Flashing page {}/{}  [FAILED]... err:{}'.format(i+1, msbl.header.numPages, ret))
                return ret

        self.bl.exit_bootloader()
        return 0

    def pb_load_msbl_clicked(self):
        try:
            retVal = self._update_target_fw(self.ui.lineEdit_image_path.text(), self.ui.progressBar_load_msbl)
            self._process_retVal(retVal, msg='Loading...')
            if retVal:  # clear progress bar in case of error
                self.ui.progressBar_load_msbl.setValue(0)
        except:
            print('Exception occurs while loading msbl')

    def pb_browse_bin_clicked(self):
        filename, _ = QFileDialog.getOpenFileName(parent=None, caption='Select File', directory='../devices/', filter='BIN File (*.bin)')
        if filename:
            self.ui.lineEdit_image_path_bin.setText(filename)

    def pb_browse_key_clicked(self):
        filename, _ = QFileDialog.getOpenFileName(parent=None, caption='Select File', directory='../devices/keys', filter='Key File (*.txt)')
        if filename:
            self.ui.lineEdit_image_path_key.setText(filename)

    def pb_load_bin_clicked(self):
        bin_file = self.ui.lineEdit_image_path_bin.text()
        key_file = self.ui.lineEdit_image_path_key.text()

        try:
            retVal = self.switch_bl_mode()
            if retVal == 0:
                retVal, target = self.bl.get_target_type()
                self.print(f'Target: {target}')
            if retVal == 0:
                retVal, bl_version = self.bl.get_target_version()
                self.print(f'Bootloader Version: {bl_version}')
            if retVal == 0:
                retVal, page_size = self.bl.get_page_size()
                self.print(f'\tPageSize: {page_size}')

            retVal = MaximSBL.generate_msbl(bin_file, target, page_size, key_file)
            self._process_retVal(retVal, msg='Generating MSBL ')
            if retVal == 0:
                msbl_file = os.path.basename(bin_file)
                msbl_file = os.path.splitext(msbl_file)[0] + '.msbl'

                retVal = self._update_target_fw(msbl_file, self.ui.progressBar_load_bin)
                self._process_retVal(retVal, msg='Loading...')
                if retVal: # clear progress bar in case of error
                    self.ui.progressBar_load_bin.setValue(0)
        except:
            print('Exception occurs while loading bin')

    def load_aes_key(self):
        key_file, _ = QFileDialog.getOpenFileName(parent=None, caption='Select File', directory='../devices/keys', filter='Text File (*.txt)')
        if not key_file:
            return -1

        retVal = -1
        with open(key_file, 'r') as keyfile:
            # Get Key
            if keyfile.readline() != 'aes_key_start\n':
                print('Invalid Key file start')
                return -1

            key = bytearray()
            for line in keyfile:
                if line.startswith("aes_key_end"):
                    break
                key += unhexlify(line.replace("0x", "").replace(", ", "").replace(",", "").replace("\n", ""))

            # Get AAD
            if keyfile.readline() != 'aes_aad_start\n':
                print('Invalid Key file start')
                return -1

            aad = bytearray()
            for line in keyfile:
                if line.startswith("aes_key_end"):
                    break
                aad += unhexlify(line.replace("0x", "").replace(", ", "").replace(",", "").replace("\n", ""))

            retVal = self.bl.load_aes_key(key, aad)

        return retVal
