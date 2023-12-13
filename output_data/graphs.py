# Read file by file in the folder ./output_data/ and plot the graphs
# each file is named: {len_of_inputs}_out
# each file has the following format:
# {n threads}
# {enc_time}/{dec_time},...*{n threads}

import matplotlib.pyplot as plt
import os

graph_array = {}

def format_size(size):
    size = int(size)
    # size is assumed to be in bytes
    if size < 1000:
        return f"{size} B"
    elif size < 1000**2:
        return f"{size / 1024} KB"
    elif size < 1000**3:
        return f"{size / 1024**2} MB"
    else:
        return f"{size / 1024**3} GB"

cores_label = []
original_dict = {}

files = os.listdir('./output_data/')
for file in files:
    if "_out" not in file:
        continue
    
    with open('./output_data/' + file, 'r') as f:
        size = file.split('_')[0]
        opt_num = file.split('_')[1][3]
        
        if opt_num == '-': # ORIGINAL!
            arr_enc = []
            arr_dec = []
            f.readline()
            lines = f.readlines()
            for l in lines:
                l = l.strip()
                enc, dec = l.split('/')
                arr_enc.append(float(enc)/1000000)
                arr_dec.append(float(dec)/1000000)
            original_dict[size] = { 'enc': sum(arr_enc) / len(arr_enc), 'dec': sum(arr_dec) / len(arr_dec) }
            continue

        cores_label = f.readline().strip().split(',')
        lines = f.readlines()

        mean_dict = {}
        mean_dict[size] = { 'enc': [], 'dec': []}

        for l in lines:
            l = l.strip()
            t_threads = l.split(',')
            t_threads_enc = [ float(t.split('/')[0])/1000000 for t in t_threads ]
            t_threads_dec = [ float(t.split('/')[1])/1000000 for t in t_threads ]
            mean_dict[size]['enc'].append(t_threads_enc)
            mean_dict[size]['dec'].append(t_threads_dec)
        
        mean_dict[size]['enc'] = [ sum(t) / len(t) for t in zip(*mean_dict[size]['enc']) ]
        mean_dict[size]['dec'] = [ sum(t) / len(t) for t in zip(*mean_dict[size]['dec']) ]
        
        if opt_num not in graph_array:
            graph_array[opt_num] = {}
        
        graph_array[opt_num][size] = mean_dict[size]

# Plot: each line should be a thread
# points: x = size, y = time

optimizations = sorted(graph_array.keys(), key=int)  # sort the optimizations
opt_speedup_enc = {}
opt_speedup_dec = {}

#for all optimizations calculate the speedup
for opt in optimizations:
    #sort the sizes
    sizes = sorted(graph_array[opt].keys(), key=int)

    speedup_enc = []
    speedup_dec = []

    for i in range(len(graph_array[opt][sizes[0]]['enc'])):  # number of threads
        speedup_enc.append(graph_array[opt][sizes[-1]]['enc'][0] / graph_array[opt][sizes[-1]]['enc'][i])
        speedup_dec.append(graph_array[opt][sizes[-1]]['dec'][0] / graph_array[opt][sizes[-1]]['dec'][i]) 
    
    opt_speedup_enc[opt] = speedup_enc
    opt_speedup_dec[opt] = speedup_dec


for opt in optimizations:
    sizes = sorted(graph_array[opt].keys(), key=int)  # sort the sizes
    # Plot for encryption
    plt.figure(figsize=(10, 6))
    for i in range(len(graph_array[opt][sizes[0]]['enc'])):  # number of threads
        plt.plot([format_size(size) for size in sizes], [graph_array[opt][size]['enc'][i] for size in sizes], marker='o', label=f'{cores_label[i]} Threads ({opt_speedup_enc[opt][i]:.2f}x) - {graph_array[opt][sizes[-1]]["enc"][i]}')
    plt.xlabel('Size of Input')
    plt.ylabel('Time (s)')
    plt.title('Encryption Time vs Size of Input for Different Threads')
    plt.legend()
    plt.grid(True)
    plt.savefig('./output_data/enc_time_opt'+ opt +'.png')

    # Plot for decryption
    plt.figure(figsize=(10, 6))
    for i in range(len(graph_array[opt][sizes[0]]['dec'])):  # number of threads
        plt.plot([format_size(size) for size in sizes], [graph_array[opt][size]['dec'][i] for size in sizes], marker='o', label=f'{cores_label[i]} Threads ({opt_speedup_dec[opt][i]:.2f}x)')
    plt.xlabel('Size of Input')
    plt.ylabel('Time (s)')
    plt.title('Decryption Time vs Size of Input for Different Threads')
    plt.legend()
    plt.grid(True)
    plt.savefig('./output_data/dec_time_opt'+ opt +'.png')


# Display single thread speedup vs original
lbl = ["enc", "dec"]
for l in lbl:
    plt.figure(figsize=(10, 6))
    for opt in optimizations:
        sizes = sorted(graph_array[opt].keys(), key=int)  # sort the sizes
        #plot the first element of each size
        plt.plot([format_size(size) for size in sizes], [graph_array[opt][size][l][0] for size in sizes], marker='o', label=f'Optimization {opt}')
    plt.plot([format_size(size) for size in sizes], [original_dict[size][l] for size in sizes], marker='o', label=f'Original')
    plt.xlabel('Size of Input')
    plt.ylabel('Time (s)')
    plt.title("Original vs Optimized Encryption Time for Single Thread")
    plt.legend()
    plt.grid(True)
    plt.savefig(f'./output_data/{l}_time_single.png')