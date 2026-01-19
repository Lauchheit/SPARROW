
import requests
import os
import numpy as np
from dotenv import load_dotenv

ACCESS_TOKEN = os.getenv("DATA_DONKEY")

# Make API requests
headers = {
    'Authorization': f'Bearer {ACCESS_TOKEN}',
    'Content-Type': 'application/json'
}

# Example GET request
response = requests.get('https://api.powertsm.com/api/repositories/dataDonkey/timeseries', headers=headers)

if response.status_code == 200:
    data = response.json()
    
    names = [item['Name'] for item in data]
else:
    print(f"Error: {response.status_code}")
    print(response.text)

path = "D:\\uni_daten\\bachelorarbeit\\data-donkey"
names_path = os.path.join(path, "names.txt")

with open(names_path, "w") as f:
    for name in names:
        f.write(f"{name}\n")

data_path = os.path.join(path, "data")
os.makedirs(data_path, exist_ok=True)

#names = names[:1]
print(len(names))

for name in names:
    request = f"https://api.powertsm.com/api/repositories/dataDonkey/timeseries/{name}/data?from=2016-01-01&to=2026-01-01"
    print(request)
    response = requests.get(request, headers=headers)
    
    if response.status_code != 200:
        print(f"Warning: Failed request on {name}. {response}")
        continue

    data = response.json()

    values = [item["Value"] for item in data["Data"]]

    if np.count_nonzero(values) == 0:
        print(f"Time Series Only Zero: {name}")
        continue

    nonzero = np.nonzero(values)[0]
    result = values[nonzero[0]:nonzero[-1]+1]

    file = os.path.join(data_path, f"{name}.txt")
    with open(file, "w") as f:
        for value in result:
            f.write(f"{value}\n")
        print(f"Successfully Downloaded Data of {name}")