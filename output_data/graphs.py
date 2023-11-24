# Read file by file in the folder ./output_data/ and plot the graphs
# each file is named: {len_of_inputs}_out
# each file has the following format:
# {n threads}
# {enc_time}/{dec_time},...*{n threads}

import matplotlib.pyplot as plt
import os

mean_dict = {}

def format_size(size):
    size = int(size)
    # size is assumed to be in bytes
    if size < 1000:
        return f"{size} B"
    elif size < 1000**2:
        return f"{size / 1000} KB"
    elif size < 1000**3:
        return f"{size / 1000**2} MB"
    else:
        return f"{size / 1000**3} GB"

files = os.listdir('./output_data/')
for file in files:
    if "_out" not in file:
        continue
    
    with open('./output_data/' + file, 'r') as f:
        size = file.split('_')[0]
        n_threads = int(f.readline().strip())
        lines = f.readlines()

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

# Plot: each line should be a thread
# points: x = size, y = time

sizes = sorted(mean_dict.keys(), key=int)  # sort the sizes

# Plot for encryption
plt.figure(figsize=(10, 6))
for i in range(len(mean_dict[sizes[0]]['enc'])):  # number of threads
    plt.plot([format_size(size) for size in sizes], [mean_dict[size]['enc'][i] for size in sizes], marker='o', label=f'{i+1} Threads')
plt.xlabel('Size of Input')
plt.ylabel('Time (s)')
plt.title('Encryption Time vs Size of Input for Different Threads')
plt.legend()
plt.grid(True)
plt.savefig('./output_data/enc_time.png')

# Plot for decryption
plt.figure(figsize=(10, 6))
for i in range(len(mean_dict[sizes[0]]['dec'])):  # number of threads
    plt.plot([format_size(size) for size in sizes], [mean_dict[size]['dec'][i] for size in sizes], marker='o', label=f'{i+1} Threads')
plt.xlabel('Size of Input')
plt.ylabel('Time (s)')
plt.title('Decryption Time vs Size of Input for Different Threads')
plt.legend()
plt.grid(True)
plt.savefig('./output_data/dec_time.png')