# Troubleshooting Guide

## Common Issues

### 1. ESP32 Won't Boot / Brownout

**Symptoms**: Reboot loop, "Brownout detector was triggered" message

**Solutions**:
- Use external 5V/2A power supply (not USB port power)
- Add 100µF capacitor between 3.3V and GND
- Use shorter USB cable

### 2. Modbus Read Errors (0xE2, 0x02)

**Symptoms**: "Error reading X (reg Y): 0xE2"

**Solutions**:
- Check RS485 A/B polarity (swap if needed)
- Verify meter slave ID matches `node.begin(1, ...)`
- Confirm baud rate: 9600, 8 data bits, no parity, 1 stop bit
- Check terminating resistor (120Ω) on long cables
- Ensure meter is powered and displaying readings

### 3. WiFi Connection Fails

**Symptoms**: "Failed to connect and hit timeout"

**Solutions**:
- Hold GPIO0 button for 3 seconds → clears credentials
- Reconnect to `AutoConnectAP` and re-enter credentials
- Check WiFi password (case-sensitive)
- Ensure 2.4GHz network (ESP32 doesn't support 5GHz)

### 4. MQTT Publish Fails

**Symptoms**: "Failed to publish message"

**Solutions**:
- Verify Zoho IoT credentials in portal/config
- Check TLS certificate validity (update `certificate.h`)
- Confirm device is registered in Zoho IoT console
- Check firewall rules (port 8883 for MQTT over TLS)

### 5. Incorrect Sensor Values

**Symptoms**: Values like "0.0000" or extremely large numbers

**Solutions**:
- Verify register addresses match meter datasheet
- Check byte order in `readFloatHex()` function
- Confirm data type (float vs integer) for each register
- Calibrate using known load values

## Debug Mode

Enable verbose logging by adding to `setup()`:

```cpp
Serial.setDebugOutput(true);

Getting Help
Open an issue on GitHub
Check Zoho IoT Documentation
Review Schneider EM6400NG Modbus manual
