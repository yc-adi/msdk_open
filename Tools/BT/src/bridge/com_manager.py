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
import serial
import glob
import struct


class MaximSerial(serial.Serial):
    """
    MaximSerial extends `Serial` by adding some extra functions.
    """

    def __init__(self, port):
        """
        :Parameter port: serial port to use (/dev/tty* or COM*)
        """
        serial.Serial.__init__(self)

        self.port = port
        self.baudrate = 115200
        self.timeout = 0.5
        #self.dtr = True
        #self.rts = True

    def set_port(self, port):
        self.port = port

    def get_port(self):
        return self.port

    def open(self, port=None):
        if port:
            self.port = port
        super(MaximSerial, self).open()

    def close(self):
        super(MaximSerial, self).close()

    def writePacket(self, packet):
        if isinstance(packet, str):
            retVal = self.write(packet.encode('utf-8'))
        else:
            retVal = self.write(packet)
        self.flush()

        return retVal

    def readPacket(self, dump=False):
        # get len
        rsp = b'\xff'
        arr = self.read(2)
        if len(arr) == 2:
            packetLen = arr[0]*256 + arr[1]
            # packet
            rsp = self.read(packetLen)

        retVal = 0xff
        if len(rsp) >= 4:
            retVal, = struct.unpack('>i', rsp[0:4])
            rsp = rsp[4:]
        
        if dump:
            print(f'RetVal: {retVal}, Response Len: {len(rsp)}')
            if (dump == True) and (len(rsp) > 0):
                    print('\tResponse (hex):', "-".join("%02X" % b for b in rsp), flush=True) 

        if len(rsp) == 0:
            rsp = b'\xff' # to do not cause crash

        return retVal, rsp

    def serial_ports(self):
        """ Lists serial port names

            :raises EnvironmentError:
                On unsupported or unknown platforms
            :returns:
                A list of the serial ports available on the system
        """
        if sys.platform.startswith('win'):
            ports = ['COM%s' % (i + 1) for i in range(256)]
        elif sys.platform.startswith('linux') or sys.platform.startswith('cygwin'):
            # this excludes your current terminal "/dev/tty"
            ports = glob.glob('/dev/tty[A-Za-z]*')
        elif sys.platform.startswith('darwin'):
            ports = glob.glob('/dev/tty.*')
        else:
            raise EnvironmentError('Unsupported platform')

        result = []
        for port in ports:
            try:
                s = serial.Serial(port)
                s.close()
                result.append(port)
            except (OSError, serial.SerialException):
                pass
        return result

