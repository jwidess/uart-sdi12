// clang-format off
/*
 * USDI12_HAL.hpp
 * Hardware Abstraction Layer (HAL) for SDI-12 communication
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

#pragma once
#include <stdint.h>

// Base HAL interface for SDI-12 hardware
class USDI12_HAL {
 public:
  virtual void set_tx() = 0;
  virtual void set_rx() = 0;
  virtual bool begin_uart(uint32_t cpuFreq) = 0;
  virtual void uart_send_byte(uint8_t data) = 0;
  virtual bool uart_data_available() = 0;
  virtual uint8_t uart_read_byte() = 0;
  virtual void wait_for_tx_complete() = 0;
  virtual uint32_t get_tick() = 0;         // Returns current tick count
  virtual float ticks_per_second() = 0;    // Returns tick frequency
  virtual void delay_ms(uint32_t ms) = 0;  // Delay for specified milliseconds
  virtual void uart_tx_pin_low() = 0;      // Force TX pin LOW (break)
  virtual void uart_tx_pin_high() = 0;     // Force TX pin HIGH (marking)
  virtual void disable_uart_tx() = 0;      // Disable UART transmitter
  virtual void enable_uart_tx() = 0;       // Enable UART transmitter
};

// =============================
// AVR_HAL: AVR SDI-12 Hardware Abstraction Layer
#ifdef __AVR__
#include <avr/io.h>

/**
 * @class AVR_HAL
 * @brief Hardware Abstraction Layer implementation for SDI-12 on AVR
 * microcontrollers
 *
 * This class provides all hardware-specific operations required by the SDI-12
 * protocol, including UART configuration and GPIO control for TX/RX enable. The
 * UART number (0-3) is selected in the constructor, and all register pointers
 * are set automatically. ticks_per_second must be >=1000.0f to ensure proper
 * timing. Use this class with the USDI12 protocol class.
 *
 * Example usage:
 * AVR_HAL avr_hal(&SDI12_TX_PORT, (1 << SDI12_TX_PIN), &SDI12_RX_PORT, (1 <<
 * SDI12_RX_PIN), UART_USDI12_NUM, &ms_tick, 1000.0f);
 */
class AVR_HAL : public USDI12_HAL {
 public:
  AVR_HAL(volatile uint8_t* enTxPort, uint8_t enTxBit,
          volatile uint8_t* enRxPort, uint8_t enRxBit, uint8_t uart_num,
          volatile uint32_t* tick_ptr, float ticks_per_second)
      : _enTxPort(enTxPort),
        _enTxBit(enTxBit),
        _enRxPort(enRxPort),
        _enRxBit(enRxBit),
        _tick_ptr(tick_ptr),
        _ticks_per_second(ticks_per_second) {
    // Set UART register pointers, UDRE bit, and TX pin for each UART
    switch (uart_num) {
      case 0:
        _ucsrNa = &UCSR0A;
        _ucsrNb = &UCSR0B;
        _ucsrNc = &UCSR0C;
        _ubrrN = &UBRR0;
        _udrN = &UDR0;
        _udreN_bit = UDRE0;
        _tx_port = &PORTE;  // TXD0 = PE1
        _tx_ddr = &DDRE;
        _tx_bit = (1 << PE1);
        break;
      case 1:
        _ucsrNa = &UCSR1A;
        _ucsrNb = &UCSR1B;
        _ucsrNc = &UCSR1C;
        _ubrrN = &UBRR1;
        _udrN = &UDR1;
        _udreN_bit = UDRE1;
        _tx_port = &PORTD;  // TXD1 = PD3
        _tx_ddr = &DDRD;
        _tx_bit = (1 << PD3);
        break;
      case 2:
        _ucsrNa = &UCSR2A;
        _ucsrNb = &UCSR2B;
        _ucsrNc = &UCSR2C;
        _ubrrN = &UBRR2;
        _udrN = &UDR2;
        _udreN_bit = UDRE2;
        _tx_port = &PORTH;  // TXD2 = PH1
        _tx_ddr = &DDRH;
        _tx_bit = (1 << PH1);
        break;
      case 3:
        _ucsrNa = &UCSR3A;
        _ucsrNb = &UCSR3B;
        _ucsrNc = &UCSR3C;
        _ubrrN = &UBRR3;
        _udrN = &UDR3;
        _udreN_bit = UDRE3;
        _tx_port = &PORTJ;  // TXD3 = PJ1
        _tx_ddr = &DDRJ;
        _tx_bit = (1 << PJ1);
        break;
      default:  // Default to UART0
        _ucsrNa = &UCSR0A;
        _ucsrNb = &UCSR0B;
        _ucsrNc = &UCSR0C;
        _ubrrN = &UBRR0;
        _udrN = &UDR0;
        _udreN_bit = UDRE0;
        _tx_port = &PORTE;
        _tx_ddr = &DDRE;
        _tx_bit = (1 << PE1);
        break;
    }
  }

