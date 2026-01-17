import matplotlib.pyplot as plt
import pandas as pd
import io
import numpy as np

try:
    df = pd.read_csv("data/stats/stats_benchmark_results.csv")
except FileNotFoundError:
    print("-------------------------")
    print("Fichier non trouvé")
    print("-------------------------")

df['speedup'] = df['global_adhoc_ms'] / df['global_insitu_ms']

#figure temps
plt.figure(figsize=(10, 6))
plt.plot(df['size'], df['global_insitu_ms'], 'o-', label='In-Situ (Global)', color='#2ca02c', linewidth=2)
plt.plot(df['size'], df['global_adhoc_ms'], 's-', label='Ad-Hoc (Global)', color='#d62728', linewidth=2)
plt.ylabel('Time (ms)', fontsize=12)
plt.xlabel('Problem size', fontsize=12)
plt.legend()
plt.grid(True, linestyle='--', alpha=0.6)
plt.tight_layout()
plt.savefig("data/stats/benchmark_time_comparison.png", dpi=300)
plt.show()

#figure speedup
plt.figure(figsize=(10, 6))
plt.plot(df['size'], df['speedup'], 'D-', color='#1f77b4', linewidth=2)
plt.axhline(1, color='black', linestyle='--', label='Seuil (x1)')
plt.xlabel('Problem size', fontsize=12)
plt.ylabel('Speedup', fontsize=12)
plt.legend()
plt.grid(True, linestyle='--', alpha=0.6)
plt.tight_layout()
plt.savefig("data/stats/benchmark_speedup.png", dpi=300)
plt.show()