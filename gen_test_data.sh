echo "Generating test data..."
mkdir -p input_data
base64 -i /dev/urandom | head -c 1000 >input_data/input_1000.txt
base64 -i /dev/urandom | head -c 10000 >input_data/input_10000.txt
base64 -i /dev/urandom | head -c 100000 >input_data/input_100000.txt
base64 -i /dev/urandom | head -c 1000000 >input_data/input_1000000.txt
base64 -i /dev/urandom | head -c 10000000 >input_data/input_10000000.txt
base64 -i /dev/urandom | head -c 100000000 >input_data/input_100000000.txt
base64 -i /dev/urandom | head -c 1000000000 >input_data/input_1000000000.txt
base64 -i /dev/urandom | head -c 5000000000 >input_data/input_5000000000.txt
echo "Done."
