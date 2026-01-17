import subprocess
import os
import numpy as np
import matplotlib.pyplot as plt
from pathlib import Path
import json
from datetime import datetime


def run_algorithm(algo_id, algo_name, data, dataset_name, signal_path, code_path, exe_path, timing_path):
    """Run a single algorithm and return results."""
    
    # Save data
    with open(signal_path, "w") as f:
        for value in data:
            f.write(f"{value}\n")
    
    # Clean up old files
    for path in [code_path, timing_path]:
        if os.path.exists(path):
            os.remove(path)
    
    # Run algorithm
    result = subprocess.run(
        [exe_path, str(algo_id)], 
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
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
    
    if not os.path.exists(code_path):
        print(f"Warning: Code file not created for {algo_name} on {dataset_name}")
        return None
    
    input_size = len(data) * 8  # bytes (assuming double precision)
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


def load_data_file(filepath):
    """Load data from a single file."""
    data = []
    try:
        with open(filepath, 'r') as f:
            for line in f:
                line = line.strip()
                if line:
                    try:
                        value = float(line.replace(',', '.'))
                        data.append(value)
                    except ValueError:
                        continue
        return data
    except Exception as e:
        print(f"Error loading {filepath}: {e}")
        return None


def evaluate_folder(data_folder_path, output_plot_path='compression_ratios.png'):
    """
    Evaluate all data files in a folder with all compression algorithms.
    
    Args:
        data_folder_path: Path to folder containing data files
        output_plot_path: Path where the plot should be saved
    """
    
    # Setup paths
    script_dir = os.path.dirname(os.path.abspath(__file__))
    data_dir = os.path.join(script_dir, "..", "data")
    signal_path = os.path.join(data_dir, "signal_data.txt")
    code_path = os.path.join(data_dir, "code.bin")
    exe_path = os.path.join(script_dir, "..", "compare.exe")
    timing_path = os.path.join(script_dir, "..", "data", "timing.json")
    
    # Create data directory if needed
    os.makedirs(data_dir, exist_ok=True)
    
    # Define algorithms
    algorithms = {
        1: "Sparrow", 
        2: "Gorilla", 
        3: "Zlib", 
        4: "LZ4", 
        5: "Zstandard",
        6: "Sparrow Elf",
        7: "Gorilla Elf"
    }
    
    # Find all data files
    data_folder = Path(data_folder_path)
    data_files = []
    
    for ext in ['*.txt', '*.csv', '*.dat']:
        data_files.extend(list(data_folder.glob(ext)))
    
    if not data_files:
        print(f"No data files found in {data_folder_path}")
        return
    
    print(f"Found {len(data_files)} data files")
    
    # Storage for results
    results = {algo_name: [] for algo_name in algorithms.values()}
    file_names = []
    
    # Process each file
    for i, filepath in enumerate(data_files):
        print(f"\n[{i+1}/{len(data_files)}] Processing: {filepath.name}")
        
        # Load data
        data = load_data_file(filepath)
        if data is None or len(data) == 0:
            print(f"  Skipping (no valid data)")
            continue
        
        file_names.append(filepath.stem)
        print(f"  Loaded {len(data)} data points")
        
        # Run each algorithm
        for algo_id, algo_name in algorithms.items():
            try:
                result = run_algorithm(
                    algo_id, algo_name, data, filepath.stem,
                    signal_path, code_path, exe_path, timing_path
                )
                
                if result and result['success']:
                    results[algo_name].append(result['ratio'])
                    print(f"  {algo_name}: ratio={result['ratio']:.4f}")
                else:
                    results[algo_name].append(None)
                    print(f"  {algo_name}: FAILED")
                    
            except Exception as e:
                print(f"  {algo_name}: ERROR - {e}")
                results[algo_name].append(None)
    
    # Create scatter plot
    print(f"\nCreating plot...")
    
    plt.figure(figsize=(12, 8))
    
    algo_names = list(algorithms.values())
    x_positions = np.arange(len(algo_names))
    
    # Add some random jitter to x-coordinates for better visibility
    np.random.seed(42)  # For reproducibility
    jitter_amount = 0.15
    
    colors = plt.cm.tab10(np.linspace(0, 1, len(algo_names)))
    
    for i, algo_name in enumerate(algo_names):
        ratios = [r for r in results[algo_name] if r is not None]
        
        if ratios:
            # Add jitter to x-coordinates
            x_coords = np.random.normal(i, jitter_amount, len(ratios))
            
            plt.scatter(x_coords, ratios, 
                       alpha=0.6, 
                       s=50,
                       color=colors[i],
                       label=algo_name,
                       edgecolors='black',
                       linewidths=0.5)
    
    plt.xlabel('Algorithm', fontsize=12, fontweight='bold')
    plt.ylabel('Compression Ratio', fontsize=12, fontweight='bold')
    plt.title('Compression Ratios Across All Datasets\n(lower is better)', 
              fontsize=14, fontweight='bold')
    plt.xticks(x_positions, algo_names, rotation=45, ha='right')
    plt.legend(loc='best', fontsize=10)
    plt.grid(axis='y', alpha=0.3, linestyle='--')
    
    plt.tight_layout()
    plt.savefig(output_plot_path, dpi=300, bbox_inches='tight')
    print(f"Plot saved to: {output_plot_path}")
    
    plt.show()
    
    # Print summary statistics
    print("\n" + "="*80)
    print("SUMMARY STATISTICS")
    print("="*80)
    
    for algo_name in algorithms.values():
        ratios = [r for r in results[algo_name] if r is not None]
        if ratios:
            print(f"\n{algo_name}:")
            print(f"  Mean ratio:   {np.mean(ratios):.4f}")
            print(f"  Median ratio: {np.median(ratios):.4f}")
            print(f"  Min ratio:    {np.min(ratios):.4f}")
            print(f"  Max ratio:    {np.max(ratios):.4f}")
            print(f"  Std dev:      {np.std(ratios):.4f}")
            print(f"  Success rate: {len(ratios)}/{len(results[algo_name])}")
        else:
            print(f"\n{algo_name}: No successful compressions")
    
    return results

if __name__ == "__main__":
    import sys
    
    
    data_folder = "D:\\uni_daten\\bachelorarbeit\\data-donkey\\data"
    output_plot = 'compression_ratios_data_donkey.png'
    
    if not os.path.exists(data_folder):
        print(f"Error: Folder '{data_folder}' does not exist")
        sys.exit(1)
    
    print(f"Starting evaluation of folder: {data_folder}")
    print(f"Output plot will be saved to: {output_plot}")
    print(f"Timestamp: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    print("="*80 + "\n")
    
    results = evaluate_folder(data_folder, output_plot)