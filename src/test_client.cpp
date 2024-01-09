#include "../include/test_client.hpp"
#include "../include/chacha.hpp"
//#include "../src/chacha.cpp"
#include <string.h>
#include <iostream>
#include <sodium.h>
#include <cstdint>

using namespace std;

#define OPT_L 5
#define IS_BIG_ENDIAN (!*(unsigned char *)&(uint16_t){1})

TestClient::TestClient(){
    bool (*fns[])(int) = {
        chacha20_rfc,
        chacha20_rfc_text,
        chacha20_enc_dec,
        chacha20_large_enc,
        0x0
    };

    int i = 0;
    int passed = 0;
    while(true){
        if (fns[i] != 0x0){
            passed += this->runTest(i, fns[i]);
            i++;
        } else {
            break;
        }
    }

    cout << "Passed " << passed << " tests out of " << i << endl;
}

bool TestClient::runTest(int i, bool (*tf)(int)){

    bool result = true;
    int j;

    for(j = 0; j < OPT_L; j++){
        result = tf(j);
        if (!result) break;
    }

    if(result) cout << "\033[32mTest " << i+1 << " passed\033[0m" << endl;
    else cout << "\033[31mTest " << i+1 << " failed (" <<  j << ")\033[0m" << endl;

    return result;
}

bool chacha20_rfc(int opt){
    // The key is set to 00:01:02:03:04:05:06:07:08:09:0a:0b:0c:0d:0e:0f...1e:1f
    uint8_t key[32];

    for (int i = 0; i < 32; i++){
        key[i] = i;
    }

    // The nonce is set to 00:00:00:09:00:00:00:4a:00:00:00:00
    uint8_t nonce[] = {0x00,0x00,0x00,0x09,0x00,0x00,0x00,0x4a,0x00,0x00,0x00,0x00};

    ChaCha20 block = ChaCha20(key,nonce);

    uint32_t temp[16];
    for (int i = 0; i < 16; i++){
        temp[i] = block.state[i];
    }
    __m256i temp_3[16];
    __m256i initial_state[16];
    __m256i rotate16 = _mm256_set_epi32(0x0d0c0f0e, 0x09080b0a, 0x05040706, 0x01000302, 0x0d0c0f0e, 0x09080b0a, 0x05040706, 0x01000302);
    __m256i rotate8 = _mm256_set_epi32(0x0e0d0c0f, 0x0a09080b, 0x06050407, 0x02010003, 0x0e0d0c0f, 0x0a09080b, 0x06050407, 0x02010003);
    for (int i = 0; i < 16; i++){
        temp_3[i] = _mm256_set1_epi32(block.state[i]);
        initial_state[i] = temp_3[i];
    }

    uint8_t result[64];
    uint8_t result_vect [512];

    switch (opt){
        case 0:
            block.block_quarter_round(result, 1);
            break;
        case 1:
            block.quarter_round_opt1(&temp[1], &temp[5], &temp[9], &temp[13]);
            block.quarter_round_opt1(&temp[2], &temp[6], &temp[10], &temp[14]);
            block.quarter_round_opt1(&temp[3], &temp[7], &temp[11], &temp[15]);
            block.block_quarter_round_opt1(result,1,temp);
            break;
        case 2:
            block.quarter_round_vect(&temp_3[1], &temp_3[5], &temp_3[9], &temp_3[13], rotate16, rotate8);
            block.quarter_round_vect(&temp_3[2], &temp_3[6], &temp_3[10], &temp_3[14], rotate16, rotate8);
            block.quarter_round_vect(&temp_3[3], &temp_3[7], &temp_3[11], &temp_3[15], rotate16, rotate8);
            block.block_quarter_round_opt2_big_endian(result_vect,1, temp_3, _mm256_set_epi32(7,6,5,4,3,2,1,0), initial_state, rotate16, rotate8);
            break;
        case 3:
            block.quarter_round_vect(&temp_3[1], &temp_3[5], &temp_3[9], &temp_3[13], rotate16, rotate8);
            block.quarter_round_vect(&temp_3[2], &temp_3[6], &temp_3[10], &temp_3[14], rotate16, rotate8);
            block.quarter_round_vect(&temp_3[3], &temp_3[7], &temp_3[11], &temp_3[15], rotate16, rotate8);
            block.block_quarter_round_opt2_big_endian(result_vect,1, temp_3, _mm256_set_epi32(7,6,5,4,3,2,1,0), initial_state, rotate16, rotate8);
            break;
        case 4:
            block.quarter_round_vect(&temp_3[1], &temp_3[5], &temp_3[9], &temp_3[13], rotate16, rotate8);
            block.quarter_round_vect(&temp_3[2], &temp_3[6], &temp_3[10], &temp_3[14], rotate16, rotate8);
            block.quarter_round_vect(&temp_3[3], &temp_3[7], &temp_3[11], &temp_3[15], rotate16, rotate8);
            block.block_quarter_round_opt2_big_endian(result_vect,1, temp_3, _mm256_set_epi32(7,6,5,4,3,2,1,0), initial_state, rotate16, rotate8);
            break;
        default:
            break;
    }

    uint8_t expected_result[] = {
       0x10, 0xf1, 0xe7, 0xe4, 0xd1, 0x3b, 0x59, 0x15, 0x50, 0x0f, 0xdd, 0x1f, 0xa3, 0x20, 0x71, 0xc4,
       0xc7, 0xd1, 0xf4, 0xc7, 0x33, 0xc0, 0x68, 0x03, 0x04, 0x22, 0xaa, 0x9a, 0xc3, 0xd4, 0x6c, 0x4e,
       0xd2, 0x82, 0x64, 0x46, 0x07, 0x9f, 0xaa, 0x09, 0x14, 0xc2, 0xd7, 0x05, 0xd9, 0x8b, 0x02, 0xa2,
       0xb5, 0x12, 0x9c, 0xd1, 0xde, 0x16, 0x4e, 0xb9, 0xcb, 0xd0, 0x83, 0xe8, 0xa2, 0x50, 0x3c, 0x4e
    };

    if (opt >= 2){
        for (int i = 0; i < 64; i++){
            if (result_vect[i] != expected_result[i]){
                return false;
            }
        }
        return true;
    }

    for (int i = 0; i < 64; i++){
        if (result[i] != expected_result[i]){
            return false;
        }
    }

    return true;
}

