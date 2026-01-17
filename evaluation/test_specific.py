import os
import subprocess
from datetime import datetime
import data_client
import numpy as np
import evaluate
import gorillacompression as gc

def measure_github_gorilla_ratio(encoded_data, original_data):
    # Path for the encoded binary file
    github_code_path = os.path.join(data_dir, "github_gorilla.bin")
    
    # Write encoded data to binary file
    with open(github_code_path, "wb") as f:
        f.write(encoded_data)
    
    # Calculate sizes
    input_size = len(original_data) * 8  # bytes (assuming 8 bytes per float64)
    code_size = os.path.getsize(github_code_path)
    ratio = code_size / input_size
    
    return {
        'compressed_size': code_size,
        'input_size': input_size,
        'ratio': ratio
    }

# Paths
script_dir = os.path.dirname(os.path.abspath(__file__))
data_dir = os.path.join(script_dir, "..", "data")
logs_dir = os.path.join(script_dir, "logs")

signal_path = os.path.join(data_dir, "signal_data.txt")
log_path = os.path.join(logs_dir, "specific_test.log")
exe_path = os.path.join(script_dir, "..", "compare.exe")

# Ensure directories exist
os.makedirs(data_dir, exist_ok=True)
os.makedirs(logs_dir, exist_ok=True)

#data = data_client.sinusoid(10000, 1, 1)
#data = data_client.NOAA_tidal_data('20231031', '20231231', interval='6')
#data = [6217.952742246921844, 0.000, 7773.410565557320297, 8845.397666189204756, 9378.329621421606134]
#data = data_client.open_meteo_data(52.52, 13.41, '2023-01-01', '2023-12-31', 'shortwave_radiation')
#data = [1.443, 1.543, 2.553, 3.332, 3.332, 2.221, 1.234]
data = data_client.sinusoid_random(duration=10000,fs=1,n_components=np.random.randint(3, 8),rounding_factor=15)

# Write signal data
with open(signal_path, "w") as f:
    for v in data:
        f.write(f"{v:.2f}\n")

# Run Sparrow-ELF (algo id = 6)
results1 = evaluate.run_algorithm(2, "Gorilla", data, "Test Data")

results2 = gc.ValuesEncoder.encode_all(data)
#print((results2['encoded']))

github_metrics = measure_github_gorilla_ratio(results2['encoded'], data)

print(f"\nComparison:")
print(f"My implementation: {results1['ratio']:.4f}")
print(f"GitHub implementation: {github_metrics['ratio']:.4f}")

print(f"Done. Log written to {log_path}")
