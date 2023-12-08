#include "include/chacha.hpp"
#include "include/test_client.hpp"
#include <iostream>
#include "/usr/local/opt/libomp/include/omp.h"
#include <string.h>
#include <chrono>
#include <sys/stat.h>
#define N_RUNS 10
using namespace std;

static long *start_run(uint8_t *message, long len, ChaCha20 c, int opt_n){
    long *times = (long *) malloc(sizeof(long) * 2);
    auto start = std::chrono::high_resolution_clock::now();
    uint8_t *enc_result = c.encryptOpt(opt_n, message, len);
    auto end = std::chrono::high_resolution_clock::now();
    times[0] = std::chrono::duration_cast<std::chrono::microseconds>(end-start).count();
    // cout << "Enc Time: " << enc_time << " microseconds" << endl;
    
    start = std::chrono::high_resolution_clock::now();
    uint8_t *dec_result = c.decryptOpt(opt_n, enc_result, len);
    end = std::chrono::high_resolution_clock::now();
    times[1] = std::chrono::duration_cast<std::chrono::microseconds>(end-start).count();
    // cout << "Dec Time: " << last_time << " microseconds" << endl;

#ifdef CHECK
printf("Checking if message and result are the same\n");
#pragma omp parallel for
    for (long i = 0; i < len; i++) {
        if(message[i] != dec_result[i]) {
            cout << "\033[31m\033[1mError: message and result are not the same\033[0m" << endl;
            free(enc_result);
            free(dec_result);
            exit(1);
        }
    }
#endif

    free(enc_result);
    free(dec_result);
    return times;
}

static bool from_file(char const *argv[]){
    // atoi converts string to int, i need a long
    long len = atol(argv[1]);
    uint8_t *message = (uint8_t *) malloc(sizeof(uint8_t) * len);

    // Read file
    FILE *fp = fopen(argv[2], "r");
    if(fp == NULL) {
        cout << "Error opening file" << endl;
        return false;
    }

    uint8_t key[32];
    uint8_t nonce[12];
    for (int i = 0; i < 32; i++) 
        key[i] = rand() % 256;

    for (int i = 0; i < 12; i++) 
        nonce[i] = rand() % 256;

    ChaCha20 chacha = ChaCha20(key, nonce);

    fread(message, sizeof(uint8_t), len, fp);
    cout << "Read file of size " << len << " bytes" << endl;
    long *time = start_run(message, len, chacha, 1);
    cout << "Enc Time: " << time[0] << " microseconds" << endl;
    cout << "Dec Time: " << time[1] << " microseconds" << endl;

    fclose(fp);
    free(time);
    free(message);
    return true;
}

static bool multiple_run(const char *argv[], ChaCha20 c, int n_opt){
    long max_len_size = atol(argv[1]);
    int n_threads = omp_get_max_threads();
    
    for (int p = 0; p < n_opt; p++){

        long start_len = 1024; // 1 KB
        while(start_len <= max_len_size) {
            // write to file output_data/{len}_out - create if not exists
            cout << "Running with len: " << start_len << endl;
            string file_name = "output_data/" + to_string(start_len) + "_opt" + to_string(p) + "_out";
            FILE *fp = fopen(file_name.c_str(), "w");
            if(fp == NULL) {
                cout << "Error opening file" << endl;
                return false;
            }

            //first line is n threads
            for(int k = 1; k <= n_threads; k*=2){
                fprintf(fp, "%d", k);
                if (k*2 <= n_threads) {
                    fprintf(fp, ",");
                }
            }
            fprintf(fp, "\n");

            int n = 0;
            while (n < N_RUNS){

                printf("\rRun %d/%d", n+1, N_RUNS);
                fflush(stdout);

                for(int k = 1; k <= n_threads; k*=2){
                    omp_set_num_threads(k);
                    uint8_t *message = (uint8_t *) malloc(sizeof(uint8_t) * start_len);
                    
                    // use less rand() calls since it's slow
                    int r = rand();
                    int b = rand();
                    for (long i = 0; i < start_len; i+=32) {
                        int v = i + 16;
                        r += 1234;
                        message[i] = r && 0xFF;
                        message[i+1] = (r >> 8) && 0xFF;
                        message[i+2] = (r >> 16) && 0xFF;
                        message[i+3] = (r >> 24) && 0xFF;
                        r -= 111; 
                        message[i+4] = r && 0xFF;
                        message[i+5] = (r >> 8) && 0xFF;
                        message[i+6] = (r >> 16) && 0xFF;
                        message[i+7] = (r >> 24) && 0xFF;
                        r += 12;
                        message[i + 8]= r && 0xFF;
                        message[i + 9] = (r >> 8) && 0xFF;
                        message[i + 10] = (r >> 16) && 0xFF;
                        message[i + 11] = (r >> 24) && 0xFF;
                        r -= 432;                        
                        message[i + 12] = r && 0xFF;
                        message[i + 13] = (r >> 8) && 0xFF;
                        message[i + 14] = (r >> 16) && 0xFF;
                        message[i + 15] = (r >> 24) && 0xFF;

                        b += 1234;
                        message[v] = b && 0xFF;
                        message[v+1] = (b >> 8) && 0xFF;
                        message[v+2] = (b >> 16) && 0xFF;
                        message[v+3] = (b >> 24) && 0xFF;
                        b -= 111; 
                        message[v+4] = b && 0xFF;
                        message[v+5] = (b >> 8) && 0xFF;
                        message[v+6] = (b >> 16) && 0xFF;
                        message[v+7] = (b >> 24) && 0xFF;
                        b += 12;
                        message[v + 8]= b && 0xFF;
                        message[v + 9] = (b >> 8) && 0xFF;
                        message[v + 10] = (b >> 16) && 0xFF;
                        message[v + 11] = (b >> 24) && 0xFF;
                        b -= 432;                        
                        message[v + 12] = b && 0xFF;
                        message[v + 13] = (b >> 8) && 0xFF;
                        message[v + 14] = (b >> 16) && 0xFF;
                        message[v + 15] = (b >> 24) && 0xFF;
                    }

                    long *time = start_run(message, start_len, c, p);
                    // cout << "Enc Time: " << time[0] << " microseconds" << endl;
                    // cout << "Dec Time: " << time[1] << " microseconds" << endl;

                    fprintf(fp, "%ld/%ld", time[0], time[1]);
                    if (k*2 <= n_threads) {
                        fprintf(fp, ",");
                    }
                    free(message);
                    free(time);
                }
                n++;
                fprintf(fp, "\n");
            }
            printf("\n");
            start_len *= 4;
            fclose(fp);
        }
    }

    return true;
}

int main(int argc, char const *argv[])
{
    TestClient test_client = TestClient();
    (void)test_client;


    if(argc == 3) {
        from_file(argv);
    }

    cout << "Running with max " << omp_get_max_threads() << " threads" << endl;

    uint8_t key[32];
    uint8_t nonce[12];
    for (int i = 0; i < 32; i++) 
        key[i] = rand() % 256;

    for (int i = 0; i < 12; i++) 
        nonce[i] = rand() % 256;

    ChaCha20 chacha = ChaCha20(key, nonce);

    switch(argc){
        case 3:
            from_file(argv);
            break;
        case 2:
            multiple_run(argv, chacha, 2);
            break;
        case 1:
            cout << "No arguments just runs the test" << endl;
            cout << "Usage 1: ./enc <len> <file> " << endl;
            cout << "Usage 2: ./enc <max_len_size> " << endl;
            break;
        default:
            cout << "Error: invalid arguments" << endl;
            break;
    }

    return 0;
}
