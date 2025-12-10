import os
import subprocess
from datetime import datetime

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

# Create dataset: 100 zeros
data = [1.2134, 0.75,1.1123, 5.2]

print((data))


# Write signal data
with open(signal_path, "w") as f:
    for v in data:
        f.write(f"{v}\n")

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
