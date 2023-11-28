echo "Generating test data..."
mkdir -p input_data
base64 -i /dev/urandom | head -c 1024 >input_data/input_1024.txt
base64 -i /dev/urandom | head -c 4096 >input_data/input_4096.txt
base64 -i /dev/urandom | head -c 16384 >input_data/input_16384.txt
base64 -i /dev/urandom | head -c 65536 >input_data/input_65536.txt
base64 -i /dev/urandom | head -c 262144 >input_data/input_262144.txt
base64 -i /dev/urandom | head -c 1048576 >input_data/input_1048576.txt
base64 -i /dev/urandom | head -c 4194304 >input_data/input_4194304.txt
base64 -i /dev/urandom | head -c 16777216 >input_data/input_16777216.txt
base64 -i /dev/urandom | head -c 67108864 >input_data/input_67108864.txt
base64 -i /dev/urandom | head -c 268435456 >input_data/input_268435456.txt
base64 -i /dev/urandom | head -c 1073741824 >input_data/input_1073741824.txt
base64 -i /dev/urandom | head -c 4294967296 >input_data/input_4294967296.txt
echo "\033[1;32mDone.\033[0m"
