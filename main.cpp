#include "include/chacha.hpp"
#include "include/test_client.hpp"
#include <iostream>
#include "/usr/local/opt/libomp/include/omp.h"
#include <string.h>
#include <chrono>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include "src/original/chacha20.hpp"
#define N_RUNS 10
#define OPT_L 3
using namespace std;

static long *start_run(uint8_t *message, long len, ChaCha20 c, int opt_n, struct chacha20_context *original){
    long *times = (long *) malloc(sizeof(long) * 2);

    auto start = std::chrono::high_resolution_clock::now();
    if(opt_n == -1){
        chacha20_xor(original, message, len);
    } else {
        c.encryptOpt(opt_n, message, len);
    }
    auto end = std::chrono::high_resolution_clock::now();
    times[0] = std::chrono::duration_cast<std::chrono::microseconds>(end-start).count();
    // cout << "Enc Time: " << enc_time << " microseconds" << endl;
    
    start = std::chrono::high_resolution_clock::now();
    if(opt_n == -1){
        chacha20_xor(original, message, len);
    } else {
        c.decryptOpt(opt_n, message, len);
    }
    end = std::chrono::high_resolution_clock::now();
    times[1] = std::chrono::duration_cast<std::chrono::microseconds>(end-start).count();
    // cout << "Dec Time: " << last_time << " microseconds" << endl;

    return times;
}

static bool from_file(char const *argv[]){
    // atoi converts string to int, i need a long
    long len = atol(argv[1]);
    int opt = atoi(argv[3]);
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
    long *time = start_run(message, len, chacha, opt, NULL);
    cout << "Enc Time: " << time[0] << " microseconds" << endl;
    cout << "Dec Time: " << time[1] << " microseconds" << endl;

    fclose(fp);
    free(time);
    free(message);
    return true;
}

static bool multiple_run(const char *argv[], ChaCha20 c, struct chacha20_context *original, int n_opt){
    long max_len_size = atol(argv[1]);
    
    for (int p = -1; p < n_opt; p++){

        // SET THE NUMBER OF THREAD TO USE - IF -1 (ORIGINAL) -> WE JUST RUN 1 THREAD
        int n_threads = omp_get_max_threads();
        if(p == -1){
            n_threads = 1;
            cout << "Running original version" << endl;
        }

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

            int thread_runs = (int)log2(n_threads)+1;
            long *gen_time = (long *) malloc(sizeof(long) * N_RUNS * thread_runs);
            while (n < N_RUNS){

                if(p == -1) chacha20_block_set_counter(original, 1);

                printf("\rRun %d/%d", n+1, N_RUNS);
                fflush(stdout);

                for(int k = 1; k <= n_threads; k*=2){
                    uint8_t *message = (uint8_t *) malloc(sizeof(uint8_t) * start_len);
                    
                    omp_set_num_threads(n_threads);
                    auto start = std::chrono::high_resolution_clock::now();
                    #pragma omp parallel for
                    for (long t = 0; t < (long)start_len/1024; t+=1){
                        int r = rand();
                        int b = rand();
                        for (long i = t*1024; i < t*1024 + 1024; i+=32) {
                            long v = i + 16;
                            r += 1234;
                            *(message + i) = r;
                            b += 1234;
                            *(message + v) = b;
                            r -= 111; 
                            *(message + i + 4) = r;
                            b -= 111;
                            *(message + v + 4) = b;
                            r += 12;
                            *(message + i + 8) = r;
                            b += 12;
                            *(message + v + 8) = b;
                            r -= 432;                        
                            *(message + i + 12) = r;
                            b -= 432;                  
                            *(message + v + 12) = b;
                        }
                    }
                    auto end = std::chrono::high_resolution_clock::now();
                    gen_time[n*thread_runs + (int)log2(k)] = std::chrono::duration_cast<std::chrono::microseconds>(end-start).count();

                    omp_set_num_threads(k);

                    long *time = start_run(message, start_len, c, p, original);
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
            long sum = 0;
            for(int k = 0; k < N_RUNS; k++){
                for(int i = 0; i < thread_runs; i++){
                    sum += gen_time[k*thread_runs + i];
                }
            }
            printf("\nAvarage gen time: %ld", sum/N_RUNS/thread_runs);
            free(gen_time);
            printf("\n\n");
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
    struct chacha20_context original;
    chacha20_init_context(&original, key, nonce, 1);

    switch(argc){
        case 4:
            from_file(argv);
            break;
        case 2:
            multiple_run(argv, chacha, &original, OPT_L);
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
