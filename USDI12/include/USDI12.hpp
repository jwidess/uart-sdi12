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
 * with SDI-12 sensors. It features the ability to communicate with upto two
 * SDI-12 buses using a single UART interface. A single direction pin controls
 * which bus is being transmitted or received on.
 *
 * Example usage:
 *  USDI12 sdi12_b0(&avr_hal, 0);  // Bus 0
 *  USDI12 sdi12_b1(&avr_hal, 1);  // Bus 1
 *  sdi12_b0.send_command(-1, "?!");  // Address query
 *  sdi12_b1.send_command('0', "I!");  // Identification
 *
 * =========================================
 */
// clang-format on

#ifndef USDI12_H
#define USDI12_H

// These version macro placeholders are updated by the GitHub Action 
// release-please workflow when a new release is created.
#define USDI12_VERSION_MAJOR 0
#define USDI12_VERSION_MINOR 0
#define USDI12_VERSION_PATCH 0
#define USDI12_VERSION_STRING "0.0.0"

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

/**
 * @brief Result codes for SDI-12 operations.
 * 0=Success, 1=InputError, 2=Timeout, 3=InvalidResponse, 4=CommandError,
 * 5=BufferOverflow, 6=NullPointer, 7=Unexpected, 8=FrameError, 9=OverrunError,
 * 10=ParityError, 11=UARTMultiErrors
 * @param n/a
 */
enum USDI12Result {
  USDI12Result_Success = 0,
  USDI12Result_InputError,       // 1
  USDI12Result_Timeout,          // 2
  USDI12Result_InvalidResponse,  // 3
  USDI12Result_CommandError,     // 4
  USDI12Result_BufferOverflow,   // 5
  USDI12Result_NullPointer,      // 6
  USDI12Result_Unexpected,       // 7
  USDI12Result_FrameError,       // 8
  USDI12Result_OverrunError,     // 9
  USDI12Result_ParityError,      // 10
  USDI12Result_UARTMultiErrors   // 11
};

/**
 * @brief Human-readable names for USDI12Result enum values.
 */
extern const char* USDI12ResultNames[];
/**
 * @brief Number of USDI12Result enum values (for bounds checking)
 */
#define USDI12_RESULT_COUNT 12

class USDI12 {
 public:
  // HAL-based constructor
  USDI12(USDI12_HAL* hal, uint8_t bus);

  // Setup Functions
  void set_tx();                      // Set GPIOs for Transmit mode
  void set_rx();                      // Set GPIOs for Receive mode
  bool begin_uart(uint32_t cpuFreq);  // Initialize UART for SDI-12

  // ============================
  // SDI-12 Functions

  void send_break_mark(uint16_t break_ms = 12, uint16_t mark_ms = 9);

  /**
   * @brief Send an SDI-12 command to a device.
   * @param address SDI-12 address (0-9), -1 to not send an address (E.g.
   * sending "?!")
   * @param command Command string to send E.g. "M!" (null-terminated)
   * @return true if command sent successfully, false on error
   */
  USDI12Result send_command(char address, const char* command);

  /**
   * @brief Reads a response from the SDI-12 device with a specified timeout.
   * This function waits for a response from the SDI-12 sensor, reading
   * characters into the provided buffer until a CRLF sequence is received, the
   * buffer is full, or the timeout expires.
   * @param buffer Pointer to the character array to store the response
   * (null-terminated).
   * @param timeout_ms Timeout in milliseconds to wait for a response.
   * @param buffer_size Size of the response buffer (including null terminator).
   * @return USDI12Result enum indicating result or error type:
   * - USDI12Result_Success: valid response received
   * - USDI12Result_Timeout: timeout occurred
   * - USDI12Result_BufferOverflow: buffer full before CRLF
   * - USDI12Result_NullPointer: buffer is null or size is zero
   */
  USDI12Result read_response(char* buffer, uint32_t timeout_ms,
                             uint16_t buffer_size);

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

  /**
   * @brief Waits for the SDI-12 bus to become idle (no RX data) for up to
   * timeout_ms milliseconds. The bus is considered idle when no data has been
   * received for at least USDI12_BUS_IDLE_THRESHOLD_MS.
   * @param timeout_ms Maximum time to wait for bus idle (ms) (default = 100)
   * @return USDI12Result_Success if bus became idle within timeout,
   * USDI12Result_Timeout if timeout occurred.
   */
  USDI12Result wait_bus_idle(uint32_t timeout_ms = 100);

 private:
  USDI12_HAL* _hal;
  uint8_t _bus;
  uint32_t get_time_ms() const;
  static const uint16_t SDI12_BAUD_RATE = 1200;
  static const uint8_t SDI12_DATA_BITS = 7;
  static const uint8_t SDI12_PARITY_EVEN = 1;
  static const uint8_t SDI12_STOP_BITS = 1;
};

#endif  // USDI12_H
