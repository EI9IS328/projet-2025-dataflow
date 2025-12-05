import numpy as np
import matplotlib.pyplot as plt
import seaborn as sns
import re


filename = "../data/snapshot/snapshot_150_order2.txt" 

match = re.search(r"order(\d+)", filename)
order = int(match.group(1)) if match else None

start_line = 1275
Y_lines = 51


data = []
with open(filename, 'r') as f:
    for i, line in enumerate(f):
        if i < start_line:
            continue
        if i >= start_line + Y_lines:
            break
        row = [float(x) for x in line.strip().split()]
        data.append(row)

data_array = np.array(data)

plt.figure(figsize=(10, 6))
sns.heatmap(data_array, cmap="viridis")
plt.title(f"Heatmap from line {start_line} to {start_line + Y_lines} in {filename}")
plt.xlabel("X")
plt.ylabel("Y")
plt.show()