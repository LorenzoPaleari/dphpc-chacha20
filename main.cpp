#include "include/chacha.hpp"
#include "include/test_client.hpp"
#include <iostream>
#include "/usr/local/opt/libomp/include/omp.h"
#include <string.h>
#include <chrono>
using namespace std;

long last_time;


static bool from_file(char const *argv[]){
    uint8_t key[32];
    uint8_t nonce[12];
    for (int i = 0; i < 32; i++) 
        key[i] = rand() % 256;

    for (int i = 0; i < 12; i++) 
        nonce[i] = rand() % 256;

    ChaCha20 chacha = ChaCha20(key, nonce);
    // atoi converts string to int, i need a long
    long len = atol(argv[1]);
    uint8_t *message = (uint8_t *) malloc(sizeof(uint8_t) * len);

    // Read file
    FILE *fp = fopen(argv[2], "r");
    if(fp == NULL) {
        cout << "Error opening file" << endl;
        return false;
    }

    fread(message, sizeof(uint8_t), len, fp);
    cout << "Read file of size " << len << " bytes" << endl;

    auto start = std::chrono::high_resolution_clock::now();
    uint8_t *result = chacha.encrypt(message, len);
    auto end = std::chrono::high_resolution_clock::now();
    last_time = std::chrono::duration_cast<std::chrono::microseconds>(end-start).count();
    cout << "Enc Time: " << last_time << " microseconds" << endl;
    
    // check if decrypt is the inverse of encrypt
    start = std::chrono::high_resolution_clock::now();
    result = chacha.decrypt(result, len);
    end = std::chrono::high_resolution_clock::now();
    last_time = std::chrono::duration_cast<std::chrono::microseconds>(end-start).count();
    cout << "Dec Time: " << last_time << " microseconds" << endl;

#pragma omp parallel for
    for (long i = 0; i < len; i++) {
        if(message[i] != result[i]) {
            cout << "\033[31m\033[1mError: message and result are not the same\033[0m" << endl;
            free(result);
            exit(1);
        }
    }

    free(result);
    return true;
}

int main(int argc, char const *argv[])
{
    cout << "argc: " << argc << endl;
    cout << "argv[0]: " << argv[0] << endl;

    TestClient test_client = TestClient();
    (void)test_client;

    if(argc == 1) return 0;

    if(argc == 3) {
        if(from_file(argv)){
            cout << "\033[32m\033[1mSuccess: message and result are the same\033[0m" << endl;
            return 0;
        } else {
            cout << "\033[31m\033[1mError: message and result are not the same\033[0m" << endl;
        }
    }

    return 0;
}