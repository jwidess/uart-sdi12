#ifndef CONFIG_H
#define CONFIG_H

// MCU Configuration
// This is also defined in Microchip Studio compile symbols
#define F_CPU 16000000UL

// UART Configuration
#define UART_USDI12_NUM 2  // Use UART2 for SDI-12

// Pin Definitions
// Direction control pin
#define SDI12_DIR_PORT PORTH
#define SDI12_DIR_PIN PH4  // For testing with Arduino Mega (pin 7)

#endif  // CONFIG_H