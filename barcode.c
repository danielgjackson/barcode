// Barcode - Generates a CODE128 barcode
// Dan Jackson, 2019

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "barcode.h"

typedef unsigned char barcode_symbol_t;

typedef struct
{
    uint8_t *buffer;
    size_t bufferSize;
    size_t offset;
    
    uint32_t checksum;
    size_t numSymbols;

    barcode_code_t code;
    bool error;
} barcode_t;

// 218 bytes
static const uint16_t code128[107] =
{
    // 16-bit packed format: rwwppppppppppppp 
    // (==0 = nothing; r = 1-bit reserved; w = 2-bits (width-10); p = 13-bit right-aligned pattern)
    // packed hex, packed bin.,      BSBSBSbs     val  CA  CB  CC = representation (0-105 are <11 bars>)
    0x2133, // 0b0010000100110011, 0x21222200, //   0  SP  SP  00 = 32
    0x2193, // 0b0010000110010011, 0x22212200, //   1   !   !  01 = 33
    0x2199, // 0b0010000110011001, 0x22222100, //   2   "   "  02 = 34
    0x2367, // 0b0010001101100111, 0x12122300, //   3   #   #  03 = 35
    0x2373, // 0b0010001101110011, 0x12132200, //   4   $   $  04 = 36
    0x23B3, // 0b0010001110110011, 0x13122200, //   5   %   %  05 = 37
    0x2337, // 0b0010001100110111, 0x12221300, //   6   &   &  06 = 38
    0x233B, // 0b0010001100111011, 0x12231200, //   7   '   '  07 = 39
    0x239B, // 0b0010001110011011, 0x13221200, //   8   (   (  08 = 40
    0x21B7, // 0b0010000110110111, 0x22121300, //   9   )   )  09 = 41
    0x21BB, // 0b0010000110111011, 0x22131200, //  10   *   *  10 = 42
    0x21DB, // 0b0010000111011011, 0x23121200, //  11   +   +  11 = 43
    0x2263, // 0b0010001001100011, 0x11223200, //  12   ,   ,  12 = 44
    0x2323, // 0b0010001100100011, 0x12213200, //  13   -   -  13 = 45
    0x2331, // 0b0010001100110001, 0x12223100, //  14   .   .  14 = 46
    0x2233, // 0b0010001000110011, 0x11322200, //  15   /   /  15 = 47
    0x2313, // 0b0010001100010011, 0x12312200, //  16   0   0  16 = 48
    0x2319, // 0b0010001100011001, 0x12322100, //  17   1   1  17 = 49
    0x218D, // 0b0010000110001101, 0x22321100, //  18   2   2  18 = 50
    0x21A3, // 0b0010000110100011, 0x22113200, //  19   3   3  19 = 51
    0x21B1, // 0b0010000110110001, 0x22123100, //  20   4   4  20 = 52
    0x211B, // 0b0010000100011011, 0x21321200, //  21   5   5  21 = 53
    0x218B, // 0b0010000110001011, 0x22311200, //  22   6   6  22 = 54
    0x2091, // 0b0010000010010001, 0x31213100, //  23   7   7  23 = 55
    0x20B3, // 0b0010000010110011, 0x31122200, //  24   8   8  24 = 56
    0x20D3, // 0b0010000011010011, 0x32112200, //  25   9   9  25 = 57
    0x20D9, // 0b0010000011011001, 0x32122100, //  26   :   :  26 = 58
    0x209B, // 0b0010000010011011, 0x31221200, //  27   ;   ;  27 = 59
    0x20CB, // 0b0010000011001011, 0x32211200, //  28   <   <  28 = 60
    0x20CD, // 0b0010000011001101, 0x32221100, //  29   =   =  29 = 61
    0x2127, // 0b0010000100100111, 0x21212300, //  30   >   >  30 = 62
    0x2139, // 0b0010000100111001, 0x21232100, //  31   ?   ?  31 = 63
    0x21C9, // 0b0010000111001001, 0x23212100, //  32   @   @  32 = 64
    0x22E7, // 0b0010001011100111, 0x11132300, //  33   A   A  33 = 65
    0x23A7, // 0b0010001110100111, 0x13112300, //  34   B   B  34 = 66
    0x23B9, // 0b0010001110111001, 0x13132100, //  35   C   C  35 = 67
    0x2277, // 0b0010001001110111, 0x11231300, //  36   D   D  36 = 68
    0x2397, // 0b0010001110010111, 0x13211300, //  37   E   E  37 = 69
    0x239D, // 0b0010001110011101, 0x13231100, //  38   F   F  38 = 70
    0x2177, // 0b0010000101110111, 0x21131300, //  39   G   G  39 = 71
    0x21D7, // 0b0010000111010111, 0x23111300, //  40   H   H  40 = 72
    0x21DD, // 0b0010000111011101, 0x23131100, //  41   I   I  41 = 73
    0x2247, // 0b0010001001000111, 0x11213300, //  42   J   J  42 = 74
    0x2271, // 0b0010001001110001, 0x11233100, //  43   K   K  43 = 75
    0x2391, // 0b0010001110010001, 0x13213100, //  44   L   L  44 = 76
    0x2227, // 0b0010001000100111, 0x11312300, //  45   M   M  45 = 77
    0x2239, // 0b0010001000111001, 0x11332100, //  46   N   N  46 = 78
    0x2389, // 0b0010001110001001, 0x13312100, //  47   O   O  47 = 79
    0x2089, // 0b0010000010001001, 0x31312100, //  48   P   P  48 = 80
    0x2171, // 0b0010000101110001, 0x21133100, //  49   Q   Q  49 = 81
    0x21D1, // 0b0010000111010001, 0x23113100, //  50   R   R  50 = 82
    0x2117, // 0b0010000100010111, 0x21311300, //  51   S   S  51 = 83
    0x211D, // 0b0010000100011101, 0x21331100, //  52   T   T  52 = 84
    0x2111, // 0b0010000100010001, 0x21313100, //  53   U   U  53 = 85
    0x20A7, // 0b0010000010100111, 0x31112300, //  54   V   V  54 = 86
    0x20B9, // 0b0010000010111001, 0x31132100, //  55   W   W  55 = 87
    0x20E9, // 0b0010000011101001, 0x33112100, //  56   X   X  56 = 88
    0x2097, // 0b0010000010010111, 0x31211300, //  57   Y   Y  57 = 89
    0x209D, // 0b0010000010011101, 0x31231100, //  58   Z   Z  58 = 90
    0x20E5, // 0b0010000011100101, 0x33211100, //  59   [   [  59 = 91
    0x2085, // 0b0010000010000101, 0x31411100, //  60   \   \  60 = 92
    0x21BD, // 0b0010000110111101, 0x22141100, //  61   ]   ]  61 = 93
    0x2075, // 0b0010000001110101, 0x43111100, //  62   ^   ^  62 = 94
    0x22CF, // 0b0010001011001111, 0x11122400, //  63   _   _  63 = 95
    0x22F3, // 0b0010001011110011, 0x11142200, //  64 NUL   '  64 = 96
    0x234F, // 0b0010001101001111, 0x12112400, //  65 SOH   a  65 = 97
    0x2379, // 0b0010001101111001, 0x12142100, //  66 STX   b  66 = 98
    0x23D3, // 0b0010001111010011, 0x14112200, //  67 ETX   c  67 = 99
    0x23D9, // 0b0010001111011001, 0x14122100, //  68 EOT   d  68 =100
    0x226F, // 0b0010001001101111, 0x11221400, //  69 ENQ   e  69 =101
    0x227B, // 0b0010001001111011, 0x11241200, //  70 ACK   f  70 =102
    0x232F, // 0b0010001100101111, 0x12211400, //  71 BEL   g  71 =103
    0x233D, // 0b0010001100111101, 0x12241100, //  72  BS   h  72 =104
    0x23CB, // 0b0010001111001011, 0x14211200, //  73  HT   i  73 =105
    0x23CD, // 0b0010001111001101, 0x14221100, //  74  LF   j  74 =106
    0x21ED, // 0b0010000111101101, 0x24121100, //  75  VT   k  75 =107
    0x21AF, // 0b0010000110101111, 0x22111400, //  76  FF   l  76 =108
    0x2045, // 0b0010000001000101, 0x41311100, //  77  CR   m  77 =109
    0x21EB, // 0b0010000111101011, 0x24111200, //  78  SO   n  78 =110
    0x2385, // 0b0010001110000101, 0x13411100, //  79  SI   o  79 =111
    0x22C3, // 0b0010001011000011, 0x11124200, //  80 DLE   p  80 =112
    0x2343, // 0b0010001101000011, 0x12114200, //  81 DC1   q  81 =113
    0x2361, // 0b0010001101100001, 0x12124100, //  82 DC2   r  82 =114
    0x221B, // 0b0010001000011011, 0x11421200, //  83 DC3   s  83 =115
    0x230B, // 0b0010001100001011, 0x12411200, //  84 DC4   t  84 =116
    0x230D, // 0b0010001100001101, 0x12421100, //  85 NAK   u  85 =117
    0x205B, // 0b0010000001011011, 0x41121200, //  86 SYN   v  86 =118
    0x206B, // 0b0010000001101011, 0x42111200, //  87 ETB   w  87 =119
    0x206D, // 0b0010000001101101, 0x42121100, //  88 CAN   x  88 =120
    0x2121, // 0b0010000100100001, 0x21214100, //  89  EM   y  89 =121
    0x2109, // 0b0010000100001001, 0x21412100, //  90 SUB   z  90 =122
    0x2049, // 0b0010000001001001, 0x41212100, //  91 ESC   {  91 =123
    0x2287, // 0b0010001010000111, 0x11114300, //  92  FS   |  92 =124
    0x22E1, // 0b0010001011100001, 0x11134100, //  93  GS   }  93 =125
    0x23A1, // 0b0010001110100001, 0x13114100, //  94  RS   ~  94 =126
    0x2217, // 0b0010001000010111, 0x11411300, //  95  US DEL  95 =127
    0x221D, // 0b0010001000011101, 0x11431100, //  96 _F3 _F3  96 =128 _F3="FNC 3"
    0x2057, // 0b0010000001010111, 0x41111300, //  97 _F2 _F2  97 =129 _F2="FNC 2"
    0x205D, // 0b0010000001011101, 0x41131100, //  98 _SH _SH  98 =130 _SH="SHIFT"
    0x2221, // 0b0010001000100001, 0x11314100, //  99 _CC _CC  99 =131 _CC="CODE C"
    0x2211, // 0b0010001000010001, 0x11413100, // 100 _CB _F4 _CB =132 _CB="CODE B" _F4="FNC 4"
    0x20A1, // 0b0010000010100001, 0x31114100, // 101 _F4 _CA _CA =133 _F4="FNC 4" _CA="CODE A"
    0x2051, // 0b0010000001010001, 0x41113100, // 102 _F1 _F1 _F1 =134 _F1="FNC 1"
    0x217B, // 0b0010000101111011, 0x21141200, // 103 _SA _SA _SA =135 _SA="START (Code A)"
    0x216F, // 0b0010000101101111, 0x21121400, // 104 _SB _SB _SB =136 _SB="START (Code B)"
    0x2163, // 0b0010000101100011, 0x21123200, // 105 _SC _SC _SC =137 _SC="START (Code C)"
    0x6714, // 0b0110011100010100, 0x23311120, // 106 _ST _ST _ST =138 _ST="STOP" <13 bars>
    //0x03FF, // 0b0000001111111111, 0x0000000A, // 107 _QZ _QZ _QZ =139 _QZ="QUIET" <10 bars>
    //0x0000, // 0b0000000000000000, 0x00000000, // 108 _NO _NO _NO =140 _NO=(none) <0 bars>
};

