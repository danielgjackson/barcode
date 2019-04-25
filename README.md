# Barcode

Generates a CODE128 barcode.

Required files: `barcode.h` `barcode.c`

Basic functions:

```c
// Initialize a barcode object, using the specified symbol buffer and its length (in symbols)
void BarcodeInit(barcode_t *barcode, barcode_symbol_t *buffer, size_t bufferLength);

// Append the specified string to the barcode
void BarcodeAppend(barcode_t *barcode, const char *text);

// Completes the barcode and writes the object as a bitmap (0=black, 1=white) using the specified buffer, returns the length in bars/bits. Optionally adds a 10-unit quiet zone either side.
size_t BarcodeBits(barcode_t *barcode, uint8_t *buffer, size_t bufferSize, bool addQuietZone);
```

Helper functions (actually implemented as macros, values are available at compile time):

```c
// The maximum number of symbols required for the specified number of digits (strictly 0-9), even numbers are more efficient
size_t BARCODE_SYMBOLS_NUMERIC(unsigned int digits);

// The maximum number of symbols required for the specified amount of (non-control-character) ASCII text
size_t BARCODE_SYMBOLS_TEXT(unsigned int characters);

// The maximum number of bytes required for a bitmap of the specified number of symbols (without quiet zone)
size_t BARCODE_BITMAP_SIZE_NO_QUIET(unsigned int symbols);

// The maximum number of bytes required for a bitmap of the specified number of symbols (with a quiet zone)
size_t BARCODE_BITMAP_SIZE_QUIET(unsigned int symbols);

// Returns the bar/bit at the specified index in the output bitmap (false=black, true=white)
bool BARCODE_BIT(uint8_t buffer, size_t offset);
```

Demonstration program usage (`main.c`): `barcode "TEXT TO BECOME BARCODE"`

