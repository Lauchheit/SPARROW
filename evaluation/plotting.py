from matplotlib import pyplot as plt
import numpy as np

def plot_compression_boxplots(results, dataset_names):
    datasets = list(results.keys())
    n_datasets = len(datasets)
    
    # Calculate grid dimensions
    n_cols = min(3, n_datasets)
    n_rows = (n_datasets + n_cols - 1) // n_cols
    
    fig, axes = plt.subplots(n_rows, n_cols, figsize=(6*n_cols, 5*n_rows))
    if n_datasets == 1:
        axes = np.array([axes])
    axes = axes.flatten()
    
    for idx, ds in enumerate(datasets):
        ax = axes[idx]
        algos = results[ds]
        data = []
        labels = []
        title = dataset_names.get(ds, ds)

        for algo, vals in algos.items():
            if len(vals["ratios"]) > 0:
                data.append(vals["ratios"])
                labels.append(algo)

        if data:
            ax.boxplot(data, labels=labels, showfliers=True)
            ax.set_ylabel("Compression Ratio")
            ax.set_title(f"{title}")
            ax.tick_params(axis='x', rotation=45)
            ax.grid(axis="y", linestyle="--", alpha=0.6)
    
    # Hide unused subplots
    for idx in range(n_datasets, len(axes)):
        axes[idx].axis('off')
    
    fig.suptitle("Compression Ratio Distribution", fontsize=16, y=1.00)
    plt.tight_layout()
    plt.show()

def plot_mean_ratios(results, dataset_names):
    datasets = list(results.keys())
    n_datasets = len(datasets)
    
    # Calculate grid dimensions
    n_cols = min(3, n_datasets)
    n_rows = (n_datasets + n_cols - 1) // n_cols
    
    fig, axes = plt.subplots(n_rows, n_cols, figsize=(6*n_cols, 5*n_rows))
    if n_datasets == 1:
        axes = np.array([axes])
    axes = axes.flatten()
    
    for idx, ds in enumerate(datasets):
        ax = axes[idx]
        algos = results[ds]
        alg_names = []
        means = []
        stds = []
        title = dataset_names.get(ds, ds)

        for algo, vals in algos.items():
            r = vals["ratios"]
            if len(r):
                alg_names.append(algo)
                means.append(np.mean(r))
                stds.append(np.std(r))

        if means:
            ax.bar(alg_names, means, yerr=stds, capsize=5)
            ax.set_ylabel("Mean Compression Ratio")
            ax.set_title(f"{title}")
            ax.tick_params(axis='x', rotation=45)
            ax.grid(axis="y", linestyle="--", alpha=0.6)
    
    # Hide unused subplots
    for idx in range(n_datasets, len(axes)):
        axes[idx].axis('off')
    
    fig.suptitle("Mean Compression Ratio ± 1σ", fontsize=16, y=1.00)
    plt.tight_layout()
    plt.show()

def plot_encoding_time(results, dataset_names):
    datasets = list(results.keys())
    n_datasets = len(datasets)
    
    # Calculate grid dimensions
    n_cols = min(3, n_datasets)
    n_rows = (n_datasets + n_cols - 1) // n_cols
    
    fig, axes = plt.subplots(n_rows, n_cols, figsize=(6*n_cols, 5*n_rows))
    if n_datasets == 1:
        axes = np.array([axes])
    axes = axes.flatten()
    
    for idx, ds in enumerate(datasets):
        ax = axes[idx]
        algos = results[ds]
        alg_names = []
        means = []
        title = dataset_names.get(ds, ds)

        for algo, vals in algos.items():
            if len(vals["encode"]):
                alg_names.append(algo)
                means.append(np.mean(vals["encode"]))

        if means:
            ax.bar(alg_names, means)
            ax.set_ylabel("Encoding Time (ms)")
            ax.set_title(f"{title}")
            ax.tick_params(axis='x', rotation=45)
            ax.grid(axis="y", linestyle="--", alpha=0.6)
    
    # Hide unused subplots
    for idx in range(n_datasets, len(axes)):
        axes[idx].axis('off')
    
    fig.suptitle("Mean Encoding Time", fontsize=16, y=1.00)
    plt.tight_layout()
    plt.show()

def plot_decoding_time(results, dataset_names):
    datasets = list(results.keys())
    n_datasets = len(datasets)
    
    # Calculate grid dimensions
    n_cols = min(3, n_datasets)
    n_rows = (n_datasets + n_cols - 1) // n_cols
    
    fig, axes = plt.subplots(n_rows, n_cols, figsize=(6*n_cols, 5*n_rows))
    if n_datasets == 1:
        axes = np.array([axes])
    axes = axes.flatten()
    
    for idx, ds in enumerate(datasets):
        ax = axes[idx]
        algos = results[ds]
        alg_names = []
        means = []
        title = dataset_names.get(ds, ds)

        for algo, vals in algos.items():
            if len(vals["decode"]):
                alg_names.append(algo)
                means.append(np.mean(vals["decode"]))

        if means:
            ax.bar(alg_names, means)
            ax.set_ylabel("Decoding Time (ms)")
            ax.set_title(f"{title}")
            ax.tick_params(axis='x', rotation=45)
            ax.grid(axis="y", linestyle="--", alpha=0.6)
    
    # Hide unused subplots
    for idx in range(n_datasets, len(axes)):
        axes[idx].axis('off')
    
    fig.suptitle("Mean Decoding Time", fontsize=16, y=1.00)
    plt.tight_layout()
    plt.show()