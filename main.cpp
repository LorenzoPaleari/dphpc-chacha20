#include "include/chacha.hpp"

#include <iostream>
using namespace std;

void chacha20(){
    uint8_t key[32];
    uint8_t nonce[12];

    for (int i = 0; i < 32; i++){
        key[i] = i;
    }
    for (int i = 0; i < 12; i++){
        nonce[i] = 0;
    }
    nonce[3] = 0x09;
    nonce[7] = 0x4a;

    ChaCha20 block = ChaCha20(key,nonce);
    block.setCounter(1);

    uint8_t result[64];
    block.block_quarter_round(result);

    for (int i = 0; i < 64; i++){
        cout << std::hex << static_cast<unsigned int>(result[i]) << "  ";
        if (i % 16 == 15){
            cout << endl;
        }
    }

}

int main(int argc, char const *argv[])
{
    cout << "argc: " << argc << endl;
    cout << "argv[0]: " << argv[0] << endl;

    chacha20();
    return 0;
}