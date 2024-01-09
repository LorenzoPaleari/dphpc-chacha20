# Read file by file in the folder ./output_data_final/ and plot the graphs
# each file is named: {len_of_inputs}_out
# each file has the following format:
# {n threads}
# {enc_time}/{dec_time},...*{n threads}

import matplotlib.pyplot as plt
import os
import numpy as np

graph_array = {}
std_dev_enc = {}
std_dev_enc_tp = {}

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
original_std_dev = []
original_std_dev_tp = []

files = os.listdir('./output_data_final/')
for file in files:
    if "_out" not in file:
        continue
    
    with open('./output_data_final/' + file, 'r') as f:
        size = file.split('_')[0]
        opt_num = file.split('_')[1][3]

        if int(size) > 1000000000:
            if opt_num == '-': # ORIGINAL!
                arr_enc = []
                #arr_dec = []
                f.readline()
                lines = f.readlines()
                for l in lines:
                    l = l.strip()
                    #enc, dec = l.split('/')
                    arr_enc.append(float(l)/1000000)
                    #arr_dec.append(float(dec)/1000000)
                original_dict[size] = { 'enc': sum(arr_enc) / len(arr_enc), 'tp': sum([int(size) / t / (1024*1024*128) for t in arr_enc]) / len(arr_enc)} # , 'dec': sum(arr_dec) / len(arr_dec) }
                original_std_dev.append(np.std(arr_enc))
                original_std_dev_tp.append(np.std([(int(size) / t) / (1024*1024*128) for t in arr_enc]))
                continue

            cores_label = f.readline().strip().split(',')
            lines = f.readlines()

            mean_dict = {}
            if opt_num not in std_dev_enc:
                std_dev_enc[opt_num] = {}
                std_dev_enc_tp[opt_num] = {}
            mean_dict[size] = { 'enc': [], 'tp': [] } #, 'dec': []}

            for l in lines:
                l = l.strip()
                t_threads = l.split(',')
                t_threads_enc = [ float(t)/1000000 for t in t_threads ]
                # t_threads_dec = [ float(t.split('/')[1])/1000000 for t in t_threads ]
                mean_dict[size]['enc'].append(t_threads_enc)
                mean_dict[size]['tp'].append(t_threads_enc)
                # mean_dict[size]['dec'].append(t_threads_dec)

            std_dev_enc[opt_num][size] = [np.std(t) for t in zip(*mean_dict[size]['enc'])]
            std_dev_enc_tp[opt_num][size] = [np.std([(int(size) / t) / (1024*1024*128) for t in thread]) for thread in zip(*mean_dict[size]['tp'])]
            
            mean_dict[size]['enc'] = [ sum(t) / len(t) for t in zip(*mean_dict[size]['enc']) ]
            mean_dict[size]['tp'] = [ sum([(int(size) / t) / (1024*1024*128) for t in thread]) / len(thread) for thread in zip(*mean_dict[size]['tp']) ]
            # mean_dict[size]['dec'] = [ sum(t) / len(t) for t in zip(*mean_dict[size]['dec']) ]
            
            if opt_num not in graph_array:
                graph_array[opt_num] = {}
            
            graph_array[opt_num][size] = mean_dict[size]

# Plot: each line should be a thread
# points: x = size, y =

optimizations = sorted(graph_array.keys(), key=int)  # sort the optimizations
opt_speedup_enc = {}
    
# opt_speedup_dec = {}

#for all optimizations calculate the speedup
for opt in optimizations:
    #sort the sizes
    sizes = sorted(graph_array[opt].keys(), key=int)

    speedup_enc = []
    # speedup_dec = []

    for i in range(len(graph_array[opt][sizes[0]]['enc'])):  # number of threads
        speedup_enc.append(graph_array[opt][sizes[-1]]['enc'][0] / graph_array[opt][sizes[-1]]['enc'][i])
        # speedup_dec.append(graph_array[opt][sizes[-1]]['dec'][0] / graph_array[opt][sizes[-1]]['dec'][i]) 
    
    opt_speedup_enc[opt] = speedup_enc
    # opt_speedup_dec[opt] = speedup_dec


for opt in optimizations:
    sizes = sorted(graph_array[opt].keys(), key=int)  # sort the sizes
    # Plot for encryption
    plt.figure(figsize=(14, 8.4))
    for i in range(len(graph_array[opt][sizes[0]]['enc'])):  # number of threads
        error = [std_dev_enc[opt][size][i] for size in sizes]
        plt.errorbar([format_size(size) for size in sizes], [graph_array[opt][size]['enc'][i] for size in sizes], yerr=error,fmt='-o',capsize=3.0, label=f'{cores_label[i]} Threads ({opt_speedup_enc[opt][i]:.2f}x) - {graph_array[opt][sizes[-1]]["enc"][i]:.2f}')
    plt.xlabel('Size of Input')
    plt.ylabel('Time (s)')
    plt.title('Encryption Time vs Size of Input for Different Threads')
    plt.legend(loc='upper left')
    plt.grid(True)
    plt.savefig('./output_data_final/enc_time_opt'+ opt +'.png')

    # Plot for decryption
    # plt.figure(figsize=(10, 6))
    # for i in range(len(graph_array[opt][sizes[0]]['dec'])):  # number of threads
    #     plt.plot([format_size(size) for size in sizes], [graph_array[opt][size]['dec'][i] for size in sizes], marker='o', label=f'{cores_label[i]} Threads ({opt_speedup_dec[opt][i]:.2f}x)')
    # plt.xlabel('Size of Input')
    # plt.ylabel('Time (s)')
    # plt.title('Decryption Time vs Size of Input for Different Threads')
    # plt.legend()
    # plt.grid(True)
    # plt.savefig('./output_data_final/dec_time_opt'+ opt +'.png')

