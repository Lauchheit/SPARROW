
import requests
import os
import numpy as np
# Paste your access token from Postman
ACCESS_TOKEN = "eyJ0eXAiOiJKV1QiLCJhbGciOiJSUzI1NiIsImtpZCI6IlBjWDk4R1g0MjBUMVg2c0JEa3poUW1xZ3dNVSJ9.eyJhdWQiOiJjNzE3OWFkNy1lYjUwLTRlN2UtOWM1OC1kYmZmYzg3NDZkYzMiLCJpc3MiOiJodHRwczovL2xvZ2luLm1pY3Jvc29mdG9ubGluZS5jb20vMGRmYjc2YzEtMDIzYy00MmZhLWIwMjctNzAxY2U1NjdkNzFlL3YyLjAiLCJpYXQiOjE3NjgyMjkwMDksIm5iZiI6MTc2ODIyOTAwOSwiZXhwIjoxNzY4MjMzNTAxLCJhaW8iOiJBWlFBYS84YkFBQUFpZTBTdDg0bVMvZ3dVTGNjdjZDTE45Y2J5endITnBWOGtsdUEvYVFON3VyNzNFVlJKL05oUS9jRW5Mby9oTnF3VHFia3B0UFJVY0dQTXJZNnp2eTNRdHRINldIdGVPYjVDUktGa2JneE1DZE1yWmtJZUtYZzZ5L0pNZ0pIaThhK2tYaUZuSjZkTis4ZHpRSXFiUGlBYzB1a05TSDBOZWJnQTFjbm4xKzNxZ0JKczcxU3AxVEF2WW1NaEQ0NXNtMkUiLCJhenAiOiJjNzE3OWFkNy1lYjUwLTRlN2UtOWM1OC1kYmZmYzg3NDZkYzMiLCJhenBhY3IiOiIwIiwiaWRwIjoiaHR0cHM6Ly9zdHMud2luZG93cy5uZXQvYmE5OTBkODYtMTM2Ny00YWJkLTk3OWItNmYzMTU0Nzg3Y2E5LyIsImxvZ2luX2hpbnQiOiJPLkNpUmhORFpsWmpBNE1pMDJaVGMxTFRRNE9UUXRZVFpoTWkxaU1qRTVPRGxtWVRCbFlqWVNKR0poT1Rrd1pEZzJMVEV6TmpjdE5HRmlaQzA1TnpsaUxUWm1NekUxTkRjNE4yTmhPUm9RWTJ4bGJXVnVjMEJvWVd0dmJTNWhkQ0RxQVE9PSIsIm5hbWUiOiJDbGVtZW5zIEZyZWkiLCJvaWQiOiJlN2I1OTBjMS04MzM2LTRjYTgtODViMy0yZWRmZTA0ZTQ1NTQiLCJwcmVmZXJyZWRfdXNlcm5hbWUiOiJjbGVtZW5zQGhha29tLmF0IiwicmgiOiIxLkFZSUF3WGI3RFR3Qy1rS3dKM0FjNVdmWEh0ZWFGOGRRNjM1T25GamJfOGgwYmNNNEFjMkNBQS4iLCJzY3AiOiJVc2VyLlJlYWQiLCJzaWQiOiIwMDExMWVlYS0zYWYxLWI4NjQtYzM4MC1kN2NlZDUwODM3ZGQiLCJzdWIiOiI4ZXlCOURJMEZCTWNEa2pxYjJfcUY0Qk5rSVFyOUxVYzlRcnVPc1R2TUxnIiwidGlkIjoiMGRmYjc2YzEtMDIzYy00MmZhLWIwMjctNzAxY2U1NjdkNzFlIiwidXRpIjoidFpteHFBakt3ay0wZC1kZHI4a0xBQSIsInZlciI6IjIuMCIsInhtc19mdGQiOiJIZzNKd1pCWXJoei1weGNnNktWWllFang5ZEJIRVdvTUctR0xSSmJKdUFJQlpYVnliM0JsZDJWemRDMWtjMjF6IiwidGVuYW50SWQiOiI2YmQxZWE4Mi01NGMwLWVlMTEtYjY2MS0wMDIyNDhlNDcyYTUifQ.GhZdKIxiD39iI2dlEagx_hB-lLZdFwGl27dtaomww5Q6qtMpL6hCytWZN3p_F__fGBY1MK-aFsJ4rfmJmhrZQsO-hXtu-uL7-1R59a9pOPYbtlM17hHwWr-B6_McMNrHxEzDx8FKnjTuDb7BWemsHdVw7VxxDGgZEufuoPMZep0FssCgPbNBQGVQzzkt550hoitHjnomGgDxxw4z2pAnBV94tj2oKKpva0s_y-wu3a2ORNDt2gNFA-8KaE2f4a0t8OxzTX7wWpkxAD0huzn9nbMyovpMkpbHygY2Tbsf8e_mqfMw-eopZNuywqRz-1Z3sfgR80kufDgdm9NKXqKJeA"

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