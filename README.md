# uart-sdi12
A C++ Library for SDI-12 Communication via a hardware UART implementation.

## Overview
This library provides SDI-12 protocol support using a hardware UART port and minimal external parts. It comes with multi-bus support, error handling, and hardware abstraction for AVR microcontrollers.

## Features
- SDI-12 protocol over UART
- Multi-bus support (controlled via a single direction (DIR) pin)
- Hardware Abstraction Layer (HAL) for portability
- Error handling with `USDI12Result` enum
- UART error detection (frame, overrun, parity, multi-error)
- Example test program for Arduino Mega/AVR

## Supported SDI-12 Commands
The project provides specific high-level support for the following SDI-12 sequences:
- **Start Measurement (`aM!`, `aMn!`)**: Fully implemented via `get_measurement()`. This handles the command transmission, waiting for the sensor service request (or timeout), and subsequent data collection.
- **Send Data (`aD0!` ... `aD9!`)**: Utilized by `get_measurement()` to retrieve all available data values.

All other SDI-12 commands (e.g., `?!`, `aI!`, `aAb!`, `aV!`) can be utilized via `send_command()` and `read_response()` methods. However, high level implementation has not yet been created for these. More info in the following Limitations section.

## Limitations
While the project supports the core SDI-12 protocol, the following features are not currently implemented in the high-level API:
- **CRC Verification:** Commands requesting CRC (e.g., `aMC!`, `aCC!`) are not explicitly supported. While you can send these commands manually, the library does not validate the checksum in the response.
- **Concurrent Measurements (`aC!`):** There is no dedicated non-blocking helper for concurrent measurements. The `get_measurement()` function is designed for standard measurements and blocks until the measurement is complete.
- **Continuous Measurements (`aR!`):** No method for continuous measurements.

