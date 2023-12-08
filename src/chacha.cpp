#include "../include/chacha.hpp"

#ifdef Darwin
#include "/usr/local/opt/libomp/include/omp.h"
#else
#include <omp.h>
#endif

using namespace std;

typedef uint8_t* (ChaCha20::*method_function)(uint8_t *input, long len);

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

void ChaCha20::block_quarter_round(uint8_t result[64], uint32_t counter){
    uint32_t temp[16];

    //MAKE A COPY OF THE STATE, SO WE CAN USE IT LATER
    for (int i = 0; i < 16; i++){
        temp[i] = this->state[i];
    }

    temp[12] = counter;

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
    temp[12] += counter;

    //COPY THE SCRUMBLED STATE TO THE RESULT POINTER AS LITTLE ENDIAN
    for (int i = 0; i < 16; i++){
        uint32_to_uint8(temp[i], result + (i * 4));
    }
}

uint8_t  *ChaCha20::encrypt(uint8_t *input, long len){

    long len_padded = (len + 63) & ~63;
    uint8_t *result = (uint8_t*) malloc(sizeof(uint8_t) * len_padded);
    long num_blocks = len/64;
    long over = len%64;

#pragma omp parallel for
    for (long i = 0; i < num_blocks; i++){
        this->encrypt_single_block(input + i*64, result + i*64, i+1);
        // this->block_quarter_round(result + i*64, i+1);
        // for (int j = 0; j < 64; j++)
        //     result[i*64 + j] ^= input[i*64 + j];
    }

    if (over != 0){
        this->block_quarter_round(result + num_blocks*64, num_blocks+1);
        for (int j = 0; j < over; j++)
            result[num_blocks*64 + j] ^= input[num_blocks*64 + j];
    }

    return result;
}

void ChaCha20::encrypt_single_block(uint8_t *input, uint8_t *result, uint32_t counter){
    
    this->block_quarter_round(result, counter);
    for (int j = 0; j < 64; j++)
        *(result + j) ^= *(input + j);

}

uint8_t  *ChaCha20::decrypt(uint8_t *input, long len){
    return this->encrypt(input, len);
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




/************
*    OPT1   *
*************/

uint8_t  *ChaCha20::encrypt_opt1(uint8_t *input, long len){

    long len_padded = (len + 63) & ~63;
    uint8_t *result = (uint8_t*) malloc(sizeof(uint8_t) * len_padded);
    long num_blocks = len/64;
    long over = len%64;

#pragma omp parallel for
    for (long i = 0; i < num_blocks; i++){
        this->block_quarter_round_opt1(result + i*64, i+1);
        for (int j = 0; j < 64; j++)
            result[i*64 + j] ^= input[i*64 + j];
    }

    if (over != 0){
        this->block_quarter_round_opt1(result + num_blocks*64, num_blocks+1);
        for (int j = 0; j < over; j++)
            result[num_blocks*64 + j] ^= input[num_blocks*64 + j];
    }

    return result;
}

#define quarter_round_opt1(x, a, b, c, d) \
        x[a] += x[b]; x[d] ^= x[a]; x[d] = (x[d] << 16) | (x[d] >> (16)); \
        x[c] += x[d]; x[b] ^= x[c]; x[b] = (x[b] << 12) | (x[b] >> (20)); \
        x[a] += x[b]; x[d] ^= x[a]; x[d] = (x[d] << 8) | (x[d] >> (24)); \
        x[c] += x[d]; x[b] ^= x[c]; x[b] = (x[b] << 7) | (x[b] >> (25)); \

#define uint32_to_uint8_opt1(x, y, n) \
        y[n] = (x >> 0) & 0xff; \
        y[n+1] = (x >> 8) & 0xff; \
        y[n+2] = (x >> 16) & 0xff; \
        y[n+3] = (x >> 24) & 0xff; \

void ChaCha20::block_quarter_round_opt1(uint8_t result[64], uint32_t counter){
    uint32_t temp[16];

    //MAKE A COPY OF THE STATE, SO WE CAN USE IT LATER
    for (int i = 0; i < 16; i++){
        temp[i] = this->state[i];
    }

    temp[12] = counter;

    for (int i = 0; i < 10; i++){
        quarter_round_opt1(temp, 0, 4, 8, 12);
        quarter_round_opt1(temp, 1, 5, 9, 13);
        quarter_round_opt1(temp, 2, 6, 10, 14);
        quarter_round_opt1(temp, 3, 7, 11, 15);
        quarter_round_opt1(temp, 0, 5, 10, 15);
        quarter_round_opt1(temp, 1, 6, 11, 12);
        quarter_round_opt1(temp, 2, 7, 8, 13);
        quarter_round_opt1(temp, 3, 4, 9, 14);
    }

    //ADD THE STATE TO THE SCRUMBLED STATE
    for (int i = 0; i < 16; i++){
        temp[i] += this->state[i];
    }
    temp[12] += counter;

    //COPY THE SCRUMBLED STATE TO THE RESULT POINTER AS LITTLE ENDIAN
    for (int i = 0; i < 16; i++){
        uint32_to_uint8_opt1(temp[i], result, i*4);
    }
}

uint8_t *ChaCha20::encryptOpt(int opt_num, uint8_t *input, long len){
    method_function method_pointer[2] = {&ChaCha20::encrypt, &ChaCha20::encrypt_opt1};
    method_function func = method_pointer[opt_num];
    return (this->*func)(input,len);
}

uint8_t *ChaCha20::decryptOpt(int opt_num, uint8_t *input, long len){
    #define NDEC 1
    method_function method_pointer[NDEC] = {&ChaCha20::decrypt};

    method_function func;
    if(opt_num > NDEC){
        func = method_pointer[0];
    } else {
        func = method_pointer[opt_num];
    }

    return (this->*func)(input,len);
}