import data_client
import evaluate
from matplotlib import pyplot as plt
import numpy as np


def plot_data_length_performance(iter):

    data = data_client.sinusoid_snr(10000, 10000, 5, 15)

    
    
    ratios = []
    data_sizes = []
    for i in range(1, iter + 1):
        size = (len(data) * i) // iter
        data_sizes.append(size)
        
        data_subset = data[:size]
        result = evaluate.run_algorithm(1, "Sparrow", data_subset, "Sinusoid")
        ratio = result["ratio"]
        ratios.append(ratio)
    
    fig, ax = plt.subplots(1,1)
    ax.plot(data_sizes, ratios, linewidth=2, markersize=8)
    ax.set_xlabel('Data Size (number of points)', fontsize=11)
    ax.set_ylabel('Compression Ratio', fontsize=11)
    ax.set_title('Compression Ratio vs Data Size', fontsize=12, fontweight='bold')
    ax.grid(True, alpha=0.3)
    plt.show()


def plot_frequency_bin_performance(iter):


    ratios = []
    frequencies = []
    for i in range(0, iter+1):
        fs = 25 + 5*i/iter
        print(fs)
        data = data_client.sinusoid_f(10000, 10000, fs, 5)
        results = evaluate.run_algorithm(1,"Sparrow",data,"Sinusoid")
        ratio = results["ratio"]
        ratios.append(ratio)
        frequencies.append(fs)
    
    fig, ax = plt.subplots(1,1)
    ax.plot(frequencies, ratios, linewidth=2, markersize=8)
    ax.set_xlabel('Index of Sole Frequency Component (Hz)', fontsize=15)
    ax.set_ylabel('Compression Ratio', fontsize=15)
    ax.set_title('Compression Ratio vs Frequency Bin Alignment', fontsize=20, fontweight='bold')
    ax.grid(True, alpha=0.3)
    plt.show()


def plot_snr_performance(snr_iter, iter):

    snrs = []
    ratios_avg = []
    p = 0
    for i in range(1, snr_iter+1):
        snrs.append(i)
        avg_ratio = 0
        for k in range(0, iter):
            p+=1
            print(f"{p}/{snr_iter*iter}")
            data = data_client.sinusoid_snr(10000, 10000, i, rounding_factor=15)
            results = evaluate.run_algorithm(1,"Sparrow",data,"Sinusoid")
            ratio = results["ratio"]
            avg_ratio += ratio
        ratios_avg.append(avg_ratio/iter)
    
    fig, ax = plt.subplots(1,1)
    ax.plot(snrs, ratios_avg, linewidth=2, markersize=8)
    ax.set_xlabel('SNR', fontsize=20)
    ax.set_ylabel('Compression Ratio', fontsize=20)
    ax.set_title('Compression Ratio vs SNR', fontsize=25, fontweight='bold')
    ax.grid(True, alpha=0.3)
    plt.show()

def plot_sparsity_performance(max_freqs, iter, amplitude=10000, N=10000, S=1):
    num_freqs = []
    ratios_avg = []
    ratios_std = []
    p = 0
    
    for i in range(1, max_freqs + 1):
        num_freqs.append(i)
        ratios_for_this_sparsity = []
        
        for k in range(0, iter):
            p += 1
            print(f"{p}/{max_freqs * iter}")
            
            # Generate random frequencies between 1 and 100
            freqs = np.random.uniform(1, 100, size=i)
            
            data = data_client.sinusoid_sparsity(amplitude, N, freqs, S)
            results = evaluate.run_algorithm(1, "Sparrow", data, "Sinusoid")
            ratio = results["ratio"]
            ratios_for_this_sparsity.append(ratio)
        
        ratios_avg.append(np.mean(ratios_for_this_sparsity))
        ratios_std.append(np.std(ratios_for_this_sparsity))
    
    num_freqs = np.array(num_freqs)
    ratios_avg = np.array(ratios_avg)
    ratios_std = np.array(ratios_std)
    
    # Plot
    fig, ax = plt.subplots(1, 1, figsize=(12, 7))
    
    # Mean line
    ax.plot(num_freqs, ratios_avg, linewidth=2, color='#1f77b4',
            markersize=6, label='Mean', zorder=3)

    
    ax.set_xlabel('Number of Frequency Components', fontsize=20)
    ax.set_ylabel('Compression Ratio', fontsize=20)
    ax.set_title('Compression Ratio vs Signal Sparsity', fontsize=25, fontweight='bold')
    ax.legend(fontsize=12, loc='best')
    ax.grid(True, alpha=0.3, zorder=1)
    
    plt.tight_layout()
    plt.show()

# Example usage:
plot_sparsity_performance(max_freqs=10, iter=3)

