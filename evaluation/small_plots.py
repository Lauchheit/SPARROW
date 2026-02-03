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
    
    fig, ax = plt.subplots(1, 1, figsize=(14, 9))
    ax.plot(data_sizes, ratios, linewidth=3, markersize=8)
    ax.set_xlabel('Data Size (number of points)', fontsize=20, fontweight='bold')
    ax.set_ylabel('Compression Ratio', fontsize=20, fontweight='bold')
    ax.set_title('Compression Ratio vs Data Size', fontsize=22, fontweight='bold', pad=20)
    ax.grid(True, alpha=0.3, linewidth=1.5)
    ax.tick_params(labelsize=18, width=2, length=6)
    plt.tight_layout(pad=2.0)
    plt.show()


def plot_frequency_bin_performance(iter, start, freq_range):

    ratios = []
    frequencies = []
    for i in range(0, iter+1):
        fs = start + freq_range*i/iter
        print(fs)
        data = data_client.sinusoid_f(10000, 10000, fs, 5)
        results = evaluate.run_algorithm(1,"Sparrow",data,"Sinusoid")
        ratio = results["ratio"]
        ratios.append(ratio)
        frequencies.append(fs)
    
    fig, ax = plt.subplots(1, 1, figsize=(14, 9))
    ax.plot(frequencies, ratios, linewidth=2, markersize=8)
    ax.set_xlabel('Index of Sole Frequency Component (Hz)', fontsize=20, fontweight='bold')
    ax.set_ylabel('Compression Ratio', fontsize=20, fontweight='bold')
    ax.set_title('Compression Ratio vs Frequency Bin Alignment', fontsize=22, fontweight='bold', pad=20)
    ax.grid(True, alpha=0.3, linewidth=1.5)
    ax.tick_params(labelsize=18, width=2, length=6)
    plt.tight_layout(pad=2.0)
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
    
    fig, ax = plt.subplots(1, 1, figsize=(14, 9))
    ax.plot(snrs, ratios_avg, linewidth=2, markersize=8)
    ax.set_xlabel('SNR', fontsize=20, fontweight='bold')
    ax.set_ylabel('Compression Ratio', fontsize=20, fontweight='bold')
    ax.set_title('Compression Ratio vs SNR', fontsize=22, fontweight='bold', pad=20)
    ax.grid(True, alpha=0.3, linewidth=1.5)
    ax.tick_params(labelsize=18, width=2, length=6)
    plt.tight_layout(pad=2.0)
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
    fig, ax = plt.subplots(1, 1, figsize=(14, 9))
    
    # Mean line
    ax.plot(num_freqs, ratios_avg, linewidth=2, color='#1f77b4',
            markersize=6, zorder=3)

    
    ax.set_xlabel('Number of Frequency Components', fontsize=20, fontweight='bold')
    ax.set_ylabel('Compression Ratio', fontsize=20, fontweight='bold')
    ax.set_title('Compression Ratio vs Signal Sparsity', fontsize=22, fontweight='bold', pad=20)
    ax.legend(fontsize=18, loc='best')
    ax.grid(True, alpha=0.3, linewidth=1.5, zorder=1)
    ax.tick_params(labelsize=18, width=2, length=6)
    
    plt.tight_layout(pad=2.0)
    plt.show()

