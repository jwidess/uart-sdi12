// clang-format off
/**
 * @file USDI12.hpp
 * @author Justin Widen (justinwiden1@gmail.com)
 *
 * @brief Header file for the UART SDI-12 library.
 *
 * ============== UART SDI-12 ==============
 *
 * This is an SDI-12 implementation that uses a UART serial port to communicate
 * with SDI-12 sensors.
 *
 * =========================================
 */
/**
 * @section physical_connections Physical Connections
 * | Signal Name | Connected To           | Functionality                                      |
 * | ----------- | ---------------------- | -------------------------------------------------- |
 * | `!EN_TX`    | - Transceiver: `!OEB`  | Enables transceiver B-side driver (TX when LOW)    |
 * |      |----> | - Analog Switch: `SEL` | Selects TX (LOW) or RX (HIGH) path                 |
 * | `EN_RX`     | - Transceiver: `OEA`   | Enables transceiver A-side receiver (RX when HIGH) |
 * | `!EN`       | - Analog Switch: `!EN` | Always LOW (switch always enabled)                 |
 *
 * @section mode_control_logic Mode Control Logic
 * | Mode | `!EN_TX` | `EN_RX` | Analog Switch Path | Transceiver Direction      |
 * | ---- | -------- | ------- | ------------------ | -------------------------- |
 * | TX   | `0`      | `0`     | `SEL = 0` → NC→COM | A=IN, B=OUT (TX to SDI-12) |
 * | RX   | `1`      | `1`     | `SEL = 1` → NO→COM | A=OUT, B=IN (SDI-12 to RX) |
 *
 */
// clang-format on

#ifndef USDI12_H
#define USDI12_H

#include <avr/io.h>
#include <stdio.h>  // For snprintf, sscanf
#include <string.h> // For strncat, strncpy

#ifndef USDI12_BUFFER_SIZE
/**
 * @brief The buffer size for incoming SDI-12 data.
 * CREDIT: Arduino SDI-12 Library
 * All responses should be less than 81 characters:
 * - address is a single (1) character
 * - values has a maximum value of 75 characters
 * - CRC is 3 characters
 * - CR is a single character
 * - LF is a single character
 */
#define USDI12_BUFFER_SIZE 81
#endif

// SDI-12 result codes for get_measurement (C++03 compatible)
enum USDI12Result {
    USDI12Result_Success = 0,
    USDI12Result_InputError,
    USDI12Result_Timeout,
    USDI12Result_InvalidResponse,
    USDI12Result_CommandError,
    USDI12Result_BufferOverflow,
    USDI12Result_NullPointer,
    USDI12Result_Unexpected
};

class USDI12 {
  public:
    // TX Port, TX Pin, RX Port, RX Pin, UARTn
    USDI12(volatile uint8_t* enTxPort,
           uint8_t enTxBit,
           volatile uint8_t* enRxPort,
           uint8_t enRxBit,
           uint8_t uartNum,
           uint32_t cpuFreq,
           volatile uint32_t* tick_ptr);

    // Setup Functions
    void set_tx(); // Set GPIOs for Transmit mode
    void set_rx(); // Set GPIOs for Receive mode
    bool begin_uart(); // Initialize UART for SDI-12 communication

    // SDI-12 Functions
    bool send_command(uint8_t address, const char* command);
    // Read response from SDI-12 device with timeout (ticks)
    // Returns true if response received, false on timeout
    bool read_response(char* buffer, uint32_t timeout_ticks, uint16_t buffer_size);

    /**
     * @brief Initiates a measurement and retrieves all measurement values from the SDI-12 sensor.
     * @param address SDI-12 address (0-9)
     * @param measurement_number Optional measurement number (0-9), default is -1 (standard M command)
     * @param result_buffer Buffer to store the concatenated measurement values (null-terminated)
     * @param buffer_size Size of the result_buffer
     * @return USDI12Result enum indicating result or error type
     */
    USDI12Result get_measurement(uint8_t address, char* result_buffer, uint16_t buffer_size, int8_t measurement_number = -1);

  private:
    // Declarations
    // GPIO pin configuration
    volatile uint8_t* _enTxPort;
    uint8_t _enTxBit;
    volatile uint8_t* _enRxPort;
    uint8_t _enRxBit;

    uint32_t _cpuFreq; // Used for calculating UBRR value

    volatile uint32_t* _tick_ptr; // Pointer to system tick counter (for timeouts)

    bool _initialized; // Tracks if DDRs have been set
    // End Declarations
    
    // Private Functions
    void begin(); // Sets DDRx bits (called automatically once)
    void setBit(volatile uint8_t* port, uint8_t bit);
    void clearBit(volatile uint8_t* port, uint8_t bit);
    void uart_send_byte(uint8_t data);
    // End Private Functions
    
    // UART Register pointers
    volatile uint8_t* _ucsrNa; // UCSRnA Control and Status Reg A (DS: 22.10.2)
    volatile uint8_t* _ucsrNb; // UCSRnB Control and Status Reg B (DS: 22.10.3)
    volatile uint8_t* _ucsrNc; // UCSRnC Control and Status Reg C (DS: 22.10.4)
    volatile uint16_t* _ubrrN; // UBRRn 12 bit reg (DS: 22.10.5)
    volatile uint8_t* _udrN;   // Pointer to UART data register
    uint8_t _udreN_bit;        // Bit position for UDREn (Data Register Empty)

    // SDI-12 UART Config
    static const uint16_t SDI12_BAUD_RATE = 1200;
    static const uint8_t SDI12_DATA_BITS = 7;
    static const uint8_t SDI12_PARITY_EVEN = 1;
    static const uint8_t SDI12_STOP_BITS = 1;
};

#endif // USDI12_H
