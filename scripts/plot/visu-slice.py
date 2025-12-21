import numpy as np
import matplotlib.pyplot as plt
import seaborn as sns
import re
import argparse
import time
from matplotlib.colors import LinearSegmentedColormap
import os


# Script python permettant de visualiser une slice soit:
# - à partir d'une snapshot du cube
# - à partir d'une snapshot d'une slice directement

# Fonctionne uniquement avec ex==ey==ez (le but de ce script étant le benchmark, cela simplifie les choses)
# La dimension fixée est la dimension Z, on visualise le plan X-Y


# Si l'input donnée est un dossier, alors on fait la visualisation pour chaque fichier .bin (snapshot) du dossier
# Sinon, uniquement pour le fichier donné en input.

parser = argparse.ArgumentParser()
parser.add_argument('-o', '--order')
parser.add_argument('-z', help="The Z fixed coordinate")
parser.add_argument('-s', '--size', help="Size of the problem, i.e ex value (ex==ey==ez)") # ex=ey=ez
parser.add_argument('--slice-snapshot', help="If present, this flag means the snapshot is a slice snapshot and not a regular full-cube snapshot",
                    action=argparse.BooleanOptionalAction)
parser.add_argument('-i', '--input', help="Input path")

args = parser.parse_args()


size = int(args.size)
order = int(args.order)

sizeDim = (size * order) + 1 # la taille d'une dimension
sizePlan = sizeDim * sizeDim # la taille d'un plan

startingLine = sizeDim * int(args.z)
if (args.slice_snapshot):
    startingLine = 0

if os.path.isdir(args.input):
    files = []
    for fName in os.listdir(args.input):
        if fName.endswith('.bin'):
            files.append(os.path.join(args.input, fName))
else:
    files = [args.input]


t0 = time.time()


for singleFile in files:
  data = []
  with open(singleFile, 'r') as f:
      for i, line in enumerate(f):
          if i < startingLine:
              continue
          if i >= startingLine + sizeDim:
              break
          row = [float(x) for x in line.strip().split()]
          data.append(row)

  data_array = np.array(data)
  plt.figure(figsize=(6, 5))

  colors = [(0, 0, 1), (0, 1, 0)] 
  cmap = LinearSegmentedColormap.from_list("my_cmap", colors)
  sns.heatmap(data_array, cmap=cmap)

  plt.xlabel("X")
  plt.ylabel("Y")
  outputPath = f"{ (singleFile.split('/'))[-1].split('.')[:-1][0]}--visu_slice.png"
  plt.savefig(outputPath)

t1 = time.time()

print("Time: ", t1 - t0)
