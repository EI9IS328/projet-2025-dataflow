import numpy as np
import pandas as pd
import sys
import os
import re
import time


def process_single_file(filepath, dt=0.001):
    if not os.path.exists(filepath):
        print(f"Erreur : Le fichier '{filepath}' est introuvable.")
        return

    filename = os.path.basename(filepath)

    coords = re.findall(r"[-+]?\d*\.\d+|\d+", filename)
    
    if len(coords) >= 3:
        x, y, z = coords[0], coords[1], coords[2]
    else:
        x, y, z = "unknown", "unknown", "unknown"

    try:
        with open(filepath, 'r') as f:
            data = np.fromstring(f.read(), sep=' ')
        
        if data.size == 0:
            print(f"Le fichier {filename} est vide.")
            return

        n = len(data)
        fourier_raw = np.fft.fft(data)
        half_n = n // 2
        
        results = []
        for k in range(half_n):
            val_complex = fourier_raw[k]
            results.append({
                "freq_idx": k,
                "magnitude": np.abs(val_complex),
                "real": val_complex.real,
                "imag": val_complex.imag
            })

        output_csv = f"./data/fourier/fourier_adhoc_{x}_{y}_{z}.csv"
        df = pd.DataFrame(results)
        df.to_csv(output_csv, index=False)
        
        print(f"Analyse terminée pour le point ({x}, {y}, {z})")
        print(f"Résultat sauvegardé dans : {output_csv}")

    except Exception as e:
        print(f"Erreur lors du traitement : {e}")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python3 adhoc_single_fourier.py <chemin_du_fichier> [dt]")
    else:
        file_path = sys.argv[1]
        delta_t = float(sys.argv[2]) if len(sys.argv) > 2 else 0.001
        t0 = time.time()
        process_single_file(file_path, delta_t)
        t1 = time.time()
        print("Time: ", t1 - t0)