def plot_sparse_signal():
    data = data_client.heathrow_temperature('2024-01-01', '2024-05-31')
    N = len(data)

    # FFT
    X = np.fft.fft(data)
    X_mag = np.abs(X)

    # Find indices of 5 largest frequency components
    # Only consider positive frequencies (first half)
    half_N = N // 2
    top_5_indices = np.argsort(X_mag[:half_N])[-5:]
    
    # Create filtered FFT (keep only top 5 frequencies)
    X_filtered = np.zeros_like(X, dtype=complex)
    for idx in top_5_indices:
        X_filtered[idx] = X[idx]
        # Also set negative frequency component for real signal
        if idx > 0:
            X_filtered[N - idx] = X[N - idx]
    
    # Inverse FFT to get reconstructed signal
    reconstructed = np.fft.ifft(X_filtered).real

    # Plot
    fig, axes = plt.subplots(2, 1, figsize=(14, 11))

    # Time domain: original vs reconstructed
    axes[0].plot(data[:1000], linewidth=2.5, label='Original Signal', alpha=0.7)
    axes[0].plot(reconstructed[:1000], linewidth=2.5, label='5-Frequency Approximation', 
                 linestyle='--', alpha=0.9)
    axes[0].set_xlabel('Sample Index', fontsize=20, fontweight='bold')
    axes[0].set_ylabel('Temperature', fontsize=20, fontweight='bold')
    axes[0].set_title('Time Domain: Original vs Sparse Reconstruction', 
                      fontsize=22, fontweight='bold', pad=20)
    axes[0].legend(fontsize=18, loc='best')
    axes[0].grid(True, alpha=0.3, linewidth=1.5)
    axes[0].tick_params(labelsize=18, width=2, length=6)

    # Frequency domain
    axes[1].plot(X_mag[:200], linewidth=2.5, alpha=0.5, label='All Frequencies')
    axes[1].scatter(top_5_indices, X_mag[top_5_indices], color='red', s=150, 
                    zorder=5, label='Top 5 Frequencies', edgecolors='black', linewidths=2)
    axes[1].set_xlabel('Frequency Bin', fontsize=20, fontweight='bold')
    axes[1].set_ylabel('Magnitude', fontsize=20, fontweight='bold')
    axes[1].set_title('Frequency Domain (Sparse)', fontsize=22, fontweight='bold', pad=20)
    axes[1].legend(fontsize=18, loc='best')
    axes[1].grid(True, alpha=0.3, linewidth=1.5)
    axes[1].tick_params(labelsize=18, width=2, length=6)

    plt.tight_layout(pad=3.0)
    plt.show()
    
    print(f"Top 5 frequency bins: {top_5_indices}")
    print(f"Their magnitudes: {X_mag[top_5_indices]}")

