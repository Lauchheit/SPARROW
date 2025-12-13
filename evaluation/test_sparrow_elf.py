import os
import subprocess
from datetime import datetime
import data_client

# Paths
script_dir = os.path.dirname(os.path.abspath(__file__))
data_dir = os.path.join(script_dir, "..", "data")
logs_dir = os.path.join(script_dir, "logs")

signal_path = os.path.join(data_dir, "signal_data.txt")
log_path = os.path.join(logs_dir, "sparrow_elf_test.log")
exe_path = os.path.join(script_dir, "..", "compare.exe")

# Ensure directories exist
os.makedirs(data_dir, exist_ok=True)
os.makedirs(logs_dir, exist_ok=True)

#data = data_client.sinusoid(10000, 1, 1)
#data = data_client.NOAA_tidal_data('20231031', '20231231', interval='6')
#data = [6217.952742246921844, 0.000, 7773.410565557320297, 8845.397666189204756, 9378.329621421606134]
data = data_client.open_meteo_data(52.52, 13.41, '2023-01-01', '2023-12-31', 'shortwave_radiation')
        

# Write signal data
with open(signal_path, "w") as f:
    for v in data:
        f.write(f"{v:.15f}\n")

# Run Sparrow-ELF (algo id = 6)
with open(log_path, "w") as log:
    log.write("=" * 80 + "\n")
    log.write("Sparrow-ELF Zero Dataset Test\n")
    log.write(f"Samples: {len(data)}\n")
    log.write(f"Timestamp: {datetime.now()}\n")
    log.write("=" * 80 + "\n\n")
    log.flush()

    subprocess.run(
        [exe_path, "6"],
        stdout=log,
        stderr=log,
        text=True
    )

print(f"Done. Log written to {log_path}")
