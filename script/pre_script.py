import logging

import serial
import serial.tools.list_ports

from external.xmodem import XMODEM

Import("env")

SDCC_OPTS = ["--model-large", "--opt-code-speed", "--peep-return",
             "--fomit-frame-pointer", "--allow-unsafe-read"]

env.Append(
    CFLAGS=SDCC_OPTS,
    LINKFLAGS=SDCC_OPTS
)


def dbg_print(line=''):
    print(line, flush=True)
    return


def upload_firmware(source, target, env):
    ports = list(serial.tools.list_ports.grep("USB"))

    if len(ports) == 0:
        raise Exception("No serial port found, exiting")

    dbg_print("Using serial port %s" % ports[0].device)
    ser = serial.Serial(ports[0].device, baudrate=115200, timeout=5)

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

    firmware = env.subst("$BUILD_DIR/HDZERO_TX.bin")
    with open(firmware, 'rb') as stream:
        ser.write(b'f')

        modem.send(stream)


env.Replace(UPLOADCMD=upload_firmware)
