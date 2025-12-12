from math import floor
import requests
from datetime import datetime, timedelta
import pandas as pd
import numpy as np


def sinusoid(duration, fs, noise_factor)->np.array:
    def generate_white_noise(len, s):
        return np.random.normal(loc=0, scale=s, size=len)
    
    samples = int(fs * duration)
    t = np.linspace(0, duration, samples)
    noise = generate_white_noise(len(t), noise_factor)
    x = 1000 * np.sin(2 * np.pi * t * 8 -19) + 10000*np.sin(2 * np.pi * t*2) + 4000*np.cos(2*np.pi * 5 * t) + noise + 2367
    return [floor(v*100)/100 for v in x]


def NOAA_tidal_data(start_date, end_date, interval='6')->np.array:
    url = "https://api.tidesandcurrents.noaa.gov/api/prod/datagetter"
    
    # Convert to datetime objects
    start = datetime.strptime(start_date, '%Y%m%d')
    end = datetime.strptime(end_date, '%Y%m%d')
    
    all_data = []
    current_start = start
    
    # Split into 30-day chunks
    while current_start < end:
        current_end = min(current_start + timedelta(days=30), end)
        
        params = {
            'station': '8454000',
            'begin_date': current_start.strftime('%Y%m%d'),
            'end_date': current_end.strftime('%Y%m%d'),
            'product': 'water_level',
            'datum': 'MLLW',
            'units': 'metric',
            'time_zone': 'gmt',
            'format': 'json',
            'interval': interval
        }
        
        print(f"Fetching data from {current_start.strftime('%Y-%m-%d')} to {current_end.strftime('%Y-%m-%d')}...")
        
        response = requests.get(url, params=params)
        
        if response.status_code != 200:
            print(f"Error: Status code {response.status_code}")
            print(response.text)
            return None
        
        data = response.json()
        
        if 'error' in data:
            print(f"API Error: {data['error']}")
            return None
        
        if 'data' not in data:
            print("No 'data' property in JSON response")
            print(data)
            return None
        
        # Extract water levels
        chunk_data = [float(entry['v']) for entry in data['data']]
        all_data.extend(chunk_data)
        
        print(f"  Retrieved {len(chunk_data)} points")
        
        # Move to next chunk
        current_start = current_end + timedelta(days=1)
    
    return all_data


import requests

def open_meteo_data(latitude, longitude, start_date, end_date, variable='shortwave_radiation')->np.array:
    url = "https://archive-api.open-meteo.com/v1/archive"
    params = {
        'latitude': latitude,
        'longitude': longitude,
        'start_date': start_date,
        'end_date': end_date,
        'hourly': variable,
        'timezone': 'UTC'
    }
    
    print(f"Fetching {variable} data from {start_date} to {end_date}...")
    
    response = requests.get(url, params=params)
    
    if response.status_code != 200:
        print(f"Error: Status code {response.status_code}")
        print(response.text)
        return None
    
    data = response.json()
    
    if 'hourly' not in data:
        print("No 'hourly' property in JSON response")
        print(data)
        return None
    
    if variable not in data['hourly']:
        print(f"Variable '{variable}' not found in response")
        print(f"Available variables: {list(data['hourly'].keys())}")
        return None
    
    # Extract values and replace None with 0
    values = data['hourly'][variable]
    values = [v if v is not None else 0 for v in values]
    
    print(f"Retrieved {len(values)} data points")
    
    return values

import pyedflib
import urllib.request
import os
def physionet_eeg_data(patient='chb01', record='chb01_03', duration_minutes=60)->np.array:
    try:
        url = f"https://physionet.org/files/chbmit/1.0.0/{patient}/{record}.edf"
        local_file = f"{record}.edf"
        
        print(f"Downloading {record}.edf...")
        urllib.request.urlretrieve(url, local_file)
        
        f = pyedflib.EdfReader(local_file)
        n_channels = f.signals_in_file
        signal = f.readSignal(0)  # First channel
        
        # Truncate to requested duration (256 Hz)
        max_samples = int(duration_minutes * 60 * 256)
        signal = signal[:max_samples]
        
        f.close()
        os.remove(local_file)
        
        print(f"Retrieved {len(signal)} samples")
        return signal
        
    except Exception as e:
        print(f"Error: {e}")
        print("Install pyedflib: pip install pyedflib")
        return None