import os
import numpy as np
from datetime import datetime, timedelta

import data_client
import plotting
from evaluate import run_algorithm

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))

ROUNDING_DIGIT = 2

# ---------------------------------------------------------------------
# Dataset generation
# ---------------------------------------------------------------------

def random_lat_lon():
    lat = np.random.uniform(-60, 60)     # avoid polar night edge cases
    lon = np.random.uniform(-180, 180)
    return lat, lon

def generate_overlapping_intervals(start, end, window_days, n):
    start_dt = datetime.strptime(start, "%Y%m%d")
    end_dt = datetime.strptime(end, "%Y%m%d")

    total_days = (end_dt - start_dt).days
    step = max(1, (total_days - window_days) // max(1, n - 1))

    intervals = []
    for i in range(n):
        s = start_dt + timedelta(days=i * step)
        e = min(s + timedelta(days=window_days), end_dt)
        intervals.append((s.strftime("%Y%m%d"), e.strftime("%Y%m%d")))

    return intervals


def get_datasets(n_runs):
    datasets = {
        "Sinusoid": [],
        "Sinusoid Rounded": [],
        "Tidal": [],
        "Solar 1": [],
        "Solar 2": [],
        "Heathrow Temperature": [],
        "Moon Distance": [],
        "Stock Prices": [],
        "Water Levels": []
    }

    dataset_names = {
        "Sinusoid": "Sinusoid",
        "Sinusoid Rounded": f"Sinusoid Rounded ({ROUNDING_DIGIT} Digits)",
        "Tidal": "Tidal",
        "Solar 1": "Solar Satellite Radiation (Open-Meteo)",
        "Solar 2": "Solar Irradiance (NASA)",
        "Heathrow Temperature": "Heathrow Temperature",
        "Moon Distance": "Moon Distance (NASA HORIZONS)",
        "Stock Prices": "Stock Prices (Yahoo Finance)",
        "Water Levels": "Water Levels (USGS)"
    }

    # Popular stocks with different volatility patterns
    stock_symbols = ['AAPL', 'MSFT', 'GOOGL', 'AMZN', 'TSLA', 'JPM', 'JNJ', 'WMT', 'V', 'PG']
    
    # USGS sites with tidal influence (periodic!)
    usgs_sites = [
        '01646500',  # Potomac River at Little Falls, DC
        '01474500',  # Delaware River at Philadelphia, PA
        '02171650',  # Lake Moultrie near Pineville, SC
        '08574680',  # Trinity River at Dallas, TX
        '14211720',  # Willamette River at Portland, OR
    ]

    # --- Tidal (overlapping 60-day windows)
    tidal_intervals = generate_overlapping_intervals(
        "20230101", "20231231", window_days=60, n=n_runs
    )
    for s, e in tidal_intervals:
        data = data_client.NOAA_tidal_data(s, e, interval="6")
        if data is not None:
            datasets["Tidal"].append(data)

    # --- Solar 1 (Open-Meteo, overlapping 90-day windows)
    solar1_intervals = generate_overlapping_intervals(
        "20230101", "20231231", window_days=90, n=n_runs
    )

    for s, e in solar1_intervals:
        lat, lon = random_lat_lon()

        s_iso = f"{s[:4]}-{s[4:6]}-{s[6:]}"
        e_iso = f"{e[:4]}-{e[4:6]}-{e[6:]}"

        data = data_client.open_meteo_data(
            lat, lon, s_iso, e_iso, "shortwave_radiation"
        )
        if data is not None:
            datasets["Solar 1"].append(data)

    # --- Solar 2 (NASA POWER)
    for s, e in solar1_intervals:
        lat, lon = random_lat_lon()

        s_iso = f"{s[:4]}-{s[4:6]}-{s[6:]}"
        e_iso = f"{e[:4]}-{e[4:6]}-{e[6:]}"

        data = data_client.nasa_power_solar_irradiance(
            lat, lon, s_iso, e_iso, "hourly"
        )
        if data is not None:
            datasets["Solar 2"].append(data)

    # --- Heathrow Temperature (overlapping 180-day/6-month windows)
    heathrow_intervals = generate_overlapping_intervals(
        "20230101", "20231231", window_days=180, n=n_runs
    )
    
    for s, e in heathrow_intervals:
        s_iso = f"{s[:4]}-{s[4:6]}-{s[6:]}"
        e_iso = f"{e[:4]}-{e[4:6]}-{e[6:]}"
        
        data = data_client.heathrow_temperature(s_iso, e_iso, "temperature_2m")
        if data is not None:
            datasets["Heathrow Temperature"].append(data)

    # --- Moon Distance (overlapping 365-day/1-year windows)
    moon_intervals = generate_overlapping_intervals(
        "20200101", "20231231", window_days=365, n=n_runs
    )
    
    for s, e in moon_intervals:
        s_iso = f"{s[:4]}-{s[4:6]}-{s[6:]}"
        e_iso = f"{e[:4]}-{e[4:6]}-{e[6:]}"
        
        data = data_client.nasa_horizons_moon_distance(s_iso, e_iso, step='6h')
        if data is not None:
            datasets["Moon Distance"].append(data)

    # --- Stock Prices (overlapping 180-day windows, different stocks)
    stock_intervals = generate_overlapping_intervals(
        "20230101", "20231231", window_days=180, n=n_runs
    )
    
    for i, (s, e) in enumerate(stock_intervals):
        s_iso = f"{s[:4]}-{s[4:6]}-{s[6:]}"
        e_iso = f"{e[:4]}-{e[4:6]}-{e[6:]}"
        
        # Cycle through different stocks
        symbol = stock_symbols[i % len(stock_symbols)]
        
        data = data_client.yfinance_stock_prices(symbol, s_iso, e_iso)
        if data is not None:
            datasets["Stock Prices"].append(data)

    # --- Water Levels (overlapping 60-day windows, different sites)
    water_intervals = generate_overlapping_intervals(
        "20230101", "20231231", window_days=60, n=n_runs
    )
    
    for i, (s, e) in enumerate(water_intervals):
        s_iso = f"{s[:4]}-{s[4:6]}-{s[6:]}"
        e_iso = f"{e[:4]}-{e[4:6]}-{e[6:]}"
        
        # Cycle through different USGS sites
        site = usgs_sites[i % len(usgs_sites)]
        
        # Use instantaneous values (15-min intervals) for strong periodicity
        data = data_client.usgs_water_levels(site, s_iso, e_iso, '00065')
        if data is not None:
            datasets["Water Levels"].append(data)

    # --- Sinusoid (different, random frequencies)
    for _ in range(n_runs):
        datasets["Sinusoid"].append(
            data_client.sinusoid_random(
                duration=10_000,
                fs=1,
                n_components=np.random.randint(3, 8),
                rounding_factor=15
            )
        )

    for _ in range(n_runs):
        datasets["Sinusoid Rounded"].append(
            data_client.sinusoid_random(
                duration=10_000,
                fs=1,
                n_components=np.random.randint(3, 8),
                rounding_factor=ROUNDING_DIGIT
            )
        )

    return datasets, dataset_names


# ---------------------------------------------------------------------
# Evaluation
# ---------------------------------------------------------------------

def evaluate_statistical(n_runs=10):
    algorithms = {
        1: "Sparrow",
        2: "Gorilla",
        3: "Zlib",
        4: "LZ4",
        5: "Zstandard",
        6: "Sparrow Elf",
        7: "Gorilla Elf"
    }

    datasets, dataset_names = get_datasets(n_runs)

    results = {
        ds: {
            algo: {
                "ratios": [],
                "encode": [],
                "decode": []
            }
            for algo in algorithms.values()
        }
        for ds in datasets.keys()
    }

    for ds_name, runs in datasets.items():
        print(f"\n{'='*80}")
        print(f"{ds_name} ({len(runs)} runs)")
        print(f"{'='*80}")

        for i, data in enumerate(runs):
            for algo_id, algo_name in algorithms.items():
                r = run_algorithm(algo_id, algo_name, data, f"{ds_name}_run{i}")
                if r["success"]:
                    results[ds_name][algo_name]["ratios"].append(r["ratio"])
                    results[ds_name][algo_name]["encode"].append(r["encode_time"])
                    results[ds_name][algo_name]["decode"].append(r["decode_time"])

    return results, dataset_names


# ---------------------------------------------------------------------
# Simple statistical summary
# ---------------------------------------------------------------------

def print_summary(results):
    for ds, algos in results.items():
        print(f"\nDataset: {ds}")
        print("-" * 70)
        print(f"{'Algorithm':<15} {'Mean':>8} {'Std':>8} {'N':>4}")
        for algo, vals in algos.items():
            r = vals["ratios"]
            if len(r):
                print(f"{algo:<15} {np.mean(r):>8.4f} {np.std(r):>8.4f} {len(r):>4}")


if __name__ == "__main__":
    N = 10
    results, dataset_names = evaluate_statistical(N)
    print_summary(results)

    plotting.plot_compression_boxplots(results, dataset_names)
    plotting.plot_mean_ratios(results, dataset_names)
    plotting.plot_encoding_time(results, dataset_names)
    plotting.plot_decoding_time(results, dataset_names)