static void BarcodeWriteBits(barcode_t *barcode, uint16_t pattern, int width)
{
    for (int i = width - 1; i >= 0; i--)
    {
        int byteOffset = (int)barcode->offset / 8;
//printf("%d %d/%d %s\n", i, byteOffset, barcode->bufferSize, barcode->error ? "ERROR" : "OK");
        if (byteOffset < barcode->bufferSize)
        {
            uint8_t *p = barcode->buffer + byteOffset;
            bool bit = (pattern & (1 << (i & 15))) != 0;
#ifdef BARCODE_MSB_FIRST
            uint8_t mask = (1 << (7 - (barcode->offset & 7)));
#else
            uint8_t mask = (1 << (barcode->offset & 7));
#endif
            if (bit)
            {
                *p |= mask;
            }
            else
            {
                *p &= ~mask;
            }
            barcode->offset++;
        }
        else
        {
            barcode->error = true;
        }
    }
}

static void BarcodeAppendSymbol(barcode_t *barcode, barcode_symbol_t symbol)
{
    if (barcode->error || barcode->code == BARCODE_CODE_STOP)
    {
        barcode->error = true;
        return;
    }

    // 16-bit packed format: rwwppppppppppppp 
    // (==0 = nothing; r = 1-bit reserved; w = 2-bits (width-10); p = 13-bit right-aligned pattern)
    // packed hex, packed bin.,      BSBSBSbs     val  CA  CB  CC = representation
    uint16_t pattern = code128[symbol];
    //bool reserved = (pattern & 0x8000) != 0;
    int width = 10 + ((pattern >> 13) & 0x03);
    BarcodeWriteBits(barcode, pattern, width);

    // Update checksum
    // checksum = SUM<(i+1) * X[i]> % 103
    barcode->checksum += (barcode->numSymbols ? (uint32_t)barcode->numSymbols : 1) * (uint32_t)symbol;
    barcode->checksum %= 103;
    barcode->numSymbols++;
}

