# uart-sdi12
A C++ Library for SDI-12 Communication via a HW UART Implementation.

## USDI12 Library: Available Methods & Usage

### Class: USDI12

**Constructor:**
```cpp
USDI12(volatile uint8_t* enTxPort, uint8_t enTxBit,
       volatile uint8_t* enRxPort, uint8_t enRxBit, uint8_t uartNum,
       uint32_t cpuFreq, volatile uint32_t* tick_ptr);
```
Initializes the library with GPIO and UART settings.

**Setup Methods:**
- `void set_tx();` — Set GPIOs for transmit mode (TX)
- `void set_rx();` — Set GPIOs for receive mode (RX)
- `bool begin_uart();` — Initialize UART for SDI-12 communication

**SDI-12 Communication Methods:**
- `bool send_command(uint8_t address, const char* command);`
  - Send a command to the SDI-12 device at the given address.
- `bool read_response(char* buffer, uint32_t timeout_ticks, uint16_t buffer_size);`
  - Read a response from the SDI-12 device into a buffer, with timeout.
- `USDI12Result get_measurement(uint8_t address, char* result_buffer, uint16_t buffer_size, int8_t measurement_number = -1);`
  - Initiate a measurement and retrieve all measurement values from the SDI-12 sensor.
  - Returns a result code (see below).

**Result Codes (enum USDI12Result):**
```cpp
  USDI12Result_Success = 0,
  USDI12Result_InputError,       // 1
  USDI12Result_Timeout,          // 2
  USDI12Result_InvalidResponse,  // 3
  USDI12Result_CommandError,     // 4
  USDI12Result_BufferOverflow,   // 5
  USDI12Result_NullPointer,      // 6
  USDI12Result_Unexpected        // 7
```

### Example Usage
```cpp
#include "USDI12.hpp"

volatile uint32_t system_tick = 0;
USDI12 sdi12(&SDI12_TX_PORT, (1 << SDI12_TX_PIN), &SDI12_RX_PORT,
             (1 << SDI12_RX_PIN), SDI12_UART_NUM, F_CPU, &system_tick);

sdi12.set_rx();
sdi12.set_tx();
sdi12.begin_uart();

char buffer[USDI12_BUFFER_SIZE] = {0};
int8_t result = sdi12.get_measurement('0', buffer, USDI12_BUFFER_SIZE, 1);
if (result == USDI12Result_Success) {
    // buffer contains SDI-12 measurement values
}
```