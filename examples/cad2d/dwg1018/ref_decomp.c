/* Minimal R2004 decompressor, transcribed from LibreDWG decode.c
 * decompress_R2004_section (1215-1345), read_literal_length (1160),
 * read_compressed_bytes (1180), two_byte_offset (1201), copy_bytes (1144).
 * Reads pm_body.bin, writes pm_ref.bin, prints first divergence trace.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char *src;
static size_t src_byte, src_size;
static unsigned char *dec;
static size_t dec_byte, dec_size;

static unsigned char bit_read_RC(void) {
    if (src_byte >= src_size) { fprintf(stderr, "overrun src %zu/%zu\n", src_byte, src_size); exit(2); }
    return src[src_byte++];
}

static unsigned int read_literal_length(unsigned char opcode) {
    unsigned int low = opcode & 0x0F;
    if (low == 0) {
        unsigned char last = 0;
        while ((last = bit_read_RC()) == 0 && src_byte < src_size)
            low += 0xFF;
        low += 0xF + last;
    }
    return low + 3;
}

static int read_compressed_bytes(unsigned char opcode, unsigned bits) {
    unsigned int cb = opcode & bits;
    if (cb == 0) {
        unsigned char last = 0;
        while ((last = bit_read_RC()) == 0 && src_byte < src_size)
            cb += 0xFF;
        cb += last + bits;
    }
    return (int)cb + 2;
}

static unsigned char two_byte_offset(int plus, int *offset) {
    unsigned char first = bit_read_RC();
    unsigned char second = bit_read_RC();
    *offset |= (first >> 2);
    *offset |= second << 6;
    *offset += plus;
    return first;
}

static unsigned char copy_bytes(unsigned int lit_length) {
    for (unsigned int i = 0; i < lit_length; ++i) {
        unsigned char b = bit_read_RC();
        dec[dec_byte++] = b;
    }
    return bit_read_RC();
}

static int decompress(void) {
    unsigned int lit_length;
    int comp_offset, comp_bytes;
    size_t pos, end;
    unsigned char opcode1 = 0, opcode2;

    opcode1 = bit_read_RC();
    if ((opcode1 & 0xF0) == 0) {
        lit_length = read_literal_length(opcode1);
        if (lit_length > dec_size - dec_byte) { fprintf(stderr, "lit too big %u\n", lit_length); return 1; }
        opcode1 = copy_bytes(lit_length);
        fprintf(stderr, "start lit=%u dec=%zu\n", lit_length, dec_byte);
    } else {
        fprintf(stderr, "first not literal 0x%02X\n", opcode1); return 1;
    }

    while (src_byte < src_size && dec_byte < dec_size && opcode1 != 0x11) {
        comp_bytes = 0; comp_offset = 0;
        if (opcode1 < 0x10 || opcode1 >= 0x40) {
            comp_bytes = (opcode1 >> 4) - 1;
            opcode2 = bit_read_RC();
            comp_offset = (((opcode1 >> 2) & 3) | (opcode2 << 2)) + 1;
        } else if (opcode1 < 0x20) {
            comp_bytes = read_compressed_bytes(opcode1, 7);
            comp_offset = (opcode1 & 8) << 11;
            opcode1 = two_byte_offset(0x4000, &comp_offset);
        } else if (opcode1 >= 0x20) {
            comp_bytes = read_compressed_bytes(opcode1, 0x1F);
            opcode1 = two_byte_offset(1, &comp_offset);
        } else if (opcode1 == 0x11) {
            break;
        } else {
            fprintf(stderr, "invalid opcode 0x%02X @src%zu\n", opcode1, src_byte); return 1;
        }
        fprintf(stderr, "op=0x%02X src=%zu dec=%zu cb=%d off=%d lit=%u\n",
                opcode1, src_byte, dec_byte, comp_bytes, comp_offset, opcode1 & 3);
        pos = dec_byte;
        end = pos + comp_bytes;
        if ((long)pos < comp_offset || comp_offset > (int)dec_size) {
            fprintf(stderr, "match invalid cb=%d off=%d dec=%zu src=%zu op=0x%02X\n",
                    comp_bytes, comp_offset, dec_byte, src_byte, opcode1);
            return 1;
        }
        for (; pos < end; pos++)
            dec[pos] = dec[pos - comp_offset];
        dec_byte = end;

        lit_length = opcode1 & 3;
        if (lit_length == 0) {
            opcode1 = bit_read_RC();
            if ((opcode1 & 0xF0) == 0)
                lit_length = read_literal_length(opcode1);
        }
        if (lit_length && end + lit_length <= dec_size) {
            opcode1 = copy_bytes(lit_length);
        } else if (lit_length) {
            break;
        }
    }
    fprintf(stderr, "done dec=%zu src=%zu\n", dec_byte, src_byte);
    return 0;
}

int main(int argc, char **argv) {
    FILE *f = fopen(argv[1], "rb");
    fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
    src = malloc(n); fread(src, 1, n, f); fclose(f); src_size = n;
    dec_size = argc > 3 ? (size_t)atol(argv[3]) : 6192;
    dec = calloc(dec_size + 1024, 1);
    int rc = decompress();
    FILE *o = fopen(argv[2], "wb"); fwrite(dec, 1, dec_byte, o); fclose(o);
    fprintf(stderr, "rc=%d out=%zu\n", rc, dec_byte);
    return rc;
}
