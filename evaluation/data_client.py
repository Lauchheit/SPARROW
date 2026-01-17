import math
import requests
from datetime import datetime, timedelta
import pandas as pd
import numpy as np

def generate_white_noise(len, s):
        return np.random.normal(loc=0, scale=s, size=len)

def sinusoid(duration, fs, noise_factor, rounding_factor = 15)->np.array:
    
    samples = int(fs * duration)
    t = np.linspace(0, duration, samples)
    noise = generate_white_noise(len(t), noise_factor)
    x = 1000 * np.sin(2 * np.pi * t * 8.4) + 10000*np.sin(2 * np.pi * t*36.221) + noise + 2367.4
    return [round(v, rounding_factor) for v in x]

def sinusoid_snr(amplitude, N, snr, rounding_factor=15):
    # N samples spanning duration=1.0
    t = np.linspace(0, 1, N, endpoint=False)
    
    f1 = 25  # Cycles per unit time
    f2 = 35  # Cycles per unit time
    
    noise = generate_white_noise(N, amplitude/snr)
    
    x = (amplitude * 0.3 * np.sin(2 * np.pi * f1 * t - 19.5) + 
         amplitude * 0.6 * np.sin(2 * np.pi * f2 * t + 5.3) + 
         amplitude * 0.1 + noise)
    
    return [round(v, rounding_factor) for v in x]

def sinusoid_sparsity(amplitude, N, freqs, S):
    t = np.linspace(0, 1, N, endpoint=False)
    
    af = amplitude / len(freqs)
    noise = generate_white_noise(N, S)
    
    # Sum up all frequency components
    x = np.zeros(N)
    for freq in freqs:
        x += af * np.sin(2 * np.pi * freq * t)
    
    x += noise
    
    return x


def sinusoid_f(A, N, fs, noise_factor):
    t = np.linspace(0, 1, N, endpoint=False)
    noise = generate_white_noise(len(t), noise_factor)
    x = A * np.cos(2 * np.pi * fs * t) + noise
    return x

def sinusoid_random(
    duration,
    fs,
    n_components=5,
    rounding_factor=15,
    freq_range=(0.1, 20.0),
    amp_range=(1000.0, 10000.0),
    phase_range=(0, 2 * np.pi),
    offset_range=(-5_000.0, 5_000.0),
):

    samples = int(duration * fs)
    t = np.linspace(0, duration, samples, endpoint=False)

    x = np.zeros_like(t)

    for _ in range(n_components):
        f = np.random.uniform(*freq_range)
        a = np.random.uniform(*amp_range)
        p = np.random.uniform(*phase_range)
        x += a * np.sin(2 * np.pi * f * t + p)

    offset = np.random.uniform(*offset_range)
    x += offset

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

# https://open-meteo.com/en/docs/satellite-radiation-api
def open_meteo_data(latitude, longitude, start_date, end_date, variable='shortwave_radiation')->np.array:
    url = "https://satellite-api.open-meteo.com/v1/archive"
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

# https://power.larc.nasa.gov/docs/tutorials/api-getting-started/
def nasa_power_solar_irradiance(latitude, longitude, start_date, end_date, temporal_api='hourly')->np.array:
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


def heathrow_temperature(start_date, end_date, temperature_type='temperature_2m')->np.array:
    """
    Fetch historical temperature data from Heathrow weather station
    
    Parameters:
    -----------
    start_date : str
        Start date in format 'YYYY-MM-DD'
    end_date : str
        End date in format 'YYYY-MM-DD'
    temperature_type : str
        Type of temperature data to fetch. Options:
        - 'temperature_2m': 2m air temperature (default)
        - 'temperature_2m_max': Daily maximum temperature
        - 'temperature_2m_min': Daily minimum temperature
        - 'apparent_temperature': Apparent temperature (feels like)
    
    Returns:
    --------
    np.array : Hourly temperature values in Celsius
    """
    # Heathrow coordinates
    latitude = 51.4700
    longitude = -0.4543
    
    url = "https://archive-api.open-meteo.com/v1/archive"
    
    params = {
        'latitude': latitude,
        'longitude': longitude,
        'start_date': start_date,
        'end_date': end_date,
        'hourly': temperature_type,
        'timezone': 'Europe/London'
    }
    
    print(f"Fetching Heathrow temperature data from {start_date} to {end_date}...")
    
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
    
    if temperature_type not in data['hourly']:
        print(f"Variable '{temperature_type}' not found in response")
        print(f"Available variables: {list(data['hourly'].keys())}")
        return None
    
    # Extract temperature values and handle None/missing data
    values = data['hourly'][temperature_type]
    values = [v if v is not None else 0 for v in values]
    
    print(f"Retrieved {len(values)} temperature readings")
    
    return values
