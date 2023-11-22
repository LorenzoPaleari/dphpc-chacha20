#include "include/chacha.hpp"
#include "include/test_client.hpp"
#include <iostream>
#include "/usr/local/opt/libomp/include/omp.h"
#include <string.h>
using namespace std;

int main(int argc, char const *argv[])
{
    cout << "argc: " << argc << endl;
    cout << "argv[0]: " << argv[0] << endl;

    TestClient test_client = TestClient();
    (void)test_client;

    if(argc == 1) return 0;

    if(argc != 3) {
        cout << "Usage: " << argv[0] << " <length> <message>" << endl;
        return 0;
    }

    // argv[1] contains the length of the message
    // argv[2] contains the nome of the file to read

    // Generate a random key and a random nonce

    uint8_t key[32];
    uint8_t nonce[12];
    for (int i = 0; i < 32; i++) 
        key[i] = rand() % 256;

    for (int i = 0; i < 12; i++) 
        nonce[i] = rand() % 256;

    ChaCha20 chacha = ChaCha20(key, nonce);
    int len = atoi(argv[1]);
    uint8_t *message = (uint8_t *) malloc(sizeof(uint8_t) * len);

    // Read file
    FILE *fp = fopen(argv[2], "r");
    if(fp == NULL) {
        cout << "Error opening file" << endl;
        return 0;
    }

    fread(message, sizeof(uint8_t), len, fp);

    uint8_t *result = chacha.encrypt(message, len);
    
    // check if decrypt is the inverse of encrypt
    result = chacha.decrypt(result, len);

    for (int i = 0; i < len; i++) {
        if(message[i] != result[i]) {
            cout << "Error: message and result are different" << endl;
            free(result);
            return 0;
        }
    }

    cout << "Success: message and result are equal" << endl;
    free(result);
    return 0;
}