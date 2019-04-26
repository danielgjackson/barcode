// Barcode - Generates a CODE128 barcode
// Dan Jackson, 2019

#ifndef BARCODE_H
#define BARCODE_H

#include <stdbool.h>
#include <stdint.h>

// The maximum number of bytes required for the EXACT specified number of digits (strictly 0-9), odd numbers are less efficient
// (Odd: START SYMBOL_PER_TWO_DIGITS CODE_CHANGE LAST_DIGIT CHECKSUM STOP; Even: START SYMBOL_PER_TWO_DIGITS CHECKSUM STOP;)
#define BARCODE_SIZE_BARE_NUMERIC_EXACT(_digits)  ((((3 + ((_digits) / 2) + (((_digits) & 1) ? 2 : 0)) * 11 + 2) + 7) / 8)
#define BARCODE_SIZE_QUIET_NUMERIC_EXACT(_digits) ((((3 + ((_digits) / 2) + (((_digits) & 1) ? 2 : 0)) * 11 + 2 + 10 + 10) + 7) / 8)

// The maximum number of bytes required for numbers up to the specified number of digits (strictly 0-9)
#define BARCODE_SIZE_BARE_NUMERIC(_digits)        ((((3 + ((_digits) / 2) + 2) * 11 + 2) + 7) / 8)
#define BARCODE_SIZE_QUIET_NUMERIC(_digits)       ((((3 + ((_digits) / 2) + 2) * 11 + 2 + 10 + 10) + 7) / 8)

// The maximum number of bytes required for the specified amount of (non-control-character) ASCII text
#define BARCODE_SIZE_BARE_TEXT(_characters)       ((((3 + (_characters)) * 11 + 2) + 7) / 8)
#define BARCODE_SIZE_QUIET_TEXT(_characters)      ((((3 + (_characters)) * 11 + 2 + 10 + 10) + 7) / 8)

// Completes the barcode and writes the object as a bitmap (0=black, 1=white) using the specified buffer, returns the length in bars/bits. Optionally adds a 10-unit quiet zone either side.
size_t Barcode(uint8_t *buffer, size_t bufferSize, bool addQuietZone, const char *text);

// Returns the bar/bit at the specified index in the output bitmap (false=black, true=white)
#define BARCODE_BIT(_buffer, _offset) ((*((uint8_t *)(_buffer) + ((_offset) / 8)) & (1 << ((_offset) & 7))) != 0)

#endif
