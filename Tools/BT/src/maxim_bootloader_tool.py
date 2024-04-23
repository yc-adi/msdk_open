#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#

AUTHOR = "MAXIM Integrated"

VERSION = "v1.0.5"
PROG = "bootloader_tool"

COPYRIGHT = "Copyright © Maxim Integrated Products"

LICENSE = """Copyright © Maxim Integrated Products, Inc., All Rights Reserved.

 Permission is hereby granted, free of charge, to any person obtaining a
 copy of this software and associated documentation files (the "Software"),
 to deal in the Software without restriction, including without limitation
 the rights to use, copy, modify, merge, publish, distribute, sublicense,
 and/or sell copies of the Software, and to permit persons to whom the
 Software is furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included
 in all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL MAXIM INTEGRATED BE LIABLE FOR ANY CLAIM, DAMAGES
 OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 OTHER DEALINGS IN THE SOFTWARE.

 Except as contained in this notice, the name of Maxim Integrated
 Products, Inc. shall not be used except as stated in the Maxim Integrated
 Products, Inc. Branding Policy.

 The mere transfer of this software does not imply any licenses
 of trade secrets, proprietary technology, copyrights, patents,
 trademarks, maskwork rights, or any other form of intellectual
 property whatsoever. Maxim Integrated Products, Inc. retains all
 ownership rights."""


# ---- IMPORTS
import sys

from PyQt5 import QtCore, QtGui, QtWidgets, uic

from bootloader_gui import BootloaderGUI
from bridge_gui import BridgeTests
from ui import *
from window_maximsplash import MaximSplashWindow

from bridge import MaximSerial, Bootloader, Bridge, BridgeSettings


class MainWindow(QtWidgets.QMainWindow):
    def __init__(self, *args, **kwargs):
        super(MainWindow, self).__init__(*args, **kwargs)
        #self.ui = uic.loadUi('ui/mainwindow.ui', self)
        self.ui = Ui_MainWindow()
        self.ui.setupUi(self)

        self.setWindowTitle("MAXIM Bootloader Tool " + VERSION)
        self.setWindowIcon(QtGui.QIcon(image_maximintegratedlogo))

        self.setStyleSheet(MAXIM_LOOK_AND_FEEL)
        self.splashs_window = MaximSplashWindow(self, VERSION)

        self.ser = MaximSerial('COM58')
        #
        self.bl     = Bootloader(self.ser, self.printf)
        self.bridge = Bridge(self.ser, self.printf)
        self.bl_gui = BootloaderGUI(self.ui, self.bl, self.bridge, self.printf)
        self.tests = BridgeTests(self.ui, self.bl, self.bridge, self.printf)

        self.ui.cb_comport.old_showPopup = self.ui.cb_comport.showPopup
        self.ui.cb_comport.showPopup = self._comboBox_showPopup
        self.ui.cb_comport.addItems(self.ser.serial_ports())
        #
        self.ser.set_port(self.ui.cb_comport.currentText())

        self.ui.tab_gui.setFocus()
        self.on_tab_gui_currentChanged(0)
        #self.ui.cb_comport.setStyleSheet("QComboBox:focus { selection-background-color: #26D1C2; }")

        self.ui.textEdit.installEventFilter(self)

    def eventFilter(self, source, event):
        if event.type() == QtCore.QEvent.ContextMenu and source is self.ui.textEdit:
            menu = QtWidgets.QMenu()

            action_clear = menu.addAction("Clear")

            selected_action = menu.exec_(event.globalPos())

            if selected_action == action_clear:
                self.ui.textEdit.clear()

            return True

        return super().eventFilter(source, event)

    def on_tab_gui_currentChanged(self, index):
        if index == 0:
            self.ui.tab_gui.tabBar().setTabTextColor(0, QtGui.QColor(f"#{MAXIM_RGB_COLOR_CODE}"))
            self.ui.tab_gui.tabBar().setTabTextColor(1, QtCore.Qt.black)
        elif index == 1:
            self.ui.tab_gui.tabBar().setTabTextColor(0, QtCore.Qt.black)
            self.ui.tab_gui.tabBar().setTabTextColor(1, QtGui.QColor(f"#{MAXIM_RGB_COLOR_CODE}"))

    def printf(self, *args, **kwargs):
        text, = args
        text = text.replace('SUCCESS', '<span style="color: #26D1C2">SUCCESS</span>')
        text = text.replace('FAIL', '<span style="color: #ffff0000">FAIL</span>')
        text = text.replace('FAILURE', '<span style="color: #ffff0000">FAILURE</span>')
        text = text.replace('\n', '<br>')

        self.ui.textEdit.append('<div contenteditable>'+text+'</div>')

    @QtCore.pyqtSlot()
    def on_pb_connect_clicked(self):
        retVal = 0xff
        try:
            self.ser.set_port(self.ui.cb_comport.currentText())
            text = self.ui.cb_bridge_mode.currentText()
            if text == 'I2C':
                BridgeSettings.bridge_mode = BridgeSettings.BRIDGE_MODE_I2C
            elif text == 'SPI':
                BridgeSettings.bridge_mode = BridgeSettings.BRIDGE_MODE_SPI
            elif text == 'UART':
                if self.ui.cb_uart_select.currentText() == 'UART0':
                    BridgeSettings.bridge_mode = BridgeSettings.BRIDGE_MODE_UART0
                else:
                    BridgeSettings.bridge_mode = BridgeSettings.BRIDGE_MODE_UART2
            else:
                raise Exception
            retVal = self.bridge.switch_bridge_mode(mode=BridgeSettings.bridge_mode)
        except:
            pass

        if retVal == 0:
            self.printf('Connect' + " --------SUCCESS--------\n", flush=True)
        else:
            self.printf('Connect' + " --------FAIL--------, retVal:{}\n".format(hex(retVal)), flush=True)

    def _comboBox_showPopup(self):
        comports = self.ser.serial_ports()
        self.ui.cb_comport.clear()
        self.ui.cb_comport.addItems(comports)
        self.ui.cb_comport.old_showPopup()

    @QtCore.pyqtSlot()
    def on_actionAbout_2_triggered(self):
        self.splashs_window.show()

    @QtCore.pyqtSlot()
    def on_actionExit_triggered(self):
        sys.exit()


def main():
    app = QtWidgets.QApplication(sys.argv)
    mainWindow = MainWindow()
    mainWindow.show()
    app.exec()


main()
