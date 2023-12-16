#include "include/chacha.hpp"
#include "include/test_client.hpp"
#include <iostream>
#include <string.h>
#include <chrono>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <sodium.h>

#ifdef Darwin
#include "/usr/local/opt/libomp/include/omp.h"
#else
#include <omp.h>
#endif

#define N_RUNS 20
#define OPT_L 6
using namespace std;

static long *start_run(uint8_t *message, long len, ChaCha20 c, int opt_n, uint8_t key[32], uint8_t nonce[12], bool decrypt = false){
    long *times = (long *) malloc(sizeof(long) * 2);

    auto start = std::chrono::high_resolution_clock::now();
    if(opt_n == -1){
        crypto_stream_chacha20_ietf_xor_ic(message, message, len, nonce, 1, key);
    } else {
        c.encryptOpt(opt_n, message, len);
    }
    auto end = std::chrono::high_resolution_clock::now();
    times[0] = std::chrono::duration_cast<std::chrono::microseconds>(end-start).count();
    
    if (decrypt){
        start = std::chrono::high_resolution_clock::now();
        if(opt_n == -1){
            crypto_stream_chacha20_ietf_xor_ic(message, message, len, nonce, 1, key);
        } else {
            c.decryptOpt(opt_n, message, len);
        }
        end = std::chrono::high_resolution_clock::now();
        times[1] = std::chrono::duration_cast<std::chrono::microseconds>(end-start).count();
    }

    return times;
}

static bool from_file(char const *argv[]){
    // atoi converts string to int, i need a long
    long len = atol(argv[1]);
    int opt = atoi(argv[3]);
    uint8_t *message = (uint8_t *) aligned_alloc(32, (sizeof(uint8_t) * len + 31) & ~31);

    // Open file
    FILE *fp = fopen(argv[2], "r");
    if(fp == NULL) {
        cout << "Error opening file" << endl;
        return false;
    }

    // Read file into message
    fread(message, sizeof(uint8_t), len, fp);
    cout << "Read file of size " << len << " bytes" << endl;
    fclose(fp);

    //Generate random key and nonce
    uint8_t key[32];
    uint8_t nonce[12];
    for (int i = 0; i < 32; i++) 
        key[i] = rand() % 256;

    for (int i = 0; i < 12; i++) 
        nonce[i] = rand() % 256;

    //Init ChaCha20 - Creation of state
    ChaCha20 chacha = ChaCha20(key, nonce);

    long *time = start_run(message, len, chacha, opt, key, nonce, true);
    cout << "Enc Time: " << time[0] << " microseconds" << endl;
    cout << "Dec Time: " << time[1] << " microseconds" << endl;

    free(time);
    free(message);
    return true;
}

// Convert long to string and add B KB MB GB
static char *parse_len(long len){
    char *str = (char *) malloc(sizeof(char) * 14);
    if(len < 1024){
        sprintf(str, "%d B", (int)len);
    } else if (len < 1024*1024){
        sprintf(str, "%d KB", (int)(len/1024));
    } else if (len < 1024*1024*1024){
        sprintf(str, "%d MB", (int)(len/1024/1024));
    } else {
        sprintf(str, "%d GB", (int)(len/1024/1024/1024));
    }
    return str;
}

static bool multiple_run(const char *argv[], ChaCha20 c, int n_opt, uint8_t key[32], uint8_t nonce[12]){
    long max_len_size = atol(argv[1]);
    int max_th = omp_get_max_threads();
    
    for (int p = -1; p < n_opt; p++){

        // SET THE NUMBER OF THREAD TO USE - IF -1 (ORIGINAL) -> WE JUST RUN 1 THREAD
        int n_threads = max_th;
        if(p == -1){
            n_threads = 1;
            cout << "\n\033[32;1mRunning original version\033[0m\n" << endl;
        } else {
            cout << "\n\033[32;1mRunning optimization " << p << "\033[0m\n"<< endl;
        }

        long start_len = 1024; // 1 KB
        while(start_len <= max_len_size) {
            // write to file output_data/{len}_out - create if not exists
            cout << "Running with len: " << parse_len(start_len) << endl;
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
                
                // Initialize the counter for original version
                if(p == -1){
                    int test = sodium_init();
                    if(test == -1){
                        cout << "Error initializing libsodium" << endl;
                        return false;
                    }
                }

                for(int k = 1; k <= n_threads; k*=2){
                    uint32_t *message = (uint32_t *) aligned_alloc(32, (sizeof(uint32_t) * (start_len / 4) + 31) & ~31);
                    
                    printf("\rRun %d/%d - %d threads - Generating data...", n+1, N_RUNS, k);
                    fflush(stdout);

                    // Generate random message with maximum threads available
                    omp_set_num_threads(max_th);
                    auto start = std::chrono::high_resolution_clock::now();
                    #pragma omp parallel for
                    for (long t = 0; t < (long)start_len/1024; t+=1){
                        int r = rand();
                        int b = rand();
                        for (long i = t*256; i < t*256 + 256; i+=8) {
                            long v = i + 4;
                            r ^= 305419896;
                            *(message + i) = r;
                            b ^= 305419896;
                            *(message + v) = b;
                            r ^= 696739139; 
                            *(message + i + 1) = r;
                            b ^= 696739139;
                            *(message + v + 1) = b;
                            r ^= 525096261;
                            *(message + i + 2) = r;
                            b ^= 525096261;
                            *(message + v + 2) = b;
                            r ^= 123456789;                        
                            *(message + i + 3) = r;
                            b ^= 123456789;                  
                            *(message + v + 3) = b;
                            r += 834567890;
                            b += 834567890;
                        }
                    }
                    auto end = std::chrono::high_resolution_clock::now();
                    gen_time[n*thread_runs + (int)log2(k)] = std::chrono::duration_cast<std::chrono::microseconds>(end-start).count();

                    printf("\rCleaning Line                                           ");
                    printf("\rRun %d/%d - %d threads", n+1, N_RUNS, k);
                    fflush(stdout);

                    omp_set_num_threads(k);
                    long *time = start_run((uint8_t *)message, start_len, c, p, key, nonce);

                    fprintf(fp, "%ld", time[0]);
                    if (k*2 <= n_threads) {
                        fprintf(fp, ",");
                    }
                    free(message);
                    free(time);
                }
                n++;
                fprintf(fp, "\n");
            }
            printf("\rRun %d/%d - \033[32mCompleted\033[0m", N_RUNS, N_RUNS);
            fflush(stdout);

            // Avarage data generation time
            long sum = 0;
            for(int k = 0; k < N_RUNS; k++){
                for(int i = 0; i < thread_runs; i++){
                    sum += gen_time[k*thread_runs + i];
                }
            }
            printf("\nAvarage gen time: %ld", sum/N_RUNS/thread_runs);
            printf("\n\n");

            free(gen_time);
            fclose(fp);

            start_len *= 4;
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
        case 4:
            from_file(argv);
            break;
        case 2:
            multiple_run(argv, chacha, OPT_L, key, nonce);
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
