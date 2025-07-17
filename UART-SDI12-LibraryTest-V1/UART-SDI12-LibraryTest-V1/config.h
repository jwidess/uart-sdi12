#ifndef CONFIG_H
#define CONFIG_H

// MCU Configuration
#define F_CPU 16000000UL

// UART Configuration
#define SDI12_UART_NUM 2  // Using UART2

// Pin Definitions
// - !EN_TX on PH2, EN_RX on PH3
#define SDI12_TX_PORT PORTH
// #define SDI12_TX_PIN  PH2
#define SDI12_TX_PIN PH4  // For testing with Arduino Mega (pin 7)
#define SDI12_RX_PORT PORTH
#define SDI12_RX_PIN PH3  // On Arduino Mega this is pin 6

#endif  // CONFIG_H