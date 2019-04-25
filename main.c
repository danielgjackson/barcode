// Barcode - Generates a CODE128 barcode
// Dan Jackson, 2019

#ifdef _WIN32
#include <Windows.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "barcode.h"

static void PrintBarcode(const char *value)
{
    barcode_t barcode;
    barcode_symbol_t barcodeBuffer[BARCODE_SYMBOLS_TEXT(64)];   // BARCODE_SYMBOLS_NUMERIC(4); BARCODE_SYMBOLS_TEXT(256);

    BarcodeInit(&barcode, barcodeBuffer, sizeof(barcodeBuffer) / sizeof(barcode_symbol_t));
    BarcodeAppend(&barcode, value);
    uint8_t bitmap[BARCODE_BITMAP_SIZE_QUIET(sizeof(barcodeBuffer) / sizeof(barcode_symbol_t))] = {0};

    size_t length = BarcodeBits(&barcode, bitmap, sizeof(bitmap), true);

    for (int repeat = 0; repeat < 5; repeat++)
    {
        for (int i = 0; i < length; i++)
        {
            bool bit = BARCODE_BIT(bitmap, i);
            printf("%s", bit ? "█" : " ");	// Assumes output will be light on dark terminal, unicode \u2588
        }
        printf("\n");
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2) { fprintf(stderr, "ERROR: Single parameter not specified.\n"); return -1; }
#ifdef _WIN32
	SetConsoleOutputCP(CP_UTF8);
#endif
    const char *value = argv[1];

    // Special decimal conversion for 6-byte addresses in the format: "01:23:45:67:89:AB"
    if (strlen(value) == 17 && value[2] == ':' && value[5] == ':' && value[8] == ':' && value[11] == ':' && value[14] == ':')
    {
        uint64_t address = ((uint64_t)strtoul(value + 0, NULL, 16) << 40) | ((uint64_t)strtoul(value + 3, NULL, 16) << 32) | ((uint64_t)strtoul(value + 6, NULL, 16) << 24) | ((uint64_t)strtoul(value + 9, NULL, 16) << 16) | ((uint64_t)strtoul(value + 12, NULL, 16) << 8) | ((uint64_t)strtoul(value + 15, NULL, 16));
        address &= (uint64_t)0x003fffffffffffull;  // Mask off top two bits (46-bit number)
        char buffer[14 + 1];
        // TODO: Use PRId64 macro instead
        sprintf(buffer, "%014llu", (long long unsigned int)address);   // Max value is 70368744177663 (14 digits)
        printf("Address: %02x:%02x:%02x:%02x:%02x:%02x\n", (int)((address >> (5*8)) & 0xff),  (int)((address >> (4*8)) & 0xff), (int)((address >> (3*8)) & 0xff), (int)((address >> (2*8)) & 0xff), (int)((address >> (1*8)) & 0xff), (int)((address) & 0xff));
        printf("Decimal: %s\n", buffer);
        value = buffer;
    }
    PrintBarcode(value);
    return 0;
}
