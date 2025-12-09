import subprocess
import os
import numpy as np
import data_client
import sys
from datetime import datetime
import matplotlib.pyplot as plt
import pandas as pd


script_dir = os.path.dirname(os.path.abspath(__file__))
signal_path = os.path.join(script_dir, "..", "data", "signal_data.txt")
code_path = os.path.join(script_dir, "..", "data", "code.txt")
exe_path = os.path.join(script_dir, "..", "compare.exe")
log_path = os.path.join(script_dir, "..", "data", "cpp_output.log")


def get_datasets():
    """Retrieve all test datasets."""
    return {
        'Tidal': data_client.NOAA_tidal_data('20230601', '20231231', interval='6'),
        'Solar': data_client.open_meteo_data(52.52, 13.41, '2023-01-01', '2023-12-31', 'shortwave_radiation'),
        'Sinusoid': data_client.sinusoid(10000, 1, 1)
    }


def run_algorithm(algo_id, algo_name, data, log_file):
    """Run a single algorithm and return results."""
    # Save data
    with open(signal_path, "w") as f:
        for value in data:
            f.write(f"{value}\n")
    
    log_file.write(f"\n--- {algo_name} Algorithm ---\n")
    log_file.flush()
    
    result = subprocess.run(
        ['compare.exe', str(algo_id)], 
        stdout=log_file, 
        stderr=subprocess.PIPE,
        text=True
    )
    
    if result.stderr:
        log_file.write(result.stderr)
        log_file.flush()
    
    # Parse results
    encode_time = decode_time = error = None
    for line in result.stderr.split('\n'):
        if line.startswith('ENCODE_TIME_MS:'):
            encode_time = int(line.split(':')[1])
        elif line.startswith('DECODE_TIME_MS:'):
            decode_time = int(line.split(':')[1])
        elif line.startswith('RECONSTRUCTION_ERROR:'):
            error = int(line.split(':')[1])
    
    input_size = len(data) * 8  # bytes
    code_size = os.path.getsize(code_path)
    ratio = code_size / input_size
    
    return {
        'compressed_size': code_size,
        'ratio': ratio,
        'encode_time': encode_time,
        'decode_time': decode_time,
        'error': error,
        'success': result.returncode == 0
    }


