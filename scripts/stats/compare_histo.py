import numpy as np

totalMismatchCount = 0

for step in range(50,1500,50): # on skip la snapshot remplie de 0
    snapshot_path = f'./data/snapshot/snapshot_{step}_order2.bin'
    histo_path = f'./data/histo/histo_{step}_order2.bin'
    data = []
    with open(snapshot_path, 'r') as f:
        content = f.read()
    content.strip()
    lines = content.split('\n')
    for l in lines:
        vals = l.strip().split(' ')
        vals = list(map(float, vals))
        data.extend(vals)

    hist, bin_edges = np.histogram(data, bins=10) 

    # comparer avec l'histogramme que j'ai compute moi
    with open(histo_path, 'r') as f:
        content = f.read()
    content.strip()
    lines = content.split('\n')
    edgesValues = list(map(float, lines[1].strip().split(' ')))
    histValues = list(map(int, lines[3].strip().split(' ')))

    for i in range(len(hist)):
        diff = abs(hist[i] - histValues[i])
        if diff > 1e-5:
            print(f"[{step}] # Diff in hist[{i}]: {diff}, python={hist[i]}, insitu={histValues[i]}")
            totalMismatchCount+=1
    for i in range(len(hist)):
        diff = abs(edgesValues[i] - bin_edges[i])
        if diff > 1e-5:
            print(f"[{step}] # Diff in edges[{i}]: {diff}, python={bin_edges[i]}, insitu={edgesValues[i]}")
            totalMismatchCount+=1
print(f"==> {totalMismatchCount} total mismatch between numpy and insitu")
