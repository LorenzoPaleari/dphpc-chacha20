#include <iostream>
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
        uint8_t *encrypt(uint8_t *input, int len);
        uint8_t *decrypt(uint8_t *input, int len);
    private:
        void _encrypt_single_block(uint8_t *input, uint8_t *result, uint32_t counter);
};