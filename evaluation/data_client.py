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


def heathrow_temperature(start_date, end_date, temperature_type='temperature_2m')->np.array:
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


def nasa_horizons_moon_distance(start_date, end_date, step='1h'):
    url = "https://ssd.jpl.nasa.gov/api/horizons.api"
    
    # Moon's NAIF ID is 301, Earth center is 500@399
    params = {
        'format': 'text',
        'COMMAND': '301',  # Moon
        'CENTER': '500@399',  # Earth center
        'START_TIME': f"'{start_date}'",
        'STOP_TIME': f"'{end_date}'",
        'STEP_SIZE': f"'{step}'",
        'QUANTITIES': '20',  # Observer range & range-rate
    }
    
    print(f"Fetching Moon distance data from {start_date} to {end_date}...")
    
    response = requests.get(url, params=params)
    
    if response.status_code != 200:
        print(f"Error: Status code {response.status_code}")
        return None
    
    # Parse the text response
    text = response.text
    
    # Find the data section (between $$SOE and $$EOE markers)
    try:
        start_marker = '$$SOE'
        end_marker = '$$EOE'
        start_idx = text.index(start_marker) + len(start_marker)
        end_idx = text.index(end_marker)
        data_section = text[start_idx:end_idx].strip()
    except ValueError:
        print("Error: Could not find data markers in response")
        return None
    
    # Parse distances (column varies, but range is typically after the date)
    values = []
    for line in data_section.split('\n'):
        if line.strip():
            parts = line.split()
            try:
                # Distance is typically in AU, convert to km
                # Format: JDTDB, Calendar_Date, range(AU), range_rate(AU/day)
                distance_au = float(parts[-2])  # Second to last column
                distance_km = distance_au * 149597870.7  # AU to km
                values.append(distance_km)
            except (ValueError, IndexError):
                continue
    
    print(f"Retrieved {len(values)} distance measurements")
    
    return np.array(values)

def yfinance_stock_prices(symbol='IBM', start_date='2023-01-01', end_date='2024-01-01'):
    try:
        import yfinance as yf
    except ImportError:
        print("Please install yfinance: pip install yfinance")
        return None
    
    print(f"Fetching stock data for {symbol} from {start_date} to {end_date}...")
    
    try:
        stock = yf.Ticker(symbol)
        df = stock.history(start=start_date, end=end_date)
        
        if df.empty:
            print(f"No data retrieved for {symbol}")
            return None
        
        values = df['Close'].values
        
        print(f"Retrieved {len(values)} daily closing prices for {symbol}")
        
        return np.array(values)
    
    except Exception as e:
        print(f"Error fetching stock data: {e}")
        return None

def alphavantage_stock_prices(symbol='IBM', outputsize='compact', api_key=None):
    if api_key is None:
        api_key = "HPB425WN7JNA85NQ"  # Get from https://www.alphavantage.co/support/#api-key
    
    url = "https://www.alphavantage.co/query"
    
    params = {
        'function': 'TIME_SERIES_DAILY',
        'symbol': symbol,
        'outputsize': outputsize,
        'apikey': api_key
    }
    
    print(f"Fetching stock data for {symbol}...")
    
    response = requests.get(url, params=params)
    
    if response.status_code != 200:
        print(f"Error: Status code {response.status_code}")
        return None
    
    data = response.json()
    
    # Check for API errors
    if 'Error Message' in data:
        print(f"API Error: {data['Error Message']}")
        return None
    
    if 'Note' in data:
        print(f"API Limit: {data['Note']}")
        return None
    
    if 'Time Series (Daily)' not in data:
        print("No time series data in response")
        print(data)
        return None
    
    # Extract closing prices and sort by date
    time_series = data['Time Series (Daily)']
    dates = sorted(time_series.keys())
    
    values = [float(time_series[date]['4. close']) for date in dates]
    
    print(f"Retrieved {len(values)} daily closing prices for {symbol}")
    
    return np.array(values)

def usgs_water_levels(site_code='01646500', start_date='2024-01-01', end_date='2024-12-31', parameter='00065'):
    url = "https://waterservices.usgs.gov/nwis/iv/"
    
    params = {
        'format': 'json',
        'sites': site_code,
        'startDT': start_date,
        'endDT': end_date,
        'parameterCd': parameter,
        'siteStatus': 'all'
    }
    
    print(f"Fetching USGS water data for site {site_code} from {start_date} to {end_date}...")
    
    response = requests.get(url, params=params)
    
    if response.status_code != 200:
        print(f"Error: Status code {response.status_code}")
        print(response.text)
        return None
    
    data = response.json()
    
    # Navigate through the JSON structure
    try:
        time_series = data['value']['timeSeries']
        
        if not time_series:
            print("No time series data found for this site/parameter combination")
            return None
        
        # Get the first time series (usually there's only one)
        values_data = time_series[0]['values'][0]['value']
        
        # Extract values
        values = [float(entry['value']) for entry in values_data 
                  if entry['value'] not in ['-999999', 'Ice']]  # Filter out error codes
        
        if not values:
            print("No valid data points found")
            return None
        
        print(f"Retrieved {len(values)} water measurements")
        
        return np.array(values)
    
    except (KeyError, IndexError) as e:
        print(f"Error parsing response: {e}")
        print("Response structure:", data.keys() if isinstance(data, dict) else type(data))
        return None


from matplotlib import pyplot as plt
