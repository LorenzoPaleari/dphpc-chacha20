#include "include/chacha.hpp"
#include "include/test_client.hpp"
#include <iostream>
#include "/usr/local/opt/libomp/include/omp.h"
#include <string.h>
#include <chrono>
#include <sys/stat.h>
#define N_RUNS 10
using namespace std;

static long *start_run(uint8_t *message, long len){
    uint8_t key[32];
    uint8_t nonce[12];
    for (int i = 0; i < 32; i++) 
        key[i] = rand() % 256;

    for (int i = 0; i < 12; i++) 
        nonce[i] = rand() % 256;

    ChaCha20 chacha = ChaCha20(key, nonce);

    long *times = (long *) malloc(sizeof(long) * 2);
    auto start = std::chrono::high_resolution_clock::now();
    uint8_t *result = chacha.encrypt(message, len);
    auto end = std::chrono::high_resolution_clock::now();
    times[0] = std::chrono::duration_cast<std::chrono::microseconds>(end-start).count();
    // cout << "Enc Time: " << enc_time << " microseconds" << endl;
    
    // check if decrypt is the inverse of encrypt
    start = std::chrono::high_resolution_clock::now();
    result = chacha.decrypt(result, len);
    end = std::chrono::high_resolution_clock::now();
    times[1] = std::chrono::duration_cast<std::chrono::microseconds>(end-start).count();
    // cout << "Dec Time: " << last_time << " microseconds" << endl;

#pragma omp parallel for
    for (long i = 0; i < len; i++) {
        if(message[i] != result[i]) {
            cout << "\033[31m\033[1mError: message and result are not the same\033[0m" << endl;
            free(result);
            exit(1);
        }
    }

    free(result);
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

    fread(message, sizeof(uint8_t), len, fp);
    cout << "Read file of size " << len << " bytes" << endl;
    long *time = start_run(message, len);
    cout << "Enc Time: " << time[0] << " microseconds" << endl;
    cout << "Dec Time: " << time[1] << " microseconds" << endl;

    fclose(fp);
    free(time);
    free(message);
    return true;
}

static bool multiple_run(const char *argv[]){
    long max_len_size = atol(argv[1]);
    int n_threads = omp_get_max_threads();

    long start_len = 1024; // 1 KB
    while(start_len <= max_len_size) {
        // write to file output_data/{len}_out - create if not exists
        cout << "Running with len: " << start_len << endl;
        string file_name = "output_data/" + to_string(start_len) + "_out";
        FILE *fp = fopen(file_name.c_str(), "w");
        if(fp == NULL) {
            cout << "Error opening file" << endl;
            return false;
        }

        //first line is n threads
        fprintf(fp, "%d\n", n_threads);
        int n = 0;
        while (n < N_RUNS){

            printf("\rRun %d/%d", n+1, N_RUNS);
            fflush(stdout);

            for(int k = 1; k <= n_threads; k++){
                omp_set_num_threads(k);
                uint8_t *message = (uint8_t *) malloc(sizeof(uint8_t) * start_len);
                for (long i = 0; i < start_len; i++) {
                    message[i] = rand() % 256;
                }
                long *time = start_run(message, start_len);
                // cout << "Enc Time: " << time[0] << " microseconds" << endl;
                // cout << "Dec Time: " << time[1] << " microseconds" << endl;

                fprintf(fp, "%ld/%ld", time[0], time[1]);
                if (k != n_threads) {
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

    switch(argc){
        case 3:
            from_file(argv);
            break;
        case 2:
            multiple_run(argv);
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