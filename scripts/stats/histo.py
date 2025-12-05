# histogramme de la distribution de la pression sur une snapshot
import matplotlib.pyplot as plt 
import sys
import time
import numpy as np
import os

if len(sys.argv) < 2:
    print("Usage: python histo.py <path_to_snapshot>")
    print("Could be path to a single snapshot or to a folder containing all snapshots for an execution")
    sys.exit(1)

filename = sys.argv[1]

# On peut passer un folder en paramètre, et cela calculera une moyenne de toutes les snapshots
# contenues dans ce folder
if os.path.isdir(filename):
    files = []
    for fName in os.listdir(filename):
        if fName.endswith('.bin'):
            files.append(os.path.join(filename, fName))
else:
    files = [filename]

print("On calcule l'histogramme pour ces fichiers:")
print(files)
# On convertit la/les snapshot en un grand tableau 1d avec toutes les valeurs
data = []

for file_snapshot in files:
    with open(file_snapshot, 'r') as f:
        content = f.read()
    content.strip()
    lines = content.split('\n')
    for l in lines:
        vals = l.strip().split(' ')
        vals = list(map(float, vals))
        data.extend(vals)




# on calcule l'histogramme avec numpy pour pouvoir le chronométrer, et faire l'affichage dans un temps second
t0 = time.time()
hist, bin_edges = np.histogram(data, bins=10) 
t1 = time.time()
# hist[i] est le nombre de valeurs dans le bin numéro i

# on sauvegarde l'histogramme ici aussi, puisqu'on le sauvegarde en insitu
outputPath = './histo-adhoc.bin'
with open(outputPath, 'w') as f:
    f.write("bin_edges:\n")
    for be in bin_edges:
        f.write(f"{be} ")
    f.write("\nhist:\n")
    for hval in hist:
        f.write(f"{hval} ")
    f.write("\n")

print("Temps calcul histogramme =", t1 - t0, "sec")

plt.bar(
    bin_edges[:-1],
    hist,
    width=np.diff(bin_edges),
    align="edge",
    edgecolor="black",
    linewidth=1.5
)

plt.ylabel("Nombre de valeurs of values")
plt.title(f"Distribution de la fréquence dans la snapshot {filename}")
plt.savefig('histogram-result.png')
# plt.show()
