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
    _delay_ms(100);

    sdi12.set_tx(); // Switches to TX mode

    sdi12.begin_uart(); // Initializes UART for SDI-12 communication

    timer5_init_2s();
    sei(); // Enable global interrupts

    while (1) {
        // Toggle between modes, etc.
    }
}