def evaluate_all():
    """Run all algorithms on all datasets and return results."""
    algorithms = {
        1: "Sparrow", 
        2: "Gorilla", 
        3: "Zlib", 
        4: "LZ4", 
        5: "Zstandard"
    }
    
    datasets = get_datasets()
    results = {}
    
    with open(log_path, "w") as log_file:
        log_file.write(f"\n{'='*60}\n")
        log_file.write(f"Run started at: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
        log_file.write(f"{'='*60}\n\n")
        
        for dataset_name, data in datasets.items():
            if data is None:
                print(f"Failed to retrieve {dataset_name} data")
                continue
            
            print(f"\n{'='*50}")
            print(f"Dataset: {dataset_name} ({len(data)} samples)")
            print(f"{'='*50}")
            
            results[dataset_name] = {}
            
            for algo_id, algo_name in algorithms.items():
                print(f"Running {algo_name}...", end=" ")
                
                try:
                    result = run_algorithm(algo_id, algo_name, data, log_file)
                    results[dataset_name][algo_name] = result
                    
                    print(f"✓ Ratio: {result['ratio']:.4f}, "
                          f"Time: {result['encode_time']}+{result['decode_time']}ms")
                
                except Exception as e:
                    print(f"✗ Error: {e}")
                    results[dataset_name][algo_name] = None
    
    return results


def plot_results(results):
    """Create comprehensive visualization of algorithm performance."""
    fig, axes = plt.subplots(2, 2, figsize=(14, 10))
    fig.suptitle('Compression Algorithm Performance Comparison', fontsize=16, fontweight='bold')
    
    # Prepare data
    datasets = list(results.keys())
    algorithms = list(next(iter(results.values())).keys())
    
    # 1. Compression Ratio by Dataset
    ax = axes[0, 0]
    x = np.arange(len(datasets))
    width = 0.15
    
    for i, algo in enumerate(algorithms):
        ratios = [results[ds][algo]['ratio'] if results[ds][algo] else 0 
                  for ds in datasets]
        ax.bar(x + i*width, ratios, width, label=algo)
    
    ax.set_xlabel('Dataset')
    ax.set_ylabel('Compression Ratio')
    ax.set_title('Compression Ratio (lower is better)')
    ax.set_xticks(x + width * 2)
    ax.set_xticklabels(datasets)
    ax.legend()
    ax.grid(axis='y', alpha=0.3)
    
    # 2. Encoding Speed
    ax = axes[0, 1]
    for i, algo in enumerate(algorithms):
        times = [results[ds][algo]['encode_time'] if results[ds][algo] else 0 
                 for ds in datasets]
        ax.bar(x + i*width, times, width, label=algo)
    
    ax.set_xlabel('Dataset')
    ax.set_ylabel('Time (ms)')
    ax.set_title('Encoding Time (lower is better)')
    ax.set_xticks(x + width * 2)
    ax.set_xticklabels(datasets)
    ax.legend()
    ax.grid(axis='y', alpha=0.3)
    
    # 3. Decoding Speed
    ax = axes[1, 0]
    for i, algo in enumerate(algorithms):
        times = [results[ds][algo]['decode_time'] if results[ds][algo] else 0 
                 for ds in datasets]
        ax.bar(x + i*width, times, width, label=algo)
    
    ax.set_xlabel('Dataset')
    ax.set_ylabel('Time (ms)')
    ax.set_title('Decoding Time (lower is better)')
    ax.set_xticks(x + width * 2)
    ax.set_xticklabels(datasets)
    ax.legend()
    ax.grid(axis='y', alpha=0.3)
    
    # 4. Overall Performance Score (ratio * time)
    ax = axes[1, 1]
    for i, algo in enumerate(algorithms):
        scores = []
        for ds in datasets:
            if results[ds][algo]:
                r = results[ds][algo]
                # Lower is better: ratio * total_time
                score = r['ratio'] * (r['encode_time'] + r['decode_time'])
                scores.append(score)
            else:
                scores.append(0)
        ax.bar(x + i*width, scores, width, label=algo)
    
    ax.set_xlabel('Dataset')
    ax.set_ylabel('Score (ratio × time)')
    ax.set_title('Combined Performance Score (lower is better)')
    ax.set_xticks(x + width * 2)
    ax.set_xticklabels(datasets)
    ax.legend()
    ax.grid(axis='y', alpha=0.3)
    
    plt.tight_layout()
    plt.savefig('compression_comparison.png', dpi=300, bbox_inches='tight')
    plt.show()
    
    print("\nVisualization saved as 'compression_comparison.png'")


def print_summary_table(results):
    """Print formatted summary table."""
    for dataset_name, algos in results.items():
        print(f"\n{'='*80}")
        print(f"Dataset: {dataset_name}")
        print(f"{'='*80}")
        print(f"{'Algorithm':<12} {'Ratio':>8} {'Enc(ms)':>9} {'Dec(ms)':>9} {'Total(ms)':>11} {'Error':>7}")
        print(f"{'-'*80}")
        
        for algo_name, result in algos.items():
            if result and result['success']:
                total = result['encode_time'] + result['decode_time']
                print(f"{algo_name:<12} {result['ratio']:>8.4f} {result['encode_time']:>9} "
                      f"{result['decode_time']:>9} {total:>11} {result['error']:>7}")
            else:
                print(f"{algo_name:<12} {'FAILED':>8}")


if __name__ == "__main__":
    print("Starting compression algorithm evaluation...")
    results = evaluate_all()
    
    print_summary_table(results)
    plot_results(results)
    
    print(f"\nDetailed logs: {log_path}")