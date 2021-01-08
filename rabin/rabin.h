#ifndef _RABIN_H
#define _RABIN_H

#include <stdint.h>

#define LBS 1   //  Logic Block Size.
#define LBS2 (LBS*LBS)

#define POLYNOMIAL 0x3DA3358B4DC173LL
#define POLYNOMIAL_DEGREE 53
// #define WINSIZE 64
#define WINSIZE (LBS2<<7)
#define AVERAGE_BITS 20

struct rabin_t {
    uint8_t window[WINSIZE];
    unsigned int wpos;
    unsigned int count;
    unsigned int pos;
    unsigned int start;
    uint64_t digest;
};

struct chunk_t {
    unsigned int start;
    unsigned int length;
    uint64_t cut_fingerprint;
};

extern struct chunk_t last_chunk;

struct rabin_t *rabin_init(void);
void rabin_reset(struct rabin_t *h);
void rabin_slide(struct rabin_t *h, uint8_t b);
void rabin_append(struct rabin_t *h, uint8_t b);
void rabin_slide_a_block(struct rabin_t *h, uint8_t *block_p);

#endif
