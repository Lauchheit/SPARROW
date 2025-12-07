import subprocess
import os
import numpy as np

# Get directory of this script
script_dir = os.path.dirname(os.path.abspath(__file__))

def generate_white_noise(len, s):
    return np.random.normal(loc=0, scale=s, size=len)

duration = 1200 # days
fs = 24 # measurements per day
samples = int(fs * duration) # number of samples
t = np.linspace(0, duration, samples)
noise = generate_white_noise(len(t), 1)
x = 1000 * np.sin(2 * np.pi * t * 2.4 -19) + 10000*np.sin(2 * np.pi * t/2) + 400*np.cos(2*np.pi * t/12) + noise

# Define paths relative to script
signal_path = os.path.join(script_dir, "..", "data", "signal_data.txt")
code_path = os.path.join(script_dir, "..", "data", "code.txt")
exe_path = os.path.join(script_dir, "..", "compare.exe")

# Write x to file (überschreibt automatisch wenn vorhanden)
with open(signal_path, "w") as f:
    for value in x:
        f.write(f"{value}\n")

input_size = os.path.getsize(signal_path)

print("=" * 50)
print(f"Input size:  {input_size:>10} bytes")
print("=" * 50)

for algo_type in [1, 2]:
    subprocess.run([exe_path, str(algo_type)], capture_output=True)
    code_size = os.path.getsize(code_path)
    ratio = code_size / input_size
    
    algo_name = "Gorilla" if algo_type == 1 else "Sparrow"
    print(f"{algo_name}:      {code_size:>10} bytes  (ratio: {ratio:.4f})")

print("=" * 50)