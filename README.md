# Arduino Mini-Based Controller

### Media

#### Hardware Image
![Hardware Image](https://github.com/payalneg/s886-adapter/blob/master/media/s886.JPG)

#### Example Setup
![Example Setup](https://github.com/payalneg/s886-adapter/blob/master/media/photo_2025-01-18_15-36-38.jpg)

This project uses an Arduino Mini (ATmega168 or ATmega328, 5V/16MHz) to control a BLDC motor and interface with an S886 module. It includes features like throttle control, speed display, cruise control, and regenerative braking.

---

## Features and Pin Configuration

### Communication with S886 Module
- **Pin 10 (RX)**: Software Serial - Receive  
- **Pin 11 (TX)**: Software Serial - Transmit  

### Throttle Control
- **Pin 9**: PWM-based throttle output.  
  - Use an RC circuit (1kΩ resistor and 1µF capacitor) to filter noise.

### Speed Measurement
- **Pin 2**: Input for the phase signal from the hall sensor.  
  - Use an RC circuit (10kΩ resistor and 10nF capacitor) to filter noise.  
  - Pulses from the hall sensor are converted to speed and displayed on an LCD.

### Cruise Control and E-Brake
- **Pin 4**: Cruise control enable.  
  - Connected to the "Q" pin on your BLDC controller.  
- **Pin 5**: E-Brake enable.  
  - Connected to the "XL" pin on your BLDC controller.  
- **Pin 6**: Brake input.  
  - Connected to the "DS" pin on your BLDC controller.  

### Regenerative Braking
- **Pin 7**: Output for regenerative braking.  
  - Used with a Step-Up DC-DC converter for battery charging.

### Speed Display Testing
- **Pin 3**: Generates a PWM signal for testing speed display functionality.

---

## Notes
- This controller is designed for use with 5V/16MHz Arduino Mini boards.  
- The S886 module may vary by manufacturer; ensure compatibility with your BLDC controller.  
- The setup includes RC circuits for noise filtering in throttle and hall sensor signals.

---

## Circuit Diagram
Include a circuit diagram here or link to a detailed schematic if available.

---

## Usage
1. Connect the components as per the pin configuration above.
2. Upload the provided Arduino sketch to the board.
3. Test individual features like throttle, speed display, and regenerative braking.

# Reverse Engineering: Controller Communication

This document describes the reverse-engineered communication protocol of a controller. The protocol uses a simple UART configuration:

- **Voltage:** 0-5V
- **Baud Rate:** 9600 bps
- **Data Format:** 8N1 (8 data bits, no parity, 1 stop bit)

---

## INPUT Communication Format

Example Messages:
```
01 14 01 01 05 80 16 01 04 01 05 01 64 05 01 86 00 00 06 66
01 14 01 01 03 90 14 01 04 03 01 00 32 01 01 86 00 00 05 24
01 14 01 01 03 90 14 01 04 03 01 00 32 01 01 86 00 A0 05 84 
01 14 01 01 06 90 14 01 04 03 01 00 32 01 01 86 00 00 05 21 
```

### Byte Descriptions

| Byte Index | Description | Notes |
|------------|-------------|-------|
| 0 | `0x01` | Header |
| 1 | `0x14` | Command |
| 2 | `0x01` | Reserved |
| 3 | Drive Mode Settings | 0 = Pedal, 1 = Electric Drive, 2 = Both |
| 4 | Gear | Values: 1 = `0x03`, 2 = `0x06`, 3 = `0x09`, 4 = `0x0C`, 5 = `0x0F`, 0 = `0x00` / `0x05` `0x0A' '0x0F` |
| 5 | Direct Start | `0xB0` = Direct Start, `0xF0` = Kick-to-Start |
| 6 | Magnets Count | |
| 7-8 | Wheel Diameter | Example: 26" = `0x0104` |
| 9 | Pedal Assist Sensitivity | |
| 10 | Pedal Assist Starting Intensity | |
| 11 | Reserved | Always `0x00` |
| 12 | Speed Limit | Example: 53 km/h = `0x35` |
| 13 | Current Limit Value | |
| 14-15 | Battery Cutoff Voltage | Examples: 24V = `0xBE`, 36V = `0x0122`, 48V = `0x0186`, 52V = `0x01A4`, 60V = `0x01EA` |
| 16-17 | Throttle | Range: `0x0000` to `0x03FF` |
| 18 | Flags | First 0-3 bits: Magnet count for Pedal Assist Sensor, Bit 6: Cruise, Bit 7: E-Brake |
| 19 | CRC | Cyclic Redundancy Check |

---

## OUTPUT Communication Format

Example Messages:
```
02 0E 01 40 80 00 00 00 17 70 00 00 FF 55 
02 0E 01 40 80 00 00 00 04 01 00 00 FF 37
02 0E 01 40 80 00 00 00 04 8F 00 00 FF B9
02 0E 01 40 80 00 00 00 04 01 00 00 FF 37
02 0E 01 40 80 00 00 00 04 F2 00 00 FF C4
```

### Byte Descriptions

| Byte Index | Description | Notes |
|------------|-------------|-------|
| 0 | `0x02` | Header |
| 1 | `0x0E` | Command |
| 2 | `0x01` | Reserved |
| 3 | Error Code | Examples: `0x40` = `E07`, `0x00` = No Error, `0x20` = `E08`, `0x01` = `E01`, `0x08` = `E06` |
| 4 | Additional Errors | |
| 5-7 | Reserved | Always `0x00` |
| 8-9 | Speed | Examples: `0x1770` = 0 km/h, `0x210` = 8.4 km/h, `0x110` = 16.4 km/h (26"), `0x50` = 55.9 km/h |
| 10-11 | Reserved | Always `0x00` |
| 12 | `0xFF` | Reserved |
| 13 | CRC | Cyclic Redundancy Check |

---

### Notes:
1. Ensure CRC is correctly calculated to validate communication integrity.
2. Adjust settings based on the device's operating manual and requirements.
3. Further exploration may reveal additional commands or responses.

---

## License
This project is licensed under the MIT License. Feel free to use, modify, and share.
