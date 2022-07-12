## HDZero VTX Firmware

Firmware runs on a build-in C51 MCU of DM5680 which is the Baseband chip of VTX.

It works on all VTXs, including VTX Whoop, VTX Whoop Lite, VTX Race V1, VTX Race V2, VTX Free Style.

Edit "./src/common.h": define MACROs for different VTXs
* #define VTX_L   --> VTX Free Style
* #define VTX_WL  --> VTX Whoop Lite
* #define VTX_S   --> VTX Whoop
* #define VTX_R   --> VTX Race V1, VTX Race V2

See: https://www.hd-zero.com/ for more production information.

