// Barcode - Generates a CODE128 barcode
// Dan Jackson, 2019

#ifndef BARCODE_H
#define BARCODE_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    BARCODE_CODE_NONE,   // Not started
    BARCODE_CODE_A,      // ASCII control and upper-case
    BARCODE_CODE_B,      // ASCII non-control characters
    BARCODE_CODE_C,      // Double-digit numeric code
    BARCODE_CODE_STOP,   // Stopped
} barcode_code_t;

typedef unsigned char barcode_symbol_t;

typedef struct {
    barcode_symbol_t *buffer;
    size_t bufferLength;
    barcode_code_t code;
    size_t length;              // In symbols
    bool error;
} barcode_t;

// The maximum number of symbols required for the specified number of digits (strictly 0-9), even numbers are more efficient
// (Odd: START SYMBOL_PER_TWO_DIGITS CODE_CHANGE LAST_DIGIT CHECKSUM STOP; Even: START SYMBOL_PER_TWO_DIGITS CHECKSUM STOP;)
#define BARCODE_SYMBOLS_NUMERIC(_digits) (3 + ((_digits) / 2) + (((_digits) & 1) ? 2 : 0))

// The maximum number of symbols required for the specified amount of (non-control-character) ASCII text
#define BARCODE_SYMBOLS_TEXT(_characters) (3 + (_characters))

// The maximum number of bytes required for a bitmap of the specified number of symbols (without quiet zone)
// All symbols except STOP are 11 bars, STOP is 13 bars
#define BARCODE_BITMAP_SIZE_NO_QUIET(_symbols) ((((_symbols) * 11 + 2) + 7) / 8)

// The maximum number of bytes required for a bitmap of the specified number of symbols (with a quiet zone)
// As above, quiet zone is 10 bars each side.
#define BARCODE_BITMAP_SIZE_QUIET(_symbols) ((((_symbols) * 11 + 2 + 10 + 10) + 7) / 8)


// Initialize a barcode object, using the specified symbol buffer and its length (in symbols)
void BarcodeInit(barcode_t *barcode, barcode_symbol_t *buffer, size_t bufferLength);

// Append the specified string to the barcode
void BarcodeAppend(barcode_t *barcode, const char *text);

// Completes the barcode and writes the object as a bitmap (0=black, 1=white) using the specified buffer, returns the length in bars/bits. Optionally adds a 10-unit quiet zone either side.
size_t BarcodeBits(barcode_t *barcode, uint8_t *buffer, size_t bufferSize, bool addQuietZone);

// Returns the bar/bit at the specified index in the output bitmap (false=black, true=white)
#define BARCODE_BIT(_buffer, _offset) ((*((uint8_t *)(_buffer) + ((_offset) / 8)) & (1 << ((_offset) & 7))) != 0)

#endif
