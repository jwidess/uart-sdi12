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
               uint8_t enRxBit,
               uint8_t uartNum,
               uint32_t cpuFreq,
               volatile uint32_t* tick_ptr)
    : _enTxPort(enTxPort),
      _enTxBit(enTxBit),
      _enRxPort(enRxPort),
      _enRxBit(enRxBit),
      _cpuFreq(cpuFreq),
      _tick_ptr(tick_ptr),
      _initialized(false) {

    // Initialize UART registers based on uartNum
    switch (uartNum) {
    case 0:
        _ucsrNa = &UCSR0A;
        _ucsrNb = &UCSR0B;
        _ucsrNc = &UCSR0C;
        _ubrrN = &UBRR0;
        _udrN = &UDR0;
        _udreN_bit = UDRE0;
        break;
    case 1:
        _ucsrNa = &UCSR1A;
        _ucsrNb = &UCSR1B;
        _ucsrNc = &UCSR1C;
        _ubrrN = &UBRR1;
        _udrN = &UDR1;
        _udreN_bit = UDRE1;
        break;
    case 2:
        _ucsrNa = &UCSR2A;
        _ucsrNb = &UCSR2B;
        _ucsrNc = &UCSR2C;
        _ubrrN = &UBRR2;
        _udrN = &UDR2;
        _udreN_bit = UDRE2;
        break;
    case 3:
        _ucsrNa = &UCSR3A;
        _ucsrNb = &UCSR3B;
        _ucsrNc = &UCSR3C;
        _ubrrN = &UBRR3;
        _udrN = &UDR3;
        _udreN_bit = UDRE3;
        break;
    default:
        //! Will add error handling later
        _udrN = 0;
        _udreN_bit = 0;
        break;
    }
}

void USDI12::begin() {
    // Set DDR bits for both GPIO pins
    *(_enTxPort - 1) |= _enTxBit; // DDRx = PORTx - 1
    *(_enRxPort - 1) |= _enRxBit;
    _initialized = true;
}

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

void USDI12::setBit(volatile uint8_t* port, uint8_t bit) {
    *port |= bit;
}

void USDI12::clearBit(volatile uint8_t* port, uint8_t bit) {
    *port &= ~bit;
}

bool USDI12::begin_uart() {
    // Calc Baud for UBRR Reg
    uint16_t ubrr = (_cpuFreq / 16 / SDI12_BAUD_RATE) - 1;
    if (ubrr > 4095 || ubrr < 0) {
        return false; // UBRR value out of range for 12-bit reg
    }
    *_ubrrN = ubrr;

    // Enable receiver and transmitter
    // RXENn = 4, TXENn = 3 in UCSRnB
    *_ucsrNb = (1 << 4) | (1 << 3);

    // Set frame format: 7 data bits, even parity, 1 stop bit
    // UCSZn1 = 2, UCSZn0 = 1 (for 7 bits: UCSZn1=1, UCSZn0=0)
    // UPMn1 = 5, UPMn0 = 4 (for even parity: UPMn1=1, UPMn0=0)
    // USBSn = 3 (for 1 stop bit: USBSn=0)
    *_ucsrNc = (1 << 2) | (1 << 5); // UCSZn1 | UPMn1

    return true;
}

void USDI12::uart_send_byte(uint8_t data) {
    // Wait for the Data Register Empty flag to be set
    // This indicates that the UART is ready to send data
    while (!(*_ucsrNa & (1 << _udreN_bit)));
    *_udrN = data;
}

bool USDI12::send_command(uint8_t address, const char* command) {
    if (!_initialized || address > '9' || address < '0') {
        return false;
    }

    set_tx();
    // Send address character
    uart_send_byte(address);
    // Send command string
    while (*command != '\0') {
        uart_send_byte(*command++);
    }
    // Send termination character (!) if not already present
    if (*(command - 1) != '!') {
        uart_send_byte('!');
    }
    // Add CRLF sequence
    uart_send_byte('\r');
    uart_send_byte('\n');

    set_rx();

    return true;
}

bool USDI12::read_response(char* buffer, uint32_t timeout_ticks) {
    set_rx();
    if (!buffer || !_tick_ptr) return false; // Check for null pointers
    if (!_initialized) begin();             // Ensure DDRs are set
    int idx = 0;                            // Index for buffer
    bool got_cr = false;
    uint32_t start_tick = *_tick_ptr;
    while (((*_tick_ptr - start_tick) < timeout_ticks + 1) &&
           (idx < USDI12_BUFFER_SIZE - 1)) {
        // RXCn is always bit 7 in UCSRnA
        if (*_ucsrNa & (1 << 7)) {      // Check if data is available
            char c = (char)(*_udrN);    // Read the received byte
            buffer[idx++] = c;         // Store it in the buffer
            if (got_cr && c == '\n') { // Check for CRLF sequence
                buffer[idx] = '\0';    // Null-terminate the string
                return true;           // Success: got CRLF
            }
            got_cr = (c == '\r'); // Check if we got a CR character and set flag
        }
    }
    buffer[idx] = '\0';
    return false; // Timeout or buffer full
}
