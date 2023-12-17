#include "../include/chacha.hpp"
#include <immintrin.h>

#ifdef Darwin
#include "/usr/local/opt/libomp/include/omp.h"
#else
#include <omp.h>
#endif

using namespace std;

typedef void (ChaCha20::*method_function)(uint8_t *input, long len);

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

void ChaCha20::encrypt(uint8_t *input, long len){

    long num_blocks = len/64;
    long over = len%64;

#pragma omp parallel for
    for (long i = 0; i < num_blocks; i++){
        this->encrypt_single_block(input + i*64, i + 1);
    }

    if (over != 0){
        uint8_t result[64];
        this->block_quarter_round(result, num_blocks + 1);
        for (int j = 0; j < over; j++)
            input[num_blocks*64 + j] ^= result[j];
    }
}

void ChaCha20::encrypt_single_block(uint8_t *input, uint32_t counter){

    uint8_t result[64];
    this->block_quarter_round(result, counter);
    for (int j = 0; j < 64; j++)
        *(input + j) ^= *(result + j);

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




/***********************************
*    OPT1 - General improvements   *
************************************/

__attribute__((always_inline)) void ChaCha20::quarter_round_opt1(uint32_t *a, uint32_t *b, uint32_t *c, uint32_t *d){
    *a += *b; *d ^= *a; *d = (*d << 16) | (*d >> 16);
    *c += *d; *b ^= *c; *b = (*b << 12) | (*b >> 20);
    *a += *b; *d ^= *a; *d = (*d << 8) | (*d >> 24);
    *c += *d; *b ^= *c; *b = (*b << 7) | (*b >> 25);
}

__attribute__((always_inline)) void ChaCha20::block_quarter_round_opt1(uint8_t result[64], uint32_t counter, uint32_t temp_c[16]){
    uint32_t temp[16];

    //MAKE A COPY OF TEMP_C
    std::copy(temp_c, temp_c + 16, temp);
    temp[12] = counter;

    quarter_round_opt1(&temp[0], &temp[4], &temp[8], &temp[12]);
    quarter_round_opt1(&temp[0], &temp[5], &temp[10], &temp[15]);
    quarter_round_opt1(&temp[1], &temp[6], &temp[11], &temp[12]);
    quarter_round_opt1(&temp[2], &temp[7], &temp[8], &temp[13]);
    quarter_round_opt1(&temp[3], &temp[4], &temp[9], &temp[14]);

    for (int i = 0; i < 9; i++){
        quarter_round_opt1(&temp[0], &temp[4], &temp[8], &temp[12]);
        quarter_round_opt1(&temp[1], &temp[5], &temp[9], &temp[13]);
        quarter_round_opt1(&temp[2], &temp[6], &temp[10], &temp[14]);
        quarter_round_opt1(&temp[3], &temp[7], &temp[11], &temp[15]);
        quarter_round_opt1(&temp[0], &temp[5], &temp[10], &temp[15]);
        quarter_round_opt1(&temp[1], &temp[6], &temp[11], &temp[12]);
        quarter_round_opt1(&temp[2], &temp[7], &temp[8], &temp[13]);
        quarter_round_opt1(&temp[3], &temp[4], &temp[9], &temp[14]);
    }

    //ADD THE STATE TO THE SCRUMBLED STATE
    for (int i = 0; i < 16; i++){
        temp[i] += this->state[i];
    }
    temp[12] += counter;

    //COPY THE SCRUMBLED STATE TO THE RESULT POINTER AS LITTLE ENDIAN
    for (int i = 0; i < 16; i++){
        *(result + (i<<2)) = (temp[i] >> 0) & 0xff;
        *(result + (i<<2) + 1) = (temp[i] >> 8) & 0xff;
        *(result + (i<<2) + 2) = (temp[i] >> 16) & 0xff;
        *(result + (i<<2) + 3) = (temp[i] >> 24) & 0xff;
    }
}

void ChaCha20::encrypt_opt1(uint8_t *input, long len){

    long num_blocks = len/64;
    long over = len%64;

    uint32_t temp[16];
    uint8_t result[64];

    //MAKE A COPY OF THE STATE, SO WE CAN USE IT LATER - also equal for every block
    std::copy(this->state, this->state + 16, temp);

    //EQUAL FOR EVERY BLOCK
    quarter_round_opt1(&temp[1], &temp[5], &temp[9], &temp[13]);
    quarter_round_opt1(&temp[2], &temp[6], &temp[10], &temp[14]);
    quarter_round_opt1(&temp[3], &temp[7], &temp[11], &temp[15]);

#pragma omp parallel for
    for (long i = 0; i < num_blocks; i++){
        this->block_quarter_round_opt1(result, i + 1, temp);
        for (int j = 0; j < 64; j++)
            *(input + (i<<6) + j) ^= *(result + j);
    }

    if (over != 0){
        this->block_quarter_round_opt1(result, num_blocks + 1, temp);
        for (int j = 0; j < over; j++)
            *(input + (num_blocks<<6) + j) ^= *(result + j);
    }
}




/*******************************************
*    OPT2 - General improvements - Fixed   *
********************************************/

__attribute__((always_inline)) void ChaCha20::quarter_round_opt2(uint32_t *a, uint32_t *b, uint32_t *c, uint32_t *d){
    *a += *b; *d ^= *a; *d = (*d << 16) | (*d >> 16);
    *c += *d; *b ^= *c; *b = (*b << 12) | (*b >> 20);
    *a += *b; *d ^= *a; *d = (*d << 8) | (*d >> 24);
    *c += *d; *b ^= *c; *b = (*b << 7) | (*b >> 25);
}

__attribute__((always_inline)) void ChaCha20::block_quarter_round_opt2(uint8_t result[64], uint32_t counter, uint32_t temp_c[16]){
    uint32_t temp[16];

    //MAKE A COPY OF TEMP_C
    std::copy(temp_c, temp_c + 16, temp);
    temp[12] = counter;

    quarter_round_opt2(&temp[0], &temp[4], &temp[8], &temp[12]);
    quarter_round_opt2(&temp[0], &temp[5], &temp[10], &temp[15]);
    quarter_round_opt2(&temp[1], &temp[6], &temp[11], &temp[12]);
    quarter_round_opt2(&temp[2], &temp[7], &temp[8], &temp[13]);
    quarter_round_opt2(&temp[3], &temp[4], &temp[9], &temp[14]);

    for (int i = 0; i < 9; i++){
        quarter_round_opt2(&temp[0], &temp[4], &temp[8], &temp[12]);
        quarter_round_opt2(&temp[1], &temp[5], &temp[9], &temp[13]);
        quarter_round_opt2(&temp[2], &temp[6], &temp[10], &temp[14]);
        quarter_round_opt2(&temp[3], &temp[7], &temp[11], &temp[15]);
        quarter_round_opt2(&temp[0], &temp[5], &temp[10], &temp[15]);
        quarter_round_opt2(&temp[1], &temp[6], &temp[11], &temp[12]);
        quarter_round_opt2(&temp[2], &temp[7], &temp[8], &temp[13]);
        quarter_round_opt2(&temp[3], &temp[4], &temp[9], &temp[14]);
    }

    //ADD THE STATE TO THE SCRUMBLED STATE
    for (int i = 0; i < 16; i++){
        temp[i] += this->state[i];
    }
    temp[12] += counter;

    //COPY THE SCRUMBLED STATE TO THE RESULT POINTER AS LITTLE ENDIAN
    for (int i = 0; i < 16; i++){
        *(result + (i<<2)) = (temp[i] >> 0) & 0xff;
        *(result + (i<<2) + 1) = (temp[i] >> 8) & 0xff;
        *(result + (i<<2) + 2) = (temp[i] >> 16) & 0xff;
        *(result + (i<<2) + 3) = (temp[i] >> 24) & 0xff;
    }
}

void ChaCha20::encrypt_opt2(uint8_t *input, long len){

    long num_blocks = len/64;
    long over = len%64;

    uint32_t temp[16];

    //MAKE A COPY OF THE STATE, SO WE CAN USE IT LATER - also equal for every block
    std::copy(this->state, this->state + 16, temp);

    //EQUAL FOR EVERY BLOCK
    quarter_round_opt2(&temp[1], &temp[5], &temp[9], &temp[13]);
    quarter_round_opt2(&temp[2], &temp[6], &temp[10], &temp[14]);
    quarter_round_opt2(&temp[3], &temp[7], &temp[11], &temp[15]);

#pragma omp parallel for shared(input, temp)
    for (long i = 0; i < num_blocks; i++){
        uint8_t result[64];
        this->block_quarter_round_opt2(result, i + 1, temp);
        for (int j = 0; j < 64; j++)
            *(input + (i<<6) + j) ^= *(result + j);
    }

    if (over != 0){
        uint8_t result[64];
        this->block_quarter_round_opt2(result, num_blocks + 1, temp);
        for (int j = 0; j < over; j++)
            *(input + (num_blocks<<6) + j) ^= *(result + j);
    }
}




/****************************
*    OPT3 - Vectorization   *
*****************************/

__attribute__((always_inline)) void ChaCha20::quarter_round_vect(__m256i *a, __m256i *b, __m256i *c, __m256i *d, __m256i rotate16, __m256i rotate8){
    *a = _mm256_add_epi32(*a, *b); *d = _mm256_shuffle_epi8(_mm256_xor_si256(*d, *a), rotate16);
    *c = _mm256_add_epi32(*c, *d); *b = _mm256_xor_si256(*b, *c); *b = _mm256_or_si256(_mm256_slli_epi32(*b, 12), _mm256_srli_epi32(*b, 20));
    *a = _mm256_add_epi32(*a, *b); *d = _mm256_shuffle_epi8(_mm256_xor_si256(*d, *a), rotate8);
    *c = _mm256_add_epi32(*c, *d); *b = _mm256_xor_si256(*b, *c); *b = _mm256_or_si256(_mm256_slli_epi32(*b, 7), _mm256_srli_epi32(*b, 25));
}

__attribute__((always_inline)) void ChaCha20::block_quarter_round_opt3_big_endian(uint8_t result[512], uint32_t counter, __m256i temp_c[16], __m256i adder, __m256i initial_state[16], __m256i rotate16, __m256i rotate8){

    //MAKE A COPY OF THE STATE
    __m256i temp[16];
    std::copy(temp_c, temp_c + 12, temp);
    std::copy(temp_c + 13, temp_c + 16, temp + 13);

    __m256i counter_vect = _mm256_add_epi32(_mm256_set1_epi32(counter), adder);
    temp[12] = counter_vect;

    quarter_round_vect(&temp[0], &temp[4], &temp[8], &temp[12], rotate16, rotate8);
    quarter_round_vect(&temp[0], &temp[5], &temp[10], &temp[15], rotate16, rotate8);
    quarter_round_vect(&temp[1], &temp[6], &temp[11], &temp[12], rotate16, rotate8);
    quarter_round_vect(&temp[2], &temp[7], &temp[8], &temp[13], rotate16, rotate8);
    quarter_round_vect(&temp[3], &temp[4], &temp[9], &temp[14], rotate16, rotate8);

    for (int i = 0; i < 9; i++){
        quarter_round_vect(&temp[0], &temp[4], &temp[8], &temp[12], rotate16, rotate8);
        quarter_round_vect(&temp[1], &temp[5], &temp[9], &temp[13], rotate16, rotate8);
        quarter_round_vect(&temp[2], &temp[6], &temp[10], &temp[14], rotate16, rotate8);
        quarter_round_vect(&temp[3], &temp[7], &temp[11], &temp[15], rotate16, rotate8);
        quarter_round_vect(&temp[0], &temp[5], &temp[10], &temp[15], rotate16, rotate8);
        quarter_round_vect(&temp[1], &temp[6], &temp[11], &temp[12], rotate16, rotate8);
        quarter_round_vect(&temp[2], &temp[7], &temp[8], &temp[13], rotate16, rotate8);
        quarter_round_vect(&temp[3], &temp[4], &temp[9], &temp[14], rotate16, rotate8);
    }

    //ADD THE STATE TO THE SCRUMBLED STATE
    for (int i = 0; i < 12; i++){
        temp[i] = _mm256_add_epi32(temp[i], initial_state[i]);
    }
    temp[12] = _mm256_add_epi32(temp[12], counter_vect);
    for (int i = 13; i < 16; i++){
        temp[i] = _mm256_add_epi32(temp[i], initial_state[i]);
    }

    //REORDER VECTOR
    __m256i temp_2[16];
    for (int i = 0; i < 16; i+=2){
        temp_2[i] = _mm256_unpacklo_epi32(temp[i], temp[i+1]);
        temp_2[i+1] = _mm256_unpackhi_epi32(temp[i], temp[i+1]);
    }
    for (int i = 0; i < 16; i+=4){
        temp[i] = _mm256_unpacklo_epi64(temp_2[i], temp_2[i+2]);
        temp[i+1] = _mm256_unpackhi_epi64(temp_2[i], temp_2[i+2]);
        temp[i+2] = _mm256_unpacklo_epi64(temp_2[i+1], temp_2[i+3]);
        temp[i+3] = _mm256_unpackhi_epi64(temp_2[i+1], temp_2[i+3]);
    }

    temp_2[0] = _mm256_permute2x128_si256(temp[0], temp[4], 0x21);
    temp[0] = _mm256_blend_epi32(temp[0], temp_2[0], 0xf0);
    temp[4] = _mm256_blend_epi32(temp_2[0], temp[4], 0xf0);
    temp_2[1] = _mm256_permute2x128_si256(temp[1], temp[5], 0x21);
    temp[1] = _mm256_blend_epi32(temp[1], temp_2[1], 0xf0);
    temp[5] = _mm256_blend_epi32(temp_2[1], temp[5], 0xf0);
    temp_2[2] = _mm256_permute2x128_si256(temp[2], temp[6], 0x21);
    temp[2] = _mm256_blend_epi32(temp[2], temp_2[2], 0xf0);
    temp[6] = _mm256_blend_epi32(temp_2[2], temp[6], 0xf0);
    temp_2[3] = _mm256_permute2x128_si256(temp[3], temp[7], 0x21);
    temp[3] = _mm256_blend_epi32(temp[3], temp_2[3], 0xf0);
    temp[7] = _mm256_blend_epi32(temp_2[3], temp[7], 0xf0);
    temp_2[4] = _mm256_permute2x128_si256(temp[8], temp[12], 0x21);
    temp[8] = _mm256_blend_epi32(temp[8], temp_2[4], 0xf0);
    temp[12] = _mm256_blend_epi32(temp_2[4], temp[12], 0xf0);
    temp_2[5] = _mm256_permute2x128_si256(temp[9], temp[13], 0x21);
    temp[9] = _mm256_blend_epi32(temp[9], temp_2[5], 0xf0);
    temp[13] = _mm256_blend_epi32(temp_2[5], temp[13], 0xf0);
    temp_2[6] = _mm256_permute2x128_si256(temp[10], temp[14], 0x21);
    temp[10] = _mm256_blend_epi32(temp[10], temp_2[6], 0xf0);
    temp[14] = _mm256_blend_epi32(temp_2[6], temp[14], 0xf0);
    temp_2[7] = _mm256_permute2x128_si256(temp[11], temp[15], 0x21);
    temp[11] = _mm256_blend_epi32(temp[11], temp_2[7], 0xf0);
    temp[15] = _mm256_blend_epi32(temp_2[7], temp[15], 0xf0);

    uint32_t temp_3[128];
    for (int i = 0; i < 8; i++){
        _mm256_store_si256((__m256i *)(temp_3 + (i<<4)), temp[i]);
        _mm256_store_si256((__m256i *)(temp_3 + (i<<4)+ 8), temp[i + 8]);
    }

    for (int i = 0; i < 128; i++){
        *(result + (i<<2)) = (temp_3[i] >> 0) & 0xff;
        *(result + (i<<2) + 1) = (temp_3[i] >> 8) & 0xff;
        *(result + (i<<2) + 2) = (temp_3[i] >> 16) & 0xff;
        *(result + (i<<2) + 3) = (temp_3[i] >> 24) & 0xff;
    }
}

__attribute__((always_inline)) void ChaCha20::block_quarter_round_opt3_little_endian(uint8_t *input, uint32_t counter, __m256i temp0, __m256i temp1, __m256i temp2, __m256i temp3, __m256i temp4, __m256i temp5, __m256i temp6, __m256i temp7, __m256i temp8, __m256i temp9, __m256i temp10, __m256i temp11, __m256i temp12, __m256i temp13, __m256i temp14, __m256i temp15, __m256i adder, __m256i initial_state[16], __m256i rotate16, __m256i rotate8){

    __m256i counter_vect = _mm256_add_epi32(_mm256_set1_epi32(counter), adder);
    temp12 = counter_vect;

    quarter_round_vect(&temp0, &temp4, &temp8, &temp12, rotate16, rotate8);
    quarter_round_vect(&temp0, &temp5, &temp10, &temp15, rotate16, rotate8);
    quarter_round_vect(&temp1, &temp6, &temp11, &temp12, rotate16, rotate8);
    quarter_round_vect(&temp2, &temp7, &temp8, &temp13, rotate16, rotate8);
    quarter_round_vect(&temp3, &temp4, &temp9, &temp14, rotate16, rotate8);

    for (int i = 0; i < 9; i++){
        quarter_round_vect(&temp0, &temp4, &temp8, &temp12, rotate16, rotate8);
        quarter_round_vect(&temp1, &temp5, &temp9, &temp13, rotate16, rotate8);
        quarter_round_vect(&temp2, &temp6, &temp10, &temp14, rotate16, rotate8);
        quarter_round_vect(&temp3, &temp7, &temp11, &temp15, rotate16, rotate8);
        quarter_round_vect(&temp0, &temp5, &temp10, &temp15, rotate16, rotate8);
        quarter_round_vect(&temp1, &temp6, &temp11, &temp12, rotate16, rotate8);
        quarter_round_vect(&temp2, &temp7, &temp8, &temp13, rotate16, rotate8);
        quarter_round_vect(&temp3, &temp4, &temp9, &temp14, rotate16, rotate8);
    }

    //ADD THE STATE TO THE SCRUMBLED STATE
    temp0 = _mm256_add_epi32(temp0, initial_state[0]);
    temp1 = _mm256_add_epi32(temp1, initial_state[1]);
    temp2 = _mm256_add_epi32(temp2, initial_state[2]);
    temp3 = _mm256_add_epi32(temp3, initial_state[3]);
    temp4 = _mm256_add_epi32(temp4, initial_state[4]);
    temp5 = _mm256_add_epi32(temp5, initial_state[5]);
    temp6 = _mm256_add_epi32(temp6, initial_state[6]);
    temp7 = _mm256_add_epi32(temp7, initial_state[7]);
    temp8 = _mm256_add_epi32(temp8, initial_state[8]);
    temp9 = _mm256_add_epi32(temp9, initial_state[9]);
    temp10 = _mm256_add_epi32(temp10, initial_state[10]);
    temp11 = _mm256_add_epi32(temp11, initial_state[11]);
    temp12 = _mm256_add_epi32(temp12, counter_vect);
    temp13 = _mm256_add_epi32(temp13, initial_state[13]);
    temp14 = _mm256_add_epi32(temp14, initial_state[14]);
    temp15 = _mm256_add_epi32(temp15, initial_state[15]);

    //TRANSPOSE VECTORS
    __m256i temp_2[16];
    temp_2[0] = _mm256_unpacklo_epi32(temp0, temp1);
    temp_2[1] = _mm256_unpackhi_epi32(temp0, temp1);
    temp_2[2] = _mm256_unpacklo_epi32(temp2, temp3);
    temp_2[3] = _mm256_unpackhi_epi32(temp2, temp3);
    temp_2[4] = _mm256_unpacklo_epi32(temp4, temp5);
    temp_2[5] = _mm256_unpackhi_epi32(temp4, temp5);
    temp_2[6] = _mm256_unpacklo_epi32(temp6, temp7);
    temp_2[7] = _mm256_unpackhi_epi32(temp6, temp7);
    temp_2[8] = _mm256_unpacklo_epi32(temp8, temp9);
    temp_2[9] = _mm256_unpackhi_epi32(temp8, temp9);
    temp_2[10] = _mm256_unpacklo_epi32(temp10, temp11);
    temp_2[11] = _mm256_unpackhi_epi32(temp10, temp11);
    temp_2[12] = _mm256_unpacklo_epi32(temp12, temp13);
    temp_2[13] = _mm256_unpackhi_epi32(temp12, temp13);
    temp_2[14] = _mm256_unpacklo_epi32(temp14, temp15);
    temp_2[15] = _mm256_unpackhi_epi32(temp14, temp15);

    temp0 = _mm256_unpacklo_epi64(temp_2[0], temp_2[2]);
    temp1 = _mm256_unpackhi_epi64(temp_2[0], temp_2[2]);
    temp2 = _mm256_unpacklo_epi64(temp_2[1], temp_2[3]);
    temp3 = _mm256_unpackhi_epi64(temp_2[1], temp_2[3]);
    temp4 = _mm256_unpacklo_epi64(temp_2[4], temp_2[6]);
    temp5 = _mm256_unpackhi_epi64(temp_2[4], temp_2[6]);
    temp6 = _mm256_unpacklo_epi64(temp_2[5], temp_2[7]);
    temp7 = _mm256_unpackhi_epi64(temp_2[5], temp_2[7]);
    temp8 = _mm256_unpacklo_epi64(temp_2[8], temp_2[10]);
    temp9 = _mm256_unpackhi_epi64(temp_2[8], temp_2[10]);
    temp10 = _mm256_unpacklo_epi64(temp_2[9], temp_2[11]);
    temp11 = _mm256_unpackhi_epi64(temp_2[9], temp_2[11]);
    temp12 = _mm256_unpacklo_epi64(temp_2[12], temp_2[14]);
    temp13 = _mm256_unpackhi_epi64(temp_2[12], temp_2[14]);
    temp14 = _mm256_unpacklo_epi64(temp_2[13], temp_2[15]);
    temp15 = _mm256_unpackhi_epi64(temp_2[13], temp_2[15]);

    temp_2[0] = _mm256_permute2x128_si256(temp0, temp4, 0x21);
    temp0 = _mm256_blend_epi32(temp0, temp_2[0], 0xf0);
    temp4 = _mm256_blend_epi32(temp_2[0], temp4, 0xf0);
    temp_2[1] = _mm256_permute2x128_si256(temp1, temp5, 0x21);
    temp1 = _mm256_blend_epi32(temp1, temp_2[1], 0xf0);
    temp5 = _mm256_blend_epi32(temp_2[1], temp5, 0xf0);
    temp_2[2] = _mm256_permute2x128_si256(temp2, temp6, 0x21);
    temp2 = _mm256_blend_epi32(temp2, temp_2[2], 0xf0);
    temp6 = _mm256_blend_epi32(temp_2[2], temp6, 0xf0);
    temp_2[3] = _mm256_permute2x128_si256(temp3, temp7, 0x21);
    temp3 = _mm256_blend_epi32(temp3, temp_2[3], 0xf0);
    temp7 = _mm256_blend_epi32(temp_2[3], temp7, 0xf0);
    temp_2[4] = _mm256_permute2x128_si256(temp8, temp12, 0x21);
    temp8 = _mm256_blend_epi32(temp8, temp_2[4], 0xf0);
    temp12 = _mm256_blend_epi32(temp_2[4], temp12, 0xf0);
    temp_2[5] = _mm256_permute2x128_si256(temp9, temp13, 0x21);
    temp9 = _mm256_blend_epi32(temp9, temp_2[5], 0xf0);
    temp13 = _mm256_blend_epi32(temp_2[5], temp13, 0xf0);
    temp_2[6] = _mm256_permute2x128_si256(temp10, temp14, 0x21);
    temp10 = _mm256_blend_epi32(temp10, temp_2[6], 0xf0);
    temp14 = _mm256_blend_epi32(temp_2[6], temp14, 0xf0);
    temp_2[7] = _mm256_permute2x128_si256(temp11, temp15, 0x21);
    temp11 = _mm256_blend_epi32(temp11, temp_2[7], 0xf0);
    temp15 = _mm256_blend_epi32(temp_2[7], temp15, 0xf0);

    //XOR INPUT WITH THE SCRUMBLED STATE
    temp0 = _mm256_xor_si256(temp0, _mm256_load_si256((__m256i *)(input + 0)));
    temp8 = _mm256_xor_si256(temp8, _mm256_load_si256((__m256i *)(input + 32)));
    temp1 = _mm256_xor_si256(temp1, _mm256_load_si256((__m256i *)(input + 64)));
    temp9 = _mm256_xor_si256(temp9, _mm256_load_si256((__m256i *)(input + 96)));
    temp2 = _mm256_xor_si256(temp2, _mm256_load_si256((__m256i *)(input + 128)));
    temp10 = _mm256_xor_si256(temp10, _mm256_load_si256((__m256i *)(input + 160)));
    temp3 = _mm256_xor_si256(temp3, _mm256_load_si256((__m256i *)(input + 192)));
    temp11 = _mm256_xor_si256(temp11, _mm256_load_si256((__m256i *)(input + 224)));
    temp4 = _mm256_xor_si256(temp4, _mm256_load_si256((__m256i *)(input + 256)));
    temp12 = _mm256_xor_si256(temp12, _mm256_load_si256((__m256i *)(input + 288)));
    temp5 = _mm256_xor_si256(temp5, _mm256_load_si256((__m256i *)(input + 320)));
    temp13 = _mm256_xor_si256(temp13, _mm256_load_si256((__m256i *)(input + 352)));
    temp6 = _mm256_xor_si256(temp6, _mm256_load_si256((__m256i *)(input + 384)));
    temp14 = _mm256_xor_si256(temp14, _mm256_load_si256((__m256i *)(input + 416)));
    temp7 = _mm256_xor_si256(temp7, _mm256_load_si256((__m256i *)(input + 448)));
    temp15 = _mm256_xor_si256(temp15, _mm256_load_si256((__m256i *)(input + 480)));

    //STORE THE RESULT
    _mm256_store_si256((__m256i *)(input + 0), temp0);
    _mm256_store_si256((__m256i *)(input + 32), temp8);
    _mm256_store_si256((__m256i *)(input + 64), temp1);
    _mm256_store_si256((__m256i *)(input + 96), temp9);
    _mm256_store_si256((__m256i *)(input + 128), temp2);
    _mm256_store_si256((__m256i *)(input + 160), temp10);
    _mm256_store_si256((__m256i *)(input + 192), temp3);
    _mm256_store_si256((__m256i *)(input + 224), temp11);
    _mm256_store_si256((__m256i *)(input + 256), temp4);
    _mm256_store_si256((__m256i *)(input + 288), temp12);
    _mm256_store_si256((__m256i *)(input + 320), temp5);
    _mm256_store_si256((__m256i *)(input + 352), temp13);
    _mm256_store_si256((__m256i *)(input + 384), temp6);
    _mm256_store_si256((__m256i *)(input + 416), temp14);
    _mm256_store_si256((__m256i *)(input + 448), temp7);
    _mm256_store_si256((__m256i *)(input + 480), temp15);
}

void ChaCha20::encrypt_opt3(uint8_t *input, long len){

    long num_blocks_8 = len/512;  //8 blocks of 64 bytes
    long over_8 = len%512;

    // make some copy of the state that are going to be used by every block later
    __m256i temp[16];
    __m256i initial_state[16];
    __m256i adder = _mm256_set_epi32(7,6,5,4,3,2,1,0);
    __m256i rotate16 = _mm256_set_epi32(0x0d0c0f0e, 0x09080b0a, 0x05040706, 0x01000302, 0x0d0c0f0e, 0x09080b0a, 0x05040706, 0x01000302);
    __m256i rotate8 = _mm256_set_epi32(0x0e0d0c0f, 0x0a09080b, 0x06050407, 0x02010003, 0x0e0d0c0f, 0x0a09080b, 0x06050407, 0x02010003);

    for (int i = 0; i < 16; i++){
        temp[i] = _mm256_set1_epi32(this->state[i]);
        initial_state[i] = temp[i];
    }

    quarter_round_vect(&temp[1], &temp[5], &temp[9], &temp[13], rotate16, rotate8);
    quarter_round_vect(&temp[2], &temp[6], &temp[10], &temp[14], rotate16, rotate8);
    quarter_round_vect(&temp[3], &temp[7], &temp[11], &temp[15], rotate16, rotate8);

#if __BYTE_ORDER__ == __BIG_ENDIAN__
    #pragma omp parallel for shared(input, temp, initial_state, adder, rotate16, rotate8)
    for (long i = 0; i < num_blocks_8; i++){
        uint8_t result[512];
        this->block_quarter_round_opt3_big_endian(result, (i<<3) + 1, temp, adder, initial_state, rotate16, rotate8);
        for (int j = 0; j < 512; j++)
            *(input + (i<<9) + j) ^= *(result + j);
    }

    if (over_8 != 0){
        uint8_t result[512];
        this->block_quarter_round_opt3_big_endian(result, (num_blocks_8<<3)+1, temp, adder, initial_state, rotate16, rotate8);
        for (int j = 0; j < over_8; j++)
            *(input + (num_blocks_8<<9) + j) ^= *(result + j);
    }
#else
    #pragma omp parallel for shared(input, temp, initial_state, adder, rotate16, rotate8)
    for (long i = 0; i < num_blocks_8; i++){
        this->block_quarter_round_opt3_little_endian(input + (i<<9), (i<<3) + 1, temp[0], temp[1], temp[2], temp[3], temp[4], temp[5], temp[6], temp[7], temp[8], temp[9], temp[10], temp[11], temp[12], temp[13], temp[14], temp[15], adder, initial_state, rotate16, rotate8);
    }

    if (over_8 != 0){
        uint8_t result[512];
        this->block_quarter_round_opt3_big_endian(result, (num_blocks_8<<3)+1, temp, adder, initial_state, rotate16, rotate8);
        for (int j = 0; j < over_8; j++)
            *(input + (num_blocks_8<<9) + j) ^= *(result + j);
    }
#endif
}




/**************************************
*    OPT4 - Vectorization - dynamic   *
***************************************/

void ChaCha20::encrypt_opt4(uint8_t *input, long len){

    long num_blocks_8 = len/512;  //8 blocks of 64 bytes
    long over_8 = len%512;

    // make some copy of the state that are going to be used by every block later
    __m256i temp[16];
    __m256i initial_state[16];
    __m256i adder = _mm256_set_epi32(7,6,5,4,3,2,1,0);
    __m256i rotate16 = _mm256_set_epi32(0x0d0c0f0e, 0x09080b0a, 0x05040706, 0x01000302, 0x0d0c0f0e, 0x09080b0a, 0x05040706, 0x01000302);
    __m256i rotate8 = _mm256_set_epi32(0x0e0d0c0f, 0x0a09080b, 0x06050407, 0x02010003, 0x0e0d0c0f, 0x0a09080b, 0x06050407, 0x02010003);

    for (int i = 0; i < 16; i++){
        temp[i] = _mm256_set1_epi32(this->state[i]);
        initial_state[i] = _mm256_set1_epi32(this->state[i]);
    }

    quarter_round_vect(&temp[1], &temp[5], &temp[9], &temp[13], rotate16, rotate8);
    quarter_round_vect(&temp[2], &temp[6], &temp[10], &temp[14], rotate16, rotate8);
    quarter_round_vect(&temp[3], &temp[7], &temp[11], &temp[15], rotate16, rotate8);

#if __BYTE_ORDER__ == __BIG_ENDIAN__
    #pragma omp parallel for schedule(dynamic, 32768) shared(input, temp, initial_state, adder, rotate16, rotate8)
    for (long i = 0; i < num_blocks_8; i++){
        uint8_t result[512];
        this->block_quarter_round_opt3_big_endian(result, (i<<3) + 1, temp, adder, initial_state, rotate16, rotate8);
        for (int j = 0; j < 512; j++)
            *(input + (i<<9) + j) ^= *(result + j);
    }

    if (over_8 != 0){
        uint8_t result[512];
        this->block_quarter_round_opt3_big_endian(result, (num_blocks_8<<3)+1, temp, adder, initial_state, rotate16, rotate8);
        for (int j = 0; j < over_8; j++)
            *(input + (num_blocks_8<<9) + j) ^= *(result + j);
    }
#else
    #pragma omp parallel for schedule(dynamic, 32768) shared(input, temp, initial_state, adder, rotate16, rotate8)
    for (long i = 0; i < num_blocks_8; i++){
        this->block_quarter_round_opt3_little_endian(input + (i<<9), (i<<3) + 1, temp[0], temp[1], temp[2], temp[3], temp[4], temp[5], temp[6], temp[7], temp[8], temp[9], temp[10], temp[11], temp[12], temp[13], temp[14], temp[15], adder, initial_state, rotate16, rotate8);
    }

    if (over_8 != 0){
        uint8_t result[512];
        this->block_quarter_round_opt3_big_endian(result, (num_blocks_8<<3)+1, temp, adder, initial_state, rotate16, rotate8);
        for (int j = 0; j < over_8; j++)
            *(input + (num_blocks_8<<9) + j) ^= *(result + j);
    }
#endif
}





/***************************************
*    OPT5 - Vectorization - guided   *
****************************************/

void ChaCha20::encrypt_opt5(uint8_t *input, long len){

    long num_blocks_8 = len/512;  //8 blocks of 64 bytes
    long over_8 = len%512;

    // make some copy of the state that are going to be used by every block later
    __m256i temp[16];
    __m256i initial_state[16];
    __m256i adder = _mm256_set_epi32(7,6,5,4,3,2,1,0);
    __m256i rotate16 = _mm256_set_epi32(0x0d0c0f0e, 0x09080b0a, 0x05040706, 0x01000302, 0x0d0c0f0e, 0x09080b0a, 0x05040706, 0x01000302);
    __m256i rotate8 = _mm256_set_epi32(0x0e0d0c0f, 0x0a09080b, 0x06050407, 0x02010003, 0x0e0d0c0f, 0x0a09080b, 0x06050407, 0x02010003);

    for (int i = 0; i < 16; i++){
        temp[i] = _mm256_set1_epi32(this->state[i]);
        initial_state[i] = _mm256_set1_epi32(this->state[i]);
    }

    quarter_round_vect(&temp[1], &temp[5], &temp[9], &temp[13], rotate16, rotate8);
    quarter_round_vect(&temp[2], &temp[6], &temp[10], &temp[14], rotate16, rotate8);
    quarter_round_vect(&temp[3], &temp[7], &temp[11], &temp[15], rotate16, rotate8);

#if __BYTE_ORDER__ == __BIG_ENDIAN__
    #pragma omp parallel for schedule(guided) shared(input, temp, initial_state, adder, rotate16, rotate8)
    for (long i = 0; i < num_blocks_8; i++){
        uint8_t result[512];
        this->block_quarter_round_opt3_big_endian(result, (i<<3) + 1, temp, adder, initial_state, rotate16, rotate8);
        for (int j = 0; j < 512; j++)
            *(input + (i<<9) + j) ^= *(result + j);
    }

    if (over_8 != 0){
        uint8_t result[512];
        this->block_quarter_round_opt3_big_endian(result, (num_blocks_8<<3)+1, temp, adder, initial_state, rotate16, rotate8);
        for (int j = 0; j < over_8; j++)
            *(input + (num_blocks_8<<9) + j) ^= *(result + j);
    }
#else
    #pragma omp parallel for schedule(guided) shared(input, temp, initial_state, adder, rotate16, rotate8)
    for (long i = 0; i < num_blocks_8; i++){
        this->block_quarter_round_opt3_little_endian(input + (i<<9), (i<<3) + 1, temp[0], temp[1], temp[2], temp[3], temp[4], temp[5], temp[6], temp[7], temp[8], temp[9], temp[10], temp[11], temp[12], temp[13], temp[14], temp[15], adder, initial_state, rotate16, rotate8);
    }

    if (over_8 != 0){
        uint8_t result[512];
        this->block_quarter_round_opt3_big_endian(result, (num_blocks_8<<3)+1, temp, adder, initial_state, rotate16, rotate8);
        for (int j = 0; j < over_8; j++)
            *(input + (num_blocks_8<<9) + j) ^= *(result + j);
    }
#endif
}






/************
 *   CALLS   *
 ************/

void ChaCha20::encryptOpt(int opt_num, uint8_t *input, long len){
    method_function method_pointer[6] = {&ChaCha20::encrypt, &ChaCha20::encrypt_opt1, &ChaCha20::encrypt_opt2, &ChaCha20::encrypt_opt3, &ChaCha20::encrypt_opt4, &ChaCha20::encrypt_opt5};
    method_function func = method_pointer[opt_num];
    (this->*func)(input,len);
}

void ChaCha20::decryptOpt(int opt_num, uint8_t *input, long len){
    #define NDEC 6
    method_function method_pointer[NDEC] = {&ChaCha20::encrypt, &ChaCha20::encrypt_opt1, &ChaCha20::encrypt_opt2, &ChaCha20::encrypt_opt3, &ChaCha20::encrypt_opt4, &ChaCha20::encrypt_opt5};

    method_function func;
    if(opt_num > NDEC){
        func = method_pointer[0];
    } else {
        func = method_pointer[opt_num];
    }

    (this->*func)(input,len);
}