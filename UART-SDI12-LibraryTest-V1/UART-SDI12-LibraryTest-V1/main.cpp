/*
 * UART-SDI12-LibraryTest-V1.cpp
 *
 * Created: 5/20/2025 4:53:58 PM
 * Author : Justin Widen
 */

#include "config.h"

#include <avr/io.h>
#include <util/delay.h>

#include "USDI12.hpp"

int main(void) {
    // - !EN_TX on PH2, EN_RX on PH3
    USDI12 sdi12(&SDI12_TX_PORT,
                 (1 << SDI12_TX_PIN),
                 &SDI12_RX_PORT,
                 (1 << SDI12_RX_PIN),
                 SDI12_UART_NUM,
                 F_CPU);

    sdi12.set_rx(); // Initializes DDRs and sets RX mode
    _delay_ms(100);

    sdi12.set_tx(); // Switches to TX mode

    while (1) {
        // Toggle between modes, etc.
    }
}
