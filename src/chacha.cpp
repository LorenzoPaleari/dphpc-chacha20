#include "../include/chacha.hpp"
#include <immintrin.h>

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

uint8_t *ChaCha20::encrypt(uint8_t *input, long len){

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

    return input;
}

void ChaCha20::encrypt_single_block(uint8_t *input, uint32_t counter){

    uint8_t result[64];
    this->block_quarter_round(result, counter);
    for (int j = 0; j < 64; j++)
        *(input + j) ^= *(result + j);

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

uint8_t *ChaCha20::encrypt_opt1(uint8_t *input, long len){

    long num_blocks = len/64;
    long over = len%64;

    uint32_t temp[16];

    //MAKE A COPY OF THE STATE, SO WE CAN USE IT LATER - also equal for every block
    std::copy(this->state, this->state + 16, temp);

    //EQUAL FOR EVERY BLOCK
    quarter_round_opt1(&temp[1], &temp[5], &temp[9], &temp[13]);
    quarter_round_opt1(&temp[2], &temp[6], &temp[10], &temp[14]);
    quarter_round_opt1(&temp[3], &temp[7], &temp[11], &temp[15]);

#pragma omp parallel for
    for (long i = 0; i < num_blocks; i++){
        uint8_t result[64];
        this->block_quarter_round_opt1(result, i + 1, temp);
        for (int j = 0; j < 64; j++)
            *(input + (i<<6) + j) ^= *(result + j);
    }

    if (over != 0){
        uint8_t result[64];
        this->block_quarter_round_opt1(result, num_blocks + 1, temp);
        for (int j = 0; j < over; j++)
            *(input + (num_blocks<<6) + j) ^= *(result + j);
    }

    return input;
}




/************
*    OPT2   *
*************/

void ChaCha20::block_quarter_round_vect(uint8_t result[512], uint32_t counter){

    //MAKE A COPY OF THE STATE, SO WE CAN USE IT LATER
    // __m256i temp0 = _mm256_set_epi32(this->state[0], this->state[1], this->state[2], this->state[3], this->state[4], this->state[5], this->state[6], this->state[7]);
    // __m256i temp1 = _mm256_set_epi32(this->state[8], this->state[9], this->state[10], this->state[11], counter, this->state[13], this->state[14], this->state[15]);
    uint32_t temp[128];
    for (int i = 0; i < 128; i++){
        temp[i] = this->state[i%16];
    }
    temp[12] = counter;
    temp[28] = counter + 1;
    temp[44] = counter + 2;
    temp[60] = counter + 3;
    temp[76] = counter + 4;
    temp[92] = counter + 5;
    temp[108] = counter + 6;
    temp[124] = counter + 7;

    for (int i = 0; i < 10; i++){
        quarter_round(&temp[0], &temp[4], &temp[8], &temp[12]);
        quarter_round(&temp[16], &temp[20], &temp[24], &temp[28]);
        quarter_round(&temp[32], &temp[36], &temp[40], &temp[44]);
        quarter_round(&temp[48], &temp[52], &temp[56], &temp[60]);
        quarter_round(&temp[64], &temp[68], &temp[72], &temp[76]);
        quarter_round(&temp[80], &temp[84], &temp[88], &temp[92]);
        quarter_round(&temp[96], &temp[100], &temp[104], &temp[108]);
        quarter_round(&temp[112], &temp[116], &temp[120], &temp[124]);

        quarter_round(&temp[1], &temp[5], &temp[9], &temp[13]);
        quarter_round(&temp[17], &temp[21], &temp[25], &temp[29]);
        quarter_round(&temp[33], &temp[37], &temp[41], &temp[45]);
        quarter_round(&temp[49], &temp[53], &temp[57], &temp[61]);
        quarter_round(&temp[65], &temp[69], &temp[73], &temp[77]);
        quarter_round(&temp[81], &temp[85], &temp[89], &temp[93]);
        quarter_round(&temp[97], &temp[101], &temp[105], &temp[109]);
        quarter_round(&temp[113], &temp[117], &temp[121], &temp[125]);

        quarter_round(&temp[2], &temp[6], &temp[10], &temp[14]);
        quarter_round(&temp[18], &temp[22], &temp[26], &temp[30]);
        quarter_round(&temp[34], &temp[38], &temp[42], &temp[46]);
        quarter_round(&temp[50], &temp[54], &temp[58], &temp[62]);
        quarter_round(&temp[66], &temp[70], &temp[74], &temp[78]);
        quarter_round(&temp[82], &temp[86], &temp[90], &temp[94]);
        quarter_round(&temp[98], &temp[102], &temp[106], &temp[110]);
        quarter_round(&temp[114], &temp[118], &temp[122], &temp[126]);

        quarter_round(&temp[3], &temp[7], &temp[11], &temp[15]);
        quarter_round(&temp[19], &temp[23], &temp[27], &temp[31]);
        quarter_round(&temp[35], &temp[39], &temp[43], &temp[47]);
        quarter_round(&temp[51], &temp[55], &temp[59], &temp[63]);
        quarter_round(&temp[67], &temp[71], &temp[75], &temp[79]);
        quarter_round(&temp[83], &temp[87], &temp[91], &temp[95]);
        quarter_round(&temp[99], &temp[103], &temp[107], &temp[111]);
        quarter_round(&temp[115], &temp[119], &temp[123], &temp[127]);

        quarter_round(&temp[0], &temp[5], &temp[10], &temp[15]);
        quarter_round(&temp[16], &temp[21], &temp[26], &temp[31]);
        quarter_round(&temp[32], &temp[37], &temp[42], &temp[47]);
        quarter_round(&temp[48], &temp[53], &temp[58], &temp[63]);
        quarter_round(&temp[64], &temp[69], &temp[74], &temp[79]);
        quarter_round(&temp[80], &temp[85], &temp[90], &temp[95]);
        quarter_round(&temp[96], &temp[101], &temp[106], &temp[111]);
        quarter_round(&temp[112], &temp[117], &temp[122], &temp[127]);

        quarter_round(&temp[1], &temp[6], &temp[11], &temp[12]);
        quarter_round(&temp[17], &temp[22], &temp[27], &temp[28]);
        quarter_round(&temp[33], &temp[38], &temp[43], &temp[44]);
        quarter_round(&temp[49], &temp[54], &temp[59], &temp[60]);
        quarter_round(&temp[65], &temp[70], &temp[75], &temp[76]);
        quarter_round(&temp[81], &temp[86], &temp[91], &temp[92]);
        quarter_round(&temp[97], &temp[102], &temp[107], &temp[108]);
        quarter_round(&temp[113], &temp[118], &temp[123], &temp[124]);

        quarter_round(&temp[2], &temp[7], &temp[8], &temp[13]);
        quarter_round(&temp[18], &temp[23], &temp[24], &temp[29]);
        quarter_round(&temp[34], &temp[39], &temp[40], &temp[45]);
        quarter_round(&temp[50], &temp[55], &temp[56], &temp[61]);
        quarter_round(&temp[66], &temp[71], &temp[72], &temp[77]);
        quarter_round(&temp[82], &temp[87], &temp[88], &temp[93]);
        quarter_round(&temp[98], &temp[103], &temp[104], &temp[109]);
        quarter_round(&temp[114], &temp[119], &temp[120], &temp[125]);

        quarter_round(&temp[3], &temp[4], &temp[9], &temp[14]);
        quarter_round(&temp[19], &temp[20], &temp[25], &temp[30]);
        quarter_round(&temp[35], &temp[36], &temp[41], &temp[46]);
        quarter_round(&temp[51], &temp[52], &temp[57], &temp[62]);
        quarter_round(&temp[67], &temp[68], &temp[73], &temp[78]);
        quarter_round(&temp[83], &temp[84], &temp[89], &temp[94]);
        quarter_round(&temp[99], &temp[100], &temp[105], &temp[110]);
        quarter_round(&temp[115], &temp[116], &temp[121], &temp[126]);
    }

    //ADD THE STATE TO THE SCRUMBLED STATE
    for (int i = 0; i < 128; i++){
        temp[i] += this->state[i%16];
    }
    temp[12] += counter;
    temp[28] += counter + 1;
    temp[44] += counter + 2;
    temp[60] += counter + 3;
    temp[76] += counter + 4;
    temp[92] += counter + 5;
    temp[108] += counter + 6;
    temp[124] += counter + 7;

    //COPY THE SCRUMBLED STATE TO THE RESULT POINTER AS LITTLE ENDIAN
    for (int i = 0; i < 128; i++){
        uint32_to_uint8(temp[i], result + (i * 4));
    }
}

uint8_t *ChaCha20::encrypt_opt2(uint8_t *input, long len){

    long num_blocks = len/64;
    long over = len%64;
    long num_blocks_8 = num_blocks/8;
    long over_8 = num_blocks%8;

    uint8_t result[512];
    uint8_t res64[64];

#pragma omp parallel for
    for (long i = 0; i < num_blocks_8; i++){
        this->block_quarter_round_vect(result, (i<<3) + 1);
        for (int j = 0; j < 512; j++)
            *(input + (i<<9) + j) ^= *(result + j);
    }

    long data_offset = num_blocks_8<<9;
    long counter_offset = (num_blocks_8<<3) + 1;
    if (over_8 != 0){
        for (long i = 0; i < over_8; i++){
            this->block_quarter_round(res64, counter_offset + i);
            for (int j = 0; j < 64; j++)
                *(input + data_offset + (i<<6) + j) ^= *(res64 + j);
        }
    }

    data_offset += over_8<<6;
    if (over != 0){
        this->block_quarter_round(res64, num_blocks+1);
        for (int j = 0; j < over; j++)
            *(input + data_offset + j) ^= *(res64 + j);
    }

    return input;
}

// uint8_t  *ChaCha20::decrypt(uint8_t *input, long len){
//     return this->encrypt(input, len);
// }

// uint32_t rotation_l32(uint32_t x, int n){
//     return (x << n) | (x >> (32 - n));
// }

// void quarter_round_vect(uint32_t *a, uint32_t *b, uint32_t *c, uint32_t *d){
//     *a += *b; *d ^= *a; *d = rotation_l32(*d, 16);
//     *c += *d; *b ^= *c; *b = rotation_l32(*b, 12);
//     *a += *b; *d ^= *a; *d = rotation_l32(*d, 8);
//     *c += *d; *b ^= *c; *b = rotation_l32(*b, 7);
// }

// void uint32_to_uint8(uint32_t x, uint8_t *y){
//     y[0] = (x >> 0) & 0xff;
//     y[1] = (x >> 8) & 0xff;
//     y[2] = (x >> 16) & 0xff;
//     y[3] = (x >> 24) & 0xff;
// }





/************
 *   CALLS   *
 ************/

uint8_t *ChaCha20::encryptOpt(int opt_num, uint8_t *input, long len){
    method_function method_pointer[3] = {&ChaCha20::encrypt, &ChaCha20::encrypt_opt1, &ChaCha20::encrypt_opt2};
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