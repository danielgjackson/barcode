# Barcode

Generates a CODE128 barcode.  

Required files: [`barcode.h`](barcode.h) [`barcode.c`](barcode.c)


## Initialize a barcode

Initialize a `barcode` object, using the specified symbol `buffer` and its length (in symbols):

```c
void BarcodeInit(barcode_t *barcode, barcode_symbol_t *buffer, size_t bufferLength);
```

To determine the required size of your symbol buffer, the following are available (actually implemented as macros, so values are available at compile time):

```c
// The maximum number of symbols required for the EXACT specified number of digits (strictly 0-9), odd numbers are less efficient
size_t BARCODE_SYMBOLS_NUMERIC_EXACT(unsigned int digits);

// The maximum number of symbols required for numbers up to the specified number of digits (strictly 0-9)
size_t BARCODE_SYMBOLS_NUMERIC(unsigned int digits);

// The maximum number of symbols required for the specified amount of (non-control-character) ASCII text
size_t BARCODE_SYMBOLS_TEXT(unsigned int characters);
```

## Add text to your barcode

Append the specified string to the barcode (you may make repeated calls):

```c
void BarcodeAppend(barcode_t *barcode, const char *text);
```

## Retrieve the finished barcode

To completes the barcode and write the object as a bitmap (0=black, 1=white) using the specified buffer:

```c
size_t BarcodeBits(barcode_t *barcode, uint8_t *buffer, size_t bufferSize, bool addQuietZone);
```

This returns the length in bars/bits. You may optionally add a 10-unit (white) quiet zone either side of the output.

To determine the required size of your bitmap (in bytes), the following are available (actually implemented as macros, so values are available at compile time):

```c
// The maximum number of bytes required for a bitmap of the specified number of symbols (without quiet zone)
size_t BARCODE_BITMAP_SIZE_NO_QUIET(unsigned int symbols);

// The maximum number of bytes required for a bitmap of the specified number of symbols (with a quiet zone)
size_t BARCODE_BITMAP_SIZE_QUIET(unsigned int symbols);
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
