#!/usr/bin/env python3
import numpy as np
import matplotlib.pyplot as plt
import argparse
import os

def main():
    parser = argparse.ArgumentParser(description="FFT d'un sismogramme.")
    parser.add_argument("input_file", type=str, help="Fichier contenant le signal (une seule ligne de valeurs séparées par des espaces)")
    parser.add_argument("--fs", type=float, default=100.0, help="Fréquence d'échantillonnage en Hz")
    args = parser.parse_args()

    filename = args.input_file
    fs = args.fs

    try:
        with open(filename, 'r') as f:
            line = f.readline().strip()
            signal = np.fromstring(line, sep=' ')
    except:
        raise RuntimeError(f"Impossible de lire le fichier : {filename}")

    N = len(signal)
    dt = 1.0 / fs
    time = np.arange(N) * dt

    fft_vals = np.fft.rfft(signal)
    fft_freq = np.fft.rfftfreq(N, d=dt)
    fft_amp = np.abs(fft_vals) / N

    plt.figure(figsize=(10,4))
    plt.plot(fft_freq, fft_amp)
    plt.title(f"Spectre fréquentiel du sismogramme : {os.path.basename(filename)}")
    plt.xlabel("Fréquence (Hz)")
    plt.ylabel("Amplitude")
    plt.grid(True)

    out_png = filename + "_fft.png"
    plt.savefig(out_png, dpi=200)
    print(f"[OK] FFT sauvegardée dans : {out_png}")

    plt.show()

if __name__ == "__main__":
    main()
