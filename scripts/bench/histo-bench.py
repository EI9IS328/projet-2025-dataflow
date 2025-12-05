import subprocess
import csv

size = 20
nb_iter = 5


def getExecStats(subprocess_stdout):
    for line in subprocess_stdout.splitlines():
        if line.startswith("Exec stats:"):
            outputPath = line.split(':')[1].strip()
    with open(f"{outputPath}", newline="\n") as f:
        reader = csv.DictReader(f)
        row = next(reader)
    return row

def run_insitu():
    # Faire exécution insitu pendant laquelle on calcule les histogrammes
    # histoTime
    # A noter: on compte les temps d'écriture des histo mais c'est peut etre pas bien à chronometrer pcq l'OS le fait pas en direct
    result = subprocess.run(
        ["./build/bin/semproxy",  "--timemax", "0.4", "-o", "2", "--compute-histogram", "--compute-histogram-delay", "50", "--ex", str(size), "--ey",  str(size), "--ez",  str(size)],
        capture_output=True,
        text=True          
    )
    row = getExecStats(result.stdout)
    insitu_time = float(row['histotime']) 
    return insitu_time

def run_adhoc():
    # Exécution adhoc avec les snapshots + python3 histo.py pour chaque snapshot
    # saveSnapshotTime + time histo python
    result = subprocess.run(
        ["./build/bin/semproxy", "--timemax", "0.4", "-o", "2", "--snapshot", "--sd", "50", "--ex", str(size), "--ey",  str(size), "--ez",  str(size)],
        capture_output=True,
        text=True          
    )
    statsExec = getExecStats(result.stdout)
    # launch histo python adhoc
    result = subprocess.run(
        ["python3", "scripts/stats/histo.py", "./data/snapshot/"],
        capture_output=True,
        text=True          
    )

    adhoc_time = float(statsExec['snapshottime'])
    found = False
    for line in result.stdout.splitlines():
        if line.startswith("Time:"):
            adhoc_time += float(line.split(':')[1].strip()) * 1e6
            found = True 
    if not(found):
        print("Did not find any time in stdout for python3 histo.py")
    return adhoc_time

total_insitu = 0
total_adhoc = 0
for i in range(nb_iter):
    insitu_time = run_insitu()
    print(f"[Iter {i}] in-situ: {insitu_time / 1e3} millisec")
    total_insitu += insitu_time

    adhoc_time = run_adhoc()
    print(f"[Iter {i}] ad-hoc: {adhoc_time / 1e3} millisec")
    total_adhoc += adhoc_time

avg_insitu = total_insitu / nb_iter
avg_adhoc = total_adhoc / nb_iter

print("")
print("-------------")
print("")
print(f"Average in-situ: {avg_insitu / 1e3} millisec")
print(f"Average ad-hoc: {avg_adhoc / 1e3} millisec")