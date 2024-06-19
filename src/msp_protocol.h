#ifndef __MSP_PROTO_H_
#define __MSP_PROTO_H_

/**
 *  MSP Protocol
 */
#define MSP_HEADER_FRAMER   0x24 // $
#define MSP_HEADER_V1       0x4D // M
#define MSP_HEADER_V2       0x58 // X
#define MSP_HEADER_COMMAND  0x3C // <
#define MSP_HEADER_RESPONSE 0x3E // >

/**
 *  MSP v1 Protocol Types
 */
#define MSP_FC_VARIANT     0x02 // 2   //out message
#define MSP_GET_VTX_CONFIG 0x58 // 88  //out message
#define MSP_SET_VTX_CONFIG 0x59 // 89  // in message
#define MSP_STATUS         0x65 // 101 //out message
#define MSP_RC             0x69 // 105 //out message
#define MSP_DISPLAYPORT    0xB6 // 182 // in message
#define MSP_SET_OSD_CANVAS 0xBC // 188 // in message
#define MSP_GET_OSD_CANVAS 0xBD // 188 //out message

#define DP_HEADER0 0x56
#define DP_HEADER1 0x80

#define FC_OSD_LOCK            0x01
#define FC_VARIANT_LOCK        0x02
#define FC_RC_LOCK             0x04
#define FC_VTX_CONFIG_LOCK     0x08
#define FC_STATUS_LOCK         0x10
#define FC_INIT_VTX_TABLE_LOCK 0x80

#define OSD_CANVAS_SD_HMAX  30
#define OSD_CANVAS_SD_VMAX  16
#define OSD_CANVAS_HD_HMAX0 50
#define OSD_CANVAS_HD_VMAX0 18
#define OSD_CANVAS_HD_HMAX1 53
#define OSD_CANVAS_HD_VMAX1 20

/**
 *  MSP v2 Protocol Command Types
 */
#define MSP_ELRS_BACKPACK_GET_CHANNEL_INDEX   0x0300
#define MSP_ELRS_BACKPACK_SET_CHANNEL_INDEX   0x0301
#define MSP_ELRS_BACKPACK_GET_FREQUENCY       0x0302
#define MSP_ELRS_BACKPACK_SET_FREQUENCY       0x0303
#define MSP_ELRS_BACKPACK_GET_RECORDING_STATE 0x0304
#define MSP_ELRS_BACKPACK_SET_RECORDING_STATE 0x0305
#define MSP_ELRS_BACKPACK_GET_VRX_MODE        0x0306
#define MSP_ELRS_BACKPACK_SET_VRX_MODE        0x0307
#define MSP_ELRS_BACKPACK_GET_RSSI            0x0308
#define MSP_ELRS_BACKPACK_GET_BATTERY_VOLTAGE 0x0309
#define MSP_ELRS_BACKPACK_GET_FIRMWARE        0x030A
#define MSP_ELRS_BACKPACK_SET_BUZZER          0x030B
#define MSP_ELRS_BACKPACK_SET_OSD_ELEMENT     0x030C
#define MSP_ELRS_BACKPACK_SET_HEAD_TRACKING   0x030D // enable/disable head-tracking forwarding packets to the TX
#define MSP_ELRS_BACKPACK_SET_RTC             0x030E

// incoming, packets originating from the VRx
#define MSP_ELRS_BACKPACK_SET_MODE    0x0380 // enable wifi/binding mode
#define MSP_ELRS_BACKPACK_GET_VERSION 0x0381 // get the bacpack firmware version
#define MSP_ELRS_BACKPACK_GET_STATUS  0x0382 // get the status of the backpack
#define MSP_ELRS_BACKPACK_SET_PTR     0x0383 // forwarded back to TX backpack

// incoming uart packets
#define MSP_VTX_GET_MODEL_NAME  0x0384 // Query VTX for name
#define MSP_VTX_GET_FC_VARIANT  0x0385 // Query VTX for flight controller variant
#define MSP_VTX_GET_FW_VERSION  0x0386 // Query VTX for firmware version
#define MSP_VTX_GET_TEMPERATURE 0x0387 // Query VTX for temperature in celcius
#define MSP_VTX_GET_HW_FAULTS   0x0388 // Query VTX for hardware errors

#endif /* __MSP_PROTO_H_ */