# Number of optimizations
num_opts = len(optimizations)
width = 0.2  # width of the bars, adjust as needed
opt_colors = {'2': '#1f77b4',  # Muted blue
              '3': '#2ca02c',  # Muted green
              '4': '#d62728'}  # Muted red
opt_labels = {'2': 'Static', '3': 'Dynamic', '4': 'Guided'}

plt.figure(figsize=(14, 8.4))

# # Iterate over each size
for j, size in enumerate(sizes):
    # For each optimization, plot a bar
    for i, opt in enumerate(optimizations):
        if int(opt) >= 2:
        # Calculate the position for each bar
            x_pos = np.arange(len(sizes)) + i * width

            # Assuming you have a way to get the throughput for the last 'i' for each 'opt' and 'size'
            throughput = graph_array[opt][size]['tp'][-1]  # Replace with your actual data retrieval method

            # Plot the bar
            plt.bar(x_pos[j], throughput, width, color=opt_colors[opt], label=f'{opt_labels[opt]}' if j == 0 else "")

# Set the x-axis labels to be in the middle of each group of bars
plt.xticks(np.arange(len(sizes)) + width, [format_size(size) for size in sizes])

plt.xlabel('Size of Input')
plt.ylabel('Throughput Gb/s')
plt.title('Comparison of Throughput for Different Scheduling Policies')

# Only add the legend if it's necessary
plt.legend()

plt.grid(True)
plt.savefig('./output_data_final/comparison_bar_graph.png')


for opt in optimizations:
        sizes = sorted(graph_array[opt].keys(), key=int)  # sort the sizes
        # Plot for encryption
        plt.figure(figsize=(14, 8.4))
        for i in range(len(graph_array[opt][sizes[0]]['tp'])):  # number of threads
            error = [std_dev_enc_tp[opt][size][i] for size in sizes]
            plt.errorbar([format_size(size) for size in sizes], [graph_array[opt][size]['tp'][i] for size in sizes], yerr=error,fmt='-o',capsize=3.0, label=f'{cores_label[i]} Threads ({opt_speedup_enc[opt][i]:.2f}x) - {max(graph_array[opt][size]["tp"][i] for size in sizes[-4:]):.2f}')
        plt.xlabel('Size of Input')
        plt.ylabel('Throughput Gb/s')
        plt.title('Encryption Throughput vs Size of Input for Different Threads')
        plt.legend(loc='upper left')
        plt.grid(True)
        plt.savefig('./output_data_final/enc_tp_opt'+ opt +'.png')

# Display single thread speedup vs original
lbl = ["enc"] # , "dec"]
for l in lbl:
    plt.figure(figsize=(14, 8.4))
    for opt in optimizations:
        sizes = sorted(graph_array[opt].keys(), key=int)  # sort the sizes
        #plot the first element of each size
        error = [std_dev_enc[opt][size][0] for size in sizes]
        plt.errorbar([format_size(size) for size in sizes], [graph_array[opt][size][l][0] for size in sizes], yerr=error, marker='o',capsize=3.0, label=f'Optimization {opt}')
    plt.errorbar([format_size(size) for size in sizes], [original_dict[size][l] for size in sizes], yerr=original_std_dev, marker='o',capsize=3.0, label=f'Original')
    plt.xlabel('Size of Input')
    plt.ylabel('Time (s)')
    plt.yscale('log')
    plt.title("Encryption Time for Single Thread")
    plt.legend()
    plt.grid(True)
    plt.savefig(f'./output_data_final/{l}_time_single.png')


lbl = ["tp"] # , "dec"]
for l in lbl:
    plt.figure(figsize=(14, 8.4))
    for opt in optimizations:
        sizes = sorted(graph_array[opt].keys(), key=int)  # sort the sizes
        #plot the first element of each size
        error = [std_dev_enc_tp[opt][size][0] for size in sizes]
        plt.errorbar([format_size(size) for size in sizes], [graph_array[opt][size][l][0] for size in sizes], yerr=error, marker='o',capsize=3.0, label=f'Optimization {opt}')
    plt.errorbar([format_size(size) for size in sizes], [original_dict[size][l] for size in sizes], yerr=original_std_dev_tp, marker='o',capsize=3.0, label=f'Original')
    plt.xlabel('Size of Input')
    plt.ylabel('Throughput Gb/s')
    plt.title("Encryption Throughput for Single Thread")
    plt.legend()
    plt.grid(True)
    plt.savefig(f'./output_data_final/enc_{l}_single.png')