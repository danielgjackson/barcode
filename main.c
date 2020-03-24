// Barcode - Generates a CODE128 barcode
// Dan Jackson, 2019

#ifdef _WIN32
#include <windows.h>
#endif
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "barcode.h"

#define DEFAULT_HEIGHT 5

typedef enum {
    OUTPUT_TEXT_WIDE,
    OUTPUT_TEXT_NARROW,
    OUTPUT_IMAGE_BITMAP,
} output_mode_t;

// Endian-independent short/long read/write
static void fputshort(uint16_t v, FILE *fp) { fputc((uint8_t)((v >> 0) & 0xff), fp); fputc((uint8_t)((v >> 8) & 0xff), fp); }
static void fputlong(uint32_t v, FILE *fp) { fputc((uint8_t)((v >> 0) & 0xff), fp); fputc((uint8_t)((v >> 8) & 0xff), fp); fputc((uint8_t)((v >> 16) & 0xff), fp); fputc((uint8_t)((v >> 24) & 0xff), fp); }

static void OutputBarcodeTextWide(FILE *fp, uint8_t *bitmap, size_t length, int scale, int height, bool invert)
{
    for (int repeat = 0; repeat < height; repeat++)
    {
        for (int i = 0; i < scale * length; i++)
        {
            bool bit = BARCODE_BIT(bitmap, i / scale) ^ invert;
            fprintf(fp, "%s", bit ? " " : "█");	// \u{2588} block
        }
        fprintf(fp, "\n");
    }
}

static void OutputBarcodeTextNarrow(FILE *fp, uint8_t *bitmap, size_t length, int scale, int height, bool invert)
{
    for (int repeat = 0; repeat < height; repeat++)
    {
        for (int i = 0; i < scale * length; i += 2)
        {
            bool bit0 = BARCODE_BIT(bitmap, i / scale);
            bool bit1 = ((i + 1) / scale) < length ? BARCODE_BIT(bitmap, (i + 1) / scale) : !invert;
            int value = ((bit0 ? 2 : 0) + (bit1 ? 1 : 0)) ^ (invert ? 0x3 : 0x0);
            switch (value)
            {
                case 0: fprintf(fp, "█"); break; // '\u{2588}' block
                case 1: fprintf(fp, "▌"); break; // '\u{258C}' left
                case 2: fprintf(fp, "▐"); break; // '\u{2590}' right
                case 3: fprintf(fp, " "); break; // '\u{0020}' space
            }
        }
        fprintf(fp, "\n");
    }
}


static void OutputBarcodeImageBitmap(FILE *fp, uint8_t *bitmap, size_t length, int scale, int height, bool invert)
{
    const int BMP_HEADERSIZE = 54;
    const int BMP_PAL_SIZE = 2 * 4;

    int width = length * scale;
    int span = ((width + 31) / 32) * 4;
    int bufferSize = span * height;

    fputc('B', fp); fputc('M', fp); // bfType
    fputlong(bufferSize + BMP_HEADERSIZE + BMP_PAL_SIZE, fp); // bfSize
    fputshort(0, fp);              // bfReserved1
    fputshort(0, fp);              // bfReserved2
    fputlong(BMP_HEADERSIZE + BMP_PAL_SIZE, fp); // bfOffBits
    fputlong(40, fp);              // biSize
    fputlong(width, fp);           // biWidth
    fputlong(height, fp);          // biHeight (negative for top-down)
    fputshort(1, fp);              // biPlanes
    fputshort(1, fp);              // biBitCount
    fputlong(0, fp);               // biCompression
    fputlong(bufferSize, fp);      // biSizeImage
    fputlong(0, fp);               // biXPelsPerMeter 3780
    fputlong(0, fp);               // biYPelsPerMeter 3780
    fputlong(0, fp);               // biClrUsed
    fputlong(0, fp);               // biClrImportant
    // Palette Entry 0 - black
    fputc(0x00, fp); fputc(0x00, fp); fputc(0x00, fp); fputc(0x00, fp); 
    // Palette Entry 1 - white
    fputc(0xff, fp); fputc(0xff, fp); fputc(0xff, fp); fputc(0x00, fp); 
    
    // Bitmap data
    for (int y = 0; y < height; y++)
    {
        for (int h = 0; h < span; h++)
        {
            uint8_t v = 0x00;
            for (int b = 0; b < 8; b++)
            {
                int i = (h * 8 + b) / scale;
                bool bit = (i < length) ? BARCODE_BIT(bitmap, i) ^ invert : 0;
                v |= bit << (7 - b);
            }
            fprintf(fp, "%c", v);
        }
    }
}


