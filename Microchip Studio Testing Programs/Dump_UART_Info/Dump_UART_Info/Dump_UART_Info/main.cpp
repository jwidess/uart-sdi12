/*
 * Dump_UART_Info.cpp
 *
 * Created: 7/8/2025 2:37:16 PM
 * Author : jwidess
 */

#define F_CPU 16000000UL // 16 MHz clock

#include <avr/io.h>

// Send a byte over UART0
void uart0_send(uint8_t data) {
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = data;
}

// Send a string over UART0
void uart0_send_str(const char* str) {
    while (*str) {
        uart0_send(*str++);
    }
}

// Send a byte as hex over UART0
void uart0_send_hex(uint8_t val) {
    const char hex[] = "0123456789ABCDEF";
    uart0_send(hex[(val >> 4) & 0xF]);
    uart0_send(hex[val & 0xF]);
}

// Print all UART2 registers to UART0, with detailed UCSR2A info
void uart0_send_bin(uint8_t val) {
    for (int i = 7; i >= 0; i--) {
        uart0_send((val & (1 << i)) ? '1' : '0');
    }
}

void print_uart2_regs(void) {
    uart0_send_str("UCSR2A: 0x");
    uart0_send_hex(UCSR2A);
    uart0_send_str(" 0b");
    uart0_send_bin(UCSR2A);
    uart0_send_str("\r\n");
    // Bitwise breakdown
    uart0_send_str("  RXC2 (7): ");
    uart0_send((UCSR2A & (1 << RXC2)) ? '1' : '0');
    uart0_send_str("  TXC2 (6): ");
    uart0_send((UCSR2A & (1 << TXC2)) ? '1' : '0');
    uart0_send_str("  UDRE2 (5): ");
    uart0_send((UCSR2A & (1 << UDRE2)) ? '1' : '0');
    uart0_send_str("\r\n");
    uart0_send_str("  FE2 (4): ");
    uart0_send((UCSR2A & (1 << FE2)) ? '1' : '0');
    uart0_send_str("  DOR2 (3): ");
    uart0_send((UCSR2A & (1 << DOR2)) ? '1' : '0');
    uart0_send_str("  UPE2 (2): ");
    uart0_send((UCSR2A & (1 << UPE2)) ? '1' : '0');
    uart0_send_str("\r\n");
    uart0_send_str("  U2X2 (1): ");
    uart0_send((UCSR2A & (1 << U2X2)) ? '1' : '0');
    uart0_send_str("  MPCM2 (0): ");
    uart0_send((UCSR2A & (1 << MPCM2)) ? '1' : '0');
    uart0_send_str("\r\n");

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
    // UART0: 38400 baud, 8N1 (for USB serial)
    // UBRR = (F_CPU / (16 * BAUD)) - 1
    // For F_CPU = 16MHz, BAUD = 38400: UBRR = 25
    UBRR0H = (uint8_t)(25 >> 8);
    UBRR0L = (uint8_t)25;
    // Set frame format: 8 data, no parity, 1 stop
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); // 8N1
    // Enable TX
    UCSR0B = (1 << TXEN0);
}

void uart2_init(void) {
    // UART2: 1200 baud, 7E1 (for SDI-12)
    // UBRR = (F_CPU / (16 * BAUD)) - 1
    // For F_CPU = 16MHz, BAUD = 1200: UBRR = 832
    UBRR2H = (uint8_t)(832 >> 8);
    UBRR2L = (uint8_t)832;
    // Set frame format: 7 data bits, even parity, 1 stop bit
    UCSR2C = (1 << UPM21) | (1 << UCSZ21); // Even parity, 7 data bits
    // Enable RX
    UCSR2B = (1 << RXEN2);
}

int main(void) {
    uart0_init();
    uart2_init();

    uart0_send_str("UART2 register dump on RX (SDI-12 test)\r\n");

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