bool chacha20_rfc_text(int opt){
    char input[] = "Ladies and Gentlemen of the class of '99: If I could offer you only one tip for the future, sunscreen would be it.";
    // The key is set to 00:01:02:03:04:05:06:07:08:09:0a:0b:0c:0d:0e:0f...1e:1f
    uint8_t key[32];

    for (int i = 0; i < 32; i++){
        key[i] = i;
    }

    // The nonce is set to 00:00:00:09:00:00:00:4a:00:00:00:00
    uint8_t nonce[] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x4a,0x00,0x00,0x00,0x00};

    ChaCha20 block = ChaCha20(key,nonce);
    block.encryptOpt(opt, (uint8_t *)input, strlen(input));

    uint8_t result_expected[] = {
        0x6e, 0x2e, 0x35, 0x9a, 0x25, 0x68, 0xf9, 0x80, 0x41, 0xba, 0x07, 0x28, 0xdd, 0x0d, 0x69, 0x81,
        0xe9, 0x7e, 0x7a, 0xec, 0x1d, 0x43, 0x60, 0xc2, 0x0a, 0x27, 0xaf, 0xcc, 0xfd, 0x9f, 0xae, 0x0b,
        0xf9, 0x1b, 0x65, 0xc5, 0x52, 0x47, 0x33, 0xab, 0x8f, 0x59, 0x3d, 0xab, 0xcd, 0x62, 0xb3, 0x57,
        0x16, 0x39, 0xd6, 0x24, 0xe6, 0x51, 0x52, 0xab, 0x8f, 0x53, 0x0c, 0x35, 0x9f, 0x08, 0x61, 0xd8,
        0x07, 0xca, 0x0d, 0xbf, 0x50, 0x0d, 0x6a, 0x61, 0x56, 0xa3, 0x8e, 0x08, 0x8a, 0x22, 0xb6, 0x5e,
        0x52, 0xbc, 0x51, 0x4d, 0x16, 0xcc, 0xf8, 0x06, 0x81, 0x8c, 0xe9, 0x1a, 0xb7, 0x79, 0x37, 0x36,
        0x5a, 0xf9, 0x0b, 0xbf, 0x74, 0xa3, 0x5b, 0xe6, 0xb4, 0x0b, 0x8e, 0xed, 0xf2, 0x78, 0x5e, 0x42,
        0x87, 0x4d
    };

    for (size_t i = 0; i < 114; i++){
        if ((uint8_t)input[i] != result_expected[i]){
            return false;
        }
    }

    return true;
}

