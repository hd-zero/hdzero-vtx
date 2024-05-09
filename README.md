![](https://raw.githubusercontent.com/hd-zero/hdzero-vtx/main/misc/HDZero.png)

# HDZero VTX Firmware

Firmware runs on a built-in 8051 MCU of DM5680 which is the Baseband chip of VTX.

It works on all VTXs, including VTX Whoop, VTX Whoop Lite, VTX Race V1, VTX Race V2, VTX Freestyle.

## For Users:

To download the latest firmware for your VTX, visit the [release page](https://github.com/hd-zero/hdzero-vtx/releases) and click the asset that matches your hardware.

## For Developers:

### To build:

- Recommended build environment is Visual Studio Code
- Install PlatformIO IDE extension in Visual Studio Code
- Build source inside of PlatformIO (ctrl + alt + b)

### To flash firmware:

- Navigate to `.pio\build\{vtx model}`
- Copy HDZERO_TX.bin to root of VRX SD Card
- Follow normal flashing proceedure

# Attention:

**Please do not arbitrarily modify the data in use in the EEPROM, that may make your VTX not work.**

# VTX troubleshooting:

For troubleshooting the VTX, refer to
[vtx troubleshooting](https://github.com/hd-zero/hdzero-vtx-docs/blob/main/docs/vtx_troubleshoot.md)
and
[vtx manual](https://github.com/hd-zero/hdzero-vtx-docs/blob/main/docs/vtx_user_manual.md)
in the VTX documentation.
