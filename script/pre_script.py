import logging

import serial
import serial.tools.list_ports

from external.send import do_send

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

    firmware = env.subst("$BUILD_DIR/HDZERO_TX.bin")
    do_send(ports[0].device, firmware)
                
env.Replace(UPLOADCMD=upload_firmware)