def plot_sparse_signal_and_spectrum():
    data = data_client.heathrow_temperature('2024-01-01', '2024-04-31')
    N = len(data)

    # FFT
    X = np.fft.fft(data)
    X_mag = np.abs(X)

    # Plot
    fig, axes = plt.subplots(2, 1, figsize=(14, 11))

    # Time domain: nur original, komplett
    axes[0].plot(data, linewidth=1.5, alpha=0.7)
    axes[0].set_xlabel('Sample Index', fontsize=20, fontweight='bold')
    axes[0].set_ylabel('Temperature', fontsize=20, fontweight='bold')
    axes[0].set_title('Time Domain: Original Signal', 
                      fontsize=22, fontweight='bold', pad=20)
    axes[0].grid(True, alpha=0.3, linewidth=1.5)
    axes[0].tick_params(labelsize=18, width=2, length=6)

    # Frequency domain: komplett, ohne markierte Punkte
    axes[1].plot(X_mag[:N // 2], linewidth=1.5, alpha=0.7)
    axes[1].set_xlabel('Frequency Bin', fontsize=20, fontweight='bold')
    axes[1].set_ylabel('Magnitude', fontsize=20, fontweight='bold')
    axes[1].set_title('Frequency Domain', fontsize=22, fontweight='bold', pad=20)
    axes[1].grid(True, alpha=0.3, linewidth=1.5)
    axes[1].tick_params(labelsize=18, width=2, length=6)

    plt.tight_layout(pad=3.0)
    plt.show()

def plot_decimal_point_performance(iter):

    dcps = []
    sparrow_ratios = []
    sparrow_elf_ratios = []
    max_rounding = 18
    p = 0
    for i in range(1, max_rounding):
        dcps.append(i)
        avg_ratio_sp = 0
        avg_ratio_spe = 0
        for k in range(0, iter):
            p+=1
            print(f"{p}/{max_rounding*iter-1}")
            data = data_client.sinusoid_snr(10000, 10000, 5, rounding_factor=i)
            
            results_sp = evaluate.run_algorithm(1,"Sparrow",data,"Sinusoid")
            ratio_sp = results_sp["ratio"]
            avg_ratio_sp += ratio_sp

            results_spe = evaluate.run_algorithm(6,"Sparrow + Elf",data,"Sinusoid")
            ratio_spe = results_spe["ratio"]
            avg_ratio_spe += ratio_spe

            
        sparrow_ratios.append(avg_ratio_sp/iter)
        sparrow_elf_ratios.append(avg_ratio_spe/iter)
    
    fig, ax = plt.subplots(1, 1, figsize=(14, 9))
    ax.plot(dcps, sparrow_ratios, linewidth=2, markersize=8, label="Sparrow")
    ax.plot(dcps, sparrow_elf_ratios, linewidth=2, markersize=8, label="Sparrow + Elf")
    ax.set_xlabel('Number of digits after decimal point', fontsize=20, fontweight='bold')
    ax.set_ylabel('Compression Ratio', fontsize=20, fontweight='bold')
    ax.set_title('Compression Ratio vs Decimal Count', fontsize=22, fontweight='bold', pad=20)
    ax.legend(fontsize=18)
    ax.grid(True, alpha=0.3, linewidth=1.5)
    ax.tick_params(labelsize=18, width=2, length=6)
    plt.tight_layout(pad=2.0)
    plt.show()

def plot_simple_example():

    fs = 1000
    duration = 1

    data, noise = data_client.sinusoid(duration, fs, 40)

    t = np.linspace(0, duration, fs*duration)
    x = 150 * np.sin(2 * np.pi * t * 14 + 2.5) + 210 * np.sin(2 * np.pi * t * 5 - 8)

    # FFT berechnen
    freqs = np.fft.fft(data)
    n = len(data)
    
    amplitude = np.abs(freqs[:n//2]) * 2 / n
    amplitude[0] = np.abs(freqs[0]) / n
    
    fig, ax = plt.subplots(1, 1, figsize=(14, 9))
    ax.plot(data, label="Noisy Data")
    ax.set_title("Example Signal", fontsize=22, fontweight='bold', pad=20)
    ax.set_xlabel("Time", fontsize=20, fontweight='bold')
    ax.set_ylabel("y", fontsize=20, fontweight='bold')
    ax.grid(True, alpha=0.3, linewidth=1.5)
    ax.tick_params(labelsize=18, width=2, length=6)
    plt.tight_layout(pad=2.0)
    plt.show()

    fig, ax = plt.subplots(1, 1, figsize=(14, 9))
    ax.plot(amplitude)
    ax.set_title("Frequency Domain", fontsize=22, fontweight='bold', pad=20)
    ax.set_xlabel("Frequency Bins", fontsize=20, fontweight='bold')
    ax.set_ylabel("Amplitude", fontsize=20, fontweight='bold')
    
    # Punkte markieren
    ax.scatter([5, 14], [210, 150], color='red', s=100, zorder=5)
    ax.annotate('(5, 210)', xy=(5, 210), xytext=(5+5, 210+0), fontsize=20, color='red', fontweight='bold')
    ax.annotate('(14, 150)', xy=(14, 150), xytext=(14+5, 150+0), fontsize=20, color='red', fontweight='bold')
    ax.grid(True, alpha=0.3, linewidth=1.5)
    ax.tick_params(labelsize=18, width=2, length=6)
    plt.tight_layout(pad=2.0)
    plt.show()

    fig, ax = plt.subplots(1, 1, figsize=(14, 9))
    ax.plot(data, label="Noisy Data")
    ax.set_title("Approximation with 2 Frequency Components", fontsize=22, fontweight='bold', pad=20)
    ax.set_xlabel("Time", fontsize=20, fontweight='bold')
    ax.set_ylabel("y", fontsize=20, fontweight='bold')
    ax.plot(x, linewidth=2.0, color='red', label="Approximation")
    ax.legend(fontsize=18)
    ax.grid(True, alpha=0.3, linewidth=1.5)
    ax.tick_params(labelsize=18, width=2, length=6)
    plt.tight_layout(pad=2.0)
    plt.show()

    fig, ax = plt.subplots(1, 1, figsize=(14, 9))
    ax.hist(noise, bins=(100))
    ax.set_title("Noise Distribution", fontsize=22, fontweight='bold', pad=20)
    ax.set_xlabel("Value", fontsize=20, fontweight='bold')
    ax.set_ylabel("Count", fontsize=20, fontweight='bold')
    ax.grid(True, alpha=0.3, linewidth=1.5)
    ax.tick_params(labelsize=18, width=2, length=6)
    plt.tight_layout(pad=2.0)
    plt.show()

plot_frequency_bin_performance(500,25,2)