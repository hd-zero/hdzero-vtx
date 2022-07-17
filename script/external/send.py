import argparse

import serial
import logging

from xmodem import XMODEM


def dbg_print(line=''):
    print(line, flush=True)
    return


parser = argparse.ArgumentParser()
parser.add_argument('port', type=str)
parser.add_argument('file', type=str)
args = parser.parse_args()

ser = serial.Serial(args.port, baudrate=115200, timeout=5)


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
    dbg_print(ser.readline().decode().rstrip())

dbg_print("Sending firmware...")
with open(args.file, 'rb') as stream:
    ser.write(b'f')

    modem.send(stream)
