/*
 * UART-SDI12-LibraryTest-V1.cpp
 *
 * Created: 5/20/2025 4:53:58 PM
 * Author : Justin Widen
 */

#include "config.h"

#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>

#include "USDI12.hpp"

volatile uint32_t system_tick = 0;

void timer5_init_2s() {
    // Timer5 is 16-bit, max count is 65535
    // 16,000,000 / 1024 = 15625 counts/sec
    // 2s = 2 * 15625 = 31250 counts
    // Use CTC mode, OCR5A = 31249 (since timer counts from 0)
    TCCR5A = 0; // Normal port operation, CTC mode
    TCCR5B = (1 << WGM52) | (1 << CS52) | (1 << CS50); // CTC, prescaler 1024
    OCR5A = 31249;
    TIMSK5 = (1 << OCIE5A); // Enable Output Compare A Match Interrupt
}

ISR(TIMER5_COMPA_vect) {
    system_tick++;
}

//======================================
// Arduino Mega USB Port Communication

void uart0_init(uint32_t baud) { // Init Arduino Mega 2560 USB UART
    uint16_t ubrr = (F_CPU / 16 / baud) - 1;
    UBRR0H = (ubrr >> 8) & 0xFF;
    UBRR0L = ubrr & 0xFF;
    UCSR0B = (1 << TXEN0); // Enable transmitter
    UCSR0C =
        (1 << UCSZ01) | (1 << UCSZ00); // 8 data bits, no parity, 1 stop bit
}

void uart0_send_char(char c) {
    while (!(UCSR0A & (1 << UDRE0))); // Wait for empty transmit buffer
    UDR0 = c;
}

void uart0_send_string(const char* str) {
    while (*str) {
        uart0_send_char(*str++);
    }
}

//======================================

int main(void) {
    // - !EN_TX on PH2, EN_RX on PH3
    USDI12 sdi12(&SDI12_TX_PORT,
                 (1 << SDI12_TX_PIN),
                 &SDI12_RX_PORT,
                 (1 << SDI12_RX_PIN),
                 SDI12_UART_NUM,
                 F_CPU,
                 &system_tick);

    sdi12.set_rx(); // Initializes DDRs and sets RX mode
    _delay_ms(500);

    sdi12.set_tx(); // Switches to TX mode

    sdi12.begin_uart(); // Initializes UART for SDI-12 communication

    timer5_init_2s();
    sei(); // Enable global interrupts

    // --- BLINK SETUP ---
    DDRB |= (1 << PB7); // Set PB7 (Arduino Mega 2560 Pin 13) as output
    // --- END BLINK SETUP ---

    char sdi12_buffer[USDI12_BUFFER_SIZE] = {0};

    uart0_init(9600); // Set baud rate to 9600
    uart0_send_string("\r\nBoot...\r\n");

    while (1) {
        //sdi12.send_command('0', "M!"); // Send command to SDI-12 device
        // Wait for and read the response
        // if (sdi12.read_response(sdi12_buffer, 1, USDI12_BUFFER_SIZE)) { // 1000 ticks timeout (adjust as needed)
        //     // Echo the received response back to the SDI-12 device
        //     sdi12.send_command('0', sdi12_buffer);
        // }

        // Get measurement from SDI-12 device
        int8_t MeasurementResult =
            sdi12.get_measurement('0', sdi12_buffer, USDI12_BUFFER_SIZE, 1);
        uart0_send_string("\r\nMeasurement Result: ");
        uart0_send_char(MeasurementResult + '0'); // Convert to char for display
        uart0_send_string("\r\n");
        uart0_send_string("SDI-12 Response: ");
        uart0_send_string(sdi12_buffer); // Send the response to the USB port
        uart0_send_string("\r\n");

        // --- BLINK LOOP ---
        PORTB |= (1 << PB7); // LED ON
        _delay_ms(10);
        PORTB &= ~(1 << PB7); // LED OFF
        _delay_ms(10);
        // --- END BLINK LOOP ---
    }
}
