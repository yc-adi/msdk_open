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

from PyQt5 import QtCore

from bridge.bridge_settings import BridgeSettings


class BridgeTests(QtCore.QObject):

    def __init__(self, ui, bl, bridge, printf):
        super(BridgeTests, self).__init__()
        self.ui = ui
        self.bl = bl
        self.bridge = bridge
        self.print = printf

        # bridge
        self.ui.pb_gpio_conf_send.clicked.connect(self.on_pb_gpio_conf_send_clicked)
        self.ui.pb_gpio_set_send.clicked.connect(self.on_pb_gpio_set_send_clicked)
        self.ui.pb_gpio_status_send.clicked.connect(self.on_pb_gpio_status_send_clicked)

        # set defaults
        self.ui.cb_reset_gpio_select.setCurrentIndex(BridgeSettings.reset_pin)
        self.ui.cb_mfio_gpio_select.setCurrentIndex(BridgeSettings.mfio_pin)
        self.ui.cb_mfio_pol_select.setCurrentIndex(BridgeSettings.mfio_pol)
        self.ui.cb_uart_select.setCurrentIndex(0)
        self.ui.lineEdit_bridge_i2c_addr.setText(hex(BridgeSettings.i2c_addr))

        self.ui.cb_reset_gpio_select.currentTextChanged.connect(self.on_cb_reset_gpio_select_changed)
        self.ui.cb_mfio_gpio_select.currentTextChanged.connect(self.on_cb_mfio_gpio_select_changed)
        self.ui.cb_mfio_pol_select.currentTextChanged.connect(self.on_cb_mfio_pol_select_changed)
        self.ui.cb_uart_select.currentTextChanged.connect(self.on_cb_uart_select_changed)

        self.ui.lineEdit_bridge_i2c_addr.textChanged.connect(self.update_i2c_addr)

        self.ui.pb_onboard_i2c_pullup_set.clicked.connect(self.on_pb_onboard_i2c_pullup_set_clicked)

    def update_i2c_addr(self):
        try:
            new_i2c_addr = int(self.ui.lineEdit_bridge_i2c_addr.text(), 16)
            BridgeSettings.i2c_addr = new_i2c_addr
        except:
            pass

    def on_cb_reset_gpio_select_changed(self, value):
        BridgeSettings.reset_pin = 1 if value == 'GPIO-1' else 0

    def on_cb_mfio_gpio_select_changed(self, value):
        BridgeSettings.mfio_pin = 1 if value == 'GPIO-1' else 0

    def on_cb_mfio_pol_select_changed(self, value):
        BridgeSettings.mfio_pol = self.ui.cb_mfio_pol_select.currentIndex()

    def on_cb_uart_select_changed(self, value):
        # it process while connecting to target
        pass

    def on_pb_onboard_i2c_pullup_set_clicked(self):
        retVal = 0xff
        try:
            retVal = self.bridge.on_board_i2c_pullup_status(self.ui.cb_onboard_i2c_pullup_status.currentText())
        except:
            pass
        self._process_retVal(retVal, 'On Board I2C Pullup Configure')

    def _process_retVal(self, retVal, msg='', raise_except=False):
        if retVal == 0:
            self.print(msg + " --------SUCCESS--------\n", flush=True)
        else:
            self.print(msg + " --------FAIL-------- retVal: {}\n".format(hex(retVal)), flush=True)
            if raise_except:
                raise Exception

    def on_pb_gpio_conf_send_clicked(self):
        retVal = 0xff
        try:
            gpio1_index = self.ui.cb_gpio1_conf.currentIndex()
            gpio0_index = self.ui.cb_gpio0_conf.currentIndex()

            if gpio1_index > 2: gpio1_index += 1
            if gpio0_index > 2: gpio0_index += 1

            retVal = self.bridge.gpio_config(gpio0_index,gpio1_index)
        except:
            pass
        self._process_retVal(retVal, 'GPIO Configure')

    def on_pb_gpio_set_send_clicked(self):
        try:
            retVal = self.bridge.gpio_set(1, state=self.ui.cb_gpio1_set.currentIndex())
            self._process_retVal(retVal, 'GPIO-1 Set/Clr', raise_except=True)

            retVal = self.bridge.gpio_set(0, state=self.ui.cb_gpio0_set.currentIndex())
            self._process_retVal(retVal, 'GPIO-0 Set/Clr', raise_except=True)
        except:
            pass

    def on_pb_gpio_status_send_clicked(self):
        try:
            retVal, gpio0_stat, gpio1_stat = self.bridge.gpio_get()
            self._process_retVal(retVal, 'GPIO Get', raise_except=True)

            self.ui.lbl_gpio0_status.setText('High' if gpio0_stat else 'Low')
            self.ui.lbl_gpio1_status.setText('High' if gpio1_stat else 'Low')
        except:
            pass