int main(int argc, char *argv[])
{
    FILE *ofp = stdout;
    const char *value = NULL;
    bool help = false;
    bool invert = false;
    int quiet = BARCODE_QUIET_STANDARD;
    output_mode_t outputMode = OUTPUT_TEXT_NARROW;
    int scale = 1;
    int height = DEFAULT_HEIGHT;
    bool address = false;
    
    for (int i = 1; i < argc; i++)
    {
        if (!strcmp(argv[i], "--help")) { help = true; }
        else if (!strcmp(argv[i], "--height")) { height = atoi(argv[++i]); }
        else if (!strcmp(argv[i], "--scale")) { scale = atoi(argv[++i]); }
        else if (!strcmp(argv[i], "--quiet")) { quiet = atoi(argv[++i]); }
        else if (!strcmp(argv[i], "--invert")) { invert = !invert; }
        else if (!strcmp(argv[i], "--file"))
        {
            ofp = fopen(argv[++i], "wb");
            if (ofp == NULL) { fprintf(stderr, "ERROR: Unable to open output filename: %s\n", argv[i]); return -1; }
        }
        else if (!strcmp(argv[i], "--output:wide")) { outputMode = OUTPUT_TEXT_WIDE; }
        else if (!strcmp(argv[i], "--output:narrow")) { outputMode = OUTPUT_TEXT_NARROW; }
        else if (!strcmp(argv[i], "--output:bmp")) { outputMode = OUTPUT_IMAGE_BITMAP; }
        else if (!strcmp(argv[i], "--address")) { address = true; }
        else if (argv[i][0] == '-')
        {
            fprintf(stderr, "ERROR: Unrecognized parameter: %s\n", argv[i]); 
            help = true;
            break;
        }
        else if (value == NULL)
        {
            value = argv[i];
        }
        else
        {
            fprintf(stderr, "ERROR: Unrecognized positional parameter: %s\n", argv[i]); 
            help = true;
            break;
        }
    }

    if (value == NULL)
    {
        fprintf(stderr, "ERROR: Value not specified.\n"); 
        help = true;
    }

    if (help)
    {
        fprintf(stderr, "USAGE: barcode [--height 5] [--scale 1] [--quiet 10] [--invert] [--output:<wide|narrow|bmp>] [--file filename] <value>\n"); 
        return -1;
    }

    if (address) 
    {
        // Special decimal conversion for 6-byte Bluetooth hex addresses given in the format: "01:23:45:67:89:AB", ignoring top two bits (signify private address), output is 128 pixel width.
        if (strlen(value) != 17 || value[2] != ':' || value[5] != ':' || value[8] != ':' || value[11] != ':' || value[14] != ':')  { fprintf(stderr, "ERROR: Address format error: %s\n", value); return -1; }
        static char buffer[14 + 1];
        uint64_t address = ((uint64_t)strtoul(value + 0, NULL, 16) << 40) | ((uint64_t)strtoul(value + 3, NULL, 16) << 32) | ((uint64_t)strtoul(value + 6, NULL, 16) << 24) | ((uint64_t)strtoul(value + 9, NULL, 16) << 16) | ((uint64_t)strtoul(value + 12, NULL, 16) << 8) | ((uint64_t)strtoul(value + 15, NULL, 16));
        address &= (uint64_t)0x003fffffffffffull;   // Mask off top two bits (46-bit number)
        sprintf(buffer, "%014"PRIu64"", address);   // Max value is 70368744177663 (14 digits)
        printf("Address: %02x:%02x:%02x:%02x:%02x:%02x\n", (int)((address >> (5*8)) & 0xff),  (int)((address >> (4*8)) & 0xff), (int)((address >> (3*8)) & 0xff), (int)((address >> (2*8)) & 0xff), (int)((address >> (1*8)) & 0xff), (int)((address) & 0xff));
        printf("Decimal: %s\n", buffer);
        value = buffer;
        quiet = 8;
    }

    // Generates the barcode as a bitmap (0=black, 1=white) using the specified buffer, returns the length in bars/bits. Optionally adds a 10-unit quiet zone either side.
    uint8_t bitmap[BARCODE_SIZE_TEXT(14, BARCODE_QUIET_STANDARD)] = {0};
    size_t length = Barcode(bitmap, sizeof(bitmap), quiet, value);
    //printf("Length = %d\n", (int)length);

#ifdef _WIN32
    if (outputMode == OUTPUT_TEXT_WIDE || outputMode == OUTPUT_TEXT_NARROW) SetConsoleOutputCP(CP_UTF8);
#endif

    switch (outputMode)
    {
        case OUTPUT_TEXT_WIDE: OutputBarcodeTextWide(ofp, bitmap, length, scale, height, invert); break;
        case OUTPUT_TEXT_NARROW: OutputBarcodeTextNarrow(ofp, bitmap, length, scale, height, invert); break;
        case OUTPUT_IMAGE_BITMAP: OutputBarcodeImageBitmap(ofp, bitmap, length, scale, height, invert); break;
        default: fprintf(ofp, "<error>"); break;
    }

    if (ofp != stdout) fclose(ofp);
    return 0;
}
