// Barcode - Generates a CODE128 barcode
// Dan Jackson, 2019

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS // This is an example program only
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
    OUTPUT_INFO,
    OUTPUT_TEXT_WIDE,
    OUTPUT_TEXT_NARROW,
    OUTPUT_IMAGE_BITMAP,
    OUTPUT_SIXEL,
    OUTPUT_TGP,
} output_mode_t;

// Endian-independent short/long read/write
static void fputshort(uint16_t v, FILE *fp) { fputc((uint8_t)((v >> 0) & 0xff), fp); fputc((uint8_t)((v >> 8) & 0xff), fp); }
static void fputlong(uint32_t v, FILE *fp) { fputc((uint8_t)((v >> 0) & 0xff), fp); fputc((uint8_t)((v >> 8) & 0xff), fp); fputc((uint8_t)((v >> 16) & 0xff), fp); fputc((uint8_t)((v >> 24) & 0xff), fp); }

static void OutputBarcodeInfo(FILE *fp, uint8_t *bitmap, size_t length, const char *input)
{
    size_t size = ((length + 7) >> 3);
    size_t inputLength = strlen(input);
    
    fprintf(fp, "{\n");

    fprintf(fp, "    \"value\": \"");
    for (size_t i = 0; i < inputLength; i++)
    {
        char c = (char)input[i];
        char out[5] = {0};
        if (c < 0x20 || c >= 127)
        {
            sprintf(out, "\\x%02x", (unsigned int)(c & 0xff));
        }
        else if (c == '\\')
        {
            sprintf(out, "\\\\");
        }
        else if (c == '\"')
        {
            sprintf(out, "\\\"");
        }
        else
        {
            sprintf(out, "%c", c);
        }
        fprintf(fp, "%s", out);
    }
    fprintf(fp, "\",\n");

    fprintf(fp, "    \"width\": %u,\n", (unsigned int)length);

    fprintf(fp, "    \"bytes\": %u,\n", (unsigned int)size);

    fprintf(fp, "    \"data\": [");
    for (size_t i = 0; i < size; i++)
    {
        fprintf(fp, "%s0x%02x", i > 0 ? ", " : "", bitmap[i]);
    }
    fprintf(fp, "]\n");

    fprintf(fp, "}\n");
}

static void OutputBarcodeTextWide(FILE *fp, uint8_t *bitmap, size_t length, int scale, int height, bool invert)
{
    for (int repeat = 0; repeat < height; repeat++)
    {
        for (int i = 0; i < scale * length; i++)
        {
            bool bit = BARCODE_BIT(bitmap, i / scale) ^ invert;
            fprintf(fp, "%s", bit ? " " : "█");    // \u{2588} block
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

    int width = (int)(length * scale);
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


static void OutputBarcodeSixel(FILE *fp, uint8_t *bitmap, size_t length, int scale, int height, bool invert)
{
    const int LINE_HEIGHT = 6;
    // Enter sixel mode
    fprintf(fp, "\x1BP7;1q");    // 1:1 ratio, 0 pixels remain at current color
    // Set color map
    fprintf(fp, "#0;2;0;0;0");       // Background
    fprintf(fp, "#1;2;100;100;100");
    for (int y = 0; y < height; y += LINE_HEIGHT)
    {
        const int passes = 2;
        for (int pass = 0; pass < passes; pass++)
        {
            // Start a pass in a specific color
            fprintf(fp, "#%d", pass);
            // Line data
            for (int x = 0; x < scale * length; x += scale)
            {
                int value = 0;
                for (int yy = 0; yy < LINE_HEIGHT; yy++) {
                    if (y + yy >= height * scale) break;
                    int bitValue = (BARCODE_BIT(bitmap, x / scale) ^ invert) ? 1 : 0;
                    int bit = (bitValue == pass) ? 1 : 0;
                    value |= (bit ? 0x01 : 0x00) << yy;
                }
                // Six pixels strip at 'scale' (repeated) width
                if (scale == 1) {
                    fprintf(fp, "%c", value + 63);
                } else if (scale == 2) {
                    fprintf(fp, "%c%c", value + 63, value + 63);
                } else if (scale == 3) {
                    fprintf(fp, "%c%c%c", value + 63, value + 63, value + 63);
                } else if (scale > 3) {
                    fprintf(fp, "!%d%c", scale, value + 63);
                }
            }
            // Return to start of the line
            if (pass + 1 < passes) {
                fprintf(fp, "$");
            }
        }
        // Next line
        if (y + LINE_HEIGHT < height * scale) {
            fprintf(fp, "-");
        }
    }
    // Exit sixel mode
    fprintf(fp, "\x1B\\");
    fprintf(fp, "\n");
}


// TGP - Terminal Graphics Protocol
static void OutputBarcodeTerminalGraphicsProtocol(FILE *fp, uint8_t *bitmap, size_t length, int scale, int height, bool invert)
{
    // Image buffer
    bool alpha = false;
    size_t imageBufferSize = (size_t)(height * length * scale * (alpha ? 4 : 3));
    unsigned char *imageBuffer = (unsigned char *)malloc(imageBufferSize);
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < scale * length; x++)
        {
            int bitValue = (BARCODE_BIT(bitmap, x / scale) ^ invert) ? 1 : 0;
            int ofs = (y * length * scale + x) * (alpha ? 4 : 3);
            imageBuffer[ofs + 0] = bitValue ? 0xff : 0x00; // R
            imageBuffer[ofs + 1] = bitValue ? 0xff : 0x00; // G
            imageBuffer[ofs + 2] = bitValue ? 0xff : 0x00; // B
        }
    }

    // Convert to Base64
    size_t base64Size = ((imageBufferSize + 2) / 3) * 4;
    char *base64Buffer = (char *)malloc(base64Size + 1);
    // Manually encode to Base64
    const char *base64Chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    for (size_t i = 0; i < imageBufferSize; i += 3)
    {
        uint32_t value = (imageBuffer[i] << 16) | (i + 1 < imageBufferSize ? (imageBuffer[i + 1] << 8) : 0) | (i + 2 < imageBufferSize ? imageBuffer[i + 2] : 0);
        size_t ofs = (i / 3) * 4;
        base64Buffer[ofs++] = base64Chars[(value >> 18) & 0x3f];
        base64Buffer[ofs++] = base64Chars[(value >> 12) & 0x3f];
        base64Buffer[ofs++] = (i + 1 < imageBufferSize) ? base64Chars[(value >> 6) & 0x3f] : '=';
        base64Buffer[ofs++] = (i + 2 < imageBufferSize) ? base64Chars[value & 0x3f] : '=';
    }

    // Chunked output
    int MAX_CHUNK_SIZE = 4096;
    char initialControls[256];
    for (size_t i = 0; i < base64Size; i += MAX_CHUNK_SIZE) {
        int chunkSize = (i + MAX_CHUNK_SIZE < base64Size) ? MAX_CHUNK_SIZE : (base64Size - i);
        char *chunk = base64Buffer + i;
        if (i == 0) {
            // action transmit and display (a=T), direct transfer (t=d), uncompressed (o=), 3/4 bytes per pixel (f=24/32 bits per pixel), no responses at all (q=2)
            sprintf(initialControls, "a=T,f=%d,s=%d,v=%d,t=d,q=2,", alpha ? 32 : 24, (int)(length * scale), (int)(height));
        } else {
            initialControls[0] = '\0';
        }
        int nonTerminal = (i + MAX_CHUNK_SIZE < base64Size) ? 1 : 0;
        fprintf(fp, "\x1B_G%sm=%d;%.*s\x1B\\", initialControls, nonTerminal, (int)chunkSize, chunk);
    }
    fprintf(fp, "\n");

    // Clear up buffers
    free(base64Buffer);
    free(imageBuffer);

    return;
}


int main(int argc, char *argv[])
{
    FILE *ofp = stdout;
    const char *value = NULL;
    bool help = false;
    bool invert = false;
    int quiet = BARCODE_QUIET_STANDARD;
    output_mode_t outputMode = OUTPUT_TEXT_NARROW;
    int scale = -1;
    int height = -1;
    bool address = false;
    barcode_code_t fixedCode = BARCODE_CODE_NONE;

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
        else if (!strcmp(argv[i], "--output:info")) { outputMode = OUTPUT_INFO; }
        else if (!strcmp(argv[i], "--output:wide")) { outputMode = OUTPUT_TEXT_WIDE; }
        else if (!strcmp(argv[i], "--output:narrow")) { outputMode = OUTPUT_TEXT_NARROW; }
        else if (!strcmp(argv[i], "--output:bmp")) { outputMode = OUTPUT_IMAGE_BITMAP; }
        else if (!strcmp(argv[i], "--output:sixel")) { outputMode = OUTPUT_SIXEL; }
        else if (!strcmp(argv[i], "--output:tgp")) { outputMode = OUTPUT_TGP; }
        else if (!strcmp(argv[i], "--code:auto")) { fixedCode = BARCODE_CODE_NONE; }
        else if (!strcmp(argv[i], "--code:a")) { fixedCode = BARCODE_CODE_A; }
        else if (!strcmp(argv[i], "--code:b")) { fixedCode = BARCODE_CODE_B; }
        else if (!strcmp(argv[i], "--code:c")) { fixedCode = BARCODE_CODE_C; }
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
        fprintf(stderr, "USAGE: barcode [--height 5] [--scale 1] [--quiet 10] [--invert] [--output:<wide|narrow|bmp|sixel|tgp>] [--file filename] <value>\n"); 
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

    // Defaults
    if (height < 0)
    {
        height = (outputMode == OUTPUT_SIXEL || outputMode == OUTPUT_TGP || outputMode == OUTPUT_IMAGE_BITMAP) ? 30 : DEFAULT_HEIGHT;
    }
    if (scale < 0)
    {
        scale = (outputMode == OUTPUT_SIXEL || outputMode == OUTPUT_TGP || outputMode == OUTPUT_IMAGE_BITMAP) ? 1 : 1;
    }

    // Generates the barcode as a bitmap (0=black, 1=white) using the specified buffer, returns the length in bars/bits. Optionally adds a 10-unit quiet zone either side.
    uint8_t bitmap[BARCODE_SIZE_TEXT(14, BARCODE_QUIET_STANDARD)] = {0};
    size_t length = Barcode(bitmap, sizeof(bitmap), quiet, value, fixedCode);
    //printf("Length = %d\n", (int)length);

#ifdef _WIN32
    if (outputMode == OUTPUT_TEXT_WIDE || outputMode == OUTPUT_TEXT_NARROW) SetConsoleOutputCP(CP_UTF8);
#endif

    switch (outputMode)
    {
        case OUTPUT_INFO: OutputBarcodeInfo(ofp, bitmap, length, value); break;
        case OUTPUT_TEXT_WIDE: OutputBarcodeTextWide(ofp, bitmap, length, scale, height, invert); break;
        case OUTPUT_TEXT_NARROW: OutputBarcodeTextNarrow(ofp, bitmap, length, scale, height, invert); break;
        case OUTPUT_IMAGE_BITMAP: OutputBarcodeImageBitmap(ofp, bitmap, length, scale, height, invert); break;
        case OUTPUT_SIXEL: OutputBarcodeSixel(ofp, bitmap, length, scale, height, invert); break;
        case OUTPUT_TGP: OutputBarcodeTerminalGraphicsProtocol(ofp, bitmap, length, scale, height, invert); break;
        default: fprintf(ofp, "<error>"); break;
    }

    if (ofp != stdout) fclose(ofp);
    return 0;
}
