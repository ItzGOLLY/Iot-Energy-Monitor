# Hardware Setup Guide

## Components Required

| Component | Specification | Quantity |
|-----------|--------------|----------|
| ESP32 DevKit | ESP32-WROOM-32 | 1 |
| Energy Meter | Schneider Conzerv EM6400NG | 1 |
| RS485 Module | MAX485/TTL to RS485 | 1 |
| Jumper Wires | Dupont M-M, M-F | 20+ |
| USB Cable | Micro-USB or Type-C | 1 |
| Power Supply | 5V/2A for ESP32 | 1 |

## Wiring Diagram

### RS485 Module to ESP32
RS485 Module        ESP32
VCC       →        3.3V
GND       →        GND
RO (RX)   →        GPIO16 (RX2)
DI (TX)   →        GPIO17 (TX2)
DE/RE     →        3.3V (Auto-transmit enable)

### RS485 Module to Energy Meter
RS485 A   →        Meter Terminal A (+)
RS485 B   →        Meter Terminal B (-)
GND       →        Meter GND (optional, for noise immunity)

### Reset Button
GPIO0     →        Button → GND

## Meter Configuration

1. **Set Modbus Address**: Configure meter slave ID = 1
2. **Verify Baud Rate**: Ensure 9600 baud, 8-N-1
3. **Check Register Map**: Download Schneider EM6400NG Modbus register datasheet

## Power On Sequence

1. Connect energy meter to mains power
2. Connect RS485 module to meter
3. Connect ESP32 to RS485 module
4. Power ESP32 via USB or 5V adapter
5. Open Serial Monitor (115200 baud) to view boot logs

## First-Time Configuration

See main README.md for WiFiManager portal instructions.

