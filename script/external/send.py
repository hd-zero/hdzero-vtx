import argparse
import sys

import serial
import logging

try:
    from external.xmodem import XMODEM
except:
    from xmodem import XMODEM

def dbg_print(line=''):
    print(line, flush=True)
    return

def do_send(port, file):
    dbg_print("Using serial port %s" % port)
    ser = serial.Serial(port, baudrate=115200, timeout=5)

    def getc(size, timeout=1):
        return ser.read(size) or None

    def putc(data, timeout=1):
        return ser.write(data)

    logging.basicConfig(level=logging.ERROR)

    modem = XMODEM(getc, putc)
    modem.log.setLevel(logging.DEBUG)

    dbg_print("Checking flash...")

    ser.write(b'p')
    for x in range(4):
        line = ser.readline().decode().rstrip();
        dbg_print(line)
        if len(line) == 0:
            raise Exception("Flash detection failed!")

    dbg_print("Sending firmware %s..." % file)
    with open(file, 'rb') as stream:
        ser.write(b'f')

        if modem.send(stream) == False:
            raise Exception("Flash failed!")

        dbg_print(ser.readline().decode().rstrip())

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('port', type=str)
    parser.add_argument('file', type=str)
    args = parser.parse_args()

    sys.exit(do_send(args.port, args.file))