# histogramme de la distribution de la pression sur une snapshot
import matplotlib.pyplot as plt 
import sys
import time
import numpy as np

if len(sys.argv) < 2:
    print("Usage: python histo.py <path_to_snapshot>")
    sys.exit(1)

filename = sys.argv[1]

with open(filename, 'r') as f:
    content = f.read()  # lit tout le fichier dans une string

content.strip()
lines = content.split('\n')

# On convertit la snapshot en un grand tableau 1d avec toutes les valeurs
data = []
for l in lines:
    vals = l.strip().split(' ')
    vals = list(map(float, vals))
    data.extend(vals)



# on calcule l'histogramme avec numpy pour pouvoir le chronométrer, et faire l'affichage dans un temps second
t0 = time.time()
hist, bin_edges = np.histogram(data, bins=15) 
t1 = time.time()
# hist[i] est le nombre de valeurs dans le bin numéro i

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
plt.show()
