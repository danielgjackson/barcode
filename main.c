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
    PrintBarcode(value);
    return 0;
}
