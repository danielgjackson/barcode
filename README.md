# Barcode

Generates a CODE128 barcode.  

Required files: [`barcode.h`](barcode.h) [`barcode.c`](barcode.c)


## Create a barcode

To generate the barcode and write the object as a bitmap (0=black, 1=white) using the specified buffer:

```c
size_t Barcode(uint8_t *buffer, size_t bufferSize, int quietZone, const char *text);
```

This returns the number of bars/bits written to the buffer (up to `8 * bufferSize`). You may optionally add a (white) quiet zone either side of the output (0-16 bits): typically `BARCODE_QUIET_STANDARD` (=10) bits, or `BARCODE_QUIET_NONE` (=0) for just the raw barcode.

To determine the required size of your bitmap (in bytes), the following utilities are available (actually implemented as macros, so available at compile time):

```c
// The maximum number of bytes required for the EXACT specified number of digits (strictly 0-9), odd numbers are less efficient
// (Odd: START SYMBOL_PER_TWO_DIGITS CODE_CHANGE LAST_DIGIT CHECKSUM STOP; Even: START SYMBOL_PER_TWO_DIGITS CHECKSUM STOP;)
int BARCODE_SIZE_NUMERIC_EXACT(int digits, int quiet);

// The maximum number of bytes required for numbers up to the specified number of digits (strictly 0-9)
int BARCODE_SIZE_NUMERIC_NUMERIC(int digits, int quiet);

// The maximum number of bytes required for the specified amount of (non-control-character) ASCII text
int BARCODE_SIZE_NUMERIC_TEXT(int characters, int quiet);
```

To find the value of a bar/bit at the specified index in the output bitmap (false=black, true=white), the following utility function is available (actually implemented as a macro):

```c
bool BARCODE_BIT(uint8_t *buffer, size_t offset);
```

## Demonstration program

Demonstration program ([`main.c`](main.c)), usage (use `--invert` if your console is light-on-dark):

```bash
barcode --invert "TEXT TO BECOME BARCODE"
```