bool chacha20_enc_dec(int opt){
    char input[] = "Hi lorenzo, how are you? I'm fine, thank you. What about you? I'm fine too, thank you. Bye bye! See you soon! I don't want to see you anymore!";
    char *input2 = (char *) malloc(strlen(input)+1);
    strcpy(input2, input);

    uint8_t key[32];

    for (int i = 0; i < 32; i++){
        key[i] = i;
    }
    uint8_t nonce[] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x4a,0x00,0x00,0x00,0x00};
    ChaCha20 block = ChaCha20(key,nonce);

    block.encryptOpt(opt, (uint8_t *)input, strlen(input)+1);
    block.decryptOpt(opt, (uint8_t *)input, strlen(input2)+1);

    bool res = strcmp((char *)input, input2) == 0;
    free(input2);
    return res;
}

bool chacha20_large_enc(int opt){
    // LARGE INPUT
    char input_text[] = "mauris sit amet massa vitae tortor condimentum lacinia quis vel eros donec ac odio tempor orci dapibus ultrices in iaculis nunc sed augue lacus viverra vitae congue eu consequat ac felis donec et odio pellentesque diam volutpat commodo sed egestas egestas fringilla phasellus faucibus scelerisque eleifend donec pretium vulputate sapien nec sagittis aliquam malesuada bibendum arcu vitae elementum curabitur vitae nunc sed velit dignissim sodales ut eu sem integer vitae justo eget magna fermentum iaculis eu non diam phasellus vestibulum lorem sed risus ultricies tristique nulla aliquet enim tortor at auctor urna nunc id cursus metus aliquam eleifend mi in nulla posuere sollicitudin aliquam ultrices sagittis orci a scelerisque purus semper eget duis at tellus at urna condimentum mattis pellentesque id nibh tortor id aliquet lectus proin nibh nisl condimentum id venenatis a condimentum vitae sapien pellentesque habitant morbi tristique senectus et netus et malesuada fames ac turpis egestas sed tempus urna et pharetra pharetra massa massa ultricies mi quis hendrerit dolor magna eget est lorem ipsum dolor sit amet consectetur adipiscing elit pellentesque habitant morbi tristique senectus et netus et malesuada fames ac turpis egestas integer eget aliquet nibh praesent tristique magna sit amet purus gravida quis blandit turpis cursus in hac habitasse platea dictumst quisque sagittis purus sit amet volutpat consequat mauris nunc congue nisi vitae suscipit tellus mauris a diam maecenas sed enim ut sem viverra aliquet eget sit amet tellus cras adipiscing enim eu turpis egestas pretium aenean pharetra";

    uint8_t key[32];

    for (int i = 0; i < 32; i++){
        key[i] = i;
    }
    uint8_t nonce[] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x4a,0x00,0x00,0x00,0x00};
    ChaCha20 block = ChaCha20(key,nonce);

    uint8_t *copy = (uint8_t *) malloc(strlen(input_text)+1);
    strcpy((char *)copy, input_text);
    uint8_t *input = (uint8_t *) aligned_alloc(32,(strlen(input_text)+32) & ~31);
    strcpy((char *)input, input_text);

    int test = sodium_init();
    if (test == -1) {
        cout << "Error initializing libsodium" << endl;
        return false;
    }
    //encrypt with libsodium
    uint8_t *original = (uint8_t *) malloc(strlen(input_text)+1);
    memcpy(original, input_text, strlen(input_text)+1);
    crypto_stream_chacha20_ietf_xor_ic(copy, copy, strlen(input_text)+1, nonce, 1, key);

    block.encryptOpt(opt, input, strlen(input_text)+1);

    //check if input is equal to copy
    for(int i = 0; i < (int)strlen(input_text)+1; i++){
        if ((uint8_t) input[i] != copy[i]){
            cout << "Input and copy are different at index " << i << endl;
            return false;
        }
    }


    return true;
}
