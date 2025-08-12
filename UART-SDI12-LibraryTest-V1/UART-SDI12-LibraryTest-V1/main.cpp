/*
 * UART-SDI12-LibraryTest-V1.cpp
 *
 * Created: 5/20/2025 4:53:58 PM
 * Author : Justin Widen
 */

#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>

#include "USDI12.hpp"
#include "config.h"

// ============================================================================
// Timer Setup:
volatile uint32_t system_tick = 0;
volatile uint32_t ms_tick = 0;  // Used for 1ms timer

void timer5_init_2s() {
  // Timer5 is 16-bit, max count is 65535
  // 16,000,000 / 1024 = 15625 counts/sec
  // 2s = 2 * 15625 = 31250 counts
  // Use CTC mode, OCR5A = 31249 (since timer counts from 0)
  TCCR5A = 0;  // Normal port operation, CTC mode
  TCCR5B = (1 << WGM52) | (1 << CS52) | (1 << CS50);  // CTC, prescaler 1024
  OCR5A = 31249;
  TIMSK5 = (1 << OCIE5A);  // Enable Output Compare A Match Interrupt
}

ISR(TIMER5_COMPA_vect) { system_tick++; }

void timer0_init_1ms() {
  TCCR0A = (1 << WGM01);               // CTC mode
  TCCR0B = (1 << CS01) | (1 << CS00);  // Prescaler 64
  OCR0A = 249;                         // 16MHz/64/250 = 1kHz (1ms)
  TIMSK0 = (1 << OCIE0A);              // Enable compare match interrupt
}

ISR(TIMER0_COMPA_vect) { ms_tick++; }

// ============================================================================
// Arduino Mega USB Port Communication
void uart0_init(uint32_t baud) {
  UCSR0A |= (1 << U2X0);  // Enable double speed mode
  uint16_t ubrr = (F_CPU / 8 / baud) - 1;
  UBRR0H = (ubrr >> 8) & 0xFF;
  UBRR0L = ubrr & 0xFF;
  UCSR0B = (1 << TXEN0);                   // Enable transmitter only
  UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);  // 8N1
}

void uart0_send_char(char c) {
  while (!(UCSR0A & (1 << UDRE0)));  // Wait for empty transmit buffer
  UDR0 = c;
}

void uart0_send_string(const char* str) {
  while (*str) {
    uart0_send_char(*str++);
  }
}
// ============================================================================

int main(void) {
  // Instantiate HAL object
  AVR_HAL avr_hal(&SDI12_TX_PORT, (1 << SDI12_TX_PIN), &SDI12_RX_PORT,
                  (1 << SDI12_RX_PIN), UART_USDI12_NUM, &ms_tick, 1000.0f);

  // Pass HAL object to USDI12
  USDI12 sdi12(&avr_hal);

  sdi12.begin_uart(F_CPU);

  char sdi12_buffer[USDI12_BUFFER_SIZE] = {0};  // Buffer for SDI-12 responses

  // Timers
  timer5_init_2s();
  timer0_init_1ms();
  sei();  // Enable global interrupts

  // Blink LED
  DDRB |= (1 << PB7);  // Set PB7 (Arduino Mega 2560 Pin 13) as output (blink)

  uart0_init(115200);  // USB UART
  uart0_send_string("\r\nBoot...\r\n");

  for (uint8_t i = 0; i < 6; i++) {  // Toggle SEL Line for testing
    sdi12.set_rx();                  // RX mode (SEL=1)
    _delay_ms(10);
    sdi12.set_tx();  // TX mode (SEL=0)
    _delay_ms(10);
  }
  // ==========================================================================

  while (1) {
    char ActiveAddress = 0;
    if (1) {
      uart0_send_string("\r\nSending break and mark...");
      sdi12.send_break_mark();
      sdi12.send_command(-1, "?!");  // Address query
      uint8_t AddrResult =
          sdi12.read_response(sdi12_buffer, 100, USDI12_BUFFER_SIZE);
      uart0_send_string("\r\nSDI-12 Address Query: ");
      if (AddrResult) {
        uart0_send_string("Success, Address: ");
        ActiveAddress = sdi12_buffer[0];
        uart0_send_char(ActiveAddress);  // Send the addr to the USB port

        if (0) {  // Used for testing address change
          // Check if the received address is '0'
          if (sdi12_buffer[0] == '0' && sdi12_buffer[1] == '\0') {
            uart0_send_string(
                "\r\nAddress is 0, sending change address to 5...\r\n");
            // SDI-12 change address command: aAb!
            sdi12.send_break_mark();
            sdi12.send_command('0', "A5!");
            sdi12.read_response(sdi12_buffer, 100, USDI12_BUFFER_SIZE);
            uart0_send_string("Change address response: ");
            uart0_send_string(sdi12_buffer);
            uart0_send_string("\r\n");
          }
        }
      } else {
        uart0_send_string("FAIL or No Address");
      }
    } else {
      ActiveAddress = '0';
    }

    uart0_send_string("\r\n");
    memset(sdi12_buffer, 0, sizeof(sdi12_buffer));  // Clear buffer

    _delay_ms(15);

    sdi12.send_break_mark();
    uart0_send_string("\r\nSending Identification Command to Address: ");
    uart0_send_char(ActiveAddress);  // Send the addr to the USB port
    uart0_send_string("\r\n");
    sdi12.send_command(ActiveAddress, "I!");  // Identification command
    sdi12.read_response(sdi12_buffer, 1000, USDI12_BUFFER_SIZE);
    uart0_send_string("ID Success: ");
    uart0_send_string(sdi12_buffer);  // Send the response to the USB port
    uart0_send_string("\r\n");
    memset(sdi12_buffer, 0, sizeof(sdi12_buffer));  // Clear buffer

    _delay_ms(15);

    // Get measurement from SDI-12 device
    uart0_send_string("\r\nGet measurement...\r\n");
    sdi12.send_break_mark();
    int8_t MeasurementResult = sdi12.get_measurement(
        ActiveAddress, sdi12_buffer, USDI12_BUFFER_SIZE, 2);
    uart0_send_string("\r\nMeasurement Result: ");
    uart0_send_char(MeasurementResult + '0');  // Convert to char
    // Print English name of USDI12Result
    const char* result_names[] = {" - Success",      " - InputError",
                                  " - Timeout",      " - InvalidResponse",
                                  " - CommandError", " - BufferOverflow",
                                  " - NullPointer",  " - Unexpected"};
    if (MeasurementResult >= 0 && MeasurementResult <= 7) {
      uart0_send_string(result_names[MeasurementResult]);
    } else {
      uart0_send_string("Unknown");
    }
    uart0_send_string("\r\n");
    uart0_send_string("SDI-12 Response: ");
    uart0_send_string(sdi12_buffer);  // Send the response to the USB port
    uart0_send_string("\r\n");

    // --- BLINK ---
    PORTB |= (1 << PB7);  // LED ON
    _delay_ms(20);
    PORTB &= ~(1 << PB7);  // LED OFF
    _delay_ms(20);
    // --- END BLINK ---

    _delay_ms(2000);  // Wait 2 seconds before next iteration
  }
}
