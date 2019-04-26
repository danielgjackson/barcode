# Barcode

Generates a CODE128 barcode.  

Required files: [`barcode.h`](barcode.h) [`barcode.c`](barcode.c)


## Create a barcode

To complete the barcode and write the object as a bitmap (0=black, 1=white) using the specified buffer:

```c
size_t Barcode(uint8_t *buffer, size_t bufferSize, bool addQuietZone, const char *text);
```

This returns the length in bars/bits. You may optionally add a 10-unit (white) quiet zone either side of the output.

To determine the required size of your bitmap (in bytes), the following are available (actually implemented as macros, so values are available at compile time):

```c
// The maximum number of bytes required for the EXACT specified number of digits (strictly 0-9), odd numbers are less efficient
// (Odd: START SYMBOL_PER_TWO_DIGITS CODE_CHANGE LAST_DIGIT CHECKSUM STOP; Even: START SYMBOL_PER_TWO_DIGITS CHECKSUM STOP;)
#define BARCODE_SIZE_BARE_NUMERIC_EXACT(_digits)
#define BARCODE_SIZE_QUIET_NUMERIC_EXACT(_digits)

// The maximum number of bytes required for numbers up to the specified number of digits (strictly 0-9)
#define BARCODE_SIZE_BARE_NUMERIC(_digits)
#define BARCODE_SIZE_QUIET_NUMERIC(_digits)

// The maximum number of bytes required for the specified amount of (non-control-character) ASCII text
#define BARCODE_SIZE_BARE_TEXT(_characters)
#define BARCODE_SIZE_QUIET_TEXT(_characters)
```

To determine a bar/bit at the specified index in the output bitmap (false=black, true=white), the following utility function is available (actually implemented as a macro):

```c
bool BARCODE_BIT(uint8_t *buffer, size_t offset);
```

## Demonstration program

Demonstration program ([`main.c`](main.c)), usage:

```bash
barcode "TEXT TO BECOME BARCODE"
```
