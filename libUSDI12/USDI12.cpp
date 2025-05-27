// clang-format off
/**
 * @file USDI12.cpp
 * @author Justin Widen (justinwiden1@gmail.com)
 *
 * @brief Implementation file for the UART SDI-12 library.
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

#include "USDI12.hpp"

USDI12::USDI12(volatile uint8_t* enTxPort,
               uint8_t enTxBit,
               volatile uint8_t* enRxPort,
               uint8_t enRxBit)
    : _enTxPort(enTxPort),
      _enTxBit(enTxBit),
      _enRxPort(enRxPort),
      _enRxBit(enRxBit),
      _initialized(false) {}

void USDI12::set_tx() {
    if (!_initialized) begin();

    clearBit(_enTxPort, _enTxBit); // - !EN_TX = 0
    clearBit(_enRxPort, _enRxBit); // - EN_RX  = 0
}

void USDI12::set_rx() {
    if (!_initialized) begin();

    setBit(_enTxPort, _enTxBit); // - !EN_TX = 1
    setBit(_enRxPort, _enRxBit); // - EN_RX  = 1
}

void USDI12::begin() {
    // Set DDR bits for both GPIO pins
    *(_enTxPort - 1) |= _enTxBit; // DDRx = PORTx - 1
    *(_enRxPort - 1) |= _enRxBit;
    _initialized = true;
}

void USDI12::setBit(volatile uint8_t* port, uint8_t bit) {
    *port |= bit;
}

void USDI12::clearBit(volatile uint8_t* port, uint8_t bit) {
    *port &= ~bit;
}