  void set_tx() {
    *_enTxPort &= ~_enTxBit;
    *_enRxPort &= ~_enRxBit;
  }
  void set_rx() {
    *_enTxPort |= _enTxBit;
    *_enRxPort |= _enRxBit;
  }

  bool begin_uart(uint32_t cpuFreq) {
    uint16_t ubrr = (cpuFreq / 16 / 1200) - 1;
    if (ubrr > 4095) return false;
    *_ubrrN = ubrr;  // Set baud rate
    // Enable receiver and transmitter
    *_ucsrNb = (1 << 4) | (1 << 3);  // RXENn = 4, TXENn = 3
    // Set frame format: 7 data bits, even parity, 1 stop bit
    // UCSZn1 = 2, UCSZn0 = 1 (for 7 bits: UCSZn1=1, UCSZn0=0)
    // UPMn1 = 5, UPMn0 = 4 (for even parity: UPMn1=1, UPMn0=0)
    // USBSn = 3 (for 1 stop bit: USBSn=0)
    *_ucsrNc = (1 << 2) | (1 << 5);  // UCSZn1 | UPMn1 (7 data, even parity)
    return true;
  }

  void uart_send_byte(uint8_t data) {
    // Wait for the Data Reg Empty flag to be set then send data
    while (!(*_ucsrNa & (1 << _udreN_bit)));
    *_udrN = data;
  }

  bool uart_data_available() { return (*_ucsrNa & (1 << 7)); }

  uint8_t uart_read_byte() { return *_udrN; }

  void wait_for_tx_complete() {
    while (!(*_ucsrNa & (1 << 6)));  // Wait for TXC flag
    *_ucsrNa |= (1 << 6);            // Clear TXC by writing 1
  }

  uint32_t get_tick() { return (_tick_ptr ? *_tick_ptr : 0); }
  float ticks_per_second() { return _ticks_per_second; }

  void delay_ms(uint32_t ms) {
    uint32_t start = get_tick();
    uint32_t ticks_needed = (uint32_t)(ms * (ticks_per_second() / 1000.0f));
    while ((get_tick() - start) < ticks_needed) {
      __asm__ __volatile__("");  // Prevent optimization
    }
  }

  void enable_uart_tx() {
    *_ucsrNb |= (1 << 3);  // Set TXENn (bit 3) to enable UART TX
  }
  void disable_uart_tx() {
    // Clear TXENn bit in UCSRnB to disable UART transmitter
    *_ucsrNb &= ~(1 << 3);  // TXENn = 3
  }
  void uart_tx_pin_low() {
    disable_uart_tx();
    *_tx_ddr |= _tx_bit;    // Set as output
    *_tx_port &= ~_tx_bit;  // Drive LOW
  }
  void uart_tx_pin_high() {
    disable_uart_tx();
    *_tx_ddr |= _tx_bit;   // Set as output
    *_tx_port |= _tx_bit;  // Drive HIGH
  }

 private:
  volatile uint8_t* _enTxPort;
  uint8_t _enTxBit;
  volatile uint8_t* _enRxPort;
  uint8_t _enRxBit;
  volatile uint8_t* _ucsrNa;   // UCSRnA Control and Status Reg A (DS: 22.10.2)
  volatile uint8_t* _ucsrNb;   // UCSRnB Control and Status Reg B (DS: 22.10.3)
  volatile uint8_t* _ucsrNc;   // UCSRnC Control and Status Reg C (DS: 22.10.4)
  volatile uint16_t* _ubrrN;   // UBRRn 12 bit reg (DS: 22.10.5)
  volatile uint8_t* _udrN;     // Pointer to UART data register
  uint8_t _udreN_bit;          // Bit position for UDREn (Data Register Empty)
  volatile uint8_t* _tx_port;  // Pointer to TX PORTx
  volatile uint8_t* _tx_ddr;   // Pointer to TX DDRx
  uint8_t _tx_bit;             // Bit mask for TX pin
  volatile uint32_t* _tick_ptr;
  float _ticks_per_second;
};
#endif  // __AVR__
