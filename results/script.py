import os

def combine_files(folder1, folder2, output_folder, filename):
    with open(os.path.join(folder1, filename), 'r') as file1, \
         open(os.path.join(folder2, filename), 'r') as file2, \
         open(os.path.join(output_folder, filename), 'w') as outfile:

        # Read lines from both files
        lines1 = file1.readlines()
        lines2 = file2.readlines()

        # Combine and write to the output file
        for line1, line2 in zip(lines1, lines2):
            combined_line = line1.strip() + ',' + line2
            outfile.write(combined_line)

# Directories
dir64 = './output_data_64'
dir128 = './output_data_128'
output_dir = './output_data_final'

# Ensure the output directory exists
os.makedirs(output_dir, exist_ok=True)

# Assuming the file names are the same in both directories
for filename in os.listdir(dir64):
    if filename.endswith('_out'):
        combine_files(dir64, dir128, output_dir, filename)