static void BarcodeChangeCode(barcode_t *barcode, barcode_code_t newCode)
{
    if (barcode->error || barcode->code == newCode) return;
    if (barcode->code == BARCODE_CODE_STOP || newCode == BARCODE_CODE_NONE || newCode == BARCODE_CODE_STOP)
    {
        barcode->error = true;
        return;
    }

    if (barcode->code == BARCODE_CODE_NONE)
    {
        switch (newCode)
        {
            case BARCODE_CODE_A: BarcodeAppendSymbol(barcode, 103); break;  // START (Code A)
            case BARCODE_CODE_B: BarcodeAppendSymbol(barcode, 104); break;  // START (Code B)
            case BARCODE_CODE_C: BarcodeAppendSymbol(barcode, 105); break;  // START (Code C)
            default: barcode->error = true; break;
        }
    }
    else
    {
        switch (newCode)
        {
            case BARCODE_CODE_A: BarcodeAppendSymbol(barcode, 101); break;  // (switch) CODE A
            case BARCODE_CODE_B: BarcodeAppendSymbol(barcode, 100); break;  // (switch) CODE B
            case BARCODE_CODE_C: BarcodeAppendSymbol(barcode, 99); break;   // (switch) CODE C
            default: barcode->error = true; break;
        }
    }
    barcode->code = newCode;
}

