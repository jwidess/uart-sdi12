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

#define UART_USDI12_NUM 2  // Use UART2 for SDI-12

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

void delay_ms(uint32_t ms) {
  uint32_t start = ms_tick;
  while ((ms_tick - start) < ms) {
    __asm__ __volatile__("");  // Prevent optimization
  }
}

//======================================
// Arduino Mega USB Port Communication

void uart0_init(uint32_t baud) {  // Init Arduino Mega 2560 USB UART
  uint16_t ubrr = (F_CPU / 16 / baud) - 1;
  UBRR0H = (ubrr >> 8) & 0xFF;
  UBRR0L = ubrr & 0xFF;
  UCSR0B = (1 << TXEN0);                   // Enable transmitter
  UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);  // 8 data bits, no parity, 1 stop bit
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

//======================================

int main(void) {
  // Instantiate HAL object
  AVR_HAL avr_hal(&SDI12_TX_PORT, (1 << SDI12_TX_PIN), &SDI12_RX_PORT,
                  (1 << SDI12_RX_PIN), UART_USDI12_NUM, &system_tick, 0.5f);

  // Pass HAL object to USDI12
  USDI12 sdi12(&avr_hal);

  sdi12.begin_uart(F_CPU);

  timer5_init_2s();
  timer0_init_1ms();
  sei();  // Enable global interrupts

  DDRB |= (1 << PB7);  // Set PB7 (Arduino Mega 2560 Pin 13) as output (blink)

  char sdi12_buffer[USDI12_BUFFER_SIZE] = {0};

  uart0_init(9600);  // USB UART at 9600 baud
  uart0_send_string("\r\nBoot...\r\n");

  for (uint8_t i = 0; i < 6; i++) {
    sdi12.set_rx();  // RX mode (SEL=1)
    delay_ms(10);
    sdi12.set_tx();  // TX mode (SEL=0)
    delay_ms(10);
  }

  while (1) {
    // sdi12.send_command('0', "M!"); // Send command to SDI-12 device
    //  Wait for and read the response
    //  if (sdi12.read_response(sdi12_buffer, 1, USDI12_BUFFER_SIZE)) { // 1000
    //  ticks timeout (adjust as needed)
    //      // Echo the received response back to the SDI-12 device
    //      sdi12.send_command('0', sdi12_buffer);
    //  }

    // Get measurement from SDI-12 device
    int8_t MeasurementResult =
        sdi12.get_measurement('0', sdi12_buffer, USDI12_BUFFER_SIZE, 1);
    uart0_send_string("\r\nMeasurement Result: ");
    uart0_send_char(MeasurementResult + '0');  // Convert to char for display
    // Print English name of USDI12Result
    const char* result_names[] = {
        " Success",      " InputError",     " Timeout",     " InvalidResponse",
        " CommandError", " BufferOverflow", " NullPointer", " Unexpected"};
    if (MeasurementResult >= 0 && MeasurementResult <= 7) {
      uart0_send_string(result_names[MeasurementResult]);
    } else {
      uart0_send_string("Unknown");
    }
    uart0_send_string("\r\n");
    uart0_send_string("SDI-12 Response: ");
    uart0_send_string(sdi12_buffer);  // Send the response to the USB port
    uart0_send_string("\r\n");

    // --- BLINK LOOP ---
    PORTB |= (1 << PB7);  // LED ON
    _delay_ms(10);
    PORTB &= ~(1 << PB7);  // LED OFF
    _delay_ms(10);
    // --- END BLINK LOOP ---
  }
}
