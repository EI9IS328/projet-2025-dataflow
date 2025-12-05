import numpy as np
import re


filename = "../data/snapshot/snapshot_150_order2.txt" 

match = re.search(r"order(\d+)", filename)
order = int(match.group(1)) if match else None


start_line = 1275
Y_lines = 51


data = []
with open(filename, 'r') as f:
    for i, line in enumerate(f):
        row = [float(x) for x in line.strip().split()]
        data.append(row)

data_array = np.array(data)

max_value = np.max(data_array)
min_value = np.min(data_array)
mean_value = np.mean(data_array)
variance_value = np.var(data_array)

print(f"Statistiques des données :")
print(f"Maximum : {max_value}")
print(f"Minimum : {min_value}")
print(f"Moyenne : {mean_value}")
print(f"Variance : {variance_value}")
