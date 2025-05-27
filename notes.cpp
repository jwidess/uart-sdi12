// Useful Code Snippets or Functions for SDI-12 Related Code:

// ========================== Arduino SDI-12 ==================================
// Section on snippets from https://github.com/EnviroDIY/Arduino-SDI-12

/**
 * @brief converts allowable address characters ('0'-'9', 'a'-'z', 'A'-'Z') to a
 * decimal number between 0 and 61 (inclusive) to cover the 62 possible
 * addresses.
 */
byte charToDec(char i) {
    if ((i >= '0') && (i <= '9')) return i - '0';
    if ((i >= 'a') && (i <= 'z')) return i - 'a' + 10;
    if ((i >= 'A') && (i <= 'Z'))
      return i - 'A' + 36;
    else
      return i;
  }
  
  /**
   * @brief maps a decimal number between 0 and 61 (inclusive) to allowable
   * address characters '0'-'9', 'a'-'z', 'A'-'Z',
   *
   * THIS METHOD IS UNUSED IN THIS EXAMPLE, BUT IT MAY BE HELPFUL.
   */
  char decToChar(byte i) {
    if (i < 10) return i + '0';
    if ((i >= 10) && (i < 36)) return i + 'a' - 10;
    if ((i >= 36) && (i <= 62))
      return i + 'A' - 36;
    else
      return i;
  }


  // ========================== Arduino SDI-12 ==================================