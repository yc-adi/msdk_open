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

MAXIM_LOOK_AND_FEEL = """
QWidget
{
  font-family:              'Arial';
  font-size:                11pt;
  font-style:               normal;
  font-weight:              normal;
  background-color:         white;
}

/*********************************************************
 * Check Box Styling                                     *
 *********************************************************/
QCheckBox
{
  spacing:                  5px;
}

QCheckBox::indicator
{
  width:                    17px;
  height:                   17px;
}

QCheckBox::indicator:unchecked
{
  image:                    url('./gui/ui/images/checkbox-unchecked.png');
}

QCheckBox::indicator:checked
{
  image:                    url('./gui/ui/images/checkbox-active.png');
}

/*********************************************************
 * Radio Button Styling                                  *
 *********************************************************/
QRadioButton
{
  spacing:                  5px;
}

QRadioButton::indicator
{
  width:                    17px;
  height:                   17px;
}

QRadioButton::indicator:unchecked
{
  image:                    url('./gui/ui/images/unchecked.png');
}

QRadioButton::indicator:checked
{
  image:                    url('./gui/ui/images/radio-butten-active.png');
}

/*********************************************************
 * Spin Box Styling                                      *
 *********************************************************/
QSpinBox
{
  padding-right:            18px;
  border-image:             url('./gui/ui/images/updownborder.png');
  border-width:             1;
}

QSpinBox::up-button
{
  subcontrol-origin:        border;
  subcontrol-position:      top right;
  width:                    18px;
  border-image:             url('./gui/ui/images/updownup.png') 1;
  border-width:             1 1 0 1;
}

QSpinBox::up-button:hover
{
  border-image:             url('./gui/ui/images/updownuphover.png') 1;
}

QSpinBox::up-arrow
{
  image:                    url('./gui/ui/images/updownplus.png');
  width:                    6px;
  height:                   6px;
}

QSpinBox::down-button
{
  subcontrol-origin:        border;
  subcontrol-position:      bottom right;
  width:                    18px;
  border-image:             url('./gui/ui/images/updowndown.png') 1;
  border-width:             0 1 1 1;
}

QSpinBox::down-button:hover
{
  border-image:             url('./gui/ui/images/updowndownhover.png') 1;
}

QSpinBox::down-arrow
{
  image:                    url('./gui/ui/images/updownminus.png');
  width:                    5px;
  height:                   3px;
}

/*********************************************************
 * Scroll Bar Styling                                    *
 *********************************************************/
QScrollBar:horizontal
{
  border:                   1px solid #C8C8C8;
  border-radius:            6px;
  margin:                   2px;
  background-color:         #E0E0E0;
  max-height:               12px;
}

QScrollBar:vertical
{
  border:                   1px solid #C8C8C8;
  border-radius:            6px;
  margin:                   2px;
  background-color:         #E0E0E0;
  max-width:                10px;
}

QScrollBar::handle:horizontal
{
  border:                   1px solid #B6B6B6;
  border-radius:            6px;
  background-color:         #f5f5f5;
  margin:                   -2px;
  min-width:                30px;
}

QScrollBar::handle:vertical
{
  border:                   1px solid #B6B6B6;
  border-radius:            6px;
  background-color:         #f5f5f5;
  margin:                   -2px;
  min-height:               30px;
}

QScrollBar::add-line:horizontal,
QScrollBar::sub-line:horizontal,
QScrollBar::left-arrow:horizontal,
QScrollBar::right-arrow:horizontal,
QScrollBar::add-page:horizontal,
QScrollBar::sub-page:horizontal
{
  width: 0;
}

QScrollBar::add-line:vertical,
QScrollBar::sub-line:vertical,
QScrollBar::left-arrow:vertical,
QScrollBar::right-arrow:vertical,
QScrollBar::add-page:vertical,
QScrollBar::sub-page:vertical
{
  height: 0;
}

/*********************************************************
 * Button Styling                                        *
 *********************************************************/
QPushButton
{
  border:                   0px solid #6C6F70;
  border-radius:            4px;
  color:                    #000000;
  background-color:         qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #F3F3F3, stop: 1 #C6C6C6);
  padding-top:              0px;
  padding-bottom:           1px;
  padding-left:             0px;
  padding-right:            1px;
  focus:
}

QPushButton:hover
{
  background-color:         qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #D8F3F1, stop: 1 #9FE0DC);
}

QPushButton:pressed
{
  background-color:         qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #A4E0E8, stop: 1 #26D1C2);
  padding-top:              1px;
  padding-bottom:           0px;
  padding-left:             1px;
  padding-right:            0px;
}

QPushButton:default
{
    background-color:       qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #A4E0E8, stop: 1 #26D1C2);
}

QMenu
{ 
  color:					#000000;
}
QMenu:hover
{ 
  color:					#000000;
}

/*
QMenuBar
{
  border:                   0px solid #6C6F70;
  border-radius:            4px;
  color:                    #000000;
  background-color:         qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #F3F3F3, stop: 1 #C6C6C6);
  padding-top:              0px;
  padding-bottom:           1px;
  padding-left:             0px;
  padding-right:            1px;
  focus:
}*/

QProgressBar {
border: 2px solid #e0e0e0;
background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #d0d0d0, stop: 1 #ebebeb);
border-radius: 10px 10px 10px 10px;
height: 18px;
text-align: center;
}

QProgressBar::chunk {
background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #ccefed, stop: 1 #1eb7b3);
border-radius: 8px;
}

"""
