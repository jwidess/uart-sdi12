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

USDI12::USDI12(USDI12_HAL* hal, uint8_t bus) : _hal(hal), _bus(bus) {}

void USDI12::set_tx() {
  if (_bus == 0)
    _hal->dir_low();  // TX for bus 0
  else
    _hal->dir_high();  // TX for bus 1
}
void USDI12::set_rx() {
  if (_bus == 0)
    _hal->dir_high();  // RX for bus 0
  else
    _hal->dir_low();  // RX for bus 1
}

uint32_t USDI12::get_time_ms() const {
  return (uint32_t)(_hal->get_tick() * (1000.0f / _hal->ticks_per_second()));
}

void USDI12::send_break_mark(uint16_t break_ms, uint16_t mark_ms) {
  set_tx();  // Ensure TX mode
  _hal->uart_tx_pin_low();
  _hal->delay_ms(break_ms);
  _hal->uart_tx_pin_high();
  _hal->delay_ms(mark_ms);
  _hal->enable_uart_tx();
}

bool USDI12::begin_uart(uint32_t cpuFreq) { return _hal->begin_uart(cpuFreq); }

void USDI12::uart_send_byte(uint8_t data) { _hal->uart_send_byte(data); }

bool USDI12::send_command(char address, const char* command) {
  set_tx();
  // Only send address if valid
  if (address >= '0' && address <= '9') {
    uart_send_byte(address);
  }
  // Send command string
  while (*command != '\0') {
    uart_send_byte(*command++);
  }

  _hal->wait_for_tx_complete();
  return true;
}  // END: send_command

USDI12Result USDI12::read_response(char* buffer, uint32_t timeout_ms,
                                   uint16_t buffer_size) {
  // Flush old data from UART RX buffer
  while (_hal->uart_data_available()) {
    (void)_hal->uart_read_byte();
  }
  set_rx();  // Once old data is flushed, set RX mode
  if (!buffer || buffer_size == 0) return USDI12Result_NullPointer;
  int idx = 0;
  uint32_t timeout_ms_margin = timeout_ms + 10;  // Extra 10ms buffer
  bool got_cr = false;
  // Take current tick and convert to milliseconds
  uint32_t start_ms = get_time_ms();
  int max_len = buffer_size - 1;
  while (((get_time_ms() - start_ms) < timeout_ms_margin) && (idx < max_len)) {
    if (_hal->uart_data_available()) {
      char c = (char)(_hal->uart_read_byte());
      if (got_cr && c == '\n') {
        buffer[idx] = '\0';
        if (idx > 0 && buffer[idx - 1] == '\r') {
          buffer[idx - 1] = '\0';
        }
        return USDI12Result_Success;  // Success: got CRLF
      }
      if (c != '\r') {
        if (idx < max_len) {
          buffer[idx] = c;  // Store it in the buffer (skip CR)
          idx++;
        } else {
          // Buffer full before CRLF
          buffer[max_len] = '\0';
          return USDI12Result_BufferOverflow;
        }
      }
      got_cr = (c == '\r');  // Check if current char is CR
    }
  }
  buffer[(idx < max_len) ? idx : max_len] = '\0';
  return USDI12Result_Timeout;  // Timeout or buffer full
}  // END: read_response

/**
 * @brief Initiates a measurement and retrieves all measurement values from the
 * SDI-12 sensor.
 * @example get_measurement('0', buffer, sizeof(buffer), 2);
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
    return USDI12Result_InputError;  // Invalid address, buf, or measurement #
  }

  uint32_t defTimeoutMs = 1000;  // Default timeout in ms
  char cmd[6] = {0};
  char response[USDI12_BUFFER_SIZE] = {0};
  // Format: `aMn!` if measurement_number is specified, else `aM!`
  if (measurement_number >= 0) {
    snprintf(cmd, sizeof(cmd), "%cM%u!", address, measurement_number);
  } else if (measurement_number == -1) {
    snprintf(cmd, sizeof(cmd), "%cM!", address);
  }
  if (!send_command(address, cmd + 1)) {  // Skip address char
    return USDI12Result_CommandError;
  }
  read_response(response, defTimeoutMs, USDI12_BUFFER_SIZE);

  // Response: atttn<CR><LF> (a=address, ttt=time, n=number of values)
  // Example: 10112<CR><LF> (1=address, 011=11s, 2=2 values)
  // Parse response: a ttt n
  int16_t ttt = 0;  // Time until data is ready in seconds
  int16_t addr, n;
  if (sscanf(response, "%1d%3d%1d", &addr, &ttt, &n) != 3) {  // 3 Values
    return USDI12Result_InvalidResponse;
  }
  if (addr != address - '0') {  // Check if address matches
    return USDI12Result_InvalidResponse;
  }
  uint8_t num_values = n;  // Number of values expected

  uint32_t wait_ms = 0;
  // Set wait_ms based on ttt (seconds until data is ready)
  if (ttt > 0) {  // If ttt is greater than 0, we need to wait
    wait_ms = ttt * 1000;
    // Wait for service request (a<CR><LF>) or timeout
    char service_req[4] = {0};
    USDI12Result got_service =
        read_response(service_req, wait_ms, sizeof(service_req));

    if (got_service == USDI12Result_Success) {
      _hal->delay_ms(9);  // If service request, mark for 9ms to avoid problems
    } else {
      send_break_mark();  // If no service request (timeout), send break mark
    }
  } else if (ttt == 0) {  // No delay, data is immediately available
    _hal->delay_ms(9);    // Mark for 9ms to avoid problems
  }

  // Send nDn! and read values
  char values[USDI12_BUFFER_SIZE * 2] = {0};
  uint8_t values_received = 0;
  for (uint8_t d = 0; values_received < num_values && d < 10; ++d) {
    char d_cmd[6] = {0};
    snprintf(d_cmd, sizeof(d_cmd), "%cD%u!", address, d);
    if (!send_command(address, d_cmd + 1)) {  // Skip address char
      return USDI12Result_CommandError;
    }
    char d_response[USDI12_BUFFER_SIZE] = {0};
    if (read_response(d_response, defTimeoutMs, USDI12_BUFFER_SIZE) !=
        USDI12Result_Success) {
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
