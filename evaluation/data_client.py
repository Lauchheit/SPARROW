from math import floor
import requests
from datetime import datetime, timedelta
import pandas as pd
import numpy as np

def sinusoid(duration, fs, noise_factor, rounding_factor = 15)->np.array:
    def generate_white_noise(len, s):
        return np.random.normal(loc=0, scale=s, size=len)
    
    samples = int(fs * duration)
    t = np.linspace(0, duration, samples)
    noise = generate_white_noise(len(t), noise_factor)
    x = 1000 * np.sin(2 * np.pi * t * 8 -19) + 10000*np.sin(2 * np.pi * t*2) + 4000*np.cos(2*np.pi * 5 * t) + noise + 2367
    return [round(v, rounding_factor) for v in x]


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


def nasa_power_solar_irradiance(latitude, longitude, start_date, end_date, temporal_api='hourly')->np.array:
    """
    Solar irradiance data - strong daily periodicity
    EXCELLENT for Gorilla when using hourly data: Very regular, dense measurements
    """
    url = "https://power.larc.nasa.gov/api/temporal/hourly/point"
    
    params = {
        'parameters': 'ALLSKY_SFC_SW_DWN',
        'community': 'RE',
        'longitude': longitude,
        'latitude': latitude,
        'start': start_date.replace('-', ''),
        'end': end_date.replace('-', ''),
        'format': 'JSON'
    }
    
    if temporal_api == 'hourly':
        url = "https://power.larc.nasa.gov/api/temporal/hourly/point"
    
    print(f"Fetching solar irradiance data from {start_date} to {end_date}...")
    response = requests.get(url, params=params)
    
    if response.status_code != 200:
        print(f"Error: Status code {response.status_code}")
        return None
    
    data = response.json()
    
    if 'properties' not in data or 'parameter' not in data['properties']:
        print(f"Error in response: {data}")
        return None
    
    # Extract irradiance values
    irradiance_data = data['properties']['parameter']['ALLSKY_SFC_SW_DWN']
    
    # Convert to list, handling missing values
    values = []
    for timestamp in sorted(irradiance_data.keys()):
        val = irradiance_data[timestamp]
        values.append(val if val != -999 else 0)
    
    print(f"Retrieved {len(values)} data points")
    return values


def openaq_air_quality(city='Los Angeles', parameter='pm25', date_from='2024-01-01', date_to='2024-01-31')->np.array:
    """
    Air quality measurements - daily/weekly periodicity (traffic patterns)
    Gorilla performance: Good if high-frequency measurements available
    """
    url = "https://api.openaq.org/v2/measurements"
    
    params = {
        'city': city,
        'parameter': parameter,
        'date_from': date_from,
        'date_to': date_to,
        'limit': 10000
    }
    
    print(f"Fetching air quality data for {city}...")
    response = requests.get(url, params=params)
    
    if response.status_code != 200:
        print(f"Error: Status code {response.status_code}")
        return None
    
    data = response.json()
    
    if 'results' not in data:
        print(f"Error in response: {data}")
        return None
    
    # Sort by time and extract values
    results = sorted(data['results'], key=lambda x: x['date']['utc'])
    values = [r['value'] for r in results]
    
    print(f"Retrieved {len(values)} data points")
    return values
