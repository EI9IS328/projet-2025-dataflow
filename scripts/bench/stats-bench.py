import subprocess
import csv
import time
import os
import glob
from pathlib import Path

NB_ITER = 4
SIZES = [10,20,30,40,50,75,100,200] 
SEM_EXE = "./build/bin/semproxy"
SNAPSHOT_DIR = "./data/snapshot/"

TIMEMAX = "1"
SNAP_DELAY = "150"  

def clean_snapshots():
    """Supprime les anciens snapshots"""
    if os.path.exists(SNAPSHOT_DIR):
        for f in glob.glob(os.path.join(SNAPSHOT_DIR, "*.bin")):
            try:
                os.remove(f)
            except OSError:
                pass

def get_exec_stats(stdout_output):
    """Récupère la dernière ligne du CSV indiqué dans stdout"""
    csv_path = None
    for line in stdout_output.splitlines():
        if "Exec stats:" in line:
            parts = line.split(':', 1)
            if len(parts) > 1:
                raw_path = parts[1].strip().strip('"').strip("'")
                
                # Résolution du chemin
                if os.path.exists(raw_path):
                    csv_path = raw_path
                elif os.path.exists(os.path.join("build/bin", raw_path)):
                    csv_path = os.path.join("build/bin", raw_path)
                elif os.path.exists(os.path.normpath(os.path.join("build/bin", raw_path))):
                     csv_path = os.path.normpath(os.path.join("build/bin", raw_path))

    if not csv_path:
        return {}

    try:
        with open(csv_path, newline='') as f:
            reader = csv.DictReader(f)
            return next(reader)
    except Exception as e:
        print(f"  [Error] Erreur lecture CSV: {e}")
        return {}

def compute_stats_python_adhoc():
    """
    Simule le post-traitement 
    return le temps en millisecondes.
    """
    start_time = time.time()
    
    files = glob.glob(os.path.join(SNAPSHOT_DIR, "*.bin"))
    if not files:
        return 0

    for file_path in files:
        try:
            with open(file_path, 'r') as f:
                data = f.read().split()
                vals = [float(x) for x in data if x.strip()]
                if vals:
                    _ = min(vals)
                    _ = max(vals)
        except:
            pass

    end_time = time.time()
    return (end_time - start_time) * 1000 # conversion en ms

def run_insitu(size):
    """
    Global In-Situ = Physique (Kernel) + Analyse (In-Situ)
    """
    cmd = [
        SEM_EXE, 
        "--timemax", TIMEMAX,
        "-o", "2",
        "--ex", str(size), "--ey", str(size), "--ez", str(size),
        "--stats-analysis",
        "--stats-interval",SNAP_DELAY
    ]
    
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, check=True)
        stats = get_exec_stats(result.stdout)
        
        # Récupération des temps en microsecondes
        k_time = float(stats.get('kerneltime', 0))
        
        # Total en millisecondes
        return (k_time) / 1e3
        
    except Exception as e:
        print(f"  [Error] In-Situ Crash: {e}")
        return 0

def run_adhoc(size):
    """
    Global Ad-Hoc = temps application ( avec sauvegarde )  + temps post calcul
    """
    clean_snapshots()
    
    cmd = [
        SEM_EXE, 
        "--timemax", TIMEMAX, 
        "-o", "2", 
        "--ex", str(size), "--ey", str(size), "--ez", str(size),
        "-s",  # on sauvegarde les snapshot pour ensuite effectuer notre calcul ad_hoc
        "--sd", SNAP_DELAY
    ]
    
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, check=True)
        stats = get_exec_stats(result.stdout)
        
        k_time_ns = float(stats.get('kerneltime', 0))
        
        py_time_ms = compute_stats_python_adhoc()

        # Total en millisecondes
        return ((k_time_ns) / 1e3) + py_time_ms
        
    except Exception as e:
        print(f"  [Error] Ad-Hoc Crash: {e}")
        return 0

#main
if not os.path.exists(SEM_EXE):
    print(f"Erreur: Exécutable introuvable: {SEM_EXE}")
    exit(1)

os.makedirs(SNAPSHOT_DIR, exist_ok=True)
os.makedirs("data/trace", exist_ok=True)

print(f"{'Size':<10} | {'In-Situ (ms)':<15} | {'Ad-Hoc (ms)':<15} | {'Speedup':<10}")
print("-" * 60)

csv_results = []

for size in SIZES:
    insitu_vals = []
    adhoc_vals = []

    print(f"Running size {size}x{size}x{size}...", end="\r")

    for _ in range(NB_ITER):
        insitu_vals.append(run_insitu(size))
        adhoc_vals.append(run_adhoc(size))

    avg_in = sum(insitu_vals) / NB_ITER
    avg_ad = sum(adhoc_vals) / NB_ITER
    
    # calcul du speedup global
    speedup = avg_ad / avg_in if avg_in > 0 else 0
    speedup_str = f"x{speedup:.2f}" if avg_in > 0 else "N/A"

    print(f"{size:<10} | {avg_in:<15.2f} | {avg_ad:<15.2f} | {speedup_str:<10}")
    csv_results.append((size, avg_in, avg_ad))

# Sauvegarde des résultats
with open("data/stats/stats_benchmark_results.csv", "w") as f:
    f.write("size,global_insitu_ms,global_adhoc_ms\n")
    for r in csv_results:
        f.write(f"{r[0]},{r[1]},{r[2]}\n")

print("\n\nTerminé. Résultats (Global Workflow) sauvegardés.")