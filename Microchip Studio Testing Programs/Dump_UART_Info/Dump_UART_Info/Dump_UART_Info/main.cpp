/*
 * Dump_UART_Info.cpp
 *
 * Created: 7/8/2025 2:37:16 PM
 * Author : jwiden2
 */

#define F_CPU 16000000UL // 16 MHz clock

#include <avr/io.h>

// Helper: Send a byte over UART0
void uart0_send(uint8_t data) {
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = data;
}

// Helper: Send a string over UART0
void uart0_send_str(const char* str) {
    while (*str) {
        uart0_send(*str++);
    }
}

// Helper: Send a byte as hex over UART0
void uart0_send_hex(uint8_t val) {
    const char hex[] = "0123456789ABCDEF";
    uart0_send(hex[(val >> 4) & 0xF]);
    uart0_send(hex[val & 0xF]);
}

// Print all UART2 registers to UART0
void print_uart2_regs(void) {
    uart0_send_str("UCSR2A: 0x");
    uart0_send_hex(UCSR2A);
    uart0_send('\r');
    uart0_send('\n');
    uart0_send_str("UCSR2B: 0x");
    uart0_send_hex(UCSR2B);
    uart0_send('\r');
    uart0_send('\n');
    uart0_send_str("UCSR2C: 0x");
    uart0_send_hex(UCSR2C);
    uart0_send('\r');
    uart0_send('\n');
    uart0_send_str("UBRR2H: 0x");
    uart0_send_hex(UBRR2H);
    uart0_send('\r');
    uart0_send('\n');
    uart0_send_str("UBRR2L: 0x");
    uart0_send_hex(UBRR2L);
    uart0_send('\r');
    uart0_send('\n');
    uart0_send_str("UDR2: 0x");
    uart0_send_hex(UDR2);
    uart0_send('\r');
    uart0_send('\n');
    uart0_send_str("---\r\n");
}

void uart0_init(void) {
    // 1200 baud, 7E1: 7 data bits, even parity, 1 stop bit
    // Set baud rate
    // UBRR = (F_CPU / (16 * BAUD)) - 1
    // For F_CPU = 16MHz, BAUD = 1200: UBRR = 832
    UBRR0H = (uint8_t)(832 >> 8);
    UBRR0L = (uint8_t)832;
    // Set frame format: 7 data, even parity, 1 stop
    UCSR0C = (1 << UPM01) | (1 << UCSZ01); // Even parity, 7 data bits
    // Enable TX
    UCSR0B = (1 << TXEN0);
}

void uart2_init(void) {
    // Set baud rate for SDI-12 (if needed, else default)
    // For now, leave as default, or set as needed
    // Enable RX
    UCSR2B = (1 << RXEN2);
}

int main(void) {
    uart0_init();
    uart2_init();

    uart0_send_str("UART2 register dump on RX\r\n");

    while (1) {
        // Poll for received data on UART2
        if (UCSR2A & (1 << RXC2)) {
            // Read data (to clear RXC2)
            volatile uint8_t d = UDR2;
            (void)d;
            print_uart2_regs();
        }
    }
}
