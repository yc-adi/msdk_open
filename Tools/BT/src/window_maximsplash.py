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
from PyQt5 import QtWidgets, QtCore
from ui import *


class MaximSplashWindow(QtWidgets.QDialog):

    def __init__(self, parent, version):
        super(MaximSplashWindow, self).__init__(parent)
        self.ui = Ui_MaximSplashScreen()
        self.ui.setupUi(self)

        self.move(self.rect().center() + QtCore.QPoint(150, 50))

        self.setWindowFlags(QtCore.Qt.CustomizeWindowHint)
        self.setStyleSheet("MaximSplashWindow {"
                           "background: rgba(32, 80, 96, 100);"
                           "background-color: rgb(255, 255, 255);" +
                           "background-image: url({}); ".format(image_splash_screen_banner_no_txt) +
                           "background-position: top left;  background-repeat: no-repeat; } ")

        self.ui.lbl_tool_name.setStyleSheet("font-size: 13pt; background: transparent;")

        self.ui.lbl_version.setText("Version " + version)
        self.ui.lbl_trademark.setText("© Maxim Integrated Products, Inc\n"
                                      "All rights reserved")

        self.ui.lbl_version.setStyleSheet("QLabel{font-size: 10pt;}")
        self.ui.lbl_trademark.setStyleSheet("QLabel{font-size: 10pt;}")

        self.ui.lbl_website_text.setText("www.maximintegrated.com")
        self.ui.lbl_support_text.setText("support.maximintegrated.com")

        self.ui.lbl_website.setStyleSheet("font-size: 10pt")
        self.ui.lbl_support.setStyleSheet("font-size: 10pt")
        self.ui.lbl_website_text.setStyleSheet(f"color: #{MAXIM_RGB_COLOR_CODE}; font-size: 10pt")
        self.ui.lbl_support_text.setStyleSheet(f"color: #{MAXIM_RGB_COLOR_CODE}; font-size: 10pt")

        self.hide()

    def on_pb_ok_clicked(self):
        self.close()