## Hardware
This library supports dual SDI-12 busses, however it is not required to utilize both. This functionality was added due to the fact the SDI-12 specification does **NOT** require sensors to support changing of their address. Thus if you where to ever need to use two identical sensors (that can't change addresses) on one device, you must use two buses. Alongside this, the AVR platform doesn't support UART inversion, thus physical inversion of the protocol is needed for TX and RX.

With this setup, it is required that a direction pin is configured to swap which bus is connected to RX and TX respectively. With the hardware setup described below the following truth table shows the connections of RX and TX with respect to the Direction pin. With the class nature of this library, the following truth table can be ignored as `set_tx` and `set_rx` handle this logic depending on bus number.

| Dir GPIO | Bus 0 (B0) | Bus 1 (B1) |
| -------- | ---------- | ---------- |
| 1        | RX         | TX         |
| 0        | TX         | RX         |

### 2 Component Hardware Design
This design uses only two ICs to provide both protocol inversion and dual-bus switching. It utilizes a **74AHCT125** Quad Bus Buffer Gates (tri-state) alongside a **74HCT04** hex inverter. Below is an example schematic

![2 Component Hardware Design Example KiCad Schematic](/2_Comp_HW_Design_Sch.png)

> [!NOTE]  
> A simpler version of this circuit could be made using the **SN74LVC2G241** as it incorporates both inverted and non inverted tri-states.


## API Summary

### Class: USDI12
**Constructor:**
```cpp
USDI12(USDI12_HAL* hal, uint8_t bus);
```
- `hal`: Pointer to HAL implementation (e.g., `AVR_HAL`)
- `bus`: Bus number (0 or 1)

**Setup Methods:**
- `void set_tx();` — Set GPIOs for transmit mode
- `void set_rx();` — Set GPIOs for receive mode
- `bool begin_uart(uint32_t cpuFreq);` — Initialize UART for SDI-12

**SDI-12 Communication Methods:**
- `void send_break_mark(uint16_t break_ms = 12, uint16_t mark_ms = 9);` — Send SDI-12 break/mark sequence
- `USDI12Result send_command(char address, const char* command);` — Send command to device
- `USDI12Result read_response(char* buffer, uint32_t timeout_ms, uint16_t buffer_size);` — Read response with timeout
- `USDI12Result get_measurement(uint8_t address, char* result_buffer, uint16_t buffer_size, int8_t measurement_number = -1);` — Get measurement values
- `USDI12Result wait_bus_idle(uint32_t timeout_ms = 100);` — Wait for bus idle for upto timeout_ms

**Error Handling:**
- All major methods return a `USDI12Result` enum value
- Use `USDI12ResultNames[result]` for human-readable error names

### Enum: USDI12Result
```cpp
enum USDI12Result {
  USDI12Result_Success = 0,
  USDI12Result_InputError,       // 1
  USDI12Result_Timeout,          // 2
  USDI12Result_InvalidResponse,  // 3
  USDI12Result_CommandError,     // 4
  USDI12Result_BufferOverflow,   // 5
  USDI12Result_NullPointer,      // 6
  USDI12Result_Unexpected,       // 7
  USDI12Result_FrameError,       // 8
  USDI12Result_OverrunError,     // 9
  USDI12Result_ParityError,      // 10
  USDI12Result_UARTMultiErrors   // 11
};
```
- Use `USDI12_RESULT_COUNT` for bounds checking

### HAL: USDI12_HAL and AVR_HAL
- `USDI12_HAL`: Abstract base class for hardware operations
- `AVR_HAL`: AVR implementation, supports all UART ports.

## Example: main.cpp
```cpp
#include "USDI12.hpp"
#include "config.h"

volatile uint32_t ms_tick = 0;
AVR_HAL avr_hal(&SDI12_DIR_PORT, (1 << SDI12_DIR_PIN), UART_USDI12_NUM, &ms_tick, 1000.0f);
USDI12 sdi12_b0(&avr_hal, 0);  // Bus 0
USDI12 sdi12_b1(&avr_hal, 1);  // Bus 1

sdi12_b0.begin_uart(F_CPU);
sdi12_b1.begin_uart(F_CPU);

char sdi12_buffer[USDI12_BUFFER_SIZE] = {0};

// Send address query to bus 0
sdi12_b0.send_break_mark();
sdi12_b0.send_command(-1, "?!"); 
USDI12Result addr_result = sdi12_b0.read_response(sdi12_buffer, 100, USDI12_BUFFER_SIZE);
if (addr_result == USDI12Result_Success) {
    // Address found
}

// Send identification command to bus 1
sdi12_b1.send_break_mark();
sdi12_b1.send_command('0', "I!");
USDI12Result id_result = sdi12_b1.read_response(sdi12_buffer, 300, USDI12_BUFFER_SIZE);
if (id_result == USDI12Result_Success) {
    // Identification received
}

// Get measurement from address 0 on bus 0
sdi12_b0.send_break_mark();
USDI12Result meas_result = sdi12_b0.get_measurement('0', sdi12_buffer, USDI12_BUFFER_SIZE);
if (meas_result == USDI12Result_Success) {
    // Measurement values in sdi12_buffer
}
```

## Example: config.h
```cpp
#ifndef CONFIG_H
#define CONFIG_H
// MCU Configuration
#define F_CPU 16000000UL // 16MHz
// UART Configuration
#define UART_USDI12_NUM 2  // Use UART2 for SDI-12
// Pin Definitions
// Direction control pin
#define SDI12_DIR_PORT PORTH
#define SDI12_DIR_PIN PH4

#endif  // CONFIG_H
```

## Error Handling Example
```cpp
if (result >= 0 && result < USDI12_RESULT_COUNT) {
    uart0_send_string(USDI12ResultNames[result]);
} else {
    uart0_send_string("Unknown");
}
```

## Hardware Abstraction Example

```cpp
// AVR_HAL constructor parameters:
//   dirPort: Pointer to direction GPIO port (e.g., &PORTx)
//   dirBit: Bit mask for direction pin (e.g., (1 << PINx))
//   uart_num: UART port number (0-3)
//   tick_ptr: Pointer to tick counter (for timing)
//   ticks_per_second: Tick frequency (e.g., 1000.0f for 1ms)
AVR_HAL avr_hal(&SDI12_DIR_PORT, (1 << SDI12_DIR_PIN), UART_USDI12_NUM, &ms_tick, 1000.0f);
USDI12 sdi12(&avr_hal, 0);
```

## Notes
- Currently supports the AVR platform, no other HALs created yet.
- See `\UART-SDI12-LibraryTest-V1\UART-SDI12-LibraryTest-V1\main.cpp` for a full test/demo program
- For info on how to add external files/"libraries", like this project, to a Microchip Studio project, see [Microchip Studio Notes](<Microchip Studio Testing Programs/MicrochipStudioNotes.md>).


## Credits

- **https://github.com/EnviroDIY/Arduino-SDI-12**
  - The EnviroDIY Arduino-SDI-12 library was very helpful for testing SDI-12 communication, and also inspired some of the program structure in this library. Thank you to all of the amazing devs that created that library!
- **Jolon Behrent, "Development of an IoT System for Environmental Monitoring", https://jolonb.github.io/assets/pdf/final_report.pdf**
  - Thanks to Jolon Behrent for his SDI-12 hardware implementation as it was the primary inspiration for my design.
