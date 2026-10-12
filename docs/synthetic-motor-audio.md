# Synthetic Motor Audio (VTX side)

Proof-of-concept: forward Betaflight motor RPM to the goggles over the existing HDZero `0xFF` service packet. No new MSP/RF opcodes, no Betaflight changes, MSP RX buffer stays 64 bytes.

## Flow

1. After FC init, VTX requests `MSP_MOTOR_TELEMETRY` (139) from Betaflight (`BTFL` only), ~8 Hz with the other status polls.
2. Parser keeps mechanical RPM for motors 0..3 (quad). Hex/octo ignored.
3. `get_tx_data_5680()` appends a 6-byte packed RPM tail to the `0xFF` service payload.

## `0xFF` service packet

| Field | Bytes | Notes |
|-------|-------|--------|
| Header | `0x56 0x80` | unchanged |
| Index | `0xFF` | service / config |
| Length | `21` | was `15` |
| Legacy payload | 15 | camera, FC variant, LQ, temp, … (unchanged) |
| Motor RPM | 6 | new; four 12-bit values |
| CRC | 2 | over header through payload |

Legacy goggles that honor the length field accept the longer packet and ignore the unknown tail. New goggles decode RPM when `length >= 21`.

## RPM packing

- Quantize: `q = min((rpm + 16) >> 5, 4095)` → step 32 RPM, max representable 131040.
- Wire (little-endian nibbles):

```
b0 = q0[7:0]
b1 = q1[3:0] << 4 | q0[11:8]
b2 = q1[11:4]
b3 = q2[7:0]
b4 = q3[3:0] << 4 | q2[11:8]
b5 = q3[11:4]
```

Motor order matches Betaflight / QUADX indices 0..3.

## MSP_MOTOR_TELEMETRY (FC → VTX)

Payload: `uint8 motorCount`, then 13 bytes per motor. First `uint32` of each record is mechanical RPM. Quad reply is 53 bytes (fits in the 64-byte `msp_rx_buf`).
