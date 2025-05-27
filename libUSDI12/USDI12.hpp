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
    USDI12(volatile uint8_t* enTxPort,
           uint8_t enTxBit,
           volatile uint8_t* enRxPort,
           uint8_t enRxBit);

    void set_tx(); // Set GPIOs for Transmit mode
    void set_rx(); // Set GPIOs for Receive mode

  private:
    // GPIO pin configuration
    volatile uint8_t* _enTxPort;
    uint8_t _enTxBit;

    volatile uint8_t* _enRxPort;
    uint8_t _enRxBit;

    bool _initialized; // Tracks if DDRs have been set

    void begin(); // Sets DDRx bits (called automatically once)
    void setBit(volatile uint8_t* port, uint8_t bit);
    void clearBit(volatile uint8_t* port, uint8_t bit);
};

#endif // USDI12_H
