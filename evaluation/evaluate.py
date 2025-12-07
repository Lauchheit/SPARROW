import subprocess
import os
import numpy as np
import data_client
import sys


# Get directory of this script
script_dir = os.path.dirname(os.path.abspath(__file__))

# Define paths relative to script
signal_path = os.path.join(script_dir, "..", "data", "signal_data.txt")
code_path = os.path.join(script_dir, "..", "data", "code.txt")
exe_path = os.path.join(script_dir, "..", "compare.exe")


#tidal_data = data_client.NOAA_tidal_data('20230601', '20231231', interval='6')
#solar_data = data_client.open_meteo_data(52.52, 13.41, '2023-01-01', '2023-12-31', 'shortwave_radiation')
sinusoid_data = data_client.sinusoid(1000, 24, 1)

data = sinusoid_data

if data is not None:
    # Extract water levels
    
    print(f"Retrieved {len(data)} data points")
    
    # Save to file for Sparrow
    with open("data/signal_data.txt", "w") as f:
        for value in data:
            f.write(f"{value}\n")
    
    print("Data saved to data/signal_data.txt")
else:
    print("Failed to retrieve data")
    sys.exit()
    

input_size = os.path.getsize(signal_path)

print("=" * 50)
print(f"Input size:  {input_size:>10} bytes")
print("=" * 50)

algorithms = {1: "Sparrow", 2: "Gorilla"}

for (algo_id, algo_name) in algorithms.items():
    result = subprocess.run(
        ['compare.exe', str(algo_id)], 
        stdout=subprocess.DEVNULL,
        stderr=subprocess.PIPE,
        text=True
    )

    # Parse timing and error from stderr
    encode_time = None
    decode_time = None
    error = None
    
    for line in result.stderr.split('\n'):
        if line.startswith('ENCODE_TIME_MS:'):
            encode_time = int(line.split(':')[1])
        elif line.startswith('DECODE_TIME_MS:'):
            decode_time = int(line.split(':')[1])
        elif line.startswith('RECONSTRUCTION_ERROR:'):
            error = int(line.split(':')[1])

    if result.returncode != 0:
        print(f"Error occurred! Exit code: {result.returncode}")
        print(f"Error output:\n{result.stderr}")

    code_size = os.path.getsize(code_path)
    ratio = code_size / input_size
    
    print(f"{algo_name}:      {code_size:>10} bytes  (ratio: {ratio:.4f})")
    print(f"             Encode: {encode_time:>6} ms  |  Decode: {decode_time:>6} ms  |  Error: {error}")

print("=" * 50)