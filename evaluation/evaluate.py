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
logs_dir = os.path.join(script_dir, "", "logs")

# Create logs directory if it doesn't exist
os.makedirs(logs_dir, exist_ok=True)


def get_datasets():
    """Retrieve all test datasets."""
    return {
        'Tidal': data_client.NOAA_tidal_data('20230601', '20231231', interval='6'),
        'Solar': data_client.open_meteo_data(52.52, 13.41, '2023-01-01', '2023-12-31', 'shortwave_radiation'),
        'Sinusoid': data_client.sinusoid(10000, 1, 1)
    }


import json

def run_algorithm(algo_id, algo_name, data, dataset_name):
    """Run a single algorithm and return results."""
    # Save data
    with open(signal_path, "w") as f:
        for value in data:
            f.write(f"{value}\n")
    
    timing_path = os.path.join(script_dir, "..", "data", "timing.json")
    
    # Create algorithm-specific log file path
    safe_algo_name = algo_name.lower().replace(' ', '_')
    algo_log_path = os.path.join(logs_dir, f"{safe_algo_name}.log")
    
    # Open log file in append mode
    with open(algo_log_path, 'a') as algo_log:
        algo_log.write(f"\n{'='*80}\n")
        algo_log.write(f"Dataset: {dataset_name} ({len(data)} samples)\n")
        algo_log.write(f"Timestamp: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
        algo_log.write(f"{'='*80}\n\n")
        algo_log.flush()
        
        # Run algorithm
        result = subprocess.run(
            ['compare.exe', str(algo_id)], 
            stdout=algo_log,
            stderr=algo_log,  # Also capture stderr in log
            text=True
        )
    
    # Read timing from JSON file
    encode_time = decode_time = error = None
    success = result.returncode == 0
    
    try:
        if os.path.exists(timing_path):
            with open(timing_path, 'r') as f:
                timing_data = json.load(f)
                encode_time = timing_data.get('encode_time_ms')
                decode_time = timing_data.get('decode_time_ms')
                error = timing_data.get('error')
                success = timing_data.get('success', False)
    except (json.JSONDecodeError, IOError) as e:
        print(f"Warning: Could not read timing file: {e}")
    
    input_size = len(data) * 8  # bytes
    code_size = os.path.getsize(code_path)
    ratio = code_size / input_size
    
    return {
        'compressed_size': code_size,
        'ratio': ratio,
        'encode_time': encode_time,
        'decode_time': decode_time,
        'error': error,
        'success': success
    }


def evaluate_all():
    """Run all algorithms on all datasets and return results."""
    algorithms = {
        1: "Sparrow", 
        2: "Gorilla", 
        3: "Zlib", 
        4: "LZ4", 
        5: "Zstandard",
        6: "Sparrow Elf"
    }
    
    # Clear/create fresh log files at the start of each run
    for algo_name in algorithms.values():
        safe_algo_name = algo_name.lower().replace(' ', '_')
        algo_log_path = os.path.join(logs_dir, f"{safe_algo_name}.log")
        with open(algo_log_path, 'w') as f:
            f.write(f"{'='*80}\n")
            f.write(f"Evaluation Run Started: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
            f.write(f"{'='*80}\n")
    
    datasets = get_datasets()
    results = {}
    
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
                result = run_algorithm(algo_id, algo_name, data, dataset_name)
                results[dataset_name][algo_name] = result
                
                if result['encode_time'] is not None and result['decode_time'] is not None:
                    print(f"✓ Ratio: {result['ratio']:.4f}, "
                          f"Time: {result['encode_time']}+{result['decode_time']}ms, "
                          f"Error: {result['error']}")
                else:
                    print(f"✓ Ratio: {result['ratio']:.4f}, Time: FAILED TO PARSE")
            
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
        ratios = []
        for ds in datasets:
            if results[ds][algo]:
                ratios.append(results[ds][algo]['ratio'])
            else:
                ratios.append(0)
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
        times = []
        for ds in datasets:
            if results[ds][algo] and results[ds][algo]['encode_time'] is not None:
                times.append(results[ds][algo]['encode_time'])
            else:
                times.append(0)
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
        times = []
        for ds in datasets:
            if results[ds][algo] and results[ds][algo]['decode_time'] is not None:
                times.append(results[ds][algo]['decode_time'])
            else:
                times.append(0)
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
            if results[ds][algo] and results[ds][algo]['encode_time'] is not None and results[ds][algo]['decode_time'] is not None:
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
    plot_path = os.path.join(script_dir, '..', 'compression_comparison.png')
    plt.savefig(plot_path, dpi=300, bbox_inches='tight')
    plt.show()
    
    print(f"\nVisualization saved as '{plot_path}'")


def print_summary_table(results):
    """Print formatted summary table."""
    for dataset_name, algos in results.items():
        print(f"\n{'='*80}")
        print(f"Dataset: {dataset_name}")
        print(f"{'='*80}")
        print(f"{'Algorithm':<12} {'Ratio':>8} {'Enc(ms)':>9} {'Dec(ms)':>9} {'Total(ms)':>11} {'Error':>7}")
        print(f"{'-'*80}")
        
        for algo_name, result in algos.items():
            if result and result['success'] and result['encode_time'] is not None and result['decode_time'] is not None:
                total = result['encode_time'] + result['decode_time']
                print(f"{algo_name:<12} {result['ratio']:>8.4f} {result['encode_time']:>9} "
                      f"{result['decode_time']:>9} {total:>11} {result['error']:>7}")
            else:
                print(f"{algo_name:<12} {'FAILED':>8}")


if __name__ == "__main__":
    print("Starting compression algorithm evaluation...")
    print(f"Logs will be saved to: {logs_dir}/")
    
    results = evaluate_all()
    
    print_summary_table(results)
    plot_results(results)
    