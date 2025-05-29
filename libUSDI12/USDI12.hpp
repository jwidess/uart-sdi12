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

class USDI12 {
  public:
    // TX Port, TX Pin, RX Port, RX Pin, UARTn
    USDI12(volatile uint8_t* enTxPort,
           uint8_t enTxBit,
           volatile uint8_t* enRxPort,
           uint8_t enRxBit,
           uint8_t uartNum,
           uint32_t cpuFreq);

    // Setup Functions
    void set_tx(); // Set GPIOs for Transmit mode
    void set_rx(); // Set GPIOs for Receive mode
    bool begin_uart(); // Initialize UART for SDI-12 communication

  private:
    // Declartions
    // GPIO pin configuration
    volatile uint8_t* _enTxPort;
    uint8_t _enTxBit;

    volatile uint8_t* _enRxPort;
    uint8_t _enRxBit;

    uint32_t _cpuFreq; // Used for calculating UBRR value

    bool _initialized; // Tracks if DDRs have been set
    // End Declarations
    
    // Private Functions
    void begin(); // Sets DDRx bits (called automatically once)
    void setBit(volatile uint8_t* port, uint8_t bit);
    void clearBit(volatile uint8_t* port, uint8_t bit);
    // End Private Functions
    
    // UART Register pointers
    volatile uint8_t* _ucsra;
    volatile uint8_t* _ucsrb;
    volatile uint8_t* _ucsrc;
    volatile uint16_t* _ubrr; // UBRR 12 bit reg (DS: 22.10.5)

    // SDI-12 UART Config
    static const uint16_t SDI12_BAUD_RATE = 1200;
    static const uint8_t SDI12_DATA_BITS = 7;
    static const uint8_t SDI12_PARITY_EVEN = 1;
    static const uint8_t SDI12_STOP_BITS = 1;
};

#endif // USDI12_H
