// Barcode - Generates a CODE128 barcode
// Dan Jackson, 2019

#ifndef BARCODE_H
#define BARCODE_H

#include <stdbool.h>
#include <stdint.h>

// Global setting to store in to most significant bit first (least significant bit first otherwise)
#define BARCODE_MSB_FIRST

// (Advanced) Allow using a fixed code type
typedef enum
{
    BARCODE_CODE_NONE,   // Auto / Not started
    BARCODE_CODE_A,      // ASCII control and upper-case
    BARCODE_CODE_B,      // ASCII non-control characters
    BARCODE_CODE_C,      // Double-digit numeric code (Caution: do not use for fixed code type unless you always pass an even number of digits)
    BARCODE_CODE_STOP,   // (Private) Stopped
} barcode_code_t;

// The width of the quiet zone (either side of the barcode)
#define BARCODE_QUIET_NONE 0
#define BARCODE_QUIET_STANDARD 10


// The maximum width (number of bars) required for the EXACT specified number of digits (strictly 0-9), odd numbers are less efficient
// (Odd: START SYMBOL_PER_TWO_DIGITS CODE_CHANGE LAST_DIGIT CHECKSUM STOP; Even: START SYMBOL_PER_TWO_DIGITS CHECKSUM STOP;)
#define BARCODE_WIDTH_NUMERIC_EXACT(_digits, _quiet) (2 * (_quiet) + ((3 + ((_digits) / 2) + (((_digits) & 1) ? 2 : 0)) * 11 + 2))

// The maximum width (number of bars) required for the specified number of digits (strictly 0-9)
#define BARCODE_WIDTH_NUMERIC(_digits, _quiet)       (2 * (_quiet) + ((3 + ((_digits) / 2) + 2) * 11 + 2))

// The maximum width (number of bars) required for the specified amount of (non-control-character) ASCII text
#define BARCODE_WIDTH_TEXT(_characters, _quiet)      ((2 * (_quiet) + (3 + (_characters)) * 11 + 2))


// The maximum number of bytes required for the EXACT specified number of digits (strictly 0-9), odd numbers are less efficient
#define BARCODE_SIZE_NUMERIC_EXACT(_digits, _quiet)  ((BARCODE_WIDTH_NUMERIC_EXACT((_digits), (_quiet)) + 7) >> 3)

// The maximum number of bytes required for numbers up to the specified number of digits (strictly 0-9)
#define BARCODE_SIZE_NUMERIC(_digits, _quiet)        ((BARCODE_WIDTH_NUMERIC((_digits), (_quiet)) + 7) >> 3)

// The maximum number of bytes required for the specified amount of (non-control-character) ASCII text
#define BARCODE_SIZE_TEXT(_digits, _quiet)           ((BARCODE_WIDTH_TEXT((_digits), (_quiet)) + 7) >> 3)


// Generates the barcode as a bitmap (0=black, 1=white) using the specified buffer, quiet zone size, and fixed code type (BARCODE_CODE_NONE = Auto), returns the length in bars/bits.
size_t Barcode(uint8_t *buffer, size_t bufferSize, int quietZone, const char *text, barcode_code_t fixedCode);

// Returns the bar/bit at the specified index in the output bitmap (false=black, true=white)
#ifdef BARCODE_MSB_FIRST
    #define BARCODE_BIT(_buffer, _offset) ((*((uint8_t *)(_buffer) + ((_offset) >> 3)) & (1 << (7 - ((_offset) & 7)))) != 0)
#else
    #define BARCODE_BIT(_buffer, _offset) ((*((uint8_t *)(_buffer) + ((_offset) >> 3)) & (1 << ((_offset) & 7))) != 0)
#endif

#endif