static void BarcodeStop(barcode_t *barcode)
{
    // Do nothing if already stopped
    if (barcode->code == BARCODE_CODE_STOP) return;

    // Empty barcodes are barcodes too!
    if (barcode->code == BARCODE_CODE_NONE)
    {
        BarcodeChangeCode(barcode, BARCODE_CODE_B); // will generate start
    }
    
    BarcodeAppendSymbol(barcode, (barcode_symbol_t)barcode->checksum);

    // Add stop symbol
    BarcodeAppendSymbol(barcode, 106);
    barcode->code = BARCODE_CODE_STOP;
}

// Initialize a barcode object, using the specified bitmap buffer and its size (in bytes)
void BarcodeInit(barcode_t *barcode, uint8_t *buffer, size_t bufferSize)
{
    memset(barcode, 0, sizeof(barcode_t));
    barcode->buffer = buffer;
    barcode->bufferSize = bufferSize;
    barcode->code = BARCODE_CODE_NONE;
    barcode->numSymbols = 0;
    barcode->error = false;
}

// Append the specified string to the barcode
void BarcodeAppend(barcode_t *barcode, const char *text, barcode_code_t fixedCode)
{
    for (const char *p = text; *p != '\0'; p++)
    {
        char c0 = p[0];
        char c1 = p[1];
        char c2 = c1 != 0 ? p[2] : 0;
        char c3 = c2 != 0 ? p[3] : 0;
        
        barcode_code_t requiredCode = BARCODE_CODE_NONE;
        if (c0 < 0) { barcode->error = true; continue; }
        if (c0 >= 0 && c0 < 32) requiredCode = BARCODE_CODE_A;
        if (c0 >= '\'' && c0 <= 0x7f) requiredCode = BARCODE_CODE_B;

        if (fixedCode != BARCODE_CODE_NONE) requiredCode = fixedCode;

        // Code C if there are two numerical digits, but not if we're already in another code and there isn't a third and fourth.
        if (c0 != 0 && c1 != 0 && isdigit(c0) && isdigit(c1) && !((!isdigit(c2) || !isdigit(c3)) && (barcode->code == BARCODE_CODE_A || barcode->code == BARCODE_CODE_B)) && fixedCode == BARCODE_CODE_NONE)
        {
            BarcodeChangeCode(barcode, BARCODE_CODE_C);
            BarcodeAppendSymbol(barcode, (barcode_symbol_t)( (c0 - '0') * 10 + (c1 - '0') ));  // Code C double digits
            p++;        // consume two numbers
        }
        else
        {
            if (requiredCode != BARCODE_CODE_NONE)
            {
                BarcodeChangeCode(barcode, requiredCode);
            }
            else
            {
                BarcodeChangeCode(barcode, BARCODE_CODE_B);
            }

            if (c0 < 32)
            {
                BarcodeAppendSymbol(barcode, (barcode_symbol_t)(c0 + 64));  // Code A
            }
            else
            {
                BarcodeAppendSymbol(barcode, (barcode_symbol_t)(c0 - 32));  // Code B
            }
        }
    }
}


// Writes the barcode as a bitmap (0=black, 1=white) using the specified buffer, returns the length in bars/bits. Optionally adds a 10-unit quiet zone either side.
size_t Barcode(uint8_t *buffer, size_t bufferSize, int quietZone, const char *text, barcode_code_t fixedCode)
{
    barcode_t barcode;
    BarcodeInit(&barcode, buffer, bufferSize);
    BarcodeWriteBits(&barcode, 0xffff, quietZone);
    BarcodeAppend(&barcode, text, fixedCode);
    BarcodeStop(&barcode);
    BarcodeWriteBits(&barcode, 0xffff, quietZone);
    return barcode.offset;
}
