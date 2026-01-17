import subprocess
import os
import numpy as np
import data_client
import sys
from datetime import datetime
import matplotlib.pyplot as plt
import pandas as pd
import evaluate
import math


script_dir = os.path.dirname(os.path.abspath(__file__))
data_dir = os.path.join(script_dir, "..", "data")
signal_path = os.path.join(data_dir, "signal_data.txt")
code_path = os.path.join(data_dir, "code.bin")
exe_path = os.path.join(script_dir, "..", "compare.exe")
logs_dir = os.path.join(script_dir, "", "logs")
algo_name = "Sparrow_Sinusoid"

os.makedirs(data_dir, exist_ok=True)
os.makedirs(logs_dir, exist_ok=True)

try:
    os.remove(os.path.join(logs_dir, f"{algo_name.lower()}.log"))
except: 
    pass

ratios = []
theoretic_ratios = []
for i in range (0,10):
    snr = pow(10,i)
    amplitude = 100
    s = (amplitude/snr)*math.sqrt(2/math.pi)
    lz = math.floor(math.floor(math.log2(2*amplitude/math.pi)) - math.log2(s) -1 )
    theoretic_ratios.append(1 - lz/64)
    data = data_client.sinusoid_snr(amplitude, 10000, snr, 1)
    result = evaluate.run_algorithm(1, algo_name, data, f"Sinusoid SNR: {snr}")
    ratios.append(result['ratio'])


print(ratios)
plt.plot(ratios)
plt.plot(theoretic_ratios)
plt.show()


# return {
#         'compressed_size': code_size,
#         'ratio': ratio,
#         'encode_time': encode_time,
#         'decode_time': decode_time,
#         'error': error,
#         'success': success
#     }