#include "../include/chacha.hpp"

using namespace std;

ChaCha20::ChaCha20(const uint8_t key[32], const uint8_t nonce[12]){
    this->state[0] = 0x61707865;
    this->state[1] = 0x3320646e;
    this->state[2] = 0x79622d32;
    this->state[3] = 0x6b206574;
    this->state[4] = uint8_to_uint32(key);
    this->state[5] = uint8_to_uint32(key + 4);
    this->state[6] = uint8_to_uint32(key + 8);
    this->state[7] = uint8_to_uint32(key + 12);
    this->state[8] = uint8_to_uint32(key + 16);
    this->state[9] = uint8_to_uint32(key + 20);
    this->state[10] = uint8_to_uint32(key + 24);
    this->state[11] = uint8_to_uint32(key + 28);
    this->state[12] = 0; // counter start from 0
    this->state[13] = uint8_to_uint32(nonce);
    this->state[14] = uint8_to_uint32(nonce + 4);
    this->state[15] = uint8_to_uint32(nonce + 8);
}

void ChaCha20::setCounter(uint32_t counter){
    this->state[12] = counter;
}

void ChaCha20::block_quarter_round(uint8_t result[64]){
    uint32_t temp[16];

    //MAKE A COPY OF THE STATE, SO WE CAN USE IT LATER
    for (int i = 0; i < 16; i++){
        temp[i] = this->state[i];
    }

    for (int i = 0; i < 10; i++){
        quarter_round(&temp[0], &temp[4], &temp[8], &temp[12]);
        quarter_round(&temp[1], &temp[5], &temp[9], &temp[13]);
        quarter_round(&temp[2], &temp[6], &temp[10], &temp[14]);
        quarter_round(&temp[3], &temp[7], &temp[11], &temp[15]);
        quarter_round(&temp[0], &temp[5], &temp[10], &temp[15]);
        quarter_round(&temp[1], &temp[6], &temp[11], &temp[12]);
        quarter_round(&temp[2], &temp[7], &temp[8], &temp[13]);
        quarter_round(&temp[3], &temp[4], &temp[9], &temp[14]);
    }

    //ADD THE STATE TO THE SCRUMBLED STATE
    for (int i = 0; i < 16; i++){
        temp[i] += this->state[i];
    }

    //COPY THE SCRUMBLED STATE TO THE RESULT POINTER AS LITTLE ENDIAN
    for (int i = 0; i < 16; i++){
        uint32_to_uint8(temp[i], result + (i * 4));
    }
}

uint32_t rotation_l32(uint32_t x, int n){
    return (x << n) | (x >> (32 - n));
}

void quarter_round(uint32_t *a, uint32_t *b, uint32_t *c, uint32_t *d){
    *a += *b; *d ^= *a; *d = rotation_l32(*d, 16);
    *c += *d; *b ^= *c; *b = rotation_l32(*b, 12);
    *a += *b; *d ^= *a; *d = rotation_l32(*d, 8);
    *c += *d; *b ^= *c; *b = rotation_l32(*b, 7);
}

uint32_t uint8_to_uint32(const uint8_t *x){
    return uint32_t(x[0] << 0) |
            uint32_t(x[1] << 8) |
            uint32_t(x[2] << 16) |
            uint32_t(x[3] << 24);
}


void uint32_to_uint8(uint32_t x, uint8_t *y){
    y[0] = (x >> 0) & 0xff;
    y[1] = (x >> 8) & 0xff;
    y[2] = (x >> 16) & 0xff;
    y[3] = (x >> 24) & 0xff;
}