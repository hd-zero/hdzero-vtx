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

# Basic VTX hardware troubleshooting:
The Red LED is a simple indicator of power status and should be ON solid during normal operation. 
- If the Red LED is Off, dim, or flickering: Test power input to VTX is stable at expected voltage and within input voltage tolerance of VTX.

The Blue LED provides these status indications:
- flicker 3x after boot indicates MSP signaling is detected.
- OFF = camera lost (check camera and cabling)
- ON/OFF @ 2hz = heat protection
- ON/OFF @ 8hz = 0MW pitt mode active
- ON/OFF @ 4hz = 0.1mw pitt mode active
- ON solid = VTX operation normal

If both Red and Blue LEDS are on solid but no video signal is detected by VRX:
- use spectrum analyzer (available on some analog VRXs) to verify VTX RF output on correct channel.
- use a PAT version of firmware for your specific VTX to verify RF output from VTX and test patern signal is received by VRX.

The test pads on the VTX can also help verify some segments of the VTX cricut. eg: pad labeled 1v8 should be approximately 1.8 Volts.
