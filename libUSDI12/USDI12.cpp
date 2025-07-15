// clang-format off
/**
 * @file USDI12.cpp
 * @author Justin Widen (justinwiden1@gmail.com)
 *
 * @brief Implementation file for the UART SDI-12 library.
 *
 * ============== UART SDI-12 ==============
 *
 * This is an SDI-12 implementation that uses a UART serial port to communicate
 * with SDI-12 sensors.
 *
 * =========================================
 */
// clang-format on

#include "USDI12.hpp"

#include <stdio.h>   // For snprintf, sscanf
#include <string.h>  // For strncat, strncpy

USDI12::USDI12(USDI12_HAL* hal, volatile uint32_t* tick_ptr)
    : _hal(hal), _tick_ptr(tick_ptr) {}

void USDI12::set_tx() { _hal->set_tx(); }

void USDI12::set_rx() { _hal->set_rx(); }

bool USDI12::begin_uart(uint32_t cpuFreq) { return _hal->begin_uart(cpuFreq); }

void USDI12::uart_send_byte(uint8_t data) { _hal->uart_send_byte(data); }

bool USDI12::send_command(uint8_t address, const char* command) {
  if (address > '9' || address < '0') {
    return false;
  }
  set_tx();
  uart_send_byte(address);
  // Send command string
  while (*command != '\0') {
    uart_send_byte(*command++);
  }
  // Send termination character (!) if not already present
  if (*(command - 1) != '!') {
    uart_send_byte('!');
  }
  // Add CRLF sequence
  uart_send_byte('\r');
  uart_send_byte('\n');
  _hal->wait_for_tx_complete();
  set_rx();
  return true;
}

bool USDI12::read_response(char* buffer, uint32_t timeout_ticks,
                           uint16_t buffer_size) {
  set_rx();
  if (!buffer || !_tick_ptr || buffer_size == 0) return false;
  int idx = 0;
  bool got_cr = false;
  uint32_t start_tick = *_tick_ptr;
  int max_len = buffer_size - 1;
  while (((*_tick_ptr - start_tick) < timeout_ticks + 1) && (idx < max_len)) {
    if (_hal->uart_data_available()) {
      char c = (char)(_hal->uart_read_byte());
      if (got_cr && c == '\n') {
        buffer[idx] = '\0';
        if (idx > 0 && buffer[idx - 1] == '\r') {
          buffer[idx - 1] = '\0';
        }
        return true;  // Success: got CRLF
      }
      if (c != '\r') {
        if (idx < max_len) {
          buffer[idx] = c;  // Store it in the buffer (skip CR)
          idx++;
        }
        // If idx >= max_len, discard extra chars to avoid overrun
      }
      got_cr = (c == '\r');  // Check if current char is CR
    }
  }
  buffer[(idx < max_len) ? idx : max_len] = '\0';
  return false;  // Timeout or buffer full
}

/**
 * @brief Initiates a measurement and retrieves all measurement values from the
 * SDI-12 sensor.
 * @param address SDI-12 address (0-9)
 * @param measurement_number Optional measurement number (0-9), default is 0
 * (standard M command)
 * @param result_buffer Buffer to store the concatenated measurement values
 * (null-terminated)
 * @param buffer_size Size of the result_buffer
 * @return true if all expected values were received, false otherwise
 */
USDI12Result USDI12::get_measurement(uint8_t address, char* result_buffer,
                                     uint16_t buffer_size,
                                     int8_t measurement_number) {
  if (address > '9' || address < '0' || !result_buffer || buffer_size == 0 ||
      measurement_number > 9) {
    return USDI12Result_InputError;  // Invalid address, buffer, or measurement
                                     // number
  }
  int defTimeOutTicks = 1;  // Default timeout ticks
  char cmd[6] = {0};
  char response[USDI12_BUFFER_SIZE] = {0};
  // Format: `aMn!` if measurement_number is specified, else `aM!`
  if (measurement_number >= 0) {
    snprintf(cmd, sizeof(cmd), "%cM%u!", address, measurement_number);
  } else if (measurement_number == -1) {
    snprintf(cmd, sizeof(cmd), "%cM!", address);
  }
  if (!send_command(address, cmd + 1)) {
    return USDI12Result_CommandError;
  }
  read_response(response, defTimeOutTicks, USDI12_BUFFER_SIZE);

  // Response: atttn<CR><LF> (a=address, ttt=time, n=number of values)
  // Example: 10112<CR><LF> (1=address, 011=11s, 2=2 values)
  // Parse response: a ttt n
  int16_t ttt = 0;  // Time in seconds
  int16_t addr, n;
  if (sscanf(response, "%1d%3d%1d", &addr, &ttt, &n) != 3) {  // 3 Values
    return USDI12Result_InvalidResponse;
  }
  if (addr != address - '0') {  // Check if address matches
    return USDI12Result_InvalidResponse;
  }
  uint8_t num_values = n;  // Number of values expected

  uint16_t wait_ticks = 0;
  // Calculate wait_ticks based on ttt and tick interval (2 seconds per tick)
  // E.g. ttt=0 or 1 -> 1 tick, ttt=2 -> 1 ticks, ttt=3 -> 2 ticks
  if (ttt == 0 || ttt == 1) {
    wait_ticks = 1;
  } else {
    wait_ticks = ttt / 2;
    if (ttt % 2 != 0) {
      wait_ticks += 1;  // Add an extra tick if there's a remainder
    }
  }

  // Wait for service request (a<CR><LF>) or timeout
  char service_req[4] = {0};
  bool got_service = read_response(service_req, wait_ticks,
                                   sizeof(service_req));  // wait for ready
  // After service request or timeout, send D0! and read values
  char values[USDI12_BUFFER_SIZE * 2] = {0};
  uint8_t values_received = 0;
  for (uint8_t d = 0; values_received < num_values && d < 10; ++d) {
    char d_cmd[6] = {0};
    snprintf(d_cmd, sizeof(d_cmd), "%cD%u!", address, d);
    if (!send_command(address, d_cmd + 1)) {
      return USDI12Result_CommandError;
    }
    char d_response[USDI12_BUFFER_SIZE] = {0};
    if (!read_response(d_response, defTimeOutTicks, USDI12_BUFFER_SIZE)) {
      return USDI12Result_CommandError;
    }
    // Skip address char, append rest to values
    char* val_start = d_response;
    if (*val_start == address) ++val_start;
    strncat(values, val_start, sizeof(values) - strlen(values) - 1);
    // Count number of values (fields separated by + or -)
    for (char* p = val_start; *p; ++p) {
      if (*p == '+' || *p == '-') {
        ++values_received;
      }
    }
    if (values_received >= num_values) break;
  }
  strncpy(result_buffer, values, buffer_size - 1);
  result_buffer[buffer_size - 1] = '\0';
  return USDI12Result_Success;
}  // END: get_measurement
