import subprocess
import csv
from tabulate import tabulate


nb_iter = 4
sizes = [10,50, 100, 200]



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


def getExecStats(subprocess_stdout):
    for line in subprocess_stdout.splitlines():
        if line.startswith("Exec stats:"):
            outputPath = line.split(':')[1].strip()[1:-1]
    with open(f"{outputPath}", newline="\n") as f:
        reader = csv.DictReader(f)
        row = next(reader)
    return row

def run_insitu(size):
    # Faire exécution insitu pendant laquelle on calcule les histogrammes
    # histoTime
    # A noter: on compte les temps d'écriture des histo mais c'est peut etre pas bien à chronometrer pcq l'OS le fait pas en direct
    result = subprocess.run(
        ["./build/bin/semproxy",  "--timemax", "1", "-o", "2", "--compute-histogram", "--compute-histogram-delay", "50", "--ex", str(size), "--ey",  str(size), "--ez",  str(size)],
        capture_output=True,
        text=True          
    )
    if len(result.stderr) > 0:
        printStderr(result.stderr)
    row = getExecStats(result.stdout)
    insitu_time = float(row['histotime']) 
    return insitu_time

def run_adhoc(size):
    # Exécution adhoc avec les snapshots + python3 histo.py pour chaque snapshot
    # saveSnapshotTime + time histo python
    result = subprocess.run(
        ["./build/bin/semproxy", "--timemax", "1", "-o", "2", "--snapshot", "--sd", "50", "--ex", str(size), "--ey",  str(size), "--ez",  str(size)],
        capture_output=True,
        text=True          
    )
    printStderr(result.stderr)
    statsExec = getExecStats(result.stdout)
    # launch histo python adhoc
    result = subprocess.run(
        ["python3", "scripts/stats/histo.py", "/tmp/insitu/data/snapshot/"],
        capture_output=True,
        text=True          
    )
    printStderr(result.stderr)
    # time to save snapshots
    adhoc_time = float(statsExec['snapshottime'])
    found = False
    for line in result.stdout.splitlines():
        if line.startswith("Time:"):
            # time to compute histogram in python script
            adhoc_time += float(line.split(':')[1].strip()) * 1e6
            found = True 
    if not(found):
        print("Did not find any time in stdout for python3 histo.py")
        print(result.stderr)
    return adhoc_time


res_insitu = []
res_adhoc = []

for size in sizes: 
    sum_insitu = 0
    sum_adhoc = 0
    for i in range(nb_iter):
        insitu_time = run_insitu(size)
        print(f"[Size {size}][Iter {i}] in-situ: {insitu_time / 1e3} millisec")
        sum_insitu += insitu_time

        adhoc_time = run_adhoc(size)
        print(f"[Size {size}][Iter {i}] ad-hoc: {adhoc_time / 1e3} millisec")
        sum_adhoc += adhoc_time
    res_insitu.append( (sum_insitu / nb_iter) / 1e3)
    res_adhoc.append( (sum_adhoc / nb_iter) / 1e3)
    print("")



print("")
print("-------------")

data = list(zip(sizes, res_insitu, res_adhoc))
print(tabulate(data, headers=["Sizes", "Insitu (ms)", "Adhoc (ms)"],tablefmt="github"))