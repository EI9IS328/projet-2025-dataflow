import subprocess
import csv
from tabulate import tabulate
import time
import os

nb_iter = 5
sizes = [10,20, 30, 50, 75, 100, 200]

# Fonction utilitaires
def getExecStats(subprocess_stdout):
    for line in subprocess_stdout.splitlines():
        if line.startswith("Exec stats:"):
            outputPath = line.split(':')[1].strip()[1:-1]
    with open(f"{outputPath}", newline="\n") as f:
        reader = csv.DictReader(f)
        row = next(reader)
    return row



def printStderr(stderr):
    # print lines except the kokkos line
    err = stderr.replace("""Kokkos::Cuda::initialize WARNING: Cuda is allocating into UVMSpace by default
                                  without setting CUDA_MANAGED_FORCE_DEVICE_ALLOC=1 or
                                  setting CUDA_VISIBLE_DEVICES.
                                  This could on multi GPU systems lead to severe performance"
                                  penalties.""", "")
    err = err.strip()
    if len(err) > 0:
        print(err)

# Benchmark:
# - adhoc basique (save snapshot + load snapshot + trouver la slice + la visualiser et la sauvegarder)
# - adhoc avec snapshot de slice (save slice snapshot + load slice snapshot + la visualiser et la sauvegarder)
# - insitu ppm (save la slice au format PPM directement)

def run_adhoc_basic(size):
  result = subprocess.run(
    ["./build/bin/semproxy", "-o", "2", "--sd", "50", "--snapshot", "--ex", 
      str(size), "--ey", str(size), "--ez", str(size)],
    capture_output=True,
    text=True
  )
  printStderr(result.stderr)
  statsExecCpp = getExecStats(result.stdout)

  # On lance la visualisation adhoc en python
  result = subprocess.run(
    ["python3", "scripts/plot/visu-slice.py", "--order", statsExecCpp["order"], "-z", str(fixedZ),
      "--size", statsExecCpp["ex"], "--input", "/tmp/insitu/data/snapshot/"],
    capture_output=True,
    text=True
  )
  printStderr(result.stderr)

  total_time = float(statsExecCpp["snapshottime"])

  # on parse l'output du script python
  for line in result.stdout.splitlines():
    if line.startswith("Time:"):
      visu_time = float(line.split(':')[-1].strip()) * 1e6
      total_time += visu_time
      part_python = visu_time / total_time
  return (total_time,part_python)


def run_adhoc_slice(size):
  result = subprocess.run(
    ["./build/bin/semproxy", "-o", "2", "--sd", "50", "--ex", 
      str(size), "--ey", str(size), "--ez", str(size), "--slice-snapshot", str(fixedZ)],
    capture_output=True,
    text=True
  )
  printStderr(result.stderr)
  statsExecCpp = getExecStats(result.stdout)

  # On lance la visualisation adhoc en python
  result = subprocess.run(
    ["python3", "scripts/plot/visu-slice.py", "--order", statsExecCpp["order"], "-z", str(fixedZ),
      "--size", statsExecCpp["ex"], "--input", "/tmp/insitu/data/slice_snapshot/", "--slice-snapshot"],
    capture_output=True,
    text=True
  )
  printStderr(result.stderr)

  total_time = float(statsExecCpp["slicesnaptime"])

  # on parse l'output du script python
  for line in result.stdout.splitlines():
    if line.startswith("Time:"):
      visu_time = float(line.split(':')[-1].strip()) * 1e6
      total_time += visu_time
      part_python = visu_time / total_time
  return (total_time,part_python)

def run_insitu(size):
  result = subprocess.run(
    ["./build/bin/semproxy", "-o", "2", "--sd", "50", "--ex", 
      str(size), "--ey", str(size), "--ez", str(size), "--slice-snapshot", str(fixedZ), "--slice-ppm"],
    capture_output=True,
    text=True
  )
  printStderr(result.stderr)
  statsExecCpp = getExecStats(result.stdout)

  total_time = float(statsExecCpp["slicesnaptime"])
  return total_time


res_adhoc_basic = []
res_adhoc_basic_part_python = []

res_adhoc_slicesnap = []
res_adhoc_slicesnap_part_python = []
res_adhoc_slicesnap_speedup = []

res_insitu = []
res_insitu_speedup = []

for size in sizes: 
    fixedZ = ((2 * size) + 1)//2

    sum_adhoc_basic = 0
    sum_adhoc_basic_part_python = 0
    sum_adhoc_slicesnap = 0
    sum_adhoc_slicesnap_part_python = 0
    sum_insitu = 0
    for i in range(nb_iter):
        (adhoc_basic_time, adhoc_basic_part_python) = run_adhoc_basic(size)
        print(f"[Size {size}][Iter {i}] ad-hoc basic: {adhoc_basic_time / 1e3} millisec")
        sum_adhoc_basic += adhoc_basic_time
        sum_adhoc_basic_part_python += adhoc_basic_part_python

        (adhoc_slicesnap_time, adhoc_slicesnap_part_python) = run_adhoc_slice(size)
        print(f"[Size {size}][Iter {i}] ad-hoc slicesnap: {adhoc_slicesnap_time / 1e3} millisec")
        sum_adhoc_slicesnap += adhoc_slicesnap_time
        sum_adhoc_slicesnap_part_python += adhoc_slicesnap_part_python

        insitu_time = run_insitu(size)
        print(f"[Size {size}][Iter {i}] in-situ: {insitu_time / 1e3} millisec")
        sum_insitu += insitu_time

    res_adhoc_basic.append( (sum_adhoc_basic / nb_iter) / 1e3)
    res_adhoc_basic_part_python.append((sum_adhoc_basic_part_python / nb_iter) * 100)

    res_adhoc_slicesnap.append( (sum_adhoc_slicesnap / nb_iter) / 1e3)
    res_adhoc_slicesnap_speedup.append(f"x{res_adhoc_basic[-1] / res_adhoc_slicesnap[-1]}")
    res_adhoc_slicesnap_part_python.append( (sum_adhoc_slicesnap_part_python / nb_iter) * 100)

    res_insitu.append( (sum_insitu / nb_iter) / 1e3)
    res_insitu_speedup.append(f"x{res_adhoc_basic[-1] / res_insitu[-1] }")
    print("")
    data = list(zip(sizes, res_adhoc_basic, res_adhoc_basic_part_python, res_adhoc_slicesnap, res_adhoc_slicesnap_speedup, 
                  res_adhoc_slicesnap_part_python, res_insitu, res_insitu_speedup))
    print(tabulate(data, headers=["Sizes", "Adhoc basic (ms)", "Part de python (%)",
                                  "Adhoc slicesnap (ms)", "Speedup", "Part de python (%)",
                                   "Insitu (ms)", "Speedup"],tablefmt="github"))


print("")
print("-------------")