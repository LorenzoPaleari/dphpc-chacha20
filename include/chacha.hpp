#include <iostream>
#include <immintrin.h>
using namespace std;

uint32_t rotation_l32(uint32_t x, int n);
void quarter_round(uint32_t *a, uint32_t *b, uint32_t *c, uint32_t *d);
uint32_t uint8_to_uint32(const uint8_t *x);
void uint32_to_uint8(uint32_t x, uint8_t *y);

class ChaCha20 {
    public:
        uint32_t state[16];
        ChaCha20(const uint8_t key[32], const uint8_t nonce[12]);
        void setCounter(uint32_t counter);
        void block_quarter_round(uint8_t result[64], uint32_t counter);
        uint8_t *encrypt(uint8_t *input, long len);
        uint8_t *decrypt(uint8_t *input, long len);
        void encrypt_single_block(uint8_t *input, uint32_t counter);

        uint8_t *encryptOpt(int opt_num, uint8_t *input, long len);
        uint8_t *decryptOpt(int opt_num, uint8_t *input, long len);

        /* OPT 1 */
        void block_quarter_round_opt1(uint8_t result[64], uint32_t counter, uint32_t temp_c[16]);
        uint8_t *encrypt_opt1(uint8_t *input, long len);
        void quarter_round_opt1(uint32_t *a, uint32_t *b, uint32_t *c, uint32_t *d);

        uint8_t *encrypt_opt2(uint8_t *input, long len);
        void block_quarter_round_vect(uint8_t result[512], uint32_t counter, __m256i temp_c[16], __m256i adder);
        void quarter_round_vect(__m256i *a, __m256i *b, __m256i *c, __m256i *d);
};