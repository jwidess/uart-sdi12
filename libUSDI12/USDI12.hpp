// clang-format off
/**
 * @file USDI12.hpp
 * @author Justin Widen (justinwiden1@gmail.com)
 *
 * @brief Header file for the UART SDI-12 library.
 *
 * ============== UART SDI-12 ==============
 *
 * This is an SDI-12 implementation that uses a UART serial port to communicate
 * with SDI-12 sensors.
 *
 * =========================================
 */
// clang-format on

#ifndef USDI12_H
#define USDI12_H

#include <avr/io.h>
#include <stdio.h>   // For snprintf, sscanf
#include <string.h>  // For strncat, strncpy

#include "USDI12_HAL.hpp"  // HAL Interface

#ifndef USDI12_BUFFER_SIZE
/**
 * @brief The buffer size for incoming SDI-12 data.
 * CREDIT: Arduino SDI-12 Library
 * All responses should be less than 81 characters:
 * - address is a single (1) character
 * - values has a maximum value of 75 characters
 * - CRC is 3 characters
 * - CR is a single character
 * - LF is a single character
 */
#define USDI12_BUFFER_SIZE 82  // 81 chars + null terminator
#endif

// SDI-12 result codes for get_measurement (C++03 compatible)
enum USDI12Result {
  USDI12Result_Success = 0,
  USDI12Result_InputError,       // 1
  USDI12Result_Timeout,          // 2
  USDI12Result_InvalidResponse,  // 3
  USDI12Result_CommandError,     // 4
  USDI12Result_BufferOverflow,   // 5
  USDI12Result_NullPointer,      // 6
  USDI12Result_Unexpected        // 7
};

class USDI12 {
 public:
  // HAL-based constructor
  USDI12(USDI12_HAL* hal);

  // Setup Functions
  void set_tx();  // Set GPIOs for Transmit mode
  void set_rx();  // Set GPIOs for Receive mode
  bool begin_uart(
      uint32_t cpuFreq);  // Initialize UART for SDI-12 communication

  // ============================
  // SDI-12 Functions

  // To not send an address, use -1 (E.g. sending "?!")
  // Command is a null-terminated string, e.g. "M!"
  bool send_command(int8_t address, const char* command);
  // Read response from SDI-12 device with timeout (milliseconds)
  // Returns true if response received, false on timeout
  bool read_response(char* buffer, uint32_t timeout_ms, uint16_t buffer_size);
  void uart_send_byte(uint8_t data);

  /**
   * @brief Initiates a measurement and retrieves all measurement values from
   * the SDI-12 sensor.
   * @param address SDI-12 address (0-9)
   * @param result_buffer Buffer to store the concatenated measurement values
   * (null-terminated)
   * @param buffer_size Size of the result_buffer
   * @param measurement_number Optional measurement number (0-9), default is -1
   * (standard M command)
   * @return USDI12Result enum indicating result or error type
   */
  USDI12Result get_measurement(uint8_t address, char* result_buffer,
                               uint16_t buffer_size,
                               int8_t measurement_number = -1);

 private:
  USDI12_HAL* _hal;
  uint32_t get_time_ms() const;
  static const uint16_t SDI12_BAUD_RATE = 1200;
  static const uint8_t SDI12_DATA_BITS = 7;
  static const uint8_t SDI12_PARITY_EVEN = 1;
  static const uint8_t SDI12_STOP_BITS = 1;
};

#endif  // USDI12_H
